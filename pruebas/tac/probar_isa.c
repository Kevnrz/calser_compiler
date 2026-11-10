/*
 * probar_isa.c — nivel ISA de representación intermedia de tres direcciones.
 * Usa: tac.h e isa_table de etapa A. Lo usa check-tac, sin Python.
 * Referencias: Aho §10.2.3; isa.md §6, §8, §9; ABI de ccom P1.
 */
#include "tac.h"
#include <assert.h>
#include <string.h>

/* Abreviatura local: los selectores empiezan ausentes. */
static TacInstruction *emit(TacBlock *block, const char *name,
                             TacOperand dest, TacOperand src1, TacOperand src2) {
    return tac_append_isa(block, isa_find(name), dest, src1, src2,
                          0, tac_none(), (TacSelectors){-1, -1, -1});
}

/* Valida y conserva diagnósticos de fallos inesperados. */
static void valid(TacProgram *program) {
    char error[256];
    if (!tac_validate(program, error, sizeof error)) {
        fprintf(stderr, "ISA inesperadamente inválida: %s\n", error);
        assert(0);
    }
    assert(!error[0]);
}

/* Rechazo con una causa concreta. */
static void invalid(TacProgram *program, const char *reason) {
    char error[256];
    assert(!tac_validate(program, error, sizeof error));
    if (!strstr(error, reason)) {
        fprintf(stderr, "esperado '%s', recibido '%s'\n", reason, error);
        assert(0);
    }
}

/* Usa el mismo cálculo de sucesores que valida los programas ISA. */
static void rebuild(TacFunction *function) {
    char error[256];
    if (!tac_rebuild_edges(function, error, sizeof error)) {
        fprintf(stderr, "ISA: %s\n", error);
        assert(0);
    }
}

/* Consulta valores explícitos y resúmenes que no caben en tres operandos. */
static void queries(TacInstruction *ifmov, TacInstruction *feis,
                     TacInstruction *call, TacInstruction *ret) {
    TacOperand values[3];
    TacOpInfo info;
    assert(tac_uses(ifmov, values) == 3);
    assert(values[0].value.id == ifmov->dest.value.id);
    assert(tac_definitions(ifmov, values) == 1);
    assert(tac_uses(feis, values) == 3);
    assert(values[0].value.id == feis->dest.value.id);
    assert(tac_definitions(feis, values) == 1);
    assert(tac_instruction_info(call, &info));
    assert(info.flow == TAC_FLOW_NEXT && info.barrier && info.nonremovable);
    assert(info.uses_globals && info.defines_globals);
    assert(info.call_clobbers == UINT16_C(0xfff0));
    assert(info.implicit_phys_reads == (1u << TAC_P1_RX));
    assert(tac_definitions(call, values) == 1);
    assert(values[0].kind == TAC_PHYS && values[0].value.id == TAC_P1_RL);
    assert(tac_instruction_info(ret, &info));
    assert(info.flow == TAC_FLOW_RETURN && info.uses_globals);
    assert(!tac_definitions(ret, values));
    assert(tac_uses(ret, values) == 1 && values[0].value.id == TAC_P1_RL);
}

