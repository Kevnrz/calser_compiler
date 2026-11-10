/*
 * probar.c — seis casos manuales de representación intermedia de tres direcciones.
 * Usa: tac.h y biblioteca C. Lo usa check-tac, sin Python.
 * Referencias: Aho §6.2, §8.4.1, §10.2.3; contratos de C2.1a.
 */
#include "tac.h"
#include <assert.h>
#include <string.h>

/* Diagnóstico legible antes de abortar una prueba de forma válida. */
static void valid(TacProgram *program) {
    char error[256];
    if (!tac_validate(program, error, sizeof error)) {
        fprintf(stderr, "forma inesperadamente inválida: %s\n", error);
        assert(0);
    }
    assert(!error[0]);
}

/* Comprueba también la clase del error, sin imprimir en el golden. */
static void invalid(TacProgram *program, const char *reason) {
    char error[256];
    assert(!tac_validate(program, error, sizeof error));
    if (!strstr(error, reason)) {
        fprintf(stderr, "esperado '%s', recibido '%s'\n", reason, error);
        assert(0);
    }
}

/* Reconstrucción canónica empleada por los casos impresos. */
static void rebuild(TacFunction *function) {
    char error[256];
    if (!tac_rebuild_edges(function, error, sizeof error)) {
        fprintf(stderr, "reconstrucción fallida: %s\n", error);
        assert(0);
    }
}

/* Aritmética y bases simbólica, calculada y recibida como parámetro. */
static void straight(TacProgram *program, Symbol *symbol, Symbol *array, Symbol *param) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand t1 = tac_temp_new(function);
    TacOperand t2 = tac_temp_new(function);
    TacOperand base = tac_temp_new(function);
    TacOperand none = tac_none();
    TacOperand values[3];
    tac_append(block, TAC_COPY, t0, tac_integer(3), none);
    tac_append(block, TAC_ADD, t1, t0, tac_integer(4));
    TacInstruction *instruction = tac_append(block, TAC_MUL, t2, t0, t1);
    assert(tac_definitions(instruction, values) == 1);
    assert(tac_uses(instruction, values) == 2);
    instruction = tac_append(block, TAC_STORE, tac_variable(array), t0, t2);
    assert(tac_definitions(instruction, values) == 0);
    assert(tac_uses(instruction, values) == 2);
    assert(values[0].kind == TAC_TEMP && values[0].value.id == t0.value.id);
    instruction = tac_append(block, TAC_LOAD, t1, tac_variable(array), t0);
    assert(tac_uses(instruction, values) == 1);
    instruction = tac_append(block, TAC_ADDRESS, base, tac_variable(array), none);
    assert(tac_definitions(instruction, values) == 1);
    assert(tac_uses(instruction, values) == 0);
    instruction = tac_append(block, TAC_LOAD, t1, base, t0);
    assert(tac_uses(instruction, values) == 2);
    instruction = tac_append(block, TAC_COPY, base, tac_variable(param), none);
    assert(tac_uses(instruction, values) == 1 && values[0].value.symbol == param);
    tac_append(block, TAC_LOAD, t1, base, t0);
    tac_append(block, TAC_RETURN, none, t2, none);
    rebuild(function);
}

/* Diamante con dos predecesores de la unión. */
static void diamond(TacProgram *program, Symbol *symbol) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *test = tac_block_new(function);
    TacBlock *no = tac_block_new(function);
    TacBlock *yes = tac_block_new(function);
    TacBlock *end = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand none = tac_none();
    tac_append(test, TAC_COPY, t0, tac_integer(3), none);
    tac_append(test, TAC_IF_LT, tac_label(yes), t0, tac_integer(10));
    tac_append(no, TAC_COPY, t0, tac_integer(0), none);
    tac_append(no, TAC_GOTO, tac_label(end), none, none);
    tac_append(yes, TAC_COPY, t0, tac_integer(1), none);
    tac_append(yes, TAC_GOTO, tac_label(end), none, none);
    tac_append(end, TAC_RETURN, none, t0, none);
    rebuild(function);
    assert(end->predecessor_count == 2);
    assert(test->successors[0] == yes && test->successors[1] == no);
}

