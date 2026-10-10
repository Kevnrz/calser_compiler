#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_RETURN
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


#define MAX_DIMS 2   // Calser admite hasta 2 dimensiones (gramática: dims)

typedef struct Symbol {
    char *name;

    SymbolKind kind;
    SymbolType type;

    SymbolModifier modifier;

    int dimensions;

    // Memoria (asignación estática, libro §6.3.4 y §7.1.1)
    int address;               // dirección en memoria de datos; -1 = sin asignar
    int width;                 // bytes que ocupa (ancho del tipo)
    int dim_sizes[MAX_DIMS];   // tamaño de cada dimensión; 0 si no aplica

    // Solo funciones
    int parameter_count;
    int area_base;             // inicio del área estática; -1 = sin asignar
    int area_size;             // tamaño del área en bytes
    struct Symbol **param_list; // parámetros EN ORDEN. Punteros NO dueños:
                                // los símbolos viven en el ámbito de la función
    int param_capacity;        // capacidad reservada de param_list

    struct Symbol *next;

} Symbol;


typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_C,          // rama inicial de un condicional
    SCOPE_A,          // rama alternativa  A [cond] ;
    SCOPE_RESIDUAL,   // rama residual     A ;
    SCOPE_L,          // ciclo por predicado
    SCOPE_S           // ciclo acotado (contiene su variable de control)
} ScopeKind;


typedef struct Scope {
    ScopeKind kind;

    Symbol *symbols;

    struct Scope *parent;

    struct Scope *next;
} Scope;

Symbol *symbol_create(const char *name, SymbolKind kind, SymbolType type);

int symbol_insert(Scope *scope, Symbol *symbol);

void symbol_add_param(Symbol *function, Symbol *param);

Symbol *symbol_lookup_current(Scope *scope, const char *name);

Symbol *symbol_lookup(Scope *scope, const char *name);

Scope *scope_create(Scope *parent, ScopeKind kind);

void scope_free(Scope *scope);

void symbol_table_print(Scope *scope);

#endif