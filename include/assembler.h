/*
 * assembler.h — Fase 5: interfaz de ensamblado de etiquetas y bundles en dos pasadas.
 *
 * Expone AsmProgram y ensamblado/liberación de líneas con cuatro slots.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: assembler.c; main.c.
 * Referencias: libro §1.1, §8.1.2; isa.md §3, §4, §6, §11.
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>

/* Ensamblador de ccom: traduce líneas en la sintaxis de isa.md §11 a los
 * bundles de 128 bits de isa.md §3 (cuatro palabras de 32 bits).
 *
 * Es el paso del ensamblador del sistema de procesamiento de lenguajes
 * (libro §1.1, Fig. 1.5): recibe el programa objeto en ensamblador (libro
 * §8.1.2) y resuelve sus direcciones simbólicas. Se usa igual con la salida
 * del generador (cg_line) y con un archivo .asm (-x). */

typedef struct {
    int count;                 /* cantidad de bundles                       */
    uint32_t (*bundles)[4];    /* [i][slot]: palabra de cada slot           */
    char **text;               /* [i]: el bundle normalizado, para -p       */
} AsmProgram;

/* ==========================================================================
 * ENSAMBLADO Y LIBERACIÓN
 * ======================================================================= */

/*
 * asm_assemble — ensambla un programa en dos pasadas.
 *
 * Entrada:
 *   lines  arreglo de nlines cadenas no NULL.
 *   nlines  cantidad de líneas no negativa.
 *   name  nombre no NULL para diagnósticos.
 *   out  programa de salida no NULL.
 * Salida:
 *   0 si ensambla; 1 si hubo errores.
 *   Efectos: inicializa *out y reserva bundles/text; reporta errores en stderr.
 * Restricciones:
 *   liberar con asm_free; ante errores deja out vacío; falta de memoria
 *   termina el proceso.
 */
int asm_assemble(const char *const *lines, int nlines, const char *name,
                 AsmProgram *out);

/*
 * asm_free — libera el programa ensamblado.
 *
 * Entrada:
 *   program  programa inicializado o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera palabras y textos; deja el programa vacío.
 * Restricciones:
 *   ninguna.
 */
void asm_free(AsmProgram *program);

#endif
