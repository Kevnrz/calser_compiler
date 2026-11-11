/* Protocolo --check-json v1. El lexer/parser/semántico siguen reportando por
 * diag_error; aquí solo se conservan y serializan sus datos. No se reinterpretan
 * mensajes para reconocer errores ni se cambia la recuperación del parser. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diagnostics.h"
#include "diagnostics_json.h"

#define MAX_DIAGNOSTICS 128
#define MAX_MESSAGES 65536
#define MAX_MESSAGE 4096

typedef struct {
    const char *category;
    const char *code;
    DiagPosition position;
    char *message;
} JsonDiagnostic;

static JsonDiagnostic entries[MAX_DIAGNOSTICS];
static int entry_count, truncated, failed;
static size_t message_bytes;

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

static void json_string(FILE *out, const char *text) {
    const unsigned char *p = (const unsigned char *) text;
    size_t remaining = strlen(text);
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

static void capture(const char *category, DiagPosition position,
                    const char *format, va_list args, void *unused) {
    (void) unused;
    if (failed || truncated) return;
    if (entry_count == MAX_DIAGNOSTICS) { truncated = 1; return; }
    va_list copy;
    va_copy(copy, args);
    int length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0) { failed = 1; return; }
    if (length > MAX_MESSAGE || message_bytes + (size_t) length > MAX_MESSAGES) {
        truncated = 1; return;
    }
    char *message = malloc((size_t) length + 1);
    if (!message) { failed = 1; return; }
    va_copy(copy, args);
    vsnprintf(message, (size_t) length + 1, format, copy);
    va_end(copy);
    const char *name = "compilation", *code = "CCOM_COMPILATION";
    if (!strcmp(category, "error lexico")) { name = "lexical"; code = "CCOM_LEXICAL"; }
    else if (!strcmp(category, "error sintactico")) { name = "syntax"; code = "CCOM_SYNTAX"; }
    else if (!strcmp(category, "error semantico")) { name = "semantic"; code = "CCOM_SEMANTIC"; }
    entries[entry_count++] = (JsonDiagnostic) {name, code, position, message};
    message_bytes += (size_t) length;
}

void diag_json_begin(void) {
    entry_count = truncated = failed = 0;
    message_bytes = 0;
    /* No duplicar cada diagnóstico en stderr. La CLI clásica mantiene intactos
     * su texto y señaladores; este modo reserva stdout para el protocolo. */
    diag_set_observer(capture, NULL, 0);
}

int diag_json_finish(FILE *out, const char *source, int exit_code) {
    const char *status, *message = "";
    if (failed || (exit_code != 0 && entry_count == 0 && !truncated)) {
        status = "error";
        message = failed ? "No se pudo capturar el informe de diagnosticos." :
                           "No se pudo completar el analisis; consulta stderr.";
        exit_code = 1;
    } else if (truncated) {
        status = "limit"; message = "El informe supera el limite de diagnosticos o mensajes.";
        exit_code = 1;
    } else if (exit_code || entry_count) {
        status = "errors"; exit_code = 1;
    } else status = "ok";

    fprintf(out, "{\"schema_version\":1,\"operation\":\"check\",\"offset_unit\":\"utf8\",\"source\":");
    json_string(out, source);
    fprintf(out, ",\"status\":\"%s\",\"exit_code\":%d,\"error_count\":%d,\"truncated\":%s,\"message\":",
            status, exit_code, diag_count(), truncated ? "true" : "false");
    json_string(out, message);
    fputs(",\"diagnostics\":[", out);
    for (int i = 0; i < entry_count; i++) {
        JsonDiagnostic *d = &entries[i];
        if (i) fputc(',', out);
        fprintf(out, "{\"code\":\"%s\",\"category\":\"%s\",\"severity\":\"error\",\"message\":", d->code, d->category);
        json_string(out, d->message);
        fputs(",\"range\":", out);
        if (d->position.located)
            fprintf(out, "{\"start\":%zu,\"end\":%zu,\"line\":%d,\"column\":%d}",
                    d->position.start, d->position.end, d->position.line, d->position.column);
        else fputs("null", out);
        fputc('}', out);
        free(d->message);
    }
    fputs("]}\n", out);
    entry_count = 0;
    diag_set_observer(NULL, NULL, 1);
    if (fflush(out) != 0 || ferror(out)) {
        fprintf(stderr, "ccom: no se pudo escribir el informe JSON\n");
        return 1;
    }
    return exit_code;
}
