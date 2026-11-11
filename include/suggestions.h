/* Asistencia sintáctica de ccom. Una consulta por proceso, sin semántica ni
 * artefactos. El parser comparte sus esperados LAC; no hay otra gramática. */
#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H
#include <stdio.h>
/* main.c llama una vez por proceso. Lee una copia acotada, instala observadores
 * de Flex/Bison y los desconecta al acabar. Retiene solo índices/palabras propios.
 * cursor: frontera UTF-8 absoluta. No modifica la fuente; libera buffers/AST.
 * Retorna 0 para ok/sin_opciones/bloqueado y 1 para error/limite de servicio.
 * La cancelación del IDE termina este proceso; aquí no hay hilos de Qt. */
int suggestions_run(FILE *out, const char *source, size_t cursor);
/* Hooks internos del parser; no son otra API de análisis del lenguaje. */
int suggestions_active(void);
void suggestions_context(int at_cursor);
void suggestions_expected(const char *name);
void suggestions_parser_error(void);
/* Contexto de función publicado por la producción real de Bison. */
void suggestions_function(const char *name);
#endif