/* Lazo con cabecera y salto hacia atrás explícito. */
static void loop(TacProgram *program, Symbol *symbol) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *entry = tac_block_new(function);
    TacBlock *test = tac_block_new(function);
    TacBlock *body = tac_block_new(function);
    TacBlock *end = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand none = tac_none();
    tac_append(entry, TAC_COPY, t0, tac_integer(0), none);
    tac_append(test, TAC_IF_GE, tac_label(end), t0, tac_integer(3));
    tac_append(body, TAC_ADD, t0, t0, tac_integer(1));
    tac_append(body, TAC_GOTO, tac_label(test), none, none);
    tac_append(end, TAC_RETURN, none, t0, none);
    rebuild(function);
    assert(test->predecessor_count == 2);
}

/* PARAM desordenados; globales y las siete sentencias CRYPTO. */
static void calls(TacProgram *program, Symbol *symbol, Symbol *callee, Symbol *global) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand none = tac_none();
    TacOpInfo info;
    tac_append(block, TAC_COPY, tac_variable(global), tac_integer(5), none);
    tac_append(block, TAC_PARAM, none, tac_integer(9), tac_integer(1));
    tac_append(block, TAC_PARAM, none, tac_integer(8), tac_integer(0));
    TacInstruction *instruction = tac_append(block, TAC_CALL, t0,
        tac_function_operand(callee), tac_integer(2));
    assert(tac_instruction_info(instruction, &info));
    assert(info.uses_globals && info.defines_globals && info.barrier);
    assert(info.call_clobbers == UINT16_C(0xfff0));
    assert(info.flow == TAC_FLOW_NEXT);
    tac_append(block, TAC_COPY, tac_variable(global), tac_integer(6), none);
    tac_append(block, TAC_AUTH, none, tac_integer(-193210613), none);
    instruction = tac_append(block, TAC_KV_WRITE, none, t0, none);
    instruction->selectors.k = 0;
    instruction->selectors.w = 1;
    tac_append(block, TAC_KV_LOCK, none, none, none);
    instruction = tac_append(block, TAC_CIFRAR, tac_integer(4096),
                             tac_integer(8192), t0);
    instruction->selectors.k = 0;
    instruction = tac_append(block, TAC_DESCIFRAR, tac_integer(8192),
                             tac_integer(12288), tac_integer(1));
    instruction->selectors.k = 0;
    tac_append(block, TAC_SANC, none, tac_integer(5), none);
    tac_append(block, TAC_LOGOUT, none, none, none);
    tac_append(block, TAC_RETURN, none, t0, none);
    rebuild(function);
    function = tac_function_new(program, callee);
    block = tac_block_new(function);
    instruction = tac_append(block, TAC_RETURN, none, tac_variable(global), none);
    assert(tac_instruction_info(instruction, &info));
    assert(info.uses_globals && !info.defines_globals);
    assert(function->symbol == callee);
    rebuild(function);
}

/* WAR/WAW no SSA; las lecturas repetidas se deduplican. */
static void hazards(TacProgram *program, Symbol *symbol) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *block = tac_block_new(function);
    TacOperand t0 = tac_temp_new(function);
    TacOperand t1 = tac_temp_new(function);
    TacOperand none = tac_none();
    TacOperand values[3];
    tac_append(block, TAC_COPY, t0, tac_integer(1), none);
    tac_append(block, TAC_ADD, t1, t0, tac_integer(2));
    tac_append(block, TAC_COPY, t0, tac_integer(7), none);
    TacInstruction *instruction = tac_append(block, TAC_ADD, t1, t0, t0);
    assert(tac_uses(instruction, values) == 1);
    tac_append(block, TAC_RETURN, none, t1, none);
    rebuild(function);
}

