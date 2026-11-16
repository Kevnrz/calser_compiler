/*
 * semantic_internal.h — Fase 3: lo que comparten los módulos del análisis semántico.
 *
 * Reúne solo lo que usan varios semantic_*.c: el reporte de errores con
 * posición del AST, la descripción del tipo de una expresión y las funciones
 * de verificación que un módulo le pide a otro. Cada módulo con estado
 * propio (ámbitos, grafo de llamadas, área de datos) tiene su propia cabecera.
 *
 * Usa: ast.h, symbol_table.h.
 * Lo usa: semantic.c, semantic_scope.c, semantic_calls.c, semantic_expr.c y
 * semantic_crypto.c.
 * Referencias: libro §1.6.3, §2.7.
 */

#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

#include "ast.h"
#include "symbol_table.h"

/* ==========================================================================
 * DIAGNÓSTICOS
 * ======================================================================= */

/*
 * semantic_error — reporta un error semántico con posición del AST.
 *
 * Entrada:
 *   node  nodo para el señalador o NULL.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe diagnóstico mediante diag_verror e incrementa su contador.
 * Restricciones:
 *   el análisis continúa después de reportar.
 */
void semantic_error(ASTNode *node, const char *fmt, ...);

/*
 * semantic_operator_symbol — obtiene la grafía de un operador.
 *
 * Entrada:
 *   kind  clase del nodo AST.
 * Salida:
 *   cadena literal o NULL si no es operador.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
const char *semantic_operator_symbol(ASTKind kind);

/* ==========================================================================
 * TIPOS DE EXPRESIONES
 * ======================================================================= */

// Tipo de una expresión.
typedef enum {
    ET_ERROR,   // ya hubo un error en esta expresión: no reportar nada más sobre ella
    ET_INT,
    ET_BOOL,
    ET_VOID,    // resultado de llamar a una función void
    ET_ARRAY    // arreglo completo (nombre suelto de un arreglo)
} ExprKind;

typedef struct {
    ExprKind kind;
    SymbolType base;       // ET_ARRAY: tipo de los elementos
    int dimensions;        // ET_ARRAY: 1 o 2
    int sizes[MAX_DIMS];   // ET_ARRAY: tamaño de cada dimensión

    int is_const;          // 1 si la expresión es una constante entera conocida
    long value;            // su valor (solo si is_const)
} ExprType;

#define TYPE_TEXT_SIZE 64   // alcanza para "bool[N][M]"

/*
 * semantic_check_expression — devuelve el tipo de una expresión y verifica
 * nombres, usos y tipos.
 *
 * Entrada:
 *   node  expresión del AST o NULL.
 * Salida:
 *   tipo de la expresión; ET_ERROR si ya se reportó un error en ella.
 *   Efectos: resuelve y anota símbolos, pliega constantes y reporta errores.
 * Restricciones:
 *   cada expresión se evalúa una sola vez, para no repetir sus errores.
 */
ExprType semantic_check_expression(ASTNode *node);

/*
 * semantic_type_text — escribe el nombre de un tipo para los mensajes.
 *
 * Entrada:
 *   type  tipo de expresión.
 *   buffer  arreglo escribible de TYPE_TEXT_SIZE bytes.
 * Salida:
 *   buffer, con el texto del tipo.
 *   Efectos: escribe en buffer.
 * Restricciones:
 *   el buffer es del llamador.
 */
const char *semantic_type_text(ExprType type, char *buffer);

/* ==========================================================================
 * VERIFICACIÓN DE USOS, CONDICIONES Y ASIGNACIONES
 * ======================================================================= */

// A-1: la gramática produce el mismo nodo para v [ i ] y para f [ x ]. Cada
// uso se resuelve contra la tabla de símbolos, según dónde aparece.
typedef enum {
    USE_EXPRESSION,   // CALL o INDEX dentro de una expresión
    USE_TARGET,       // CALL o INDEX como destino de una asignación
    USE_STATEMENT,    // CALL de un CALL_STATEMENT
    USE_REWIND        // REWIND
} UseContext;

