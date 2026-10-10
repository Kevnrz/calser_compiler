#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

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

    // Memoria: las direcciones y el área se asignan después; -1 = sin asignar.
    symbol->address = -1;
    symbol->width = 0;
    symbol->dim_sizes[0] = 0;
    symbol->dim_sizes[1] = 0;

    symbol->parameter_count = 0;
    symbol->area_base = -1;
    symbol->area_size = 0;
    symbol->param_list = NULL;
    symbol->param_capacity = 0;

    symbol->next = NULL;

    return symbol;
}

// Agrega param al final de la lista ordenada de parámetros de la función.
// La lista guarda punteros NO dueños: el símbolo pertenece al ámbito de la función.
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

Scope *scope_create(Scope *parent, ScopeKind kind) {
    Scope *scope = malloc(sizeof(Scope));

    if (scope == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    scope->kind = kind;
    scope->symbols = NULL;
    scope->parent = parent;
    scope->next = NULL;

    return scope;
}

int symbol_insert(Scope *scope, Symbol *symbol) {
    if (scope == NULL || symbol == NULL)
        return 0;

    // Don't allow duplicate declarations in the same scope.
    if (symbol_lookup_current(scope, symbol->name) != NULL)
        return 0;

    // Insert at the beginning of the list.
    symbol->next = scope->symbols;
    scope->symbols = symbol;

    return 1;
}

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


void scope_free(Scope *scope) {
    if (scope == NULL)
        return;

    symbol_free(scope->symbols);

    free(scope);
}

// Printing
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
static void print_address(const char *label, int address) {
    if (address < 0)
        printf(" %s=----", label);
    else
        printf(" %s=0x%04X", label, (unsigned int) address);
}


void symbol_table_print(Scope *scope) {
    int scope_number = 0;

    Scope *current_scope = scope;

    while (current_scope != NULL) {

        // Un ámbito sin símbolos se imprime igual, solo con su cabecera.
        printf("Scope %d: %s\n",
               scope_number,
               scope_kind_string(current_scope->kind));

        Symbol *symbol = current_scope->symbols;

        while (symbol != NULL) {

            printf("    %s : %s",
                   symbol->name,
                   symbol_type_string(symbol->type));

            if (symbol->kind == SYMBOL_FUNCTION) {

                // Funciones: lista ordenada de parámetros y área estática,
                // sin width.
                printf(" (%s) params=[",
                       symbol_kind_string(symbol->kind));

                for (int i = 0; i < symbol->parameter_count; i++) {
                    printf("%s%s",
                           i > 0 ? ", " : "",
                           symbol->param_list[i]->name);
                }

                printf("]");

                print_address("area", symbol->area_base);
                printf(" size=%d", symbol->area_size);
            }
            else {

                // Tamaño de cada dimensión, pegado al tipo.
                for (int i = 0; i < symbol->dimensions; i++)
                    printf("[%d]", symbol->dim_sizes[i]);

                if (symbol->kind == SYMBOL_RETURN) {
                    printf(" (%s)",
                           symbol_kind_string(symbol->kind));
                }

                printf(" width=%d", symbol->width);
                print_address("addr", symbol->address);
            }
            printf("\n");
            symbol = symbol->next;
        }
        printf("\n");
        current_scope = current_scope->next;
        scope_number++;
    }
}