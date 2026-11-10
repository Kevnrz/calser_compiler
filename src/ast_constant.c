/*
 * ast_constant.c — evaluación constante compartida de la imagen inicial.
 * Extrae la política de generate_init.c sin depender de GenCtx ni del emisor.
 * Usa: ast_constant.h y símbolos prestados del AST.
 * Lo usan el generador actual, las pruebas y el futuro constructor TAC.
 * Referencias: Aho §6.4 (expresiones), §6.3.4 (almacenamiento).
 */
#include "ast_constant.h"
#include "symbol_table.h"

/* ast_declaration_initializer — véase el contrato en ast_constant.h. */
ASTNode *ast_declaration_initializer(ASTNode *node) {
    ASTNode *type = ast_find_child(node, AST_TYPE);
    ASTNode *init = type ? type->next : NULL;
    return init && init->kind == AST_DIMENSIONS ? init->next : init;
}

/*
 * constant_value — conserva el límite recursivo y el orden de evaluación.
 *
 * Entrada:
 *   node: expresión o NULL; out: palabra escribible; depth: profundidad actual.
 * Salida:
 *   1 si obtiene un valor; 0 si no, sin cambiar *out.
 * Efectos:
 *   escribe *out al obtener el valor; solo consulta el AST y sus símbolos.
 * Restricciones:
 *   profundidad máxima 64; sigue Vi sin evaluar llamadas. Primero evalúa
 *   izquierda y después derecha, incluso para AND/OR (Aho §6.4).
 */
static int constant_value(ASTNode *node, uint32_t *out, int depth) {
    if (!node || depth > 64) {
        return 0;
    }
    uint32_t a;
    uint32_t b;
    if (node->kind == AST_NUMBER || node->kind == AST_BOOLEAN) {
        *out = (uint32_t)node->number;
        return 1;
    }
    if (node->kind == AST_IDENTIFIER) {
        Symbol *sym = (Symbol *)node->symbol;
        return sym && sym->modifier == MODIFIER_VI && sym->decl_node &&
            constant_value(ast_declaration_initializer(sym->decl_node), out, depth + 1);
    }
    if (!constant_value(node->child, &a, depth + 1)) {
        return 0;
    }
    if (node->kind == AST_NEG) {
        *out = 0u - a;
        return 1;
    }
    if (node->kind == AST_NOT) {
        *out = !a;
        return 1;
    }
    if (!constant_value(node->child->next, &b, depth + 1)) {
        return 0;
    }
    int64_t x = a <= INT32_MAX ? (int64_t)a : (int64_t)a - INT64_C(4294967296);
    int64_t y = b <= INT32_MAX ? (int64_t)b : (int64_t)b - INT64_C(4294967296);
    switch (node->kind) {
        case AST_ADD:
            *out = a + b;
            break;
        case AST_SUB:
            *out = a - b;
            break;
        case AST_MUL:
            *out = a * b;
            break;
        case AST_DIV:
            if (!b) {
                return 0;
            }
            *out = (uint32_t)(x / y);
            break;
        case AST_MOD:
            if (!b) {
                return 0;
            }
            *out = (uint32_t)(x % y);
            break;
        case AST_POWER: {
            if (y < 0) {
                return 0;
            }
            uint32_t result = 1;
            while (b) {
                if (b & 1) {
                    result *= a;
                }
                a *= a;
                b >>= 1;
            }
            *out = result;
            break;
        }
        case AST_AND:
            *out = a != 0 && b != 0;
            break;
        case AST_OR:
            *out = a != 0 || b != 0;
            break;
        case AST_EQUAL:
            *out = a == b;
            break;
        case AST_NOT_EQUAL:
            *out = a != b;
            break;
        case AST_LESS:
            *out = x < y;
            break;
        case AST_GREATER:
            *out = x > y;
            break;
        case AST_LESS_EQUAL:
            *out = x <= y;
            break;
        case AST_GREATER_EQUAL:
            *out = x >= y;
            break;
        default:
            return 0;
    }
    return 1;
}

/* ast_constant_value — véase el contrato en ast_constant.h. */
int ast_constant_value(ASTNode *node, uint32_t *out) {
    return constant_value(node, out, 0);
}
