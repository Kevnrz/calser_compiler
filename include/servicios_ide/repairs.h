/* Reparaciones conservadoras: inserciones de cierres esperados por Bison/LAC,
 * verificadas sobre copias completas. Una consulta por proceso; sin guardar. */
#ifndef REPAIRS_H
#define REPAIRS_H
#include <stdio.h>
/* Lo llama main.c para --repair-json. UTF-8 [start,end) en fuente original;
 * cada candidato se analiza en memoria con syntax_service. Buffers locales
 * liberados antes de retornar; no se guarda nada. Retorna código JSON 0/1.
 * Cancelar desde el IDE mata el proceso, no modifica el texto del editor. */
int repairs_run(FILE *out, const char *source);
#endif
