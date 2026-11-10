/*
 * probar.c — casos manuales y contratos de representación intermedia de tres direcciones.
 * Usa: tac.h, assert.h. Lo usa make check-tac.
 * Referencias: Aho §6.2, §8.4; enunciado §4.1.
 */
#include "tac.h"
#include <assert.h>
#include <string.h>

/* emit — abreviatura local para cuádruplas, sin estado global. */
static TacInstruction *emit(TacBlock *b, TacOpcode op, TacOperand d, TacOperand a, TacOperand c) {
    return tac_append(b, op, d, a, c);
}
/* valid — verifica forma antes de imprimir cualquier caso de referencia. */
static void valid(const TacProgram *p) {
    char error[256];
    if (!tac_validate(p, error, sizeof error)) { fprintf(stderr, "%s\n", error); assert(0); }
    assert(error[0] == '\0');
}
/* straight — aritmética, memoria indexada y consultas de identidad. */
static void straight(TacProgram *p, Symbol *array) {
    TacFunction *f = tac_function_new(p, "recto");
    TacBlock *b = tac_block_new(f);
    TacOperand t0 = tac_temp_new(f), t1 = tac_temp_new(f), t2 = tac_temp_new(f), n = tac_none(), out[3];
    emit(b, TAC_COPY, t0, tac_integer(3), n);
    emit(b, TAC_ADD, t1, t0, tac_integer(4));
    TacInstruction *i = emit(b, TAC_MUL, t2, t0, t1);
    assert(tac_definitions(i, out) == 1 && out[0].value.id == 2);
    assert(tac_uses(i, out) == 2 && out[0].value.id == 0 && out[1].value.id == 1);
    i = emit(b, TAC_STORE, tac_variable(array), t0, t2);
    assert(tac_definitions(i, out) == 0 && tac_uses(i, out) == 3);
    i = emit(b, TAC_LOAD, t1, tac_variable(array), t0);
    assert(tac_definitions(i, out) == 1 && tac_uses(i, out) == 2);
    emit(b, TAC_RETURN, n, t2, n);
}
/* diamond — rama verdadera explícita y caída falsa, unión con dos predecesores. */
static void diamond(TacProgram *p) {
    TacFunction *f = tac_function_new(p, "diamante");
    TacBlock *a = tac_block_new(f), *no = tac_block_new(f), *yes = tac_block_new(f), *end = tac_block_new(f);
    TacOperand t = tac_temp_new(f), n = tac_none();
    emit(a, TAC_COPY, t, tac_integer(3), n);
    emit(a, TAC_IF_LT, tac_label(yes), t, tac_integer(10));
    emit(no, TAC_COPY, t, tac_integer(0), n); emit(no, TAC_GOTO, tac_label(end), n, n);
    emit(yes, TAC_COPY, t, tac_integer(1), n); emit(yes, TAC_GOTO, tac_label(end), n, n);
    emit(end, TAC_RETURN, n, t, n);
    assert(tac_connect(a, yes) && tac_connect(a, no));
    assert(tac_connect(no, end) && tac_connect(yes, end));
    assert(end->predecessor_count == 2);
    assert(tac_connect(no, end) && end->predecessor_count == 2);
}
/* loop — cabecera con predecesor del cuerpo y salto hacia atrás. */
static void loop(TacProgram *p) {
    TacFunction *f = tac_function_new(p, "lazo");
    TacBlock *a = tac_block_new(f), *head = tac_block_new(f), *body = tac_block_new(f), *end = tac_block_new(f);
    TacOperand t = tac_temp_new(f), n = tac_none();
    emit(a, TAC_COPY, t, tac_integer(0), n);
    emit(head, TAC_IF_GE, tac_label(end), t, tac_integer(3));
    emit(body, TAC_ADD, t, t, tac_integer(1));
    emit(body, TAC_GOTO, tac_label(head), n, n);
    emit(end, TAC_RETURN, n, t, n);
    assert(tac_connect(a, head) && tac_connect(head, end) && tac_connect(head, body));
    assert(tac_connect(body, head) && head->predecessor_count == 2);
}
/* calls — CALL permanece dentro del bloque; memoria no eliminable de CRYPTO. */
static void calls(TacProgram *p, Symbol *array, Symbol *callee) {
    TacFunction *f = tac_function_new(p, "llamadas");
    TacBlock *b = tac_block_new(f);
    TacOperand t = tac_temp_new(f), n = tac_none(), out[3];
    TacInstruction *i = emit(b, TAC_PARAM, n, tac_integer(8), n);
    assert(tac_uses(i, out) == 0);
    i = emit(b, TAC_CALL, t, tac_variable(callee), tac_integer(1));
    assert(tac_definitions(i, out) == 1 && tac_uses(i, out) == 0);
    i = emit(b, TAC_CRYPTO, tac_variable(array), tac_variable(array), t);
    assert(tac_uses(i, out) == 2 && tac_definitions(i, out) == 0);
    emit(b, TAC_RETURN, n, t, n);
    f = tac_function_new(p, "aux"); b = tac_block_new(f);
    emit(b, TAC_RETURN, n, tac_integer(8), n);
}
/* hazards — WAR: lectura t0 antes de redefinir; WAW: dos escrituras de t1. */
static void hazards(TacProgram *p) {
    TacFunction *f = tac_function_new(p, "war_waw"); TacBlock *b = tac_block_new(f);
    TacOperand t0 = tac_temp_new(f), t1 = tac_temp_new(f), n = tac_none(), out[3];
    emit(b, TAC_COPY, t0, tac_integer(1), n);
    emit(b, TAC_ADD, t1, t0, tac_integer(2));
    emit(b, TAC_COPY, t0, tac_integer(7), n);
    TacInstruction *i = emit(b, TAC_ADD, t1, t0, t0);
    assert(tac_uses(i, out) == 1 && out[0].value.id == 0);
    emit(b, TAC_RETURN, n, t1, n);
}
/* invalid — rechazos independientes y restauración tras cada mutación. */
static void invalid(void) {
    TacProgram *p = tac_program_new(); TacFunction *f = tac_function_new(p, "invalid");
    TacBlock *a = tac_block_new(f), *b = tac_block_new(f);
    TacOperand n = tac_none(), t = tac_temp_new(f), out[3]; char error[256];
    TacInstruction *jump = emit(a, TAC_GOTO, tac_label(b), n, n);
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "sucesores"));
    assert(tac_connect(a, b)); valid(p);
    jump->dest.value.id = 999;
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "etiqueta"));
    jump->dest = tac_label(b);
    TacInstruction *extra = emit(a, TAC_COPY, t, tac_integer(1), n);
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "final"));
    jump->next = NULL; a->last = jump;
    /* extra se libera al volverlo a enlazar antes de liberar propietarios. */
    jump->op = TAC_IF_EQ; jump->src1 = t; jump->src2 = tac_integer(0); valid(p);
    assert(a->successor_count == 1); /* destino = caída */
    jump->op = TAC_RETURN; jump->dest = n; jump->src1 = n; jump->src2 = n;
    assert(!tac_validate(p, error, sizeof error));
    jump->op = TAC_GOTO; jump->dest = tac_label(b);
    b->predecessor_count = 0;
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "recíproca"));
    b->predecessor_count = 1;
    extra->op = TAC_COPY; extra->dest = tac_integer(0);
    jump->next = extra; a->last = extra;
    jump->op = TAC_COPY; jump->dest = t; jump->src1 = tac_integer(0);
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "operando"));
    extra->dest = t; valid(p);
    TacFunction *other = tac_function_new(p, "other"); TacBlock *c = tac_block_new(other);
    assert(!tac_connect(a, c));
    emit(c, TAC_RETURN, n, n, n); valid(p);
    assert(!tac_op_info((TacOpcode)-1));
    assert(tac_uses(NULL, out) == 0 && tac_definitions(NULL, out) == 0);
    assert(!tac_validate(NULL, error, sizeof error));
    assert(!tac_validate(NULL, NULL, 0));
    /* No hay techo de doce temporales físicos. */
    for (size_t k = 1; k < 10000; ++k) assert(tac_temp_new(f).value.id == k);
    tac_program_free(p); tac_program_free(NULL);
}
/* properties — todas las operaciones tienen tabla y máscaras coherentes. */
static void properties(void) {
    for (int op = 0; op < TAC_OP_COUNT; ++op) {
        const TacOpInfo *i = tac_op_info((TacOpcode)op);
        assert(i && i->text && !(i->reads & ~7u) && !(i->writes & ~7u));
        assert(i->allowed[0] && i->allowed[1] && i->allowed[2]);
    }
    const TacOpInfo *i = tac_op_info(TAC_CRYPTO);
    assert(i->memory_read && i->memory_write && i->barrier && i->side_effect);
    i = tac_op_info(TAC_CALL); assert(i->barrier && i->side_effect && i->flow == TAC_FLOW_NEXT);
    i = tac_op_info(TAC_MUL); assert(!i->barrier && !i->side_effect && i->writes == TAC_DEST);
}
/* queries — valores escalares, llamada void y retorno; símbolo por identidad. */
static void queries(void) {
    Symbol a = {.name = "x", .kind = SYMBOL_VARIABLE};
    Symbol b = {.name = "x", .kind = SYMBOL_VARIABLE};
    Symbol callee = {.name = "callee", .kind = SYMBOL_FUNCTION};
    TacOperand out[3], n = tac_none();
    TacInstruction i = {.op = TAC_ADD, .dest = tac_variable(&a),
                        .src1 = tac_variable(&a), .src2 = tac_variable(&b)};
    assert(tac_uses(&i, out) == 2 && out[0].value.symbol == &a && out[1].value.symbol == &b);
    assert(tac_definitions(&i, out) == 1 && out[0].value.symbol == &a);
    i.op = TAC_NEG; i.src2 = n;
    assert(tac_uses(&i, out) == 1 && tac_definitions(&i, out) == 1);
    i.op = TAC_RETURN; i.dest = n;
    assert(tac_uses(&i, out) == 1 && tac_definitions(&i, out) == 0);
    i.src1 = n; assert(tac_uses(&i, out) == 0);
    i.op = TAC_CALL; i.src1 = tac_variable(&callee); i.src2 = tac_integer(0);
    assert(tac_uses(&i, out) == 0 && tac_definitions(&i, out) == 0);
    i.op = (TacOpcode)TAC_OP_COUNT;
    assert(tac_uses(&i, out) == 0 && tac_definitions(&i, out) == 0);
}
/* graph_errors — etiquetas duplicadas, caída ausente y orden de ramas. */
static void graph_errors(void) {
    TacProgram *p = tac_program_new(); TacFunction *f = tac_function_new(p, "graph");
    TacBlock *a = tac_block_new(f), *b = tac_block_new(f), *c = tac_block_new(f);
    TacOperand n = tac_none(); char error[256];
    emit(a, TAC_IF_EQ, tac_label(c), tac_integer(0), tac_integer(1));
    emit(b, TAC_RETURN, n, n, n);
    TacInstruction *i = emit(c, TAC_RETURN, n, n, n);
    assert(tac_connect(a, b) && tac_connect(a, c));
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "sucesor"));
    a->successors[0] = c; a->successors[1] = b; valid(p);
    b->label = a->label;
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "duplicada"));
    b->label = 1; valid(p);
    f->entry = b;
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "entrada"));
    f->entry = a;
    i->op = TAC_IF_EQ; i->dest = tac_label(c); i->src1 = i->src2 = tac_integer(0);
    assert(tac_connect(c, c));
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "caída"));
    i->op = TAC_GOTO; i->src1 = i->src2 = n; valid(p); /* lazo a sí mismo */
    i->op = (TacOpcode)TAC_OP_COUNT;
    assert(!tac_validate(p, error, sizeof error) && strstr(error, "operación"));
    tac_program_free(p);
}
/* main — salida exclusiva de los cinco casos para comparación exacta. */
int main(void) {
    Symbol array = {.name = "a", .kind = SYMBOL_VARIABLE};
    Symbol callee = {.name = "aux", .kind = SYMBOL_FUNCTION};
    TacProgram *p = tac_program_new();
    properties(); invalid(); queries(); graph_errors();
    straight(p, &array); diamond(p); loop(p); calls(p, &array, &callee); hazards(p);
    valid(p); tac_print(stdout, p); tac_program_free(p);
    return ferror(stdout) ? 1 : 0;
}
