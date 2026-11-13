/*
 * tac_build_stmt.c — asignaciones, copia de arreglos y las siete CRYPTO.
 * Usa: tac_build_internal.h. Referencias: Aho §6.4, §6.4.3, §6.6 y §8.4.1.
 * La copia completa es un lazo de LOAD/STORE que podrá ver el unrolling.
 */
#include "tac_build_internal.h"
#include <string.h>

/* Copia ascendente palabra a palabra; conserva el orden del generador actual. */
static void array_copy(TacBuilder *builder, ASTNode *node, ASTNode *dest, ASTNode *src) {
    const Symbol *symbol = tb_symbol(dest);
    TacOperand source = tb_array_base(builder, src);
    TacOperand target = tb_array_base(builder, dest);
    int words = symbol->dim_sizes[0];
    if (symbol->dimensions == 2) {
        words *= symbol->dim_sizes[1];
    }
    TacOperand offset = tb_value(builder, node, TAC_COPY, tac_integer(0), tac_none());
    TacBlock *loop = tb_reserve_block(builder);
    tb_jump(builder, node, loop);
    builder->block = loop;
    TacBlock *end = tb_reserve_block(builder);
    TacBlock *body = tb_reserve_block(builder);
    tb_emit(builder, node, TAC_IF_GE, tac_label(end), offset, tac_integer(words * 4));
    builder->block = body;
    TacOperand value = tb_value(builder, node, TAC_LOAD, source, offset);
    tb_emit(builder, node, TAC_STORE, target, offset, value);
    tb_emit(builder, node, TAC_ADD, offset, offset, tac_integer(4));
    tb_jump(builder, node, loop);
    builder->block = end;
}

/* Primero el valor derecho, después los índices del destino (generate_stmt.c). */
static void assignment(TacBuilder *builder, ASTNode *node) {
    ASTNode *target = tb_child(node, 0);
    ASTNode *right = tb_child(node, 1);
    const Symbol *symbol = tb_symbol(target);
    if (target->kind == AST_IDENTIFIER && symbol->dimensions) {
        array_copy(builder, node, target, right);
        return;
    }
    TacOperand value = tb_expression(builder, right);
    if (target->kind == AST_IDENTIFIER) {
        tb_emit(builder, node, TAC_COPY, tac_variable(symbol), value, tac_none());
    } else {
        symbol = tb_symbol(tb_child(target, 0));
        TacOperand offset = tb_array_offset(builder, target);
        TacOperand base = tac_variable(symbol);
        if (symbol->kind == SYMBOL_PARAMETER) {
            base = tb_array_base(builder, tb_child(target, 0));
        }
        tb_emit(builder, node, TAC_STORE, base, offset, value);
    }
}

/* Los selectores son literales; captura solo valores y conserva su orden. */
static void crypto(TacBuilder *builder, ASTNode *node) {
    const char *name = node->text;
    ASTNode *arg = tb_child(ast_find_child(node, AST_ARGUMENTS), 0);
    TacOperand none = tac_none();
    TacInstruction *instruction;
    if (!strcmp(name, "cifrar") || !strcmp(name, "descifrar")) {
        TacOperand origin = tb_expression(builder, arg);
        TacOperand dest = tb_expression(builder, arg->next);
        TacOperand count = tb_expression(builder, arg->next->next);
        instruction = tb_emit(builder, node, !strcmp(name, "cifrar") ?
                              TAC_CIFRAR : TAC_DESCIFRAR, origin, dest, count);
        if (instruction) {
            instruction->selectors.k = (int)arg->next->next->next->number;
        }
    } else if (!strcmp(name, "cargar_llave")) {
        TacOperand value = tb_expression(builder, arg->next->next);
        instruction = tb_emit(builder, node, TAC_KV_WRITE, none, value, none);
        if (instruction) {
            instruction->selectors.k = (int)arg->number;
            instruction->selectors.w = (int)arg->next->number;
        }
    } else if (!strcmp(name, "autenticar") || !strcmp(name, "cambiar_ancla")) {
        TacOperand value = tb_expression(builder, arg);
        tb_emit(builder, node, !strcmp(name, "autenticar") ? TAC_AUTH : TAC_SANC,
                none, value, none);
    } else if (!strcmp(name, "sellar") || !strcmp(name, "cerrar_sesion")) {
        tb_emit(builder, node, !strcmp(name, "sellar") ? TAC_KV_LOCK : TAC_LOGOUT,
                none, none, none);
    } else {
        tb_fail(builder, node, "sentencia CRYPTO desconocida");
    }
}

/* Despacha solo sentencias; las clases de metadatos no generan instrucciones. */
void tb_statement(TacBuilder *builder, ASTNode *node) {
    switch (node->kind) {
        case AST_BLOCK:
            tb_statements(builder, node);
            break;
        case AST_DECLARATION:
            tb_initialize(builder, node, TB_LOCAL);
            break;
        case AST_ASSIGNMENT:
            assignment(builder, node);
            break;
        case AST_CALL_STATEMENT:
            tb_call(builder, node->child, 0);
            break;
        case AST_CRYPTO:
            crypto(builder, node);
            break;
        default:
            tb_flow(builder, node);
            break;
    }
}

/* E/R cierran el camino; nunca se agrega una sentencia detrás de un terminador. */
void tb_statements(TacBuilder *builder, ASTNode *block) {
    for (ASTNode *node = tb_child(block, 0); node && builder->block && !builder->failed;
         node = node->next) {
        tb_statement(builder, node);
    }
}
