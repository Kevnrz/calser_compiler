/*
 * symbol_table.c — Fase 3: símbolos y ámbitos con búsqueda estática.
 *
 * Mantiene listas de símbolos por ámbito y enlaces al ámbito padre.
 * Conserva tipo, forma, direcciones y parámetros ordenados de las funciones.
 *
 * Usa: symbol_table.h; biblioteca C.
 * Lo usa: semantic.c; generate.c y codegen_* leen símbolos anotados.
 * Referencias: libro §2.7, §6.3.4, §7.1.1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

/* ==========================================================================
 * CREACIÓN DE SÍMBOLOS Y ÁMBITOS
 * ======================================================================= */


/*
 * str_duplicate — duplica el nombre de un símbolo.
 *
 * Entrada:
 *   str  texto o NULL.
 * Salida:
 *   cadena propia o NULL si str es NULL.
 *   Efectos: reserva memoria; termina el proceso sin memoria.
 * Restricciones:
 *   ninguna.
 */
static char *str_duplicate(const char *str) {
    if (str == NULL)
        return NULL;

    char *copy = malloc(strlen(str) + 1);

    if (copy == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    strcpy(copy, str);
    return copy;
}


/* symbol_create — véase el contrato en symbol_table.h. */
Symbol *symbol_create(const char *name,
                      SymbolKind kind,
                      SymbolType type) {
    Symbol *symbol = malloc(sizeof(Symbol));

    if (symbol == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    symbol->name = str_duplicate(name);
    symbol->kind = kind;
    symbol->type = type;

    symbol->modifier = MODIFIER_NONE;
    symbol->dimensions = 0;
    symbol->line = 0;
    symbol->decl_node = NULL;

    // Memoria: las direcciones y el área se asignan después; -1 = sin asignar.
    symbol->address = -1;
    symbol->width = 0;
    symbol->dim_sizes[0] = 0;
    symbol->dim_sizes[1] = 0;

    symbol->parameter_count = 0;
    symbol->area_base = -1;
    symbol->area_size = 0;
    symbol->temp_base = -1;
    symbol->limit_address = -1;
    symbol->param_list = NULL;
    symbol->param_capacity = 0;

    symbol->next = NULL;

    return symbol;
}

/* symbol_add_param — véase el contrato en symbol_table.h. */
void symbol_add_param(Symbol *function, Symbol *param) {
    if (function == NULL || param == NULL)
        return;

    if (function->parameter_count == function->param_capacity) {

        // Duplica la capacidad, empezando en 4.
        int capacity =
            function->param_capacity == 0 ? 4 : function->param_capacity * 2;

        Symbol **list =
            realloc(function->param_list, capacity * sizeof(Symbol *));

        if (list == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        function->param_list = list;
        function->param_capacity = capacity;
    }

    function->param_list[function->parameter_count] = param;
    function->parameter_count++;
}

/* scope_create — véase el contrato en symbol_table.h. */
Scope *scope_create(Scope *parent, ScopeKind kind) {
    Scope *scope = malloc(sizeof(Scope));

    if (scope == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    scope->kind = kind;
    scope->line = 0;
    scope->owner = NULL;
    scope->symbols = NULL;
    scope->parent = parent;
    scope->next = NULL;

    return scope;
}

/* ==========================================================================
 * INSERCIÓN Y BÚSQUEDA
 * ======================================================================= */


/* symbol_insert — véase el contrato en symbol_table.h. */
int symbol_insert(Scope *scope, Symbol *symbol) {
    if (scope == NULL || symbol == NULL)
        return 0;

    // Impide declaraciones duplicadas dentro del mismo ámbito.
    if (symbol_lookup_current(scope, symbol->name) != NULL)
        return 0;

    // Inserta al inicio de la lista.
    symbol->next = scope->symbols;
    scope->symbols = symbol;

    return 1;
}

/* symbol_lookup_current — véase el contrato en symbol_table.h. */
Symbol *symbol_lookup_current(Scope *scope,
                               const char *name) {
    if (scope == NULL || name == NULL)
        return NULL;

    Symbol *current = scope->symbols;

    while (current != NULL) {

        if (strcmp(current->name, name) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}


/* symbol_lookup — véase el contrato en symbol_table.h. */
Symbol *symbol_lookup(Scope *scope, const char *name) {
    Scope *current_scope = scope;

    while (current_scope != NULL) {

        Symbol *symbol =
            symbol_lookup_current(current_scope, name);

        if (symbol != NULL)
            return symbol;

        current_scope = current_scope->parent;
    }

    return NULL;
}

/* ==========================================================================
 * LIBERACIÓN
 * ======================================================================= */


/*
 * symbol_free — libera una lista de símbolos.
 *
 * Entrada:
 *   symbol  primer símbolo o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera nombres, arreglos param_list y símbolos.
 * Restricciones:
 *   no libera los parámetros prestados de param_list.
 */
static void symbol_free(Symbol *symbol) {
    while (symbol != NULL) {

        Symbol *next = symbol->next;

        free(symbol->name);

        // Solo el arreglo de punteros: los parámetros se liberan con su
        // ámbito (liberarlos aquí sería un double free).
        free(symbol->param_list);

        free(symbol);

        symbol = next;
    }
}


/* scope_free — véase el contrato en symbol_table.h. */
void scope_free(Scope *scope) {
    if (scope == NULL)
        return;

    symbol_free(scope->symbols);

    free(scope);
}

/* ==========================================================================
 * IMPRESIÓN DE LA TABLA DE SÍMBOLOS
 * ======================================================================= */


/*
 * symbol_kind_string — obtiene el texto de clase de símbolo.
 *
 * Entrada:
 *   kind  clase del símbolo.
 * Salida:
 *   cadena literal o unknown.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *symbol_kind_string(SymbolKind kind) {
    switch (kind) {

        case SYMBOL_VARIABLE:
            return "variable";

        case SYMBOL_FUNCTION:
            return "function";

        case SYMBOL_PARAMETER:
            return "parameter";

        case SYMBOL_RETURN:
            return "return";    

        default:
            return "unknown";
    }
}

/*
 * symbol_type_string — obtiene el texto de tipo de símbolo.
 *
 * Entrada:
 *   type  tipo del símbolo.
 * Salida:
 *   cadena literal o unknown.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *symbol_type_string(SymbolType type) {
    switch (type) {

        case TYPE_INT:
            return "int";

        case TYPE_BOOL:
            return "bool";

        case TYPE_VOID:
            return "void";

        default:
            return "unknown";
    }
}


/*
 * scope_kind_string — obtiene el texto de clase de ámbito.
 *
 * Entrada:
 *   kind  clase del ámbito.
 * Salida:
 *   cadena literal o UNKNOWN.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *scope_kind_string(ScopeKind kind) {
    switch (kind) {

        case SCOPE_GLOBAL:
            return "GLOBAL";

        case SCOPE_FUNCTION:
            return "FUNCTION";

        case SCOPE_C:
            return "C";

        case SCOPE_A:
            return "A";

        case SCOPE_RESIDUAL:
            return "RESIDUAL";

        case SCOPE_L:
            return "L";

        case SCOPE_S:
            return "S";

        default:
            return "UNKNOWN";
    }
}

// Imprime una dirección en hexadecimal; ---- si todavía no se asignó (-1).
/*
 * print_address — escribe una dirección o su ausencia.
 *
 * Entrada:
 *   out  flujo no NULL.
 *   label  nombre del campo.
 *   address  dirección en bytes o negativa.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe hexadecimal o ---- en out.
 * Restricciones:
 *   ninguna.
 */
static void print_address(FILE *out, const char *label, int address) {
    if (address < 0)
        fprintf(out, " %s=----", label);
    else
        fprintf(out, " %s=0x%04X", label, (unsigned int) address);
}

/*
 * line_start — escribe un prefijo de línea.
 *
 * Entrada:
 *   out  flujo no NULL.
 *   prefix  prefijo no NULL.
 *   empty  1 si la línea queda vacía.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe prefijo, sin espacios finales cuando empty.
 * Restricciones:
 *   ninguna.
 */
static void line_start(FILE *out, const char *prefix, int empty) {
    if (!empty) {
        fputs(prefix, out);
        return;
    }

    size_t length = strlen(prefix);

    while (length > 0 && prefix[length - 1] == ' ')
        length--;

    fwrite(prefix, 1, length, out);
}

/* symbol_table_print — véase el contrato en symbol_table.h. */
void symbol_table_print(Scope *scope) {
    symbol_table_fprint(stdout, scope, "", 0);
}

/* symbol_table_fprint — véase el contrato en symbol_table.h. */
void symbol_table_fprint(FILE *out, Scope *scope, const char *prefix,
                         int detailed) {
    int scope_number = 0;

    Scope *current_scope = scope;

    line_start(out, prefix, 1);
    fprintf(out, "\n");
    line_start(out, prefix, 0);
    fprintf(out, "===== SYMBOL TABLE =====\n");

    while (current_scope != NULL) {

        // Un ámbito sin símbolos se imprime igual, solo con su cabecera.
        line_start(out, prefix, 0);
        fprintf(out, "Scope %d: %s",
                scope_number,
                scope_kind_string(current_scope->kind));

        // La función a la que pertenece el ámbito (NULL en el global).
        if (detailed && current_scope->owner != NULL)
            fprintf(out, " %s", current_scope->owner->name);

        fprintf(out, "\n");

        Symbol *symbol = current_scope->symbols;

        while (symbol != NULL) {

            line_start(out, prefix, 0);
            fprintf(out, "    %s : %s",
                    symbol->name,
                    symbol_type_string(symbol->type));

            if (symbol->kind == SYMBOL_FUNCTION) {

                // Funciones: lista ordenada de parámetros y área estática,
                // sin width.
                fprintf(out, " (%s) params=[",
                        symbol_kind_string(symbol->kind));

                for (int i = 0; i < symbol->parameter_count; i++) {
                    fprintf(out, "%s%s",
                            i > 0 ? ", " : "",
                            symbol->param_list[i]->name);
                }

                fprintf(out, "]");

                print_address(out, "area", symbol->area_base);
                fprintf(out, " size=%d", symbol->area_size);
                print_address(out, "temp_base", symbol->temp_base);
            }
            else {

                // Tamaño de cada dimensión, pegado al tipo.
                for (int i = 0; i < symbol->dimensions; i++)
                    fprintf(out, "[%d]", symbol->dim_sizes[i]);

                if (symbol->kind == SYMBOL_RETURN ||
                    (detailed && symbol->kind == SYMBOL_PARAMETER)) {
                    fprintf(out, " (%s)",
                            symbol_kind_string(symbol->kind));
                }

                fprintf(out, " width=%d", symbol->width);
                print_address(out, "addr", symbol->address);

                if (detailed)
                    fprintf(out, " line=%d", symbol->line);
            }
            fprintf(out, "\n");
            symbol = symbol->next;
        }
        line_start(out, prefix, 1);
        fprintf(out, "\n");
        current_scope = current_scope->next;
        scope_number++;
    }
    line_start(out, prefix, 0);
    fprintf(out, "=======================\n");
}