/*
 * generate.h — Fase 4: interfaz de generación de ensamblador desde el AST anotado.
 *
 * Expone generación del AST anotado en el emisor de instrucciones.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: ast.h, label_table.h.
 * Lo usa: generate.c; main.c.
 * Referencias: libro §6.4, §6.6, §6.7, §7.2; isa.md §6, §9.
 */

#ifndef GENERATE_H
#define GENERATE_H
 
#include "ast.h"
#include "label_table.h"
 
/*
 * generate_code — genera ensamblador e imagen de datos desde el AST anotado.
 *
 * Entrada:
 *   root  raíz AST_PROGRAM validada semánticamente.
 *   labels  tabla de etiquetas construida en Fase 3.
 * Salida:
 *   0 si genera; 1 si detecta error interno o de generación.
 *   Efectos: reinicia codegen, llena imagen de datos y emite funciones y destinos terminales.
 * Restricciones:
 *   llamar antes de semantic_free; organizar después con cg_schedule salvo
 *   --no-schedule; ante error descartar emisión parcial.
 */
int generate_code(ASTNode *root, const LabelTable *labels);
 
#endif
 
