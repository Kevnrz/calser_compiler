/* Contexto del primer error y verificación completa sobre copias. Los bits de
 * cierres proceden de los esperados LAC; ninguna producción se duplica aquí.
 * Consumidores: repairs.c (varias copias) y main.c (--syntax-json). current y
 * spans son globales reiniciados por llamada, no un parser concurrente. Se
 * copian rangos UTF-8, no AST. Los destructores Bison liberan los árboles de
 * consulta; se desconectan observadores y se destruye Flex antes de retornar.
 * El llamador conserva la propiedad del texto y recibe un resultado por valor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntax_service.h"
#include "lexer_service.h"
#include "diagnostics.h"
#include "diagnostics_json.h"

typedef struct { size_t start, end; const char *kind; } Span;
static Span spans[SYNTAX_TOKEN_LIMIT];
static size_t span_count, span_total, source_length;
static SyntaxAnalysis current;
static int active;
int syntax_service_active(void) { return active; }
void syntax_service_failed(void) { current.failed = 1; }

static void observe(const char *kind, size_t start, size_t end) {
    if (++span_total > SYNTAX_TOKEN_LIMIT) { current.limited = 1; return; }
    if (!strcmp(kind, "comentario") || !strcmp(kind, "invalido")) return;
    spans[span_count++] = (Span) {start, end, kind};
}
static void lexical_error(const char *category, DiagPosition position,
                          const char *format, va_list args, void *unused) {
    (void) category; (void) position; (void) format; (void) args; (void) unused;
    current.lexical = 1;
}
void syntax_service_error(int eof) {
    current.eof = eof;
    current.offset = eof ? source_length : lexer_current_start();
    current.end = eof ? source_length : lexer_current_end();
    for (size_t i = 0; i < span_count; i++) {
        Span s = spans[i];
        if (s.end <= current.offset) {
            current.previous_start = s.start;
            current.previous_end = s.end;
        } else if (s.start == current.offset) {
            /* Reservadas de inicio de sentencia; tipos y booleanos tienen
             * estilos propios y no cuentan como comienzo inequívoco. */
            current.next_statement = !strcmp(s.kind, "reservada");
            break;
        }
    }
}
void syntax_service_expected(const char *name) {
    /* Alias concretos de Bison, no mensajes de error ni una gramática local. */
    size_t n = strlen(name);
    if (n == 3 && name[0] == '"' && name[2] == '"') { name++; n = 1; }
    const char *closers = ";.])";
    if (n == 1) for (int i = 0; i < 4; i++) if (name[0] == closers[i]) current.closers |= 1u << i;
}

SyntaxAnalysis syntax_analyze(const char *text, size_t length) {
    memset(&current, 0, sizeof(current));
    span_count = span_total = 0;
    source_length = length;
    current.offset = current.end = length;
    if (length > SYNTAX_SOURCE_LIMIT) { current.limited = 1; return current; }
    diag_set_observer(lexical_error, NULL, 0);
    diag_init(NULL);
    lexer_set_observer(observe);
    lexer_scan_text(text, length);
    lexer_set_observer(NULL);
    if (!current.lexical && !current.limited) {
        active = 1;
        int parsed = lexer_parse_text(text, length); /* EOF real, sin TOKEN_CURSOR */
        active = 0;
        current.accepted = parsed == 0 && !current.failed;
        if (parsed == 2) current.failed = 1;
    }
    diag_set_observer(NULL, NULL, 1);
    diag_free();
    return current;
}

char *syntax_read_source(const char *source, size_t *length, int *limited) {
    *limited = 0; *length = 0;
    FILE *file = fopen(source, "rb");
    if (!file) return NULL;
    char *text = malloc(SYNTAX_SOURCE_LIMIT + 1);
    if (!text) { fclose(file); return NULL; }
    *length = fread(text, 1, SYNTAX_SOURCE_LIMIT + 1, file);
    int failed = ferror(file);
    fclose(file);
    if (failed || *length > SYNTAX_SOURCE_LIMIT) {
        *limited = *length > SYNTAX_SOURCE_LIMIT;
        free(text); return NULL;
    }
    return text;
}
int syntax_json_run(FILE *out, const char *source) {
    size_t length; int limited;
    char *text = syntax_read_source(source, &length, &limited);
    SyntaxAnalysis result = {0};
    if (text) result = syntax_analyze(text, length);
    const char *status = limited || result.limited ? "limite" : !text || result.failed ? "error" :
                         result.lexical ? "error_lexico" : result.accepted ? "aceptado" : "error_sintactico";
    int code = strcmp(status, "aceptado") != 0;
    fputs("{\"schema_version\":1,\"operation\":\"syntax\",\"offset_unit\":\"utf8\",\"source\":", out);
    diag_json_write_string(out, source);
    fprintf(out, ",\"exit_code\":%d,\"status\":\"%s\"}\n", code, status);
    free(text);
    return fflush(out) == 0 && !ferror(out) ? code : 1;
}
