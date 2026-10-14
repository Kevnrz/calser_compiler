/*
 * semantic.h — Fase 3: interfaz de análisis de ámbitos, tipos y direcciones estáticas.
 *
 * Expone análisis del AST, consulta de direcciones y salida de símbolos.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: ast.h.
 * Lo usa: semantic.c; main.c y generate.c.
 * Referencias: libro §1.6.3, §2.7, §6.3.4, §6.5, §7.1.1.
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>

#include "ast.h"

/* ==========================================================================
 * ANÁLISIS, TABLAS Y MEMORIA
 * ======================================================================= */

/*
 * semantic_analyze — analiza ámbitos, tipos y direcciones del programa.
 *
 * Entrada:
 *   root  raíz AST_PROGRAM.
 * Salida:
 *   1 sin nuevos errores; 0 por error semántico o interno.
 *   Efectos: crea tabla de símbolos, anota AST y reporta errores.
 * Restricciones:
 *   diag_init previo; liberar tabla con semantic_free; reserva al menos
 *   256 bytes para pila sin probar su máximo uso.
 */
int semantic_analyze(ASTNode *root);

/*
 * semantic_print_symbols — imprime la tabla de símbolos en stdout.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe ámbitos, símbolos y direcciones.
 * Restricciones:
 *   tabla vigente del análisis.
 */
void semantic_print_symbols(void);

/*
 * semantic_fprint_symbols — imprime tabla detallada con prefijo.
 *
 * Entrada:
 *   out  flujo de salida no NULL.
 *   prefix  prefijo no NULL por línea.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe ámbitos, parámetros, líneas y direcciones.
 * Restricciones:
 *   tabla vigente del análisis; el llamador comprueba errores del flujo.
 */
void semantic_fprint_symbols(FILE *out, const char *prefix);

/*
 * semantic_data_end — consulta la primera dirección libre del área estática.
 *
 * Entrada: ninguna
 * Salida:
 *   desplazamiento en bytes, múltiplo de 4.
 *   Efectos: ninguno.
 * Restricciones:
 *   consultar después de un análisis exitoso para usarlo como base de
 *   pila.
 */
int semantic_data_end(void);

/*
 * semantic_free — libera ámbitos, símbolos y grafo del análisis.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera memoria y reinicia punteros de ámbitos/función.
 * Restricciones:
 *   invalida las anotaciones symbol del AST; no usar después para generar.
 */
void semantic_free(void);

#endif