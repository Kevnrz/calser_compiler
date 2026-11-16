/*
 * semantic_scope.c — Fase 3: ámbitos del análisis semántico.
 *
 * Mantiene la lista de ámbitos, el ámbito actual y la función en análisis,
 * y resuelve nombres: primero en los ámbitos abiertos y, si falla, en los ya
 * cerrados de la misma función para distinguir "fuera de alcance" de "no
 * declarado". Es el único módulo que escribe estos punteros.
 *
 * Usa: semantic_scope.h, semantic_internal.h, symbol_table.c.
 * Lo usa: semantic.c y semantic_expr.c.
 * Referencias: libro §1.6.3, §2.7.
 */

#include <stdio.h>
#include <stdlib.h>

#include "semantic_scope.h"
#include "semantic_internal.h"

static Scope *global_scope = NULL;
static Scope *current_scope = NULL;

//Lista de todos los ámbitos.
static Scope *scope_list_head = NULL;
static Scope *scope_list_tail = NULL;

// Función que se está analizando; NULL fuera de las funciones (globales).
static Symbol *current_function = NULL;

/* ==========================================================================
 * ÁMBITOS
 * ======================================================================= */


/*
 * create_scope — crea y abre un ámbito anidado.
 *
 * Entrada:
 *   kind  clase de ámbito.
 *   line  línea de apertura.
 * Salida:
 *   ámbito creado; NULL si scope_create devolviera NULL.
 *   Efectos: actualiza current_scope y la lista de ámbitos; enlaza la función dueña.
 * Restricciones:
 *   ninguna.
 */
static Scope *create_scope(ScopeKind kind, int line) {
    Scope *scope = scope_create(current_scope, kind);

    if (scope == NULL)
        return NULL;

    // P-03: línea donde abre el bloque y función a la que pertenece.
    scope->line = line;
    scope->owner = current_function;

    // Agrega el ámbito a la lista de ámbitos.
    if (scope_list_head == NULL) {
        scope_list_head = scope;
        scope_list_tail = scope;
    }
    else {
        scope_list_tail->next = scope;
        scope_list_tail = scope;
    }

    current_scope = scope;

    return scope;
}

/* scopes_open_global — véase el contrato en semantic_scope.h. */
void scopes_open_global(void) {
    current_scope = NULL;

    global_scope = scope_create(NULL, SCOPE_GLOBAL);

    if (global_scope == NULL) {
        fprintf(stderr, "Error: could not create global scope\n");
        exit(EXIT_FAILURE);
    }

    scope_list_head = global_scope;
    scope_list_tail = global_scope;

    current_scope = global_scope;
}

/* scopes_enter — véase el contrato en semantic_scope.h. */
void scopes_enter(ScopeKind kind, int line) {
    create_scope(kind, line);
}

/* scopes_exit — véase el contrato en semantic_scope.h. */
void scopes_exit(void) {
    if (current_scope != NULL)
        current_scope = current_scope->parent;
}

/*
 * find_closed_scope — busca un nombre en los ámbitos de la función actual.
 *
 * Entrada:
 *   name  nombre no NULL.
 * Salida:
 *   primer ámbito con el nombre o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   se usa después de fallar la búsqueda en ámbitos abiertos.
 */
static Scope *find_closed_scope(const char *name) {
    if (current_function == NULL)
        return NULL;

    Scope *scope = scope_list_head;

    while (scope != NULL) {

        if (scope->owner == current_function &&
            symbol_lookup_current(scope, name) != NULL)
            return scope;

        scope = scope->next;
    }

    return NULL;
}

/* scopes_resolve — véase el contrato en semantic_scope.h. */
Symbol *scopes_resolve(ASTNode *identifier) {
    if (identifier == NULL || identifier->text == NULL)
        return NULL;

    Symbol *symbol = symbol_lookup(current_scope, identifier->text);

    if (symbol != NULL) {

        // Enlace uso -> símbolo: lo usará la generación de código.
        identifier->symbol = symbol;

        return symbol;
    }

    // Fuera de alcance: el nombre existe en un bloque de esta función que ya
    // se cerró. No hace falta excluir los ámbitos abiertos: symbol_lookup ya
    // buscó en el actual y en sus padres.
    Scope *closed = find_closed_scope(identifier->text);

    if (closed != NULL) {

        semantic_error(identifier,
                       "'%s' esta fuera de alcance; "
                       "fue declarado en el bloque de la linea %d",
                       identifier->text,
                       closed->line);

        return NULL;
    }

    semantic_error(identifier,
                   "'%s' no ha sido declarado",
                   identifier->text);

    return NULL;
}

/* scopes_insert — véase el contrato en semantic_scope.h. */
int scopes_insert(Symbol *symbol, ASTNode *node) {
    Symbol *existing =
        symbol_lookup_current(current_scope, symbol->name);

    if (existing != NULL ||
        !symbol_insert(current_scope, symbol)) {

        semantic_error(node,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       symbol->name,
                       existing ? existing->line : 0);

        // symbol_insert no tomó propiedad porque la inserción falló.
        scopes_discard(symbol);

        return 0;
    }

    return 1;
}

/* scopes_discard — véase el contrato en semantic_scope.h. */
void scopes_discard(Symbol *symbol) {
    free(symbol->name);
    free(symbol);
}

/* scopes_current — véase el contrato en semantic_scope.h. */
Scope *scopes_current(void) {
    return current_scope;
}

/* scopes_global — véase el contrato en semantic_scope.h. */
Scope *scopes_global(void) {
    return global_scope;
}

/* scopes_first — véase el contrato en semantic_scope.h. */
Scope *scopes_first(void) {
    return scope_list_head;
}

/* scopes_function — véase el contrato en semantic_scope.h. */
Symbol *scopes_function(void) {
    return current_function;
}

/* scopes_set_function — véase el contrato en semantic_scope.h. */
void scopes_set_function(Symbol *function) {
    current_function = function;
}

/* scopes_release — véase el contrato en semantic_scope.h. */
void scopes_release(void) {
    Scope *scope = scope_list_head;

    while (scope != NULL) {

        Scope *next = scope->next;

        /*
         * Desenlaza el ámbito antes de liberarlo;
         * la lista se recorre con next guardado.
         */
        scope->next = NULL;

        scope_free(scope);

        scope = next;
    }

    scope_list_head = NULL;
    scope_list_tail = NULL;
    global_scope = NULL;
    current_scope = NULL;
    current_function = NULL;
}
