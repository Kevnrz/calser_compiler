/* D: expansión P1 y preservación desde el plan C2; Aho §7.2, §8.6 y §8.8.
 * Las plantillas y su calendarización fija permanecen en codegen_arith/crypto.
 */
#include "tac_emit_internal.h"
#include "codegen_crypto.h"

/* Emisor ISA del mismo componente; el protocolo común lo envuelve (§9.13). */
int te_emit_isa(TacEmitter *e, const TacInstruction *i);

typedef struct {
    int dest;
    int source;
} RegisterCopy;

/* Entrada: plan, frame y modo. Salida: 1 emitido, 0 con diagnóstico.
 * Efectos: guarda/restaura solo preservados en sus casillas ABI.
 * Restricciones: el plan decide los vivos; cg_preserve comparte las casillas
 * y el orden ascendente con CRYPTO (Aho §7.2 y §8.8).
 */
static int preserve(TacEmitter *e, const TacFunction *f, const TacInstruction *i,
                     const TacEmissionSite *site, int restore) {
    if (!site || !site->preserved_count)
        return 1;
    uint16_t mask = 0;
    for (size_t k = 0; k < site->preserved_count; k++)
        mask |= (uint16_t)(1u << site->preserved[k].physical_register);
    if (!cg_preserve(&f->symbol->frame, mask, restore))
        return te_error(e, i, "área de preservación inválida");
    return 1;
}

/* Movimientos del plan, sin elegir registros ni casillas; §11.6 y Aho §8.6. */
static void spill_moves(const TacEmissionSite *site, int stores) {
    if (!site)
        return;
    const TacSpillMove *moves = stores ? site->stores : site->loads;
    size_t count = stores ? site->store_count : site->load_count;
    for (size_t k = 0; k < count; k++)
        cg_op(SLOT_LSU, "%s %s, %d(rz)", stores ? "sw" : "lw",
              isa_register_name((int)moves[k].physical_register), moves[k].address);
}

/* Los argumentos cargados al registro fijo no participan en las copias (§11.6). */
static int loaded_argument(const TacEmissionSite *site, unsigned reg) {
    for (size_t k = 0; site && k < site->load_count; k++)
        if (site->loads[k].physical_register == reg)
            return 1;
    return 0;
}

/* Entrada: hasta tres copias con destinos distintos. Salida: ninguna.
 * Efectos: elimina identidades, emite copias seguras y rompe ciclos con r4.
 * Restricciones: destinos CRYPTO 6/7/9/10, nunca r4. Una fuente r4 se consume
 * antes de quedar solo ciclos; vivos externos ya guardados. Aho §8.6.
 */
static void parallel_copies(RegisterCopy copies[3], size_t count) {
    for (size_t k = 0; k < count;) {
        if (copies[k].dest == copies[k].source)
            copies[k] = copies[--count];
        else
            k++;
    }
    while (count) {
        size_t ready = count;
        for (size_t k = 0; k < count && ready == count; k++) {
            int needed = 0;
            for (size_t n = 0; n < count; n++)
                needed |= copies[n].source == copies[k].dest;
            if (!needed)
                ready = k;
        }
        if (ready != count) {
            cg_op(SLOT_ALU0, "add %s, %s, rz", isa_register_name(copies[ready].dest),
                  isa_register_name(copies[ready].source));
            copies[ready] = copies[--count];
        } else {
            int old = copies[0].dest;
            cg_op(SLOT_ALU0, "add t0, %s, rz", isa_register_name(old));
            for (size_t k = 0; k < count; k++)
                if (copies[k].source == old)
                    copies[k].source = TAC_P1_TEMP_FIRST;
        }
    }
}

/* Entrada: PSEUDO CRYPTO física. Salida: 1 éxito, 0 con diagnóstico.
 * Efectos: prepara argumentos y llama la plantilla original con contador común.
 * Restricciones: la plantilla no guarda vivos; el llamador sí. Un gap explícito
 * separa también argumentos ya situados en su registro fijo.
 */
