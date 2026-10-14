/*
 * label_table.c — Fase 3: construcción y resolución de la tabla de etiquetas.
 *
 * Asocia puntos de control del AST con nombres y roles. Las direcciones
 * quedan pendientes hasta resolverlas con el calendario final de Fase 4.
 *
 * Usa: AST; label_table.h.
 * Lo usa: main.c y generate.c.
 * Referencias: libro §6.7 (backpatching); enunciado §2.3–§2.5.
 */

#include <stdlib.h>
#include <string.h>

#include "label_table.h"

/* ==========================================================================
 * REGISTRO DE ETIQUETAS
 * ======================================================================= */


/*
 * copy_string — duplica el nombre de una etiqueta.
 *
 * Entrada:
 *   text  texto no NULL.
 * Salida:
 *   cadena nueva o NULL sin memoria.
 *   Efectos: reserva cadena; el receptor la libera.
 * Restricciones:
 *   ninguna.
 */
static char *copy_string(const char *text) {
    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy == NULL)
        return NULL;

    memcpy(copy, text, length);

    return copy;
}

/* label_table_init — véase el contrato en label_table.h. */
void label_table_init(LabelTable *table) {
    if (table == NULL)
        return;

    table->entries = NULL;
    table->next_id = 0;
}

/*
 * label_table_add_named — registra un nombre simbólico todavía sin dirección.
 *
 * Entrada:
 *   table  tabla no NULL.
 *   node  nodo o NULL para etiqueta global.
 *   role  rol.
 *   name  nombre no NULL.
 * Salida:
 *   0 si agrega; -1 sin memoria.
 *   Efectos: reserva entrada/nombre y agrega al inicio.
 * Restricciones:
 *   table inicializada; address queda -1.
 */
static int label_table_add_named(LabelTable *table, ASTNode *node,
                                 LabelRole role, const char *name) {
    char *copy = copy_string(name);

    if (copy == NULL)
        return -1;

    LabelEntry *entry = malloc(sizeof(LabelEntry));

    if (entry == NULL) {
        free(copy);
        return -1;
    }

    entry->node = node;
    entry->role = role;
    entry->name = copy;
    entry->address = -1;
    entry->next = table->entries;
    table->entries = entry;

    return 0;
}

/*
 * label_table_add — crea un nombre de control con identificador consecutivo.
 *
 * Entrada:
 *   table  tabla no NULL.
 *   node  nodo.
 *   role  rol.
 *   prefix  prefijo no NULL.
 * Salida:
 *   0 si agrega; -1 si formato o memoria falla.
 *   Efectos: registra entrada e incrementa next_id.
 * Restricciones:
 *   nombre completo debe caber en 128 bytes.
 */
static int label_table_add(LabelTable *table, ASTNode *node,
                           LabelRole role, const char *prefix) {
    char buffer[128];
    int length = snprintf(buffer, sizeof(buffer), ".L_%s_%u",
                          prefix, table->next_id);

    if (length < 0 || (size_t) length >= sizeof(buffer))
        return -1;

    if (label_table_add_named(table, node, role, buffer) != 0)
        return -1;

    table->next_id++;
    return 0;
}

/*
 * label_function — registra etiquetas de entrada y cuerpo de función.
 *
 * Entrada:
 *   table  tabla no NULL.
 *   node  nodo FUNCTION.
 * Salida:
 *   0 si agrega ambas; -1 si falla.
 *   Efectos: reserva etiquetas .L_func_ y .L_body_.
 * Restricciones:
 *   el identificador del nodo tiene texto; un fallo puede dejar entrada
 *   parcial.
 */
static int label_function(LabelTable *table, ASTNode *node) {
    ASTNode *identifier = node->child;
    while (identifier != NULL && identifier->kind != AST_IDENTIFIER)
        identifier = identifier->next;

    if (identifier == NULL || identifier->text == NULL)
        return -1;

    /* El nombre completo se conserva, sin un límite fijo de caracteres. */
    size_t length = strlen(identifier->text) + sizeof(".L_func_");
    char *name = malloc(length);
    if (name == NULL)
        return -1;

    snprintf(name, length, ".L_func_%s", identifier->text);
    int result = label_table_add_named(table, node, LABEL_FUNCTION, name);
    if (result == 0) {
        snprintf(name, length, ".L_body_%s", identifier->text);
        result = label_table_add_named(table, node, LABEL_BODY, name);
    }

    free(name);
    return result;
}

/*
 * label_node — registra etiquetas según la clase de nodo.
 *
 * Entrada:
 *   table  tabla no NULL.
 *   node  nodo no NULL.
 * Salida:
 *   0 si agrega o no necesita; -1 si falla.
 *   Efectos: agrega puntos de control a la tabla.
 * Restricciones:
 *   ninguna.
 */
static int label_node(LabelTable *table, ASTNode *node) {
    switch (node->kind) {
        case AST_FUNCTION:
            return label_function(table, node);
        case AST_AND:
        case AST_OR:
            return label_table_add(table, node, LABEL_SC_END, "sc_end");
        case AST_CONDITIONAL:
            if (label_table_add(table, node, LABEL_NEXT, "if_next") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "if_end");
        case AST_ALTERNATIVE:
            return label_table_add(table, node, LABEL_NEXT, "elif_next");
        case AST_LOOP_L:
            if (label_table_add(table, node, LABEL_START, "while_start") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "while_end");
        case AST_LOOP_S:
            if (label_table_add(table, node, LABEL_START, "for_start") != 0)
                return -1;

            if (label_table_add(table, node, LABEL_UPDATE, "for_update") != 0)
                return -1;

            return label_table_add(table, node, LABEL_END, "for_end");
        default:
            return 0;
    }
}

