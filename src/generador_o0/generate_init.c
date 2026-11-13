/*
 * generate_init.c — Fase 4: inicialización de variables, locales y globales.
 *
 * Escribe los inicializadores constantes en la imagen de datos y emite el
 * código de los que dependen de valores en ejecución, celda por celda.
 *
 * El evaluador y la búsqueda del inicializador viven en ast_constant.c,
 * compartidos con el constructor TAC de tac_build_init.c.
 * Usa: generate_internal.h, codegen.h, ast_constant.h.
 * Lo usa: generate.c y generate_stmt.c.
 * Referencias: libro §6.3.4.
 */

#include <stdint.h>

#include "generate_internal.h"
#include "codegen.h"
#include "ast_constant.h"


/* ==========================================================================
 * CONSTANTES E INICIALIZACIÓN
 * ======================================================================= */


/*
 * initialize_cells — inicializa casillas escalares o una lista por filas.
 *
 * Entrada:
 *   init  inicializador o NULL para cero.
 *   symbol  símbolo con dirección.
 *   offset  desplazamiento en bytes no NULL.
 *   image  1 para imagen estática, 0 para instrucciones.
 *   dynamic_only  1 para emitir solo valores no constantes.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: avanza *offset de cuatro en cuatro; escribe imagen o emite sw; reporta errores.
 * Restricciones:
 *   forma y tipo de la lista ya verificados; no comprueba aquí el ancho
 *   del símbolo.
 */
static void initialize_cells(GenCtx *gen, ASTNode *init, Symbol *symbol, int *offset,
                             int image, int dynamic_only) {
    if (init && init->kind == AST_INITIALIZER_LIST) {
        for (ASTNode *item = init->child; item; item = item->next)
            initialize_cells(gen, item, symbol, offset, image, dynamic_only);
        return;
    }
    uint32_t value = 0;
    int constant = !init || ast_constant_value(init, &value);
    int address = symbol->address + *offset;
    *offset += 4;
    if (image) {
        if (cg_data_set(address, constant ? value : 0) != 0)
            gen_report_error(gen, init, "inicializador fuera de la imagen de datos");
    } else if (!dynamic_only || !constant) {
        int reg = init ? gen_expression(gen, init) : gen_alloc_reg(gen);
        if (reg < 0) { gen_report_error(gen, init, "fallo al inicializar variable"); return; }
        if (!init && gen_mov(gen, reg, 0, NULL) != 0) { gen_free_reg(gen, reg); return; }
        cg_op(SLOT_LSU, "sw %s, %d(rz)", isa_register_name(reg), address);
        gen_free_reg(gen, reg);
    }
}

/* gen_initialize_declaration — véase el contrato en generate_internal.h. */
void gen_initialize_declaration(GenCtx *gen, ASTNode *node, int image, int dynamic_only) {
    Symbol *symbol = gen_get_symbol(ast_find_child(node, AST_IDENTIFIER));
    if (!symbol || symbol->address < 0) { gen_report_error(gen, node, "declaracion sin direccion"); return; }
    ASTNode *init = ast_declaration_initializer(node);
    int offset = 0;
    if (init) initialize_cells(gen, init, symbol, &offset, image, dynamic_only);
    while (offset < symbol->width)
        initialize_cells(gen, NULL, symbol, &offset, image, dynamic_only);
}
