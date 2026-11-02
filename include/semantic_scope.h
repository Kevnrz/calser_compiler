/*
 * semantic_scope.h — Fase 3: interfaz de los ámbitos del análisis semántico.
 *
 * Guarda el estado de los ámbitos (global, actual, lista de todos) y de la
 * función que se está analizando, y ofrece apertura, cierre, inserción y
 * resolución de nombres. Nadie fuera de semantic_scope.c toca esos punteros:
 * el resto del semántico consulta por estas funciones.
 *
 * Usa: ast.h, symbol_table.h.
 * Lo usa: semantic_scope.c; semantic.c y semantic_expr.c.
 * Referencias: libro §1.6.3, §2.7.
 */

#ifndef SEMANTIC_SCOPE_H
#define SEMANTIC_SCOPE_H

#include "ast.h"
#include "symbol_table.h"

/* ==========================================================================
 * APERTURA, CIERRE Y CONSULTA
 * ======================================================================= */

/*
 * scopes_open_global — crea el ámbito global del análisis.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reinicia punteros de ámbitos y reserva el ámbito global; deja
 *   el global como ámbito actual.
 * Restricciones:
 *   liberar un análisis anterior con scopes_release antes de comenzar otro.
 */
void scopes_open_global(void);

/*
 * scopes_enter — abre un ámbito anidado del tipo indicado.
 *
 * Entrada:
 *   kind  clase de ámbito.
 *   line  línea de apertura.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: crea el ámbito, lo agrega a la lista y lo deja como actual; el
 *   ámbito queda enlazado a la función que se analiza (scopes_function).
 * Restricciones:
 *   scopes_open_global previo.
 */
void scopes_enter(ScopeKind kind, int line);

/*
 * scopes_exit — regresa al ámbito padre.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza el ámbito actual; conserva el ámbito cerrado en la
 *   lista, para que la tabla de símbolos lo imprima.
 * Restricciones:
 *   ninguna.
 */
void scopes_exit(void);

/*
 * scopes_current — consulta el ámbito actual.
 *
 * Entrada: ninguna
 * Salida:
 *   ámbito prestado; NULL antes de scopes_open_global.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
Scope *scopes_current(void);

/*
 * scopes_global — consulta el ámbito global.
 *
 * Entrada: ninguna
 * Salida:
 *   ámbito global prestado; NULL antes de scopes_open_global.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
Scope *scopes_global(void);

/*
 * scopes_first — consulta el primer ámbito de la lista de todos los ámbitos.
 *
 * Entrada: ninguna
 * Salida:
 *   primer ámbito (el global) o NULL; la lista sigue por Scope.next en el
 *   orden de creación.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
Scope *scopes_first(void);

/*
 * scopes_function — consulta la función que se está analizando.
 *
 * Entrada: ninguna
 * Salida:
 *   símbolo de la función prestado; NULL fuera de las funciones (globales).
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
Symbol *scopes_function(void);

/*
 * scopes_set_function — fija la función que se está analizando.
 *
 * Entrada:
 *   function  símbolo de la función, o NULL al salir de ella.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: los ámbitos que se abran después quedan enlazados a esa función.
 * Restricciones:
 *   ninguna.
 */
void scopes_set_function(Symbol *function);

/* ==========================================================================
 * SÍMBOLOS Y NOMBRES
 * ======================================================================= */

/*
 * scopes_insert — inserta un símbolo en el ámbito actual.
 *
 * Entrada:
 *   symbol  símbolo nuevo no NULL.
 *   node  nodo declarador.
 * Salida:
 *   1 si transfiere propiedad al ámbito; 0 si rechaza.
 *   Efectos: inserta o reporta redeclaración y libera symbol.
 * Restricciones:
 *   ámbito actual vigente; el llamador no usa symbol después del rechazo.
 */
int scopes_insert(Symbol *symbol, ASTNode *node);

/*
 * scopes_discard — libera un símbolo todavía sin ámbito dueño.
 *
 * Entrada:
 *   symbol  símbolo no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera el nombre y el símbolo.
 * Restricciones:
 *   sin parámetros propios; no debe estar enlazado a un ámbito.
 */
void scopes_discard(Symbol *symbol);

/*
 * scopes_resolve — resuelve un identificador según el ámbito más cercano.
 *
 * Entrada:
 *   identifier  identificador o NULL.
 * Salida:
 *   símbolo prestado o NULL si no resuelve.
 *   Efectos: anota identifier->symbol o reporta nombre inexistente/fuera de ámbito.
 * Restricciones:
 *   ámbito actual configurado; no permite uso antes de declarar variables.
 */
Symbol *scopes_resolve(ASTNode *identifier);

/*
 * scopes_release — libera todos los ámbitos y sus símbolos.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera la lista de ámbitos y reinicia ámbito global, actual y
 *   función en análisis.
 * Restricciones:
 *   invalida las anotaciones symbol del AST.
 */
void scopes_release(void);

#endif
