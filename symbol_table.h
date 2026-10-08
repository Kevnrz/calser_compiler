#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER
} SymbolKind;


typedef enum {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_VOID
} SymbolType;


typedef enum {
    MODIFIER_NONE,
    MODIFIER_VM,
    MODIFIER_VI
} SymbolModifier;


typedef struct Symbol {
    char *name;

    SymbolKind kind;
    SymbolType type;

    SymbolModifier modifier;

    int dimensions;

    // functions.
    struct Symbol *parameters;
    int parameter_count;

    struct Symbol *next;

} Symbol;


typedef struct Scope {
    Symbol *symbols;

    struct Scope *parent;

    struct Scope *next;
} Scope;

Symbol *symbol_create(const char *name, SymbolKind kind, SymbolType type);

int symbol_insert(Scope *scope, Symbol *symbol);

Symbol *symbol_lookup_current(Scope *scope, const char *name);

Symbol *symbol_lookup(Scope *scope, const char *name);

Scope *scope_create(Scope *parent);

void scope_free(Scope *scope);

void symbol_table_print(Scope *scope);

#endif