/* Sexto caso: coincide con ejemplo.cls; RETURN de main todavía es abstracto. */
static void example(TacProgram *program, Symbol *symbol, Symbol *res) {
    TacFunction *function = tac_function_new(program, symbol);
    TacBlock *entry = tac_block_new(function);
    TacBlock *end = tac_block_new(function);
    TacBlock *body = tac_block_new(function);
    TacOperand value = tac_variable(res);
    TacOperand none = tac_none();
    tac_append(entry, TAC_COPY, value, tac_integer(3), none);
    tac_append(entry, TAC_IF_LT, tac_label(body), value, tac_integer(10));
    tac_append(end, TAC_RETURN, none, none, none);
    tac_append(body, TAC_MUL, value, value, tac_integer(2));
    tac_append(body, TAC_GOTO, tac_label(end), none, none);
    rebuild(function);
}

typedef struct {
    Symbol symbol;
    Scope scope;
    TacProgram *program;
    TacFunction *function;
    TacBlock *block;
} Fixture;

/* Fixture mínima válida, con símbolo cuya vida coincide con la prueba. */
static void fixture_init(Fixture *fixture) {
    fixture->symbol = (Symbol){.name = "contratos", .kind = SYMBOL_FUNCTION};
    fixture->scope = (Scope){.kind = SCOPE_GLOBAL};
    fixture->program = tac_program_new(&fixture->scope);
    fixture->function = tac_function_new(fixture->program, &fixture->symbol);
    fixture->block = tac_block_new(fixture->function);
    tac_append(fixture->block, TAC_RETURN, tac_none(), tac_none(), tac_none());
}

/* Clasificación por puntero; no confunde funciones ni símbolos homónimos. */
static void globals_and_properties(void) {
    Symbol global = {.name = "g", .kind = SYMBOL_VARIABLE};
    Symbol local = {.name = "g", .kind = SYMBOL_VARIABLE};
    Symbol function = {.name = "f", .kind = SYMBOL_FUNCTION, .next = &global};
    Scope scope = {.kind = SCOPE_GLOBAL, .symbols = &function};
    TacProgram *program = tac_program_new(&scope);
    assert(tac_is_global(program, tac_variable(&global)));
    assert(!tac_is_global(program, tac_variable(&local)));
    assert(!tac_is_global(program, tac_variable(&function)));
    assert(!tac_is_global(program, tac_function_operand(&function)));
    assert(!tac_is_global(NULL, tac_variable(&global)));
    assert(!tac_is_global(program, tac_variable(NULL)));
    for (int op = TAC_AUTH; op <= TAC_DESCIFRAR; op++) {
        const TacOpInfo *info = tac_op_info((TacOpcode)op);
        assert(info->nonremovable && info->barrier && info->crypto_state);
        assert(!info->writes);
        assert(info->memory_read == (op >= TAC_CIFRAR));
        assert(info->memory_write == (op >= TAC_CIFRAR));
    }
    for (int op = TAC_MUL; op <= TAC_POW; op++) {
        const TacOpInfo *info = tac_op_info((TacOpcode)op);
        assert(info->may_create_blocks);
        assert(info->may_trap == (op != TAC_MUL));
    }
    assert(tac_op_info(TAC_STORE)->nonremovable);
    assert(!tac_op_info((TacOpcode)-1));
    TacOpInfo info;
    TacOperand values[3];
    assert(!tac_instruction_info(NULL, &info));
    assert(!tac_instruction_info(NULL, NULL));
    assert(!tac_uses(NULL, values) && !tac_definitions(NULL, values));
    valid(program);
    tac_program_free(program);
    tac_program_free(NULL);
}

/* Todas las selectividades inválidas, y una dirección constante válida. */
static void crypto_errors(void) {
    Fixture fixture;
    fixture_init(&fixture);
    TacOperand none = tac_none();
    TacInstruction *instruction = tac_insert_after(fixture.block, NULL,
        TAC_KV_WRITE, none, tac_integer(1), none);
    invalid(fixture.program, "selector");
    instruction->selectors = (TacSelectors){0, 3, -1};
    valid(fixture.program);
    instruction->selectors.k = 4;
    invalid(fixture.program, "selector");
    instruction->selectors.k = -2;
    invalid(fixture.program, "selector");
    instruction->selectors.k = 0;
    instruction->selectors.w = 4;
    invalid(fixture.program, "selector");
    instruction->selectors.w = 0;
    instruction->selectors.round = 0;
    invalid(fixture.program, "selector");
    instruction->op = TAC_AUTH;
    instruction->selectors = (TacSelectors){-1, -1, -1};
    valid(fixture.program);
    instruction->selectors.k = 0;
    invalid(fixture.program, "selector");
    instruction->op = TAC_CIFRAR;
    instruction->dest = tac_integer(4096);
    instruction->src1 = tac_integer(8192);
    instruction->src2 = tac_integer(1);
    instruction->selectors = (TacSelectors){3, -1, -1};
    valid(fixture.program);
    instruction->selectors.k = -1;
    invalid(fixture.program, "selector");
    tac_program_free(fixture.program);
}

/* Argumentos por posición: desorden válido, errores de cantidad y unicidad. */
static void parameter_errors(void) {
    Fixture fixture;
    fixture_init(&fixture);
    Symbol callee = {.name = "callee", .kind = SYMBOL_FUNCTION};
    TacOperand none = tac_none();
    TacInstruction *first = tac_insert_after(fixture.block, NULL, TAC_PARAM,
        none, tac_integer(9), tac_integer(1));
    TacInstruction *second = tac_insert_after(fixture.block, first, TAC_PARAM,
        none, tac_integer(8), tac_integer(0));
    TacInstruction *call = tac_insert_after(fixture.block, second, TAC_CALL,
        none, tac_function_operand(&callee), tac_integer(2));
    valid(fixture.program);
    second->src2 = tac_integer(1);
    invalid(fixture.program, "duplicada");
    second->src2 = tac_integer(-1);
    invalid(fixture.program, "rango");
    second->src2 = tac_integer(2);
    invalid(fixture.program, "rango");
    second->src2 = tac_integer(0);
    call->src2 = tac_integer(1);
    invalid(fixture.program, "sobrantes");
    call->src2 = tac_integer(3);
    invalid(fixture.program, "faltantes");
    call->src2 = tac_integer(-1);
    invalid(fixture.program, "aridad");
    call->src2 = tac_integer(2);
    TacInstruction *interposed = tac_insert_after(fixture.block, first,
        TAC_CALL, none, tac_function_operand(&callee), tac_integer(0));
    invalid(fixture.program, "PARAM");
    assert(tac_remove(fixture.block, interposed));
    valid(fixture.program);
    assert(tac_remove(fixture.block, call));
    invalid(fixture.program, "sin CALL");
    assert(tac_remove(fixture.block, first));
    assert(tac_remove(fixture.block, second));
    tac_insert_after(fixture.block, NULL, TAC_CALL, none,
                     tac_function_operand(&callee), tac_integer(0));
    valid(fixture.program);
    tac_program_free(fixture.program);
}

/* Inserción y eliminación en todos los extremos, incluidos nodos ajenos. */
static void editing(void) {
    Fixture fixture;
    fixture_init(&fixture);
    TacBlock *block = fixture.block;
    TacOperand none = tac_none();
    TacOperand temp = tac_temp_new(fixture.function);
    assert(tac_remove(block, block->first));
    assert(!block->first && !block->last);
    TacInstruction *first = tac_insert_after(block, NULL, TAC_COPY,
        temp, tac_integer(1), none);
    TacInstruction *last = tac_append(block, TAC_RETURN, none, temp, none);
    TacInstruction *middle = tac_insert_after(block, first, TAC_NEG, temp, temp, none);
    assert(block->first == first && first->next == middle && middle->next == last);
    assert(block->last == last);
    valid(fixture.program);
    TacBlock *other = tac_block_new(fixture.function);
    TacInstruction *foreign = tac_append(other, TAC_RETURN, none, none, none);
    assert(!tac_insert_after(block, foreign, TAC_COPY, temp, temp, none));
    assert(!tac_remove(block, foreign));
    assert(!tac_remove(block, NULL));
    assert(tac_remove(block, middle));
    assert(first->next == last);
    assert(tac_remove(block, last));
    assert(block->last == first);
    assert(tac_remove(block, first));
    assert(!block->first && !block->last);
    tac_append(block, TAC_RETURN, none, none, none);
    TacBlock *inserted = tac_block_insert_after(fixture.function, block);
    tac_append(inserted, TAC_GOTO, tac_label(other), none, none);
    assert(block->next == inserted && inserted->next == other);
    TacBlock *entry = tac_block_insert_after(fixture.function, NULL);
    assert(fixture.function->entry == entry && entry->next == block);
    assert(fixture.function->last == other);
    TacBlock alien = {0};
    assert(!tac_block_insert_after(fixture.function, &alien));
    rebuild(fixture.function);
    assert(entry->successors[0] == block);
    valid(fixture.program);
    assert(tac_disconnect(entry, block));
    assert(!entry->successor_count && !block->predecessor_count);
    assert(!tac_disconnect(entry, block));
    invalid(fixture.program, "sucesores");
    assert(tac_connect(entry, block));
    assert(tac_connect(entry, block));
    assert(block->predecessor_count == 1);
    assert(tac_connect(other, other));
    assert(tac_disconnect(other, other));
    assert(!tac_connect(NULL, block));
    assert(!tac_disconnect(block, NULL));
    TacFunction alien_function = {0};
    alien.function = &alien_function;
    assert(!tac_connect(block, &alien));
    assert(!tac_disconnect(block, &alien));
    valid(fixture.program);
    tac_program_free(fixture.program);
}

/* Errores de control y reconstrucción transaccional, no solo de terminadores. */
static void graph_errors(void) {
    Fixture fixture;
    fixture_init(&fixture);
    TacBlock *entry = fixture.block;
    TacBlock *end = tac_block_new(fixture.function);
    TacOperand none = tac_none();
    TacInstruction *jump = entry->first;
    jump->op = TAC_GOTO;
    jump->dest = tac_label(end);
    TacInstruction *ret = tac_append(end, TAC_RETURN, none, none, none);
    rebuild(fixture.function);
    valid(fixture.program);
    TacBlock **old_successors = entry->successors;
    TacBlock **old_predecessors = end->predecessors;
    char error[256];
    fixture.program->global_scope = NULL;
    invalid(fixture.program, "ámbito global");
    assert(!tac_rebuild_edges(fixture.function, error, sizeof error));
    assert(strstr(error, "ámbito global"));
    assert(entry->successors == old_successors && entry->successor_count == 1);
    assert(end->predecessors == old_predecessors && end->predecessor_count == 1);
    assert(entry->successors[0] == end && end->predecessors[0] == entry);
    fixture.program->global_scope = &fixture.scope;
    valid(fixture.program); /* ámbito global vacío, con función manual válida */
    ret->op = TAC_COPY;
    ret->dest = tac_integer(3);
    assert(!tac_rebuild_edges(fixture.function, error, sizeof error));
    assert(entry->successors == old_successors && entry->successor_count == 1);
    assert(end->predecessors == old_predecessors && end->predecessor_count == 1);
    ret->op = TAC_RETURN;
    ret->dest = none;
    TacBlock alien = {0};
    jump->dest = tac_label(&alien);
    assert(!tac_rebuild_edges(fixture.function, error, sizeof error));
    assert(entry->successors == old_successors && end->predecessors == old_predecessors);
    jump->dest = tac_label(end);
    jump->op = TAC_IF_EQ;
    jump->src1 = tac_integer(0);
    jump->src2 = tac_integer(1);
    rebuild(fixture.function);
    assert(entry->successor_count == 1); /* tomado y caída coinciden */
    valid(fixture.program);
    TacInstruction *extra = tac_append(entry, TAC_RETURN, none, none, none);
    invalid(fixture.program, "al final");
    assert(tac_remove(entry, extra));
    end->predecessor_count = 0;
    invalid(fixture.program, "recíproca");
    end->predecessor_count = 1;
    ret->op = TAC_COPY;
    ret->dest = tac_temp_new(fixture.function);
    ret->src1 = tac_integer(1);
    invalid(fixture.program, "último bloque");
    ret->op = TAC_IF_EQ;
    ret->dest = tac_label(entry);
    ret->src2 = tac_integer(0);
    invalid(fixture.program, "caída");
    ret->op = TAC_GOTO;
    ret->src1 = none;
    ret->src2 = none;
    rebuild(fixture.function); /* lazo infinito: permitido */
    valid(fixture.program);
    size_t old_label = end->label;
    end->label = entry->label;
    invalid(fixture.program, "duplicada");
    end->label = old_label;
    fixture.program->stage = TAC_STAGE_ISA_VIRTUAL;
    invalid(fixture.program, "etapa");
    fixture.program->stage = TAC_STAGE_HIGH;
    fixture.function->entry = end;
    invalid(fixture.program, "entrada");
    fixture.function->entry = entry;
    assert(!tac_validate(NULL, error, sizeof error));
    assert(!tac_rebuild_edges(NULL, NULL, 0));
    for (size_t k = 1; k < 10000; k++) {
        assert(tac_temp_new(fixture.function).value.id == k);
    }
    tac_program_free(fixture.program);
}

