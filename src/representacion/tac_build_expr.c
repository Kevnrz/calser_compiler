/*
 * tac_build_expr.c — valores, argumentos y direcciones desde el AST validado.
 * Usa: tac_build_internal.h. No usa el generador ni expande operaciones a ISA.
 * Referencias: Aho §6.4 y §6.4.3; corto circuito en tac_build_flow.c (§6.6).
 */
#include "tac_build_internal.h"
#include <stdlib.h>

/* Relación explícita AST/TAC, independiente del orden numérico de los enums. */
static TacOpcode binary_opcode(ASTKind kind) {
    switch (kind) {
        case AST_ADD: return TAC_ADD;
        case AST_SUB: return TAC_SUB;
        case AST_MUL: return TAC_MUL;
        case AST_DIV: return TAC_DIV;
        case AST_MOD: return TAC_MOD;
        case AST_POWER: return TAC_POW;
        case AST_EQUAL: return TAC_EQ;
        case AST_NOT_EQUAL: return TAC_NE;
        case AST_LESS: return TAC_LT;
        case AST_LESS_EQUAL: return TAC_LE;
        case AST_GREATER: return TAC_GT;
        case AST_GREATER_EQUAL: return TAC_GE;
        default: return TAC_OP_COUNT;
    }
}

/* Declarado: dirección estática; parámetro: valor de la dirección recibida. */
TacOperand tb_array_base(TacBuilder *builder, ASTNode *node) {
    const Symbol *symbol = tb_symbol(node);
    TacOpcode op = symbol->kind == SYMBOL_PARAMETER ? TAC_COPY : TAC_ADDRESS;
    return tb_value(builder, node, op, tac_variable(symbol), tac_none());
}

/* Índices una vez, de izquierda a derecha; filas y desplazamiento en bytes. */
TacOperand tb_array_offset(TacBuilder *builder, ASTNode *node) {
    const Symbol *array = tb_symbol(tb_child(node, 0));
    ASTNode *arguments = ast_find_child(node, AST_ARGUMENTS);
    ASTNode *first = tb_child(arguments, 0);
    TacOperand row = tb_expression(builder, first);
    if (array->dimensions == 2) {
        TacOperand column = tb_expression(builder, first->next);
        row = tb_value(builder, node, TAC_MUL, row, tac_integer(array->dim_sizes[1]));
        row = tb_value(builder, node, TAC_ADD, row, column);
    }
    return tb_value(builder, node, TAC_MUL, row, tac_integer(4));
}

/* Captura escalares primero y direcciones después, como push/store_arguments. */
TacOperand tb_call(TacBuilder *builder, ASTNode *node, int result_needed) {
    const Symbol *callee = tb_symbol(tb_child(node, 0));
    size_t count = (size_t)callee->parameter_count;
    TacOperand *values = tb_allocate(count * sizeof *values);
    ASTNode **arguments = tb_allocate(count * sizeof *arguments);
    ASTNode *argument = tb_child(ast_find_child(node, AST_ARGUMENTS), 0);
    for (size_t i = 0; i < count; i++, argument = argument->next) {
        arguments[i] = argument;
        if (!callee->param_list[i]->dimensions) {
            values[i] = tb_expression(builder, argument);
        }
    }
    for (size_t i = count; i > 0; i--) {
        if (callee->param_list[i - 1]->dimensions) {
            values[i - 1] = tb_array_base(builder, arguments[i - 1]);
        }
    }
    for (size_t i = 0; i < count; i++) {
        tb_emit(builder, node, TAC_PARAM, tac_none(), values[i], tac_integer((int64_t)i));
    }
    TacOperand result = result_needed ? tac_temp_new(builder->function) : tac_none();
    tb_emit(builder, node, TAC_CALL, result, tac_function_operand(callee),
            tac_integer((int64_t)count));
    free(arguments);
    free(values);
    return result;
}

/*
 * tb_expression — captura valores antes de cualquier evaluación posterior.
 * Entrada: builder con camino abierto; node semánticamente válido.
 * Salida: literal o TEMP con el valor; NONE si detecta nodo no soportado.
 * Efectos: agrega operaciones/bloques; no modifica símbolos (Aho §6.4).
 * Restricciones: las operaciones aritméticas conservan semántica de 32 bits;
 *   DIV/MOD/POW permanecen abstractas y pueden terminar por error en ejecución.
 */
TacOperand tb_expression(TacBuilder *builder, ASTNode *node) {
    if (!node || builder->failed) {
        tb_fail(builder, node, "expresion ausente");
        return tac_none();
    }
    switch (node->kind) {
        case AST_NUMBER:
        case AST_BOOLEAN:
            return tac_integer(node->number);
        case AST_IDENTIFIER:
            return tb_value(builder, node, TAC_COPY, tac_variable(tb_symbol(node)), tac_none());
        case AST_AND:
        case AST_OR:
            return tb_short_circuit(builder, node);
        case AST_NEG:
        case AST_NOT: {
            TacOperand value = tb_expression(builder, node->child);
            return tb_value(builder, node, node->kind == AST_NEG ? TAC_NEG : TAC_NOT,
                            value, tac_none());
        }
        case AST_CALL:
        case AST_INDEX: {
            const Symbol *symbol = tb_symbol(tb_child(node, 0));
            if (symbol->kind == SYMBOL_FUNCTION) {
                return tb_call(builder, node, 1);
            }
            TacOperand offset = tb_array_offset(builder, node);
            TacOperand base = tac_variable(symbol);
            if (symbol->kind == SYMBOL_PARAMETER) {
                base = tb_array_base(builder, tb_child(node, 0));
            }
            return tb_value(builder, node, TAC_LOAD, base, offset);
        }
        default: {
            TacOpcode op = binary_opcode(node->kind);
            if (op == TAC_OP_COUNT) {
                tb_fail(builder, node, "nodo no soportado como expresion");
                return tac_none();
            }
            TacOperand left = tb_expression(builder, tb_child(node, 0));
            TacOperand right = tb_expression(builder, tb_child(node, 1));
            return tb_value(builder, node, op, left, right);
        }
    }
}
