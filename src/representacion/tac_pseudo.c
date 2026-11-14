/*
 * tac_pseudo.c — operaciones de destino pendientes de expansión con el P1.
 * Usa: tac_internal.h. Lo usan consultas, validación y futuras pasadas C3.3/C3.4.
 * No agrega instrucciones al ISA ni expande código; Aho §8.1–8.2, §8.6 y §8.8.
 */
#include "tac_internal.h"
#include <string.h>

/* Tabla única HIGH/PSEUDO; propiedades compartidas según §9.8 y Aho §8.2. */
static const struct {
    TacOpcode high;
    TacPseudoKind kind;
    const char *text;
    unsigned scratch_count;
    uint16_t clobbers;
} operations[] = {
    {TAC_MUL, TAC_PSEUDO_MUL, "mul", 8, 0},
    {TAC_DIV, TAC_PSEUDO_DIV, "div", 8, 0},
    {TAC_MOD, TAC_PSEUDO_MOD, "mod", 8, 0},
    {TAC_POW, TAC_PSEUDO_POW, "pow", 8, 0},
    {TAC_AUTH, TAC_PSEUDO_AUTH, "auth", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_LOGOUT, TAC_PSEUDO_LOGOUT, "logout", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_KV_WRITE, TAC_PSEUDO_KV_WRITE, "kv_write", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_KV_LOCK, TAC_PSEUDO_KV_LOCK, "kv_lock", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_SANC, TAC_PSEUDO_SANC, "sanc", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_CIFRAR, TAC_PSEUDO_CIFRAR, "cifrar", 0, TAC_P1_CALL_CLOBBERS},
    {TAC_DESCIFRAR, TAC_PSEUDO_DESCIFRAR, "descifrar", 0, TAC_P1_CALL_CLOBBERS},
};
static const TacOpInfo exit_operation = {
    .text = "exit", .barrier = 1, .nonremovable = 1, .uses_globals = 1,
    .flow = TAC_FLOW_RETURN, .format = TAC_FMT_PSEUDO,
};

/* Consultas sin efectos; NONE/COUNT indican ausencia de pareja (§9.8). */
TacPseudoKind tac_pseudo_for(TacOpcode high) {
    for (size_t k = 0; k < sizeof operations / sizeof operations[0]; k++)
        if (operations[k].high == high)
            return operations[k].kind;
    return TAC_PSEUDO_NONE;
}
TacOpcode tac_pseudo_high(TacPseudoKind kind) {
    for (size_t k = 0; k < sizeof operations / sizeof operations[0]; k++)
        if (operations[k].kind == kind)
            return operations[k].high;
    return TAC_OP_COUNT;
}
int tac_pseudo_is_arithmetic(TacPseudoKind kind) {
    for (size_t k = 0; k < sizeof operations / sizeof operations[0]; k++)
        if (operations[k].kind == kind)
            return operations[k].scratch_count == 8;
    return 0;
}

/*
 * tac_pseudo_info — consulta efectos y operandos por el contrato de expansión.
 * Entrada:
 *   instruction TAC_PSEUDO y out escribible, no NULL.
 * Salida:
 *   1 con propiedades; 0 si el discriminante no existe.
 * Efectos:
 *   copia la fila y agrega la máscara de auxiliares físicos aritméticos.
 * Restricciones:
 *   no valida etapa ni forma; Aho §8.2, §8.8 y §9.2.5.
 */
int tac_pseudo_info(const TacInstruction *instruction, TacOpInfo *out) {
    TacPseudoKind kind = instruction->pseudo.kind;
    if (kind <= TAC_PSEUDO_NONE || (unsigned)kind >= TAC_PSEUDO_COUNT) {
        return 0;
    }
    if (kind == TAC_PSEUDO_EXIT) {
        *out = exit_operation;
    } else {
        for (size_t k = 0; k < sizeof operations / sizeof operations[0]; k++) {
            if (operations[k].kind != kind)
                continue;
            *out = *tac_op_info(operations[k].high);
            out->text = operations[k].text;
            out->format = TAC_FMT_PSEUDO;
            out->scratch_count = operations[k].scratch_count;
            out->call_clobbers = operations[k].clobbers;
            break;
        }
    }
    unsigned positions = out->reads | out->writes;
    for (unsigned k = 0; k < 3; k++) {
        out->allowed[k] = positions & (1u << k) ?
                          TAC_KIND(TAC_TEMP) | TAC_KIND(TAC_PHYS) : TAC_KIND(TAC_NONE);
    }
    if (out->scratch_count) {
        out->call_clobbers = instruction->pseudo.scratch_clobbers;
    }
    return 1;
}

/*
 * valid_arithmetic_registers — verifica el contrato físico de cg_arith_emit.
 * Entrada:
 *   instruction aritmética con operandos ya validados; stage ISA válido.
 * Salida:
 *   1 si acepta registros y auxiliares; 0 con diagnóstico.
 * Efectos:
 *   solo escribe error/cap.
 * Restricciones:
 *   destino y ocho auxiliares distintos de fuentes; Aho §8.8.
 */
static int valid_arithmetic_registers(const TacInstruction *instruction, TacStage stage,
                                      char *error, size_t cap) {
    TacOperand operands[3] = {instruction->dest, instruction->src1, instruction->src2};
    uint16_t occupied = 0;
    for (unsigned k = 0; k < 3; k++) {
        if (operands[k].kind == TAC_PHYS) {
            unsigned id = (unsigned)operands[k].value.id;
            if (id < TAC_P1_TEMP_FIRST) {
                return tac_error(error, cap, "aritmética PSEUDO requiere temporales t0-t11");
            }
            occupied |= (uint16_t)(1u << id);
        }
    }
    uint16_t mask = instruction->pseudo.scratch_clobbers;
    if (stage == TAC_STAGE_ISA_VIRTUAL) {
        if (mask) {
            return tac_error(error, cap, "auxiliares físicos en PSEUDO virtual");
        }
        if (operands[0].kind == TAC_TEMP &&
            ((operands[1].kind == TAC_TEMP && operands[0].value.id == operands[1].value.id) ||
             (operands[2].kind == TAC_TEMP && operands[0].value.id == operands[2].value.id))) {
            return tac_error(error, cap, "destino aritmético PSEUDO coincide con fuente");
        }
        return 1;
    }
    unsigned count = 0;
    for (unsigned bits = mask; bits; bits >>= 1) {
        count += bits & 1u;
    }
    if (count != 8 || (mask & ~TAC_P1_CALL_CLOBBERS) || (mask & occupied)) {
        return tac_error(error, cap, "PSEUDO requiere ocho auxiliares físicos distintos");
    }
    if (operands[0].value.id == operands[1].value.id ||
        operands[0].value.id == operands[2].value.id) {
        return tac_error(error, cap, "destino aritmético PSEUDO coincide con fuente");
    }
    return 1;
}

/*
 * tac_pseudo_validate — comprueba las restricciones adicionales de expansión.
 * Entrada:
 *   function e instruction con discriminante, operandos y selectores válidos.
 * Salida:
 *   1 válida; 0 con diagnóstico en error/cap.
 * Efectos:
 *   ninguno salvo el diagnóstico.
 * Restricciones:
 *   etapas ISA; EXIT solo en main; Aho §8.2 y §8.6.
 */
int tac_pseudo_validate(const TacFunction *function, const TacInstruction *instruction,
                        char *error, size_t cap) {
    if (tac_pseudo_is_arithmetic(instruction->pseudo.kind)) {
        return valid_arithmetic_registers(instruction, function->program->stage, error, cap);
    }
    if (instruction->pseudo.scratch_clobbers) {
        return tac_error(error, cap, "auxiliares aritméticos en PSEUDO no aritmética");
    }
    if (instruction->pseudo.kind == TAC_PSEUDO_EXIT &&
        strcmp(function->symbol->name, "main")) {
        return tac_error(error, cap, "PSEUDO exit solo admite main");
    }
    return 1;
}

/*
 * tac_append_pseudo — agrega una operación de destino sin expandirla.
 * Entrada:
 *   block o NULL; kind, D/A/B, selectors y máscara física por valor.
 * Salida:
 *   instrucción nueva; NULL si block es NULL.
 * Efectos:
 *   agrega al final, con línea/columna cero; no reserva auxiliares.
 * Restricciones:
 *   validar forma y etapa después; Aho §8.1–8.2 y §10.2.3.
 */
TacInstruction *tac_append_pseudo(TacBlock *block, TacPseudoKind kind,
                                 TacOperand dest, TacOperand src1, TacOperand src2,
                                 TacSelectors selectors, uint16_t scratch_clobbers) {
    return tac_insert_pseudo_after(block, block ? block->last : NULL, kind,
        dest, src1, src2, selectors, scratch_clobbers);
}
