/*
 * json_text.h — escapado común de texto para las salidas JSON de ccom.
 * Usa: biblioteca C. Lo usan AST, métricas y servicios del IDE.
 * Referencias: enunciado §4.4; contrato UTF-8 de los servicios de C6.
 */
#ifndef JSON_TEXT_H
#define JSON_TEXT_H
#include <stdio.h>
#include <stddef.h>

/* Entrada: flujo y length bytes legibles. Salida: ninguna; escribe comillas y
 * escapes. Restricciones: UTF-8 canónico se conserva; bytes inválidos se escapan.
 * No cierra el flujo, no interpreta el lenguaje ni modifica el texto. */
void json_write_bytes(FILE *out, const char *text, size_t length);

/* Entrada: flujo y texto no NULL terminado en NUL. Salida: ninguna.
 * Efectos y restricciones: los de json_write_bytes; longitud obtenida con strlen. */
void json_write_string(FILE *out, const char *text);
#endif
