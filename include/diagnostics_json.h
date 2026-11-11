/* Servicio JSON de análisis: captura diagnósticos reales, sin otro analizador.
 * Un proceso atiende una solicitud. begin instala el observador y finish escribe
 * una respuesta acotada, libera la captura y devuelve el código final de salida. */
#ifndef DIAGNOSTICS_JSON_H
#define DIAGNOSTICS_JSON_H
#include <stdio.h>
void diag_json_begin(void);
int diag_json_finish(FILE *out, const char *source, int exit_code);
#endif
