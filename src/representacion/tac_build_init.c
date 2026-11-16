/*
 * tac_build_init.c — imagen inicial y declaraciones ejecutables.
 * Usa: ast_constant compartido con generate_init.c y tac_build_internal.h.
 * Referencias: Aho §6.3.4 y §6.4.3. La imagen se aplica una vez al arrancar;
 * globales dinámicos se ejecutan en main, y locales en cada visita, incluso R.
 */
#include "tac_build_internal.h"
#include "ast_constant.h"

/* Aplana listas por filas y avanza siempre una palabra, incluso si no emite. */
static void initialize_cells(TacBuilder *builder, ASTNode *node, const Symbol *symbol,
                              size_t *offset, TbInitialization phase) {
    if (node && node->kind == AST_INITIALIZER_LIST) {
        for (ASTNode *item = node->child; item; item = item->next) {
            initialize_cells(builder, item, symbol, offset, phase);
        }
        return;
    }
    uint32_t word = 0;
    int constant = !node || ast_constant_value(node, &word);
    size_t position = *offset;
    *offset += 4;
    if (phase == TB_IMAGE) {
        size_t address = (size_t)symbol->address + position;
        builder->result->initial_data[address / 4] = constant ? word : 0;
    } else if (phase == TB_LOCAL || !constant) {
        TacOperand value = node ? tb_expression(builder, node) : tac_integer(0);
        if (symbol->dimensions) {
            tb_emit(builder, node, TAC_STORE, tac_variable(symbol),
                    tac_integer((int64_t)position), value);
        } else {
            tb_emit(builder, node, TAC_COPY, tac_variable(symbol), value, tac_none());
        }
    }
}

/* No pliega inicializadores locales: el generador también los evalúa al entrar. */
void tb_initialize(TacBuilder *builder, ASTNode *node, TbInitialization phase) {
    const Symbol *symbol = tb_symbol(ast_find_child(node, AST_IDENTIFIER));
    ASTNode *init = ast_declaration_initializer(node);
    size_t offset = 0;
    if (init) {
        initialize_cells(builder, init, symbol, &offset, phase);
    }
    while (offset < (size_t)symbol->width) {
        initialize_cells(builder, NULL, symbol, &offset, phase);
    }
}
