/* Emisión P1 reutilizada por D, sin dependencia de GenCtx.
 * Implementaciones en generate_support.c y generate_expr.c; Aho §7.2 y §8.6.
 */
#ifndef GENERATE_EMIT_H
#define GENERATE_EMIT_H
#include "codegen_arith.h"

/*
 * gen_reg_name — obtiene el alias de un registro ISA.
 *
 * Entrada:
 *   reg  número ISA.
 * Salida:
 *   cadena literal; rz si reg está fuera de 0–15.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
const char *gen_reg_name(int reg);

/*
 * gen_stack_push — apila una palabra en rx.
 *
 * Entrada:
 *   reg  registro ISA cuyo valor se conserva.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite sw y avance de rx en cuatro bytes.
 * Restricciones:
 *   rx apunta a la primera casilla libre; no verifica desborde en
 *   ejecución.
 */
void gen_stack_push(int reg);

/*
 * gen_stack_pop — desapila una palabra desde rx.
 *
 * Entrada:
 *   reg  registro ISA que recibe la palabra.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite retroceso de rx y lw.
 * Restricciones:
 *   debe existir una palabra apilada; no verifica subdesborde.
 */
void gen_stack_pop(int reg);

/* Entrada: operación aritmética o etiqueta. Salida: 1 emitida, 0 inválida.
 * Efectos: llama a cg_label/cg_op. Restricciones: contrato en generate_expr.c.
 */
int gen_arith_emit_op(void *user, const CGArithOp *operation);
#endif
