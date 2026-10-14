/*
 * codegen_array.h — Fase 4: interfaz de cálculo de direcciones y operaciones de arreglos.
 *
 * Expone cálculo de base/dirección, accesos, paso, copia y puesta en cero.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: codegen_arith.h, symbol_table.h.
 * Lo usa: codegen_array.c; generate.c y pruebas/arreglos.
 * Referencias: libro §6.4.3, §7.3; isa.md §6.3.
 */

#ifndef CODEGEN_ARRAY_H
#define CODEGEN_ARRAY_H
#include "codegen_arith.h"
#include "symbol_table.h"

/* ==========================================================================
 * BASE Y ACCESO A ELEMENTOS
 * ======================================================================= */

/*
 * cg_array_base — obtiene la dirección base de un arreglo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   array  arreglo validado.
 *   dst  destino ISA 4–15.
 * Salida:
 *   no cero si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite mov o lw de casilla de parámetro.
 * Restricciones:
 *   análisis semántico exitoso; el llamador planifica la separación RAW.
 */
int cg_array_base(CGArithContext *ctx, const Symbol *array, int dst);

/*
 * cg_array_address — calcula dirección de un elemento por filas.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   array  arreglo validado.
 *   indices  registros de índices.
 *   count  cantidad igual a dimensions.
 *   dst  destino ISA de dirección.
 *   work  dos auxiliares ISA.
 * Salida:
 *   no cero si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite cálculo, modifica destino/auxiliares y preserva índices.
 * Restricciones:
 *   registros 4–15; destino/auxiliares distintos de índices; límites
 *   dinámicos no comprobados; descartar emisión parcial.
 */
int cg_array_address(CGArithContext *ctx, const Symbol *array,
                     const int *indices, int count, int dst, const int work[2]);

/*
 * cg_array_load — genera lectura de un elemento de arreglo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   array  arreglo validado.
 *   indices  registros de índices.
 *   count  cantidad igual a dimensions.
 *   address  registro de dirección.
 *   value  registro que recibe valor.
 *   work  dos auxiliares.
 * Salida:
 *   no cero si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite cálculo y lw; modifica dirección/valor/auxiliares.
 * Restricciones:
 *   registros 4–15 distintos según cg_array_address; value también
 *   distinto de índices; límites dinámicos válidos.
 */
int cg_array_load(CGArithContext *ctx, const Symbol *array, const int *indices,
                  int count, int address, int value, const int work[2]);
/*
 * cg_array_store — genera escritura de un elemento de arreglo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   array  arreglo destino.
 *   indices  registros de índices.
 *   count  cantidad igual a dimensions.
 *   address  registro de dirección.
 *   value  registro del valor.
 *   work  dos auxiliares.
 * Salida:
 *   no cero si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite cálculo y sw; modifica dirección/auxiliares.
 * Restricciones:
 *   rechaza Vi; value puede coincidir con índice, nunca con
 *   dirección/auxiliares; límites dinámicos válidos.
 */
int cg_array_store(CGArithContext *ctx, const Symbol *array, const int *indices,
                   int count, int address, int value, const int work[2]);

/* ==========================================================================
 * PASO, COPIA E INICIALIZACIÓN
 * ======================================================================= */

/*
 * cg_array_pass — pasa la dirección de un arreglo completo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   source  arreglo origen.
 *   parameter  parámetro arreglo destino.
 *   temp  temporal ISA 4–15.
 * Salida:
 *   no cero si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite obtención de base y escritura en casilla del parámetro.
 * Restricciones:
 *   igual forma/tipo; no pasa Vi a parámetro mutable; descartar emisión
 *   parcial.
 */
int cg_array_pass(CGArithContext *ctx, const Symbol *source,
                  const Symbol *parameter, int temp);

/*
 * cg_array_copy — copia todas las palabras de un arreglo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   destination  arreglo destino mutable.
 *   source  arreglo origen.
 *   work  cuatro auxiliares ISA distintos.
 * Salida:
 *   1 si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite ciclo de copia y modifica auxiliares/next_label.
 * Restricciones:
 *   misma forma/tipo; alias exacto admitido, demás áreas disjuntas;
 *   llamador planifica RAW y descarta emisión parcial.
 */
int cg_array_copy(CGArithContext *ctx, const Symbol *destination,
                  const Symbol *source, const int work[4]);
/*
 * cg_array_zero — genera inicialización a cero de un arreglo.
 *
 * Entrada:
 *   ctx  contexto con callback.
 *   array  arreglo destino.
 *   work  dos auxiliares ISA distintos.
 * Salida:
 *   1 si emite; 0 ante contrato/callback inválido.
 *   Efectos: emite ciclo de escritura y modifica auxiliares/next_label.
 * Restricciones:
 *   admite inicializar Vi; no recorre listas de inicialización; llamador
 *   planifica RAW.
 */
int cg_array_zero(CGArithContext *ctx, const Symbol *array, const int work[2]);
#endif
