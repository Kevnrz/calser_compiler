/*
 * semantic_calls.h — Fase 3: interfaz del grafo de llamadas y la detección de ciclos.
 *
 * Guarda las funciones analizadas y las llamadas entre ellas, en orden de
 * aparición, y detecta recursión indirecta. La recursión directa y la
 * indirecta están prohibidas (S-05): el generador asigna memoria estática a
 * cada función y solo usa pila para rl, argumentos y temporales vivos.
 *
 * Usa: ast.h, symbol_table.h.
 * Lo usa: semantic_calls.c; semantic.c y semantic_expr.c.
 * Referencias: libro §7.1.1.
 */

#ifndef SEMANTIC_CALLS_H
#define SEMANTIC_CALLS_H

#include "ast.h"
#include "symbol_table.h"

/*
 * calls_reset — libera el grafo y lo deja vacío.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera las listas de aristas y de funciones y reinicia contadores.
 * Restricciones:
 *   seguro de llamar con el grafo vacío; invalida lo registrado antes.
 */
void calls_reset(void);

/*
 * calls_add_function — registra una función para recorrer el grafo.
 *
 * Entrada:
 *   function  símbolo de función.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: amplía la lista de funciones y conserva un puntero prestado.
 * Restricciones:
 *   se llama una vez por función, en el orden del archivo.
 */
void calls_add_function(Symbol *function);

/*
 * calls_add_edge — registra una llamada entre dos funciones.
 *
 * Entrada:
 *   caller  símbolo de función llamadora.
 *   callee  símbolo de función llamada.
 *   call  nodo CALL para diagnóstico.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: amplía la lista de aristas y registra punteros prestados.
 * Restricciones:
 *   R no se registra como llamada.
 */
void calls_add_edge(Symbol *caller, Symbol *callee, ASTNode *call);

/*
 * calls_check_cycles — detecta recursión indirecta en el grafo.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: recorre el grafo, reporta cada ciclo como error semántico y
 *   libera sus auxiliares.
 * Restricciones:
 *   cuerpos ya analizados; R no es una arista.
 */
void calls_check_cycles(void);

#endif