/*
 * semantic_check_condition — exige condición bool en C, A o L.
 *
 * Entrada:
 *   node  nodo de control.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: verifica expresión y reporta condición de tipo distinto de bool.
 * Restricciones:
 *   ninguna.
 */
void semantic_check_condition(ASTNode *node);

/*
 * semantic_check_assignment — verifica destino y valor de una asignación.
 *
 * Entrada:
 *   statement  nodo AST_ASSIGNMENT no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: anota nombres y reporta mutabilidad/tipos incompatibles.
 * Restricciones:
 *   ninguna.
 */
void semantic_check_assignment(ASTNode *statement);

/*
 * semantic_check_call — resuelve llamada, acceso o reinicio según su contexto.
 *
 * Entrada:
 *   node  nodo CALL, INDEX o REWIND no NULL.
 *   context  contexto de uso.
 * Salida:
 *   tipo escalar/VOID; ET_ERROR si resolución/cantidad falla o hay recursión directa.
 *   Efectos: anota nombres, comprueba operandos y agrega llamadas al grafo.
 * Restricciones:
 *   argumentos en AST_ARGUMENTS; no evalúa una llamada para obtener
 *   constante.
 */
ExprType semantic_check_call(ASTNode *node, UseContext context);

/*
 * semantic_check_assignable — comprueba compatibilidad de una asignación.
 *
 * Entrada:
 *   target  nodo destino para diagnóstico.
 *   destination  tipo destino.
 *   value  tipo del valor.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta incompatibilidad de tipo o forma.
 * Restricciones:
 *   no comprueba mutabilidad; ET_ERROR evita cascadas.
 */
void semantic_check_assignable(ASTNode *target, ExprType destination,
                             ExprType value);

/*
 * semantic_check_initializer_list — verifica forma y tipos de un inicializador de arreglo.
 *
 * Entrada:
 *   symbol  símbolo declarado.
 *   list  lista de inicialización.
 *   node  nodo de declaración.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: verifica valores y reporta tamaños/filas/tipos incompatibles.
 * Restricciones:
 *   una o dos dimensiones; también reporta lista aplicada a escalar.
 */
void semantic_check_initializer_list(Symbol *symbol, ASTNode *list,
                                   ASTNode *node);

/*
 * semantic_expect_type — exige una clase de tipo sin repetir errores previos.
 *
 * Entrada:
 *   node  nodo para diagnóstico.
 *   type  tipo ya calculado.
 *   wanted  clase requerida.
 *   message  mensaje no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta incompatibilidad salvo ET_ERROR.
 * Restricciones:
 *   ninguna.
 */
void semantic_expect_type(ASTNode *node, ExprType type, ExprKind wanted,
                        const char *message);

/*
 * semantic_symbol_value_type — describe el valor escalar o arreglo de un símbolo.
 *
 * Entrada:
 *   symbol  símbolo no NULL.
 * Salida:
 *   tipo escalar o ET_ARRAY; ET_ERROR para tamaños no positivos.
 *   Efectos: ninguno.
 * Restricciones:
 *   dimensions no mayor que MAX_DIMS.
 */
ExprType semantic_symbol_value_type(Symbol *symbol);

/* ==========================================================================
 * OPERACIONES DE CIFRADO
 * ======================================================================= */

/*
 * semantic_analyze_crypto — verifica argumentos y selectores de una operación de cifrado.
 *
 * Entrada:
 *   node  nodo AST_CRYPTO.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: resuelve nombres y tipos de argumentos; puede agregar sus llamadas al grafo.
 * Restricciones:
 *   cantidad y forma garantizadas por gramática; no verifica direcciones
 *   dinámicas.
 */
void semantic_analyze_crypto(ASTNode *node);

#endif
