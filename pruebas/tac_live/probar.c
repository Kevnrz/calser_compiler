/* Pruebas directas de vivacidad y clasificación C1. Aho §9.2.5, §10.2.3. */
#include "../tac_analysis/fixture.h"

/* Entrada: fixture y raíces. Salida: análisis propio válido. Efectos: analiza.
 * Restricciones: valida antes y no modifica el programa.
 */
static TacLiveAnalysis *analyze(Fixture *f, const TacOperand *roots, size_t count) {
    fixture_validate(f);
    char error[256];
    TacLiveAnalysis *a = tac_live_analyze(f->function, roots, count, error, sizeof error);
    assert(a && !error[0]);
    return a;
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba unión, redefiniciones y raíces.
 * Restricciones: ambas ramas definen b; a atraviesa la unión, b no la precede.
 */
static void diamond(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *left = tac_block_new(f.function);
    TacBlock *right = tac_block_new(f.function);
    TacBlock *join = tac_block_new(f.function);
    tac_append(f.entry, TAC_COPY, f.a, tac_integer(1), tac_none());
    TacInstruction *branch = tac_append(f.entry, TAC_IF_EQ, tac_label(right), f.a, tac_integer(0));
    tac_append(left, TAC_COPY, f.b, f.a, tac_none());
    tac_append(left, TAC_GOTO, tac_label(join), tac_none(), tac_none());
    tac_append(right, TAC_COPY, f.b, tac_integer(2), tac_none());
    tac_append(right, TAC_GOTO, tac_label(join), tac_none(), tac_none());
    TacInstruction *sum = tac_append(join, TAC_ADD, f.d, f.a, f.b);
    TacInstruction *store = tac_append(join, TAC_COPY, tac_variable(&f.local), f.d, tac_none());
    TacInstruction *end = fixture_return(join);
    TacOperand roots[] = {tac_variable(&f.local), tac_variable(&f.global)};
    TacLiveAnalysis *a = analyze(&f, roots, 2);
    const TacLiveInstruction *v = tac_live_instruction(a, branch);
    assert(tac_live_contains(a, v->before, f.a));
    assert(!tac_live_contains(a, v->before, f.b));
    assert(tac_live_contains(a, tac_live_instruction(a, sum)->before, f.b));
    assert(tac_live_contains(a, tac_live_instruction(a, sum)->definitions, f.d));
    assert(tac_live_contains(a, tac_live_instruction(a, store)->after, roots[0]));
    assert(!tac_live_contains(a, tac_live_instruction(a, store)->before, roots[0]));
    assert(tac_live_contains(a, tac_live_instruction(a, end)->after, roots[0]));
    assert(tac_live_contains(a, tac_live_block(a, join)->live_out, roots[1]));
    assert(tac_live_contains(a, tac_live_block(a, join)->use, f.a));
    assert(tac_live_contains(a, tac_live_block(a, join)->def, f.d));
    tac_live_free(a);
    a = analyze(&f, NULL, 0);
    assert(!tac_live_contains(a, tac_live_instruction(a, store)->after, roots[0]));
    tac_live_free(a);
    char error[256];
    TacFunction alien = {.program = f.program, .symbol = &f.symbol};
    assert(!tac_live_analyze(&alien, NULL, 0, error, sizeof error));
    TacOperand invalid = tac_integer(0);
    assert(!tac_live_analyze(f.function, &invalid, 1, error, sizeof error));
    assert(!tac_live_analyze(f.function, NULL, 1, error, sizeof error));
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba convergencia en ciclo.
 * Restricciones: TEMP a se redefine; no se supone SSA ni una sola definición.
 */
static void cycle(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    TacBlock *test = tac_block_new(f.function);
    TacBlock *body = tac_block_new(f.function);
    TacBlock *end = tac_block_new(f.function);
    tac_append(f.entry, TAC_COPY, f.a, tac_integer(0), tac_none());
    tac_append(f.entry, TAC_GOTO, tac_label(test), tac_none(), tac_none());
    tac_append(test, TAC_IF_GE, tac_label(end), f.a, tac_integer(4));
    TacInstruction *increment = tac_append(body, TAC_ADD, f.a, f.a, tac_integer(1));
    tac_append(body, TAC_GOTO, tac_label(test), tac_none(), tac_none());
    fixture_return(end);
    TacLiveAnalysis *a = analyze(&f, NULL, 0);
    assert(tac_live_contains(a, tac_live_block(a, test)->live_in, f.a));
    assert(tac_live_contains(a, tac_live_block(a, body)->live_out, f.a));
    assert(tac_live_contains(a, tac_live_instruction(a, increment)->before, f.a));
    assert(tac_live_contains(a, tac_live_instruction(a, increment)->after, f.a));
    tac_live_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: verifica globales en CALL/RETURN.
 * Restricciones: la escritura anterior a CALL sigue viva aunque haya otra posterior.
 */
static void global_effects(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    Symbol callee = {.kind = SYMBOL_FUNCTION, .name = "aux", .type = TYPE_VOID};
    TacOperand g = tac_variable(&f.global);
    TacInstruction *first = tac_append(f.entry, TAC_COPY, g, tac_integer(5), tac_none());
    TacInstruction *call = tac_append(f.entry, TAC_CALL, tac_none(),
                                      tac_function_operand(&callee), tac_integer(0));
    tac_append(f.entry, TAC_COPY, g, tac_integer(6), tac_none());
    TacInstruction *end = fixture_return(f.entry);
    TacLiveAnalysis *a = analyze(&f, NULL, 0);
    assert(tac_live_contains(a, tac_live_instruction(a, first)->after, g));
    assert(tac_live_contains(a, tac_live_instruction(a, call)->uses, g));
    assert(tac_live_contains(a, tac_live_instruction(a, call)->definitions, g));
    assert(tac_live_contains(a, tac_live_instruction(a, end)->uses, g));
    tac_live_free(a);
    f.symbol.name = "aux";
    TacOperand root = tac_variable(&f.local);
    a = analyze(&f, &root, 1);
    assert(!tac_live_contains(a, tac_live_instruction(a, end)->after, root));
    tac_live_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: verifica valores ISA atravesando CALL.
 * Restricciones: raíces VARIABLE son memoria, rz no consume ubicación asignable.
 */
static void virtual_effects(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    Symbol callee = {.kind = SYMBOL_FUNCTION, .name = "aux", .type = TYPE_VOID};
    fixture_isa(f.entry, "mov", f.a, tac_none(), tac_integer(3), tac_none());
    TacInstruction *call = fixture_isa(f.entry, "jas", tac_physical(TAC_P1_RL),
        tac_none(), tac_none(), tac_function_operand(&callee));
    fixture_isa(f.entry, "add", f.d, f.a, tac_physical(TAC_P1_RZ), tac_none());
    TacInstruction *store = fixture_isa(f.entry, "sw", tac_none(),
        tac_physical(TAC_P1_RZ), f.d, tac_none());
    store->isa.displacement = 16;
    fixture_exit(f.entry);
    TacOperand root = tac_variable(&f.local);
    TacLiveAnalysis *a = analyze(&f, &root, 1);
    const TacLiveInstruction *v = tac_live_instruction(a, call);
    assert(tac_live_contains(a, v->before, f.a) && tac_live_contains(a, v->after, f.a));
    assert(tac_live_contains(a, v->preserve, f.a));
    assert(!tac_live_contains(a, v->definitions, f.a));
    assert(v->call_clobbers == TAC_P1_CALL_CLOBBERS);
    assert(v->physical_writes & (1u << TAC_P1_RL));
    assert(v->physical_reads & (1u << TAC_P1_RX));
    assert(!tac_live_contains(a, v->after, root));
    for (size_t k = 0; k < tac_live_location_count(a); k++) {
        TacOperand location = tac_live_location(a, k);
        assert(location.kind != TAC_VARIABLE);
        assert(location.kind != TAC_PHYS || location.value.id != TAC_P1_RZ);
    }
    TacOpInfo info;
    assert(tac_instruction_info(store, &info) && info.memory_write && info.nonremovable);
    tac_live_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: verifica preserves y máscaras físicas.
 * Restricciones: preserves es uso lógico, sin añadir una lectura física al P1.
 */
static void preserved_destinations(void) {
    for (int feis = 0; feis < 2; feis++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_ISA_PHYSICAL);
        TacOperand d = tac_physical(4);
        TacInstruction *instruction = fixture_isa(f.entry, feis ? "feis" : "ifmov",
            d, tac_physical(5), tac_physical(6), tac_none());
        if (feis)
            instruction->selectors = (TacSelectors){0, -1, 1};
        fixture_isa(f.entry, "sw", tac_none(), tac_physical(0), d, tac_none());
        fixture_exit(f.entry);
        TacLiveAnalysis *a = analyze(&f, NULL, 0);
        const TacLiveInstruction *v = tac_live_instruction(a, instruction);
        assert(tac_live_contains(a, v->uses, d));
        assert(tac_live_contains(a, v->definitions, d));
        assert(tac_live_contains(a, v->before, d));
        assert(v->physical_writes == (1u << 4));
        assert(!(v->physical_reads & (1u << 4)));
        tac_live_free(a);
        tac_program_free(f.program);
    }
}

/* Entrada: ninguna. Salida: ninguna. Efectos: prueba candidatos alrededor de MUL/CRYPTO.
 * Restricciones: los auxiliares virtuales no son definiciones de los valores vivos.
 */
static void pseudo_effects(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    TacInstruction *mul = tac_append_pseudo(f.entry, TAC_PSEUDO_MUL, f.d, f.a, f.b,
                                           (TacSelectors){-1, -1, -1}, 0);
    TacInstruction *crypto = tac_append_pseudo(f.entry, TAC_PSEUDO_AUTH, tac_none(), f.a,
        tac_none(), (TacSelectors){-1, -1, -1}, 0);
    fixture_isa(f.entry, "add", f.d, f.a, f.b, tac_none());
    fixture_exit(f.entry);
    TacLiveAnalysis *a = analyze(&f, NULL, 0);
    assert(tac_live_instruction(a, mul)->scratch_count == 8);
    assert(tac_live_contains(a, tac_live_instruction(a, mul)->preserve, f.a));
    assert(tac_live_contains(a, tac_live_instruction(a, crypto)->preserve, f.b));
    assert(!tac_live_contains(a, tac_live_instruction(a, mul)->definitions, f.a));
    TacOpInfo info;
    assert(tac_instruction_info(crypto, &info) && info.nonremovable && info.crypto_state);
    tac_live_free(a);
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: ninguna. Efectos: verifica propiedad, clase y exclusiones.
 * Restricciones: no depende de nombres, direcciones ni estado global del semántico.
 */
static void classification(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    Scope local = {.kind = SCOPE_FUNCTION, .owner = &f.symbol, .symbols = &f.local};
    f.scope.next = &local;
    assert(tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    assert(!tac_variable_is_rename_candidate(f.function, &f.global, &f.scope));
    const SymbolKind kinds[] = {SYMBOL_PARAMETER, SYMBOL_RETURN};
    for (unsigned k = 0; k < 2; k++) {
        f.local.kind = kinds[k];
        assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    }
    f.local.kind = SYMBOL_VARIABLE;
    f.local.modifier = MODIFIER_VI;
    assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    f.local.modifier = MODIFIER_VM;
    f.local.limit_address = 20;
    assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    f.local.limit_address = -1;
    f.local.dimensions = 1;
    assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    f.local.dimensions = 0;
    local.owner = &f.global;
    assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    f.local.modifier = MODIFIER_NONE;
    local.owner = &f.symbol;
    assert(!tac_variable_is_rename_candidate(f.function, &f.local, &f.scope));
    assert(!tac_variable_is_rename_candidate(NULL, &f.local, &f.scope));
    tac_program_free(f.program);
}

/* Entrada: ninguna. Salida: cero si pasan contratos. Efectos: ejecuta casos manuales.
 * Restricciones: no se enlaza con el generador ni el frontend.
 */
int main(void) {
    diamond();
    cycle();
    global_effects();
    virtual_effects();
    preserved_destinations();
    pseudo_effects();
    classification();
    char error[128];
    assert(!tac_live_analyze(NULL, NULL, 0, error, sizeof error));
    assert(!tac_live_analyze(NULL, NULL, 0, NULL, 0));
    puts("PASS vivacidad: uniones, ciclos, raíces HIGH/ISA, globales, preserves y destruidos separados");
    return 0;
}
