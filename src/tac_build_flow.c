/*
 * tac_build_flow.c — corto circuito, C/A/residual, L, S, E y R.
 * Usa: tac_build_internal.h. Referencias: Aho §6.6 y §8.4.1.
 * Reserva destinos estructurados y bloques de caída; no necesita backpatching.
 */
#include "tac_build_internal.h"
#include "tac_loops.h"
#include <stdlib.h>

/* Materializa el lógico mediante flujo, nunca mediante AND/OR de evaluación plena. */
TacOperand tb_short_circuit(TacBuilder *builder, ASTNode *node) {
    TacOperand result = tb_expression(builder, tb_child(node, 0));
    result = tb_value(builder, node, TAC_COPY, result, tac_none());
    TacBlock *end = tb_reserve_block(builder);
    TacBlock *right = tb_reserve_block(builder);
    tb_emit(builder, node, node->kind == AST_AND ? TAC_IF_EQ : TAC_IF_NE,
            tac_label(end), result, tac_integer(0));
    builder->block = right;
    TacOperand value = tb_expression(builder, tb_child(node, 1));
    tb_emit(builder, node, TAC_COPY, result, value, tac_none());
    tb_jump(builder, node, end);
    builder->block = end;
    return result;
}

/* Una prueba y su cuerpo; la caída va al cuerpo y el salto a la siguiente prueba. */
static void conditional_arm(TacBuilder *builder, ASTNode *condition,
                             ASTNode *body, TacBlock *end) {
    TacOperand value = tb_expression(builder, condition);
    TacBlock *following = tb_reserve_block(builder);
    TacBlock *taken = tb_reserve_block(builder);
    tb_emit(builder, condition, TAC_IF_EQ, tac_label(following), value, tac_integer(0));
    builder->block = taken;
    tb_statements(builder, body);
    tb_jump(builder, condition, end);
    builder->block = following;
}

/* Las alternativas solo se evalúan al fallar las pruebas anteriores. */
static void conditional(TacBuilder *builder, ASTNode *node) {
    TacBlock *end = tb_reserve_block(builder);
    conditional_arm(builder, tb_child(node, 0), tb_child(node, 1), end);
    ASTNode *alternatives = ast_find_child(node, AST_ALTERNATIVES);
    for (ASTNode *arm = tb_child(alternatives, 0); arm; arm = arm->next) {
        conditional_arm(builder, tb_child(arm, 0), tb_child(arm, 1), end);
    }
    ASTNode *residual = ast_find_child(node, AST_RESIDUAL);
    if (residual) {
        tb_statements(builder, residual->child);
    }
    tb_jump(builder, node, end);
    builder->block = end;
}

/* Cada entrada a la cabecera vuelve a evaluar toda la condición, incluidas llamadas. */
static void loop_l(TacBuilder *builder, ASTNode *node) {
    TacBlock *test = tb_reserve_block(builder);
    tb_jump(builder, node, test);
    builder->block = test;
    TacOperand value = tb_expression(builder, tb_child(node, 0));
    TacBlock *end = tb_reserve_block(builder);
    TacBlock *body = tb_reserve_block(builder);
    tb_emit(builder, node, TAC_IF_EQ, tac_label(end), value, tac_integer(0));
    builder->block = body;
    size_t depth = ++builder->loop_depth;
    tb_statements(builder, tb_child(node, 1));
    builder->loop_depth--;
    TacBlock *latch = builder->block;
    tb_jump(builder, node, test);
    TacLoopMark mark = {.kind = TAC_LOOP_L, .header = test, .body = body,
                        .latch = latch, .structural_depth = depth};
    tac_loop_mark_add(builder->function, &mark);
    builder->block = end;
}

