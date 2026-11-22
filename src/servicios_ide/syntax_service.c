/* Análisis sintáctico LL(1) para reparaciones, copias y diagnóstico del editor.
 * Flex aporta rangos reales y detecta errores léxicos antes de reparar.
 * El reconocedor nativo decide aceptación y terminales esperados, sin AST.
 * Cada copia reinicia el estado y cada consulta se ejecuta de forma serial.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntax_service.h"
#include "lexer_service.h"
#include "diagnostics.h"
#include "diagnostics_json.h"
#include "ll1_parser.h"

typedef struct { size_t start, end; const char *kind; } Span;
static Span spans[SYNTAX_TOKEN_LIMIT];
static size_t span_count, span_total;
static SyntaxAnalysis current;

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

static void error_context(const LL1Result *result) {
    current.eof = result->unexpected == 0;
    current.offset = result->offset;
    current.end = result->end;
    for (size_t i = 0; i < span_count; i++) {
        Span s = spans[i];
        if (s.end <= current.offset) {
            current.previous_start = s.start;
            current.previous_end = s.end;
        } else if (s.start == current.offset) {
            current.next_statement = !strcmp(s.kind, "reservada");
            break;
        }
    }
    const char *closers = ";.])";
    for (int i = 0; i < 4; i++) {
        char terminal[] = {closers[i], 0};
        if (ll1_expects(result, terminal)) current.closers |= 1u << i;
    }
}

SyntaxAnalysis syntax_analyze(const char *text, size_t length) {
    memset(&current, 0, sizeof(current));
    span_count = span_total = 0;
    current.offset = current.end = length;
    if (length > SYNTAX_SOURCE_LIMIT) { current.limited = 1; return current; }
    diag_set_observer(lexical_error, NULL, 0);
    diag_init(NULL);
    lexer_set_observer(observe);
    lexer_scan_text(text, length);
    lexer_set_observer(NULL);
    if (!current.lexical && !current.limited) {
        LL1Result parsed = ll1_analyze(text, length, NULL, NULL, NULL);
        current.accepted = parsed.status == LL1_ACCEPTED;
        current.lexical = parsed.status == LL1_LEXICAL_ERROR;
        current.limited = parsed.status == LL1_LIMIT;
        current.failed = parsed.status != LL1_ACCEPTED && parsed.status != LL1_SYNTAX_ERROR &&
                         parsed.status != LL1_LEXICAL_ERROR && parsed.status != LL1_LIMIT;
        if (parsed.status == LL1_SYNTAX_ERROR) error_context(&parsed);
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
    json_write_string(out, source);
    fprintf(out, ",\"exit_code\":%d,\"status\":\"%s\"}\n", code, status);
    free(text);
    return fflush(out) == 0 && !ferror(out) ? code : 1;
}


/* El reconocedor limpia sus observadores, por lo que la captura del informe
 * se instala después de consultarlo. Flex conserva los mensajes léxicos reales.
 * Un error sintáctico se publica desde la posición y pila LL(1), sin Bison.
 */
int syntax_check_ll1(const char *source) {
    size_t length;
    int limited;
    char *text = syntax_read_source(source, &length, &limited);
    LL1Result result = { .status = LL1_INTERNAL_ERROR };
    if (text) result = ll1_analyze(text, length, NULL, NULL, NULL);
    diag_json_ll1_begin();
    diag_init(text ? source : NULL);
    if (limited || result.status == LL1_LIMIT) {
        diag_json_limit();
    } else if (!text || result.status == LL1_INTERNAL_ERROR || result.status == LL1_CANCELLED) {
        /* Sin diagnóstico del lenguaje, finish emite un fallo del servicio. */
        fprintf(stderr, "ccom: no se pudo completar la consulta LL(1)\n");
    } else if (result.status == LL1_LEXICAL_ERROR) {
        lexer_scan_text(text, length);
    } else if (result.status == LL1_SYNTAX_ERROR) {
        char expected[512] = "";
        size_t used = 0;
        for (size_t i = 0; ll1_terminal_name(i); i++) {
            if (!(result.expected & (UINT64_C(1) << i))) continue;
            const char *name = i ? ll1_terminal_name(i) : "fin de archivo";
            int n = snprintf(expected + used, sizeof(expected) - used,
                             "%s'%s'", used ? ", " : "", name);
            if (n < 0 || (size_t)n >= sizeof(expected) - used) break;
            used += (size_t)n;
        }
        int width = (int)(result.end - result.offset);
        const char *call = ll1_expects(&result, "[") && result.offset < length &&
                           text[result.offset] == '(' ? ". Las llamadas usan nombre[argumentos]" : "";
        DiagPosition position = {1, result.line, result.column, result.offset, result.end};
        if (result.unexpected == 0)
            diag_error_at("error sintactico", position,
                       "fin de archivo inesperado. Se esperaba %s%s", expected, call);
        else
            diag_error_at("error sintactico", position,
                       "token inesperado '%.*s'. Se esperaba %s%s",
                       width > 80 ? 80 : width, text + result.offset, expected, call);
    }
    free(text);
    return result.status == LL1_ACCEPTED && !limited ? 0 : 1;
}
