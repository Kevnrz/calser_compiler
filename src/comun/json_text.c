/*
 * json_text.c — escritura común de cadenas JSON y bytes UTF-8.
 * Usa: json_text.h y biblioteca C; no depende del AST ni de los servicios IDE.
 * Lo usan el AST, las métricas y los servicios estructurados del compilador.
 * Referencias: enunciado §4.4; preserva el protocolo de consultas de C6.
 */
#include "json_text.h"
#include <string.h>

/* Aceptar solo secuencias UTF-8 canónicas; los bytes inválidos se escapan al
 * serializar para no producir JSON ilegible. No es tokenización del lenguaje. */
static size_t utf8_sequence(const unsigned char *p, size_t remaining) {
    unsigned c = p[0];
    size_t n = c >= 0xc2 && c <= 0xdf ? 2 :
               c >= 0xe0 && c <= 0xef ? 3 : c >= 0xf0 && c <= 0xf4 ? 4 : 0;
    if (!n || n > remaining) return 0;
    for (size_t i = 1; i < n; i++)
        if ((p[i] & 0xc0) != 0x80) return 0;
    if ((c == 0xe0 && p[1] < 0xa0) || (c == 0xed && p[1] >= 0xa0) ||
        (c == 0xf0 && p[1] < 0x90) || (c == 0xf4 && p[1] >= 0x90)) return 0;
    return n;
}

void json_write_bytes(FILE *out, const char *text, size_t length) {
    const unsigned char *p = (const unsigned char *) text;
    size_t remaining = length;
    fputc('"', out);
    while (remaining) {
        unsigned c = *p;
        if (c == '"' || c == '\\') { fputc('\\', out); fputc(c, out); }
        else if (c < 0x20) fprintf(out, "\\u%04x", c);
        else if (c < 0x80) fputc(c, out);
        else {
            size_t n = utf8_sequence(p, remaining);
            if (n) { fwrite(p, 1, n, out); p += n; remaining -= n; continue; }
            fprintf(out, "\\u%04x", c);
        }
        p++; remaining--;
    }
    fputc('"', out);
}

void json_write_string(FILE *out, const char *text) {
    json_write_bytes(out, text, strlen(text));
}

