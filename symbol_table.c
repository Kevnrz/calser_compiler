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

    symbol->parameters = NULL;
    symbol->parameter_count = 0;

    symbol->next = NULL;

    return symbol;
}

Scope *scope_create(Scope *parent) {
    Scope *scope = malloc(sizeof(Scope));

    if (scope == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

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

        // Parameters are also symbols.
        symbol_free(symbol->parameters);

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


void symbol_table_print(Scope *scope) {
    int scope_number = 0;

    Scope *current_scope = scope;

    while (current_scope != NULL) {

        printf("Scope %d", scope_number);

        // The global scope is the scope with no parent.
        if (current_scope->parent == NULL)
            printf(": GLOBAL\n");
        else
            printf(": FUNCTION\n");

        Symbol *symbol = current_scope->symbols;

        while (symbol != NULL) {

            printf("    %s : %s",
                   symbol->name,
                   symbol_type_string(symbol->type));

            if (symbol->kind == SYMBOL_FUNCTION)
                printf(" (%s)",
                       symbol_kind_string(symbol->kind));

            printf("\n");

            symbol = symbol->next;
        }

        printf("\n");

        current_scope = current_scope->next;
        scope_number++;
    }
}