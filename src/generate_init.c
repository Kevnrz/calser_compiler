/*
 * generate_init.c — Fase 4: inicialización de variables, locales y globales.
 *
 * Escribe los inicializadores constantes en la imagen de datos y emite el
 * código de los que dependen de valores en ejecución, celda por celda.
 *
 * Usa: generate_internal.h, codegen.h.
 * Lo usa: generate.c y generate_stmt.c.
 * Referencias: libro §6.3.4.
 */

#include <stdint.h>

#include "generate_internal.h"
#include "codegen.h"


/* ==========================================================================
 * CONSTANTES E INICIALIZACIÓN
 * ======================================================================= */


/*
 * declaration_initializer — localiza el inicializador de una declaración.
 *
 * Entrada:
 *   node  nodo AST_DECLARATION.
 * Salida:
 *   nodo prestado de inicializador o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static ASTNode *declaration_initializer(ASTNode *node) {
    ASTNode *type = ast_find_child(node, AST_TYPE);
    ASTNode *init = type ? type->next : NULL;
    return init && init->kind == AST_DIMENSIONS ? init->next : init;
}

/*
 * constant_value — evalúa una constante para la imagen de datos.
 *
 * Entrada:
 *   node  expresión o NULL.
 *   out  puntero no NULL para la palabra.
 *   depth  profundidad de evaluación, inicialmente 0.
 * Salida:
 *   1 si conoce el valor; 0 si no puede evaluarlo.
 *   Efectos: escribe *out al obtener un valor.
 * Restricciones:
 *   profundidad máxima 64; solo sigue identificadores Vi con decl_node; no
 *   evalúa llamadas.
 */
static int constant_value(ASTNode *node, uint32_t *out, int depth) {
    if (!node || depth > 64) return 0;
    uint32_t a, b;
    if (node->kind == AST_NUMBER || node->kind == AST_BOOLEAN) {
        *out = (uint32_t) node->number; return 1;
    }
    if (node->kind == AST_IDENTIFIER) {
        Symbol *sym = gen_get_symbol(node);
        return sym && sym->modifier == MODIFIER_VI && sym->decl_node &&
            constant_value(declaration_initializer(sym->decl_node), out, depth + 1);
    }
    if (!constant_value(node->child, &a, depth + 1)) return 0;
    if (node->kind == AST_NEG) { *out = 0u - a; return 1; }
    if (node->kind == AST_NOT) { *out = !a; return 1; }
    if (!constant_value(node->child->next, &b, depth + 1)) return 0;
    int64_t x = a <= INT32_MAX ? (int64_t) a : (int64_t) a - INT64_C(4294967296);
    int64_t y = b <= INT32_MAX ? (int64_t) b : (int64_t) b - INT64_C(4294967296);
    switch (node->kind) {
        case AST_ADD: *out = a + b; break;
        case AST_SUB: *out = a - b; break;
        case AST_MUL: *out = a * b; break;
        case AST_DIV: if (!b) return 0; *out = (uint32_t) (x / y); break;
        case AST_MOD: if (!b) return 0; *out = (uint32_t) (x % y); break;
        case AST_POWER: {
            if (y < 0) return 0;
            uint32_t result = 1;
            while (b) { if (b & 1) result *= a; a *= a; b >>= 1; }
            *out = result; break;
        }
        case AST_AND: *out = a != 0 && b != 0; break;
        case AST_OR: *out = a != 0 || b != 0; break;
        case AST_EQUAL: *out = a == b; break;
        case AST_NOT_EQUAL: *out = a != b; break;
        case AST_LESS: *out = x < y; break;
        case AST_GREATER: *out = x > y; break;
        case AST_LESS_EQUAL: *out = x <= y; break;
        case AST_GREATER_EQUAL: *out = x >= y; break;
        default: return 0;
    }
    return 1;
}

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
    int constant = !init || constant_value(init, &value, 0);
    int address = symbol->address + *offset;
    *offset += 4;
    if (image) {
        if (cg_data_set(address, constant ? value : 0) != 0)
            gen_report_error(gen, init, "inicializador fuera de la imagen de datos");
    } else if (!dynamic_only || !constant) {
        int reg = init ? gen_expression(gen, init) : gen_alloc_reg(gen);
        if (reg < 0) { gen_report_error(gen, init, "fallo al inicializar variable"); return; }
        if (!init && gen_mov(gen, reg, 0, NULL) != 0) { gen_free_reg(gen, reg); return; }
        cg_op(SLOT_LSU, "sw %s, %d(rz)", gen_reg_name(reg), address);
        gen_free_reg(gen, reg);
    }
}

/* gen_initialize_declaration — véase el contrato en generate_internal.h. */
void gen_initialize_declaration(GenCtx *gen, ASTNode *node, int image, int dynamic_only) {
    Symbol *symbol = gen_get_symbol(ast_find_child(node, AST_IDENTIFIER));
    if (!symbol || symbol->address < 0) { gen_report_error(gen, node, "declaracion sin direccion"); return; }
    ASTNode *init = declaration_initializer(node);
    int offset = 0;
    if (init) initialize_cells(gen, init, symbol, &offset, image, dynamic_only);
    while (offset < symbol->width)
        initialize_cells(gen, NULL, symbol, &offset, image, dynamic_only);
}
