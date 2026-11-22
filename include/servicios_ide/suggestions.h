/* Asistencia sintáctica de ccom. Una consulta por proceso, sin semántica ni
 * artefactos. El parser predictivo LL(1) aporta las continuaciones posibles. */
#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H
#include <stdio.h>
/* main.c llama una vez por proceso. Lee una copia acotada, instala observadores
 * de Flex y LL(1) y los desconecta al acabar. Retiene solo índices/palabras propios.
 * cursor: frontera UTF-8 absoluta. No modifica la fuente; libera los buffers de consulta.
 * Retorna 0 para ok/sin_opciones/bloqueado y 1 para error/limite de servicio.
 * La cancelación del IDE termina este proceso; aquí no hay hilos de Qt. */
int suggestions_run(FILE *out, const char *source, size_t cursor);
#endif
