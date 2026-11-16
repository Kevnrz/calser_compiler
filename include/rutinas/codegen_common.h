/*
 * codegen_common.h — secuencias fijas de máquina para los dos generadores.
 * Usa: codegen_arith.h y symbol_table.h (FrameLayout).
 * Lo usan O0, O1 y CRYPTO; no depende de GenCtx ni de data_area.c.
 * Referencias: Aho §7.2 y §8.6–8.8; isa.md §6 y §9.
 */
#ifndef CODEGEN_COMMON_H
#define CODEGEN_COMMON_H
#include <stdint.h>
#include "codegen_arith.h"
#include "symbol_table.h"

/* Entrada: registro ISA. Salida: ninguna. Efectos: sw reg, 0(rx); avance de 4.
 * Restricciones: rx apunta a la primera casilla libre; no comprueba desborde. */
void cg_stack_push(int reg);

/* Entrada: registro ISA. Salida: ninguna. Efectos: retroceso de 4; lw reg, 0(rx).
 * Restricciones: la palabra fue apilada; no comprueba desborde. */
void cg_stack_pop(int reg);

/* Entrada: user no usado y operación. Salida: 1 emitida, 0 inválida.
 * Efectos: etiquetas u operaciones. Restricciones: slots -1/0/2/3;
 * etiquetas terminadas en dos puntos y de menos de 128 bytes. */
int cg_emit_arith_op(void *user, const CGArithOp *operation);

/* Entrada: frame, máscara de bits 4..15 y restore (0 guarda, 1 restaura).
 * Salida: 1 éxito; 0 si el área no es válida, sin emitir.
 * Efectos: guarda/restaura en temp_base + 4·(r−4), en orden ascendente;
 * al guardar separa antes. mask=0 no emite ni exige frame.
 * Restricciones: casillas propias del frame; registros escritos como r4..r15.
 * La preservación implementa el convenio de llamada (Aho §7.2 y §8.8). */
int cg_preserve(const FrameLayout *frame, uint16_t mask, int restore);

/* Entrada: etiqueta. Salida: ninguna. Efectos: etiqueta y salto a sí misma.
 * Restricciones: bundle fijo, sin separación automática ni continuación. */
void cg_program_end(const char *label);

/* Entrada: primera dirección libre calculada por el llamador.
 * Salida: ninguna. Efectos: mov rx, stack_base; no consulta data_area. */
void cg_stack_init(int stack_base);
#endif