static int crypto(TacEmitter *e, const TacInstruction *i, const TacEmissionSite *site) {
    static const CGCryptoOperation operations[] = {
        [TAC_PSEUDO_AUTH] = CG_CRYPTO_AUTH,
        [TAC_PSEUDO_LOGOUT] = CG_CRYPTO_LOGOUT,
        [TAC_PSEUDO_KV_WRITE] = CG_CRYPTO_KV_WRITE,
        [TAC_PSEUDO_KV_LOCK] = CG_CRYPTO_KV_LOCK,
        [TAC_PSEUDO_SANC] = CG_CRYPTO_SANC,
        [TAC_PSEUDO_CIFRAR] = CG_CRYPTO_ENCRYPT,
        [TAC_PSEUDO_DESCIFRAR] = CG_CRYPTO_DECRYPT
    };
    CGCryptoTemplate operation = {.operation = operations[i->pseudo.kind],
        .registers = {-1, -1, -1}, .k = i->selectors.k, .w = i->selectors.w};
    int loop = operation.operation == CG_CRYPTO_ENCRYPT || operation.operation == CG_CRYPTO_DECRYPT;
    int argument = operation.operation != CG_CRYPTO_LOGOUT && operation.operation != CG_CRYPTO_KV_LOCK;
    RegisterCopy copies[3];
    size_t count = 0;
    if (loop) {
        TacOperand sources[] = {i->dest, i->src1, i->src2};
        const int fixed[] = {CG_CRYPTO_REG_SOURCE, CG_CRYPTO_REG_DEST, CG_CRYPTO_REG_COUNT};
        for (size_t k = 0; k < 3; k++) {
            operation.registers[k] = fixed[k];
            if (!loaded_argument(site, (unsigned)fixed[k]))
                copies[count++] = (RegisterCopy){fixed[k], (int)sources[k].value.id};
        }
    } else if (argument) {
        operation.registers[0] = CG_CRYPTO_REG_ARGUMENT;
        if (!loaded_argument(site, CG_CRYPTO_REG_ARGUMENT))
            copies[count++] = (RegisterCopy){CG_CRYPTO_REG_ARGUMENT, (int)i->src1.value.id};
    }
    parallel_copies(copies, count);
    spill_moves(site, 0);
    cg_gap(); /* La primera lectura de un bundle fijo no puede ver una escritura pendiente. */
    if (cg_crypto_emit_template(&operation, &e->arithmetic.next_label))
        return te_error(e, i, "falló la plantilla CRYPTO");
    return 1;
}

/* Entrada: aritmética física y plan. Salida: 1 éxito, 0 con diagnóstico.
 * Efectos: llama cg_arith_emit con auxiliares ordenados por registro.
 * Restricciones: máscaras garantizadas por C2; no duplicar su validación.
 */
static int arithmetic(TacEmitter *e, const TacInstruction *i, const TacEmissionSite *site) {
    int work[8];
    size_t count = 0;
    for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++)
        if (site->scratch_mask & (1u << r))
            work[count++] = (int)r;
    static const CGArithKind kinds[] = {[TAC_PSEUDO_MUL] = CG_ARITH_MUL, [TAC_PSEUDO_DIV] = CG_ARITH_DIV,
        [TAC_PSEUDO_MOD] = CG_ARITH_MOD, [TAC_PSEUDO_POW] = CG_ARITH_POWER};
    CGArithKind kind = kinds[i->pseudo.kind];
    if (!cg_arith_emit(&e->arithmetic, kind, (int)i->src1.value.id, (int)i->src2.value.id,
                      (int)i->dest.value.id, work, kind == CG_ARITH_MUL ? NULL : e->error_label))
        return te_error(e, i, "falló la expansión aritmética");
    return 1;
}

/* Entrada: jas o PSEUDO, función y contexto. Salida: 1 éxito, 0 con diagnóstico.
 * Efectos: preserva, expande o emite llamada/EXIT; comparte etiquetas globalmente.
 * Restricciones: sin evaluación AST ni reserva de registros; Aho §7.2 y §8.8.
 */
int te_emit_special(TacEmitter *e, const TacFunction *f, const TacInstruction *i) {
    const TacEmissionSite *site = tac_allocation_site(e->input->allocation, i);
    if (!preserve(e, f, i, site, 0))
        return 0;
    int is_crypto = i->op == TAC_PSEUDO && i->pseudo.kind != TAC_PSEUDO_EXIT &&
                    !tac_pseudo_is_arithmetic(i->pseudo.kind);
    if (!is_crypto)
        spill_moves(site, 0);
    if (i->op == TAC_PSEUDO && i->pseudo.kind == TAC_PSEUDO_EXIT) {
        cg_op(SLOT_BRU, "jef %s", e->end_label);
    } else if (i->op == TAC_ISA) {
        if (i->isa.instruction->form == ISA_FORM_JAS) {
            const char *callee = te_function_label(e, i->isa.target.value.symbol);
            if (!callee)
                return te_error(e, i, "llamada sin etiqueta de función");
            cg_stack_push(TAC_P1_RL);
            cg_op(SLOT_BRU, "jas rl, %s", callee);
            cg_stack_pop(TAC_P1_RL);
        } else if (!te_emit_isa(e, i)) {
            return 0;
        }
    } else if (tac_pseudo_is_arithmetic(i->pseudo.kind)) {
        if (!site)
            return te_error(e, i, "sitio sin plan de preservación");
        if (!arithmetic(e, i, site))
            return 0;
    } else if (!crypto(e, i, site)) {
        return 0;
    }
    spill_moves(site, 1);
    return preserve(e, f, i, site, 1);
}