/* Las tres etapas, rangos y formas restringidas de llamada/retorno. */
static void invalid_forms(void) {
    Symbol symbol = {.name = "errores_isa", .kind = SYMBOL_FUNCTION};
    Symbol callee = {.name = "callee", .kind = SYMBOL_FUNCTION};
    Scope scope = {.kind = SCOPE_GLOBAL};
    TacProgram *program = tac_program_new(&scope);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    TacFunction *function = tac_function_new(program, &symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand none = tac_none();
    TacOperand temp = tac_temp_new(function);
    TacOperand rz = tac_physical(TAC_P1_RZ);
    TacOperand rl = tac_physical(TAC_P1_RL);
    TacInstruction *move = emit(block, "mov", temp, none, tac_integer(1));
    TacInstruction *ret = emit(block, "jefR", rz, rl, none);
    valid(program);
    program->stage = TAC_STAGE_HIGH;
    invalid(program, "etapa");
    program->stage = TAC_STAGE_ISA_PHYSICAL;
    invalid(program, "etapa");
    move->dest = tac_physical(4);
    valid(program);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    invalid(program, "etapa");
    move->dest = tac_physical(3);
    invalid(program, "etapa");
    move->dest = tac_physical(16);
    invalid(program, "etapa");
    move->dest = temp;
    move->src2 = tac_integer(131072);
    invalid(program, "18 bits");
    move->src2 = tac_integer(-131073);
    invalid(program, "18 bits");
    move->src2 = tac_integer(-131072);
    valid(program);
    move->selectors.k = 0;
    invalid(program, "selector");
    move->selectors.k = -1;
    move->isa.displacement = 1;
    invalid(program, "desplazamiento");
    move->isa.displacement = 0;
    move->isa.instruction = NULL;
    invalid(program, "fila ISA");
    move->isa.instruction = isa_find("mov");
    ret->dest = rl;
    invalid(program, "jefR");
    ret->dest = rz;
    ret->src1 = temp;
    invalid(program, "jefR");
    ret->src1 = rl;
    TacInstruction *call = tac_insert_after(block, move, TAC_ISA, rl, none, none);
    call->isa.instruction = isa_find("jas");
    call->isa.target = tac_function_operand(&callee);
    valid(program);
    call->dest = temp;
    invalid(program, "jas");
    call->dest = rl;
    call->isa.target = tac_label(block);
    invalid(program, "jas");
    call->isa.target = tac_integer(5);
    invalid(program, "jas");
    call->isa.target = tac_function_operand(&callee);
    call->isa.displacement = 2;
    invalid(program, "desplazamiento");
    call->isa.displacement = 0;
    call->op = TAC_COPY;
    invalid(program, "etapa");
    call->op = TAC_ISA;
    assert(tac_remove(block, call));
    assert(tac_remove(block, ret));
    invalid(program, "último bloque");
    move->isa.instruction = isa_find("jef");
    move->dest = none;
    move->src2 = none;
    move->isa.target = tac_integer(0);
    invalid(program, "LABEL");
    move->isa.target = tac_label(block);
    rebuild(function);
    valid(program);
    assert(block->successor_count == 1 && block->successors[0] == block);
    tac_program_free(program);
}

/* Selectores por forma ISA, desplazamiento y destino previo preservado. */
static void selectors_and_memory(void) {
    Symbol symbol = {.name = "selectores", .kind = SYMBOL_FUNCTION};
    Scope scope = {.kind = SCOPE_GLOBAL};
    TacProgram *program = tac_program_new(&scope);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    TacFunction *function = tac_function_new(program, &symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand t1 = tac_temp_new(function);
    TacOperand t2 = tac_temp_new(function);
    TacOperand none = tac_none();
    TacOperand values[3];
    TacInstruction *feis = emit(block, "feis", t0, t1, t2);
    emit(block, "jefR", tac_physical(0), tac_physical(1), none);
    invalid(program, "selector");
    feis->selectors = (TacSelectors){0, -1, 3};
    valid(program);
    feis->selectors.round = 4;
    invalid(program, "selector");
    feis->selectors.round = 3;
    feis->selectors.w = 0;
    invalid(program, "selector");
    feis->selectors.w = -1;
    feis->src2 = t0;
    assert(tac_uses(feis, values) == 2); /* DEST coincide con SRC2 */
    feis->dest = tac_physical(0);
    assert(!tac_definitions(feis, values));
    assert(tac_uses(feis, values) == 2); /* rz no es destino preservable */
    valid(program);
    feis->isa.instruction = isa_find("kv_write");
    feis->dest = none;
    feis->src2 = none;
    feis->selectors = (TacSelectors){3, 0, -1};
    valid(program);
    feis->selectors.w = -1;
    invalid(program, "selector");
    feis->isa.instruction = isa_find("lw");
    feis->dest = t0;
    feis->selectors = (TacSelectors){-1, -1, -1};
    feis->isa.displacement = 131071;
    valid(program);
    feis->isa.displacement = 131072;
    invalid(program, "desplazamiento");
    feis->isa.displacement = -131072;
    valid(program);
    feis->isa.instruction = isa_find("sw");
    feis->dest = none;
    feis->src2 = t2;
    assert(!tac_definitions(feis, values));
    assert(tac_uses(feis, values) == 2);
    valid(program);
    tac_program_free(program);
}

/* Caso ISA virtual: formas ALU, memoria, CRYPTO, llamada interior y diamante. */
static void virtual_case(void) {
    Symbol symbol = {.name = "isa_virtual", .kind = SYMBOL_FUNCTION};
    Symbol callee = {.name = "aux", .kind = SYMBOL_FUNCTION};
    Scope scope = {.kind = SCOPE_GLOBAL};
    TacProgram *program = tac_program_new(&scope);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    TacFunction *function = tac_function_new(program, &symbol);
    TacBlock *entry = tac_block_new(function);
    TacBlock *fall = tac_block_new(function);
    TacBlock *taken = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand t1 = tac_temp_new(function);
    TacOperand t2 = tac_temp_new(function);
    TacOperand none = tac_none();
    emit(entry, "mov", t0, none, tac_integer(3));
    emit(entry, "saddi", t1, t0, tac_integer(1));
    emit(entry, "sll", t2, t0, t1);
    emit(entry, "slt", t1, t0, t2);
    TacInstruction *ifmov = emit(entry, "ifmov", t2, t0, t1);
    TacInstruction *instruction = emit(entry, "lw", t0, tac_physical(2), none);
    instruction->isa.displacement = 4;
    instruction = emit(entry, "sw", none, tac_physical(2), t0);
    instruction->isa.displacement = 8;
    instruction = emit(entry, "kv_write", none, t0, none);
    instruction->selectors = (TacSelectors){0, 1, -1};
    TacInstruction *feis = emit(entry, "feis", t2, t0, t1);
    feis->selectors = (TacSelectors){0, -1, 2};
    TacInstruction *call = emit(entry, "jas", tac_physical(1), none, none);
    call->isa.target = tac_function_operand(&callee);
    emit(entry, "add", t1, t0, t2); /* después de jas dentro del bloque */
    instruction = emit(entry, "bge", none, t1, t0);
    instruction->isa.target = tac_label(taken);
    emit(fall, "auth", none, t0, none);
    instruction = emit(fall, "jef", none, none, none);
    instruction->isa.target = tac_label(taken);
    TacInstruction *ret = emit(taken, "jefR", tac_physical(0), tac_physical(1), none);
    queries(ifmov, feis, call, ret);
    rebuild(function);
    valid(program);
    assert(entry->successors[0] == taken && entry->successors[1] == fall);
    tac_print(stdout, program);
    tac_program_free(program);
}

/* Las mismas identidades físicas no son IDs de temporales virtuales. */
static void physical_case(void) {
    Symbol symbol = {.name = "isa_fisica", .kind = SYMBOL_FUNCTION};
    Scope scope = {.kind = SCOPE_GLOBAL};
    TacProgram *program = tac_program_new(&scope);
    program->stage = TAC_STAGE_ISA_PHYSICAL;
    TacFunction *function = tac_function_new(program, &symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand none = tac_none();
    TacOperand r4 = tac_physical(4);
    TacOperand r5 = tac_physical(5);
    TacOperand r6 = tac_physical(6);
    emit(block, "mov", r4, none, tac_integer(3));
    emit(block, "xori", r5, r4, tac_integer(7));
    TacInstruction *instruction = emit(block, "lb", r6, r4, none);
    instruction->isa.displacement = -4;
    emit(block, "sb", none, r4, r6);
    emit(block, "ifmov", r6, r4, r5);
    emit(block, "logout", none, none, none);
    emit(block, "kv_lock", none, none, none);
    emit(block, "sanc", none, r4, none);
    emit(block, "nopi", none, none, none);
    emit(block, "jefR", tac_physical(0), tac_physical(1), none);
    rebuild(function);
    valid(program);
    tac_print(stdout, program);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    invalid(program, "etapa");
    tac_program_free(program);
}

/* Verifica el texto realmente impreso, sin incorporarlo al golden. */
static void printed_names(TacStage stage) {
    static const char *const aliases[] = {
        "rz", "rl", "rx", "rg", "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7", "t8", "t9", "t10", "t11"
    };
    Scope scope = {.kind = SCOPE_GLOBAL};
    Symbol symbol = {.name = "nombres", .kind = SYMBOL_FUNCTION};
    TacProgram *program = tac_program_new(&scope);
    program->stage = stage;
    TacFunction *function = tac_function_new(program, &symbol);
    TacBlock *block = tac_block_new(function);
    int physical = stage == TAC_STAGE_ISA_PHYSICAL;
    int count = physical ? 16 : 13; /* incluye v0, v11 y v12 */
    for (int i = 0; i < count; i++) {
        TacOperand dest = physical ? tac_physical((unsigned)i) : tac_temp_new(function);
        emit(block, "mov", dest, tac_none(), tac_integer(i));
    }
    emit(block, "jefR", tac_physical(0), tac_physical(1), tac_none());
    valid(program);
    FILE *out = tmpfile();
    assert(out);
    tac_print(out, program);
    assert(!ferror(out));
    rewind(out);
    char line[128];
    assert(fgets(line, sizeof line, out)); /* cabecera de función */
    assert(fgets(line, sizeof line, out)); /* cabecera de bloque */
    for (int i = 0; i < count; i++) {
        char name[32];
        char expected[32];
        int immediate = -1;
        assert(fgets(line, sizeof line, out));
        assert(sscanf(line, " mov %31[^,], %d", name, &immediate) == 2);
        assert(immediate == i);
        if (physical) {
            assert(!strcmp(name, aliases[i]));
            assert(isa_parse_register(name) == i);
        } else {
            snprintf(expected, sizeof expected, "v%d", i);
            assert(!strcmp(name, expected));
            assert(isa_parse_register(name) == -1);
        }
    }
    assert(fgets(line, sizeof line, out));
    assert(!strcmp(line, "  jefR rz, rl\n"));
    assert(fclose(out) == 0);
    tac_program_free(program);
}

/* Imprime solo casos válidos; el resto se verifica con aserciones. */
int main(void) {
    printed_names(TAC_STAGE_ISA_VIRTUAL);
    printed_names(TAC_STAGE_ISA_PHYSICAL);
    invalid_forms();
    selectors_and_memory();
    virtual_case();
    physical_case();
    return ferror(stdout) ? 1 : 0;
}
