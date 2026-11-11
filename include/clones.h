/* Servicio de clones léxicos: recibe un paquete de instantáneas, nunca abre
 * las rutas de sus documentos. stdout contiene exclusivamente JSON v1. */
#ifndef CLONES_H
#define CLONES_H
#include <stdio.h>
/* main.c aporta paquete CCL1 y mínimo validado 5..100000. Rutas de documentos
 * son metadatos: solo se abre snapshot. Usa un contexto global prestado para
 * Flex; no reentrante. Libera fuentes/tablas y nunca devuelve pares parciales.
 * Rangos de salida: bytes UTF-8. Retorna 0 completo, 1 error/límite/léxico. */
int clones_run(FILE *out, const char *snapshot, unsigned minimum);
#endif