/* Rechaza bases escalares/parámetro y valores arreglo sin materialización. */
static void array_errors(void) {
    Fixture fixture;
    fixture_init(&fixture);
    Symbol array = {.name = "a", .kind = SYMBOL_VARIABLE, .dimensions = 1};
    Symbol parameter = {.name = "p", .kind = SYMBOL_PARAMETER, .dimensions = 1};
    Symbol scalar = {.name = "x", .kind = SYMBOL_VARIABLE};
    TacOperand temp = tac_temp_new(fixture.function);
    TacInstruction *load = tac_insert_after(fixture.block, NULL, TAC_LOAD,
        temp, tac_variable(&array), tac_integer(0));
    valid(fixture.program);
    load->src1 = tac_variable(&parameter);
    invalid(fixture.program, "arreglo");
    load->src1 = tac_variable(&scalar);
    invalid(fixture.program, "arreglo");
    load->op = TAC_COPY;
    load->src1 = tac_variable(&parameter);
    load->src2 = tac_none();
    valid(fixture.program);
    load->src1 = tac_variable(&array);
    invalid(fixture.program, "arreglo");
    load->op = TAC_ADDRESS;
    valid(fixture.program);
    load->src1 = temp;
    invalid(fixture.program, "operando");
    load->op = TAC_COPY;
    load->dest = tac_variable(&parameter);
    load->src1 = temp;
    load->src2 = tac_none();
    valid(fixture.program); /* R redefine la dirección del parámetro arreglo. */
    TacOperand values[3];
    assert(tac_definitions(load, values) == 1);
    assert(values[0].value.symbol == &parameter);
    assert(tac_uses(load, values) == 1 && values[0].kind == TAC_TEMP);
    load->src1 = tac_variable(&parameter);
    valid(fixture.program); /* COPY admite leer y escribir el parámetro. */
    load->dest = tac_variable(&array);
    invalid(fixture.program, "arreglo"); /* No habilita copia escalar de arreglos declarados. */
    tac_program_free(fixture.program);
}

