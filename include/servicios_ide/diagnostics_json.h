/* Servicio JSON de análisis que captura diagnósticos reales de los motores C.
 * Un proceso atiende una solicitud. begin instala el observador y finish escribe
 * una respuesta acotada, libera la captura y devuelve el código final de salida. */
#ifndef DIAGNOSTICS_JSON_H
#define DIAGNOSTICS_JSON_H
#include <stdio.h>
#include "json_text.h"
void diag_json_begin(void);
/* §12: permite anclar errores de módulos solo cuando hay captura JSON. */
int diag_json_active(void);
/* Informe del editor con sintaxis LL(1). Conserva rangos y categorías v1. */
void diag_json_ll1_begin(void);
/* Impide publicar un análisis limitado como un documento correcto. */
void diag_json_limit(void);
/* begin/finish corresponden a una consulta M1/M2 por proceso, no son
 * reentrantes. Captura mensajes copiados; kind de Flex es estático prestado.
 * finish desconecta el observador y libera captura. No autoriza reparaciones.
 * Cancelación: el padre termina el proceso; este módulo no toca widgets.
 */
/* Modo léxico independiente del parser: el propio lexer emite los estilos y
 * rangos de bytes; comentarios incluidos, sin copiar lexemas en el informe. */
void diag_json_lexical_begin(void);
void diag_json_token(const char *kind, size_t start, size_t end);
int diag_json_finish(FILE *out, const char *source, int exit_code);
#endif
