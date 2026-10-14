/*
 * symbol_table.h — Fase 3: interfaz de símbolos y ámbitos con búsqueda estática.
 *
 * Expone símbolos, tipos, ámbitos, búsqueda e impresión de sus direcciones.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: symbol_table.c; semantic.c, generate.c y codegen_array.c/codegen_crypto.c.
 * Referencias: libro §2.7, §6.3.4, §7.1.1.
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>

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

    int line;                  // línea de su declaración; 0 = sin línea
    void *decl_node;           // nodo del AST que lo declara (no dueño)

    // Memoria (asignación estática, libro §6.3.4 y §7.1.1)
    int address;               // dirección en memoria de datos; -1 = sin asignar
    int width;                 // bytes que ocupa (ancho del tipo)
    int dim_sizes[MAX_DIMS];   // tamaño de cada dimensión; 0 si no aplica

    // Solo funciones
    int parameter_count;
    int area_base;             // inicio del área estática; -1 = sin asignar
    int area_size;             // tamaño del área en bytes
    int temp_base;             // inicio de las 12 casillas de temporales; -1 = sin asignar
    int limit_address;         // variable de control de un S: casilla oculta donde
                               // se guarda el fin del ciclo; -1 = no aplica
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

    int line;                  // línea donde abre el bloque; 0 = sin línea
    struct Symbol *owner;      // función a la que pertenece; NULL en el global

    Symbol *symbols;

    struct Scope *parent;

    struct Scope *next;
} Scope;

/* ==========================================================================
 * SÍMBOLOS Y BÚSQUEDA
 * ======================================================================= */

/*
 * symbol_create — crea un símbolo sin dirección ni ámbito.
 *
 * Entrada:
 *   name  nombre o NULL.
 *   kind  clase del símbolo.
 *   type  tipo declarado.
 * Salida:
 *   símbolo nuevo.
 *   Efectos: reserva nombre y símbolo; inicializa direcciones a -1.
 * Restricciones:
 *   el llamador asigna ámbito/forma; sin memoria termina el proceso.
 */
Symbol *symbol_create(const char *name, SymbolKind kind, SymbolType type);

/*
 * symbol_insert — inserta un símbolo sin duplicar nombres del ámbito.
 *
 * Entrada:
 *   scope  ámbito o NULL.
 *   symbol  símbolo o NULL.
 * Salida:
 *   1 si inserta; 0 si inválido o duplicado.
 *   Efectos: enlaza símbolo al inicio de scope->symbols en éxito.
 * Restricciones:
 *   transfiere propiedad solo al insertar; name válido para búsquedas.
 */
int symbol_insert(Scope *scope, Symbol *symbol);

/*
 * symbol_add_param — agrega un parámetro a la lista ordenada de función.
 *
 * Entrada:
 *   function  símbolo de función o NULL.
 *   param  parámetro o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: amplía param_list y parameter_count.
 * Restricciones:
 *   la lista presta punteros; el ámbito de función es dueño de parámetros.
 */
void symbol_add_param(Symbol *function, Symbol *param);

/*
 * symbol_lookup_current — busca un nombre solo en el ámbito dado.
 *
 * Entrada:
 *   scope  ámbito o NULL.
 *   name  nombre o NULL.
 * Salida:
 *   símbolo prestado o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
Symbol *symbol_lookup_current(Scope *scope, const char *name);

/*
 * symbol_lookup — busca desde el ámbito hasta sus padres.
 *
 * Entrada:
 *   scope  ámbito inicial o NULL.
 *   name  nombre o NULL.
 * Salida:
 *   símbolo más cercano o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   enlaces parent sin ciclos.
 */
Symbol *symbol_lookup(Scope *scope, const char *name);

/* ==========================================================================
 * ÁMBITOS
 * ======================================================================= */

/*
 * scope_create — crea un ámbito sin símbolos.
 *
 * Entrada:
 *   parent  ámbito padre o NULL.
 *   kind  clase de ámbito.
 * Salida:
 *   ámbito nuevo.
 *   Efectos: reserva memoria e inicializa enlaces.
 * Restricciones:
 *   sin memoria termina el proceso; el llamador enlaza la lista de
 *   ámbitos.
 */
Scope *scope_create(Scope *parent, ScopeKind kind);

/*
 * scope_free — libera un ámbito y sus símbolos.
 *
 * Entrada:
 *   scope  ámbito o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera símbolos y ámbito.
 * Restricciones:
 *   no libera parent ni next; llamador recorre la lista.
 */
void scope_free(Scope *scope);

/* ==========================================================================
 * IMPRESIÓN
 * ======================================================================= */

/*
 * symbol_table_print — imprime ámbitos y símbolos en stdout.
 *
 * Entrada:
 *   scope  primer ámbito o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe la tabla sin detalle adicional.
 * Restricciones:
 *   ninguna.
 */
void symbol_table_print(Scope *scope);

/*
 * symbol_table_fprint — imprime la tabla de símbolos con prefijo y detalle.
 *
 * Entrada:
 *   out  flujo no NULL.
 *   scope  primer ámbito o NULL.
 *   prefix  prefijo no NULL por línea.
 *   detailed  no cero para dueño, parámetros y líneas.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe ámbitos, tipos, formas y direcciones.
 * Restricciones:
 *   recorre next; símbolos vivos; llamador comprueba errores del flujo.
 */
void symbol_table_fprint(FILE *out, Scope *scope, const char *prefix,
                         int detailed);

#endif