/*
 * label_traverse — recorre nodos y hermanos para etiquetar controles.
 *
 * Entrada:
 *   table  tabla no NULL.
 *   node  nodo o NULL.
 * Salida:
 *   0 si completa; -1 si falla.
 *   Efectos: agrega etiquetas del subárbol y hermanos.
 * Restricciones:
 *   AST sin ciclos.
 */
static int label_traverse(LabelTable *table, ASTNode *node) {
    while (node != NULL) {

        if (label_node(table, node) != 0)
            return -1;

        if (label_traverse(table, node->child) != 0)
            return -1;

        node = node->next;
    }

    return 0;
}

/* ==========================================================================
 * CONSTRUCCIÓN Y CONSULTA
 * ======================================================================= */


/* label_table_build — véase el contrato en label_table.h. */
int label_table_build(LabelTable *table, ASTNode *root) {
    if (table == NULL)
        return -1;

    if (table->entries != NULL)
        return -1;

    table->next_id = 0;

    if (label_traverse(table, root) != 0)
        return -1;

    // Etiquetas globales: no pertenecen a ningún nodo del AST.
    if (label_table_add_named(table, NULL, LABEL_PROGRAM_END,
                              "fin_main") != 0)
        return -1;

    return label_table_add_named(table, NULL, LABEL_ARITH_ERROR,
                                 ".L_arith_error");
}

/* label_table_get — véase el contrato en label_table.h. */
const char *label_table_get(const LabelTable *table, const ASTNode *node, LabelRole role) {
    if (table == NULL)
        return NULL;

    const LabelEntry *entry = table->entries;

    while (entry != NULL) {

        if (entry->node == node && entry->role == role)
            return entry->name;

        entry = entry->next;
    }

    return NULL;
}

/* label_table_destroy — véase el contrato en label_table.h. */
void label_table_destroy(LabelTable *table) {
    if (table == NULL)
        return;

    LabelEntry *entry = table->entries;

    while (entry != NULL) {

        LabelEntry *next = entry->next;

        free(entry->name);
        free(entry);

        entry = next;
    }

    table->entries = NULL;
    table->next_id = 0;
}

/* ==========================================================================
 * RESOLUCIÓN E IMPRESIÓN
 * ======================================================================= */


/*
 * label_role_name — obtiene el nombre imprimible del rol.
 *
 * Entrada:
 *   role  rol de etiqueta.
 * Salida:
 *   cadena literal; UNKNOWN si no reconoce.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *label_role_name(LabelRole role) {
    switch (role) {
        case LABEL_FUNCTION: return "FUNCTION";
        case LABEL_START:    return "START";
        case LABEL_NEXT:     return "NEXT";
        case LABEL_END:      return "END";
        case LABEL_UPDATE:   return "UPDATE";
        case LABEL_BODY:     return "BODY";
        case LABEL_SC_END:   return "SC_END";
        case LABEL_ARITH_ERROR: return "ARITH_ERROR";
        case LABEL_PROGRAM_END: return "PROGRAM_END";
        default:             return "UNKNOWN";
    }
}

/*
 * label_node_kind_name — obtiene la clase de control para la tabla.
 *
 * Entrada:
 *   node  nodo o NULL.
 * Salida:
 *   cadena literal; GLOBAL para NULL, UNKNOWN si no es control.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *label_node_kind_name(const ASTNode *node) {
    if (node == NULL)
        return "GLOBAL";

    switch (node->kind) {
        case AST_FUNCTION:    return "FUNCTION";
        case AST_CONDITIONAL: return "CONDITIONAL";
        case AST_ALTERNATIVE: return "ALTERNATIVE";
        case AST_LOOP_L:      return "LOOP_L";
        case AST_LOOP_S:      return "LOOP_S";
        case AST_AND:         return "AND";
        case AST_OR:          return "OR";
        default:             return "UNKNOWN";
    }
}

/* label_table_resolve — véase el contrato en label_table.h. */
void label_table_resolve(LabelTable *table, int (*lookup)(const char *name)) {
    if (table == NULL || lookup == NULL)
        return;

    for (LabelEntry *entry = table->entries; entry != NULL; entry = entry->next)
        entry->address = lookup(entry->name);
}

/* label_table_print — véase el contrato en label_table.h. */
void label_table_print(const LabelTable *table) {
    if (table == NULL)
        return;

    printf("\n===== LABEL TABLE =====\n");

    const LabelEntry *entry = table->entries;

    while (entry != NULL) {
        char address[16];

        // "-" mientras la dirección no se conoce (libro §6.7).
        if (entry->address >= 0)
            snprintf(address, sizeof(address), "%d", entry->address);
        else
            snprintf(address, sizeof(address), "-");

        printf(
            "%-10s | %-12s | AST kind: %s | Line: %d | Address: %s\n",
            entry->name,
            label_role_name(entry->role),
            label_node_kind_name(entry->node),
            entry->node != NULL ? entry->node->line : 0,
            address
        );

        entry = entry->next;
    }

    printf("=======================\n");
}