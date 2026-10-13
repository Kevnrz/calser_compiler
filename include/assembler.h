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

/* Ensambla nlines líneas. name es el nombre para los mensajes de error (el
 * archivo .asm o "<generado>"); las líneas se numeran desde 1.
 * Devuelve 0 si no hubo errores y deja el programa en *out (liberar con
 * asm_free). Si hubo errores los reporta todos por stderr con el formato
 *   ccom: <nombre>:<linea>: error de ensamblado: <mensaje>
 * devuelve 1 y deja *out vacío. */
int asm_assemble(const char *const *lines, int nlines, const char *name,
                 AsmProgram *out);

void asm_free(AsmProgram *program);

#endif