/* Símbolo privado en la casilla existente; no reserva memoria ni altera ámbitos. */
static Symbol *loop_limit(TacBuilder *builder, const Symbol *control) {
    char name[64];
    snprintf(name, sizeof name, "$limite_S%zu", builder->next_auxiliary++);
    Symbol *limit = symbol_create(name, SYMBOL_VARIABLE, TYPE_INT);
    limit->address = control->limit_address;
    limit->width = 4;
    limit->next = builder->result->auxiliary_symbols;
    builder->result->auxiliary_symbols = limit;
    return limit;
}

/* Fin exclusivo capturado antes de escribir el control; incremento con signo. */
static void loop_s(TacBuilder *builder, ASTNode *node) {
    const Symbol *control = tb_symbol(tb_child(node, 0));
    TacOperand start = tb_expression(builder, tb_child(node, 1));
    TacOperand finish = tb_expression(builder, tb_child(node, 2));
    Symbol *limit = loop_limit(builder, control);
    int64_t step = tb_child(node, 3)->number;
    tb_emit(builder, node, TAC_COPY, tac_variable(control), start, tac_none());
    tb_emit(builder, node, TAC_COPY, tac_variable(limit), finish, tac_none());
    TacBlock *test = tb_reserve_block(builder);
    tb_jump(builder, node, test);
    builder->block = test;
    TacBlock *end = tb_reserve_block(builder);
    TacBlock *body = tb_reserve_block(builder);
    tb_emit(builder, node, step > 0 ? TAC_IF_GE : TAC_IF_LE, tac_label(end),
            tac_variable(control), tac_variable(limit));
    builder->block = body;
    size_t depth = ++builder->loop_depth;
    tb_statements(builder, tb_child(node, 4));
    builder->loop_depth--;
    TacBlock *latch = builder->block;
    if (builder->block) {
        tb_emit(builder, node, TAC_ADD, tac_variable(control),
                tac_variable(control), tac_integer(step));
        tb_jump(builder, node, test);
    }
    TacLoopMark mark = {.kind = TAC_LOOP_S, .header = test, .body = body,
        .latch = latch, .structural_depth = depth, .control = control, .limit = limit,
        .initial = start, .bound = finish, .step = step};
    tac_loop_mark_add(builder->function, &mark);
    builder->block = end;
}

/* Captura todos los argumentos de R antes de redefinir incluso parámetros arreglo. */
static void rewind_function(TacBuilder *builder, ASTNode *node) {
    const Symbol *function = builder->function->symbol;
    size_t count = (size_t)function->parameter_count;
    TacOperand *values = tb_allocate(count * sizeof *values);
    ASTNode *argument = tb_child(ast_find_child(node, AST_ARGUMENTS), 0);
    for (size_t i = 0; i < count; i++, argument = argument->next) {
        values[i] = function->param_list[i]->dimensions ?
                    tb_array_base(builder, argument) : tb_expression(builder, argument);
    }
    for (size_t i = 0; i < count; i++) {
        tb_emit(builder, node, TAC_COPY, tac_variable(function->param_list[i]),
                values[i], tac_none());
    }
    free(values);
    tb_jump(builder, node, builder->body);
}

/* main tiene RETURN abstracto sin valor; C2.3 debe bajarlo a fin del programa. */
void tb_return(TacBuilder *builder, ASTNode *node) {
    TacOperand value = builder->return_symbol ?
                       tac_variable(builder->return_symbol) : tac_none();
    tb_emit(builder, node, TAC_RETURN, tac_none(), value, tac_none());
    builder->block = NULL;
}

/* Despacho de transferencias; nunca reinicia retorno ni enlace al traducir R. */
void tb_flow(TacBuilder *builder, ASTNode *node) {
    switch (node->kind) {
        case AST_CONDITIONAL:
            conditional(builder, node);
            break;
        case AST_LOOP_L:
            loop_l(builder, node);
            break;
        case AST_LOOP_S:
            loop_s(builder, node);
            break;
        case AST_EMIT:
            tb_return(builder, node);
            break;
        case AST_REWIND:
            rewind_function(builder, node);
            break;
        default:
            tb_fail(builder, node, "sentencia no soportada");
            break;
    }
}