/* Numeración por lista, sin modificar identidades ni aristas; Aho §8.4.1. */
static void presentation_numbers(void) {
    Fixture fixture;
    fixture_init(&fixture);
    TacFunction *function = fixture.function;
    TacBlock *end = fixture.block;
    TacBlock *entry = tac_block_insert_after(function, NULL);
    TacBlock *middle = tac_block_insert_after(function, entry);
    TacOperand none = tac_none();
    tac_append(entry, TAC_IF_EQ, tac_label(end), tac_integer(1), tac_integer(0));
    tac_append(middle, TAC_GOTO, tac_label(end), none, none);
    rebuild(function);
    valid(fixture.program);
    assert(entry->label == 1 && middle->label == 2 && end->label == 0);
    unsigned char saved[3][sizeof *entry];
    memcpy(saved[0], entry, sizeof *entry);
    memcpy(saved[1], middle, sizeof *middle);
    memcpy(saved[2], end, sizeof *end);
    size_t next_label = function->next_label;
    FILE *out = tmpfile();
    assert(out);
    tac_print(out, fixture.program);
    assert(!ferror(out));
    rewind(out);
    char text[512] = {0};
    size_t length = fread(text, 1, sizeof text - 1, out);
    assert(!ferror(out) && feof(out) && length > 0);
    assert(!strcmp(text,
        "function contratos (entry B0)\n"
        "B0: successors [B2, B1]\n"
        "  if 1 == 0 goto B2\n"
        "B1: successors [B2]\n"
        "  goto B2\n"
        "B2: successors []\n"
        "  return\n\n"));
    assert(fclose(out) == 0);
    assert(function->next_label == next_label);
    assert(function->entry == entry && function->first == entry && function->last == end);
    assert(!memcmp(entry, saved[0], sizeof *entry));
    assert(!memcmp(middle, saved[1], sizeof *middle));
    assert(!memcmp(end, saved[2], sizeof *end));
    assert(entry->successors[0] == end && entry->successors[1] == middle);
    assert(middle->successors[0] == end && middle->predecessors[0] == entry);
    assert(end->predecessors[0] == entry && end->predecessors[1] == middle);
    char error[256];
    middle->successor_count = 0;
    assert(!tac_validate(fixture.program, error, sizeof error));
    assert(!strcmp(error, "B1: sucesores incorrectos"));
    middle->successor_count = 1;
    TacInstruction *extra = tac_append(middle, TAC_RETURN, none, none, none);
    assert(!tac_rebuild_edges(function, error, sizeof error));
    assert(!strcmp(error, "B1: terminador no está al final"));
    assert(tac_remove(middle, extra));
    valid(fixture.program);
    tac_program_free(fixture.program);
}

/* Las pruebas de contratos no alteran las seis impresiones de referencia. */
int main(void) {
    presentation_numbers();
    globals_and_properties();
    crypto_errors();
    parameter_errors();
    editing();
    graph_errors();
    array_errors();
    Symbol functions[] = {
        {.name = "recto", .kind = SYMBOL_FUNCTION},
        {.name = "diamante", .kind = SYMBOL_FUNCTION},
        {.name = "lazo", .kind = SYMBOL_FUNCTION},
        {.name = "llamadas", .kind = SYMBOL_FUNCTION},
        {.name = "aux", .kind = SYMBOL_FUNCTION},
        {.name = "war_waw", .kind = SYMBOL_FUNCTION},
        {.name = "main", .kind = SYMBOL_FUNCTION, .type = TYPE_VOID}
    };
    Symbol array = {.name = "a", .kind = SYMBOL_VARIABLE, .dimensions = 1};
    Symbol parameter = {.name = "p", .kind = SYMBOL_PARAMETER, .dimensions = 1};
    Symbol global = {.name = "g", .kind = SYMBOL_VARIABLE, .next = &array};
    Symbol res = {.name = "res", .kind = SYMBOL_VARIABLE};
    array.next = functions;
    for (size_t k = 0; k + 1 < sizeof functions / sizeof functions[0]; k++) {
        functions[k].next = &functions[k + 1];
    }
    Scope scope = {.kind = SCOPE_GLOBAL, .symbols = &global};
    TacProgram *program = tac_program_new(&scope);
    straight(program, &functions[0], &array, &parameter);
    diamond(program, &functions[1]);
    loop(program, &functions[2]);
    calls(program, &functions[3], &functions[4], &global);
    hazards(program, &functions[5]);
    example(program, &functions[6], &res);
    valid(program);
    tac_print(stdout, program);
    tac_program_free(program);
    return ferror(stdout) ? 1 : 0;
}
