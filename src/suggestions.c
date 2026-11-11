/* Sugerencias sintácticas desde Flex + Bison/LAC, no desde otra gramática.
 * Conserva los criterios de la entrega Python: fragmento completo, comentarios,
 * prefijo válido, terminales concretos y orden determinista. Sin símbolos
 * inventados, reparaciones ni escritura de la fuente. Una consulta por proceso.
 * main.c -> suggestions_run -> observador Flex -> parser Bison con TOKEN_CURSOR.
 * Estado de contexto estático, no reentrante. text presta el buffer local solo
 * hasta emit; los esperados se copian a words. Todos los rangos son UTF-8 y se
 * refieren al original, aunque solo su prefijo se entregue al parser. Al salir
 * se liberan buffer/AST y observadores. La cancelación la gestiona el padre. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "suggestions.h"
#include "lexer_service.h"
#include "diagnostics.h"
#include "diagnostics_json.h"

#define MAX_SOURCE 200000
#define MAX_TOKENS 50000
#define MAX_OPTIONS 64
static const char *text;
static size_t length, cursor, fragment_start, fragment_end, token_count;
static size_t first_error;
static int in_comment, limited, probe, context_ok, internal_error;
static char words[MAX_OPTIONS][64];
static size_t word_count;
extern FILE *yyin;

int suggestions_active(void) { return probe; }
void suggestions_context(int at_cursor) { context_ok = at_cursor; }
void suggestions_parser_error(void) { internal_error = 1; }

/* Bison entrega sus alias concretos. Se eliminan las comillas externas, nunca
 * se infiere una producción a partir del mensaje humano de error. */
void suggestions_expected(const char *name) {
    size_t n = strlen(name);
    if (n >= 2 && name[0] == '"' && name[n - 1] == '"') { name++; n -= 2; }
    if (!n || n >= sizeof(words[0]) || word_count == MAX_OPTIONS) {
        limited = 1; return;
    }
    for (size_t i = 0; i < word_count; i++)
        if (strlen(words[i]) == n && !memcmp(words[i], name, n)) return;
    memcpy(words[word_count], name, n);
    words[word_count++][n] = 0;
}

static void lexical_error(const char *category, DiagPosition position,
                          const char *format, va_list args, void *data) {
    (void) category; (void) format; (void) args; (void) data;
    size_t start = position.located ? position.start : 0;
    if (start < first_error) first_error = start;
}

/* Observa rangos ya reconocidos por Flex. El sufijo completo puede delimitar el
 * reemplazo, pero sus errores no bloquean una consulta anterior al cursor. */
static void observe(const char *kind, size_t start, size_t end) {
    if (++token_count > MAX_TOKENS) limited = 1;
    if (!strcmp(kind, "comentario")) {
        int block = end > start + 1 && text[start + 1] == '[';
        int closed = block && end >= start + 4 && text[end - 2] == ']' && text[end - 1] == '#';
        if (start < cursor && (cursor < end || (cursor == end && (!block || !closed)))) in_comment = 1;
        return;
    }
    int word = !strcmp(kind, "reservada") || !strcmp(kind, "tipo") ||
               !strcmp(kind, "booleano") || !strcmp(kind, "identificador") || !strcmp(kind, "numero");
    int invalid_prefix = !strcmp(kind, "invalido") && end == start + 1 &&
                         isalpha((unsigned char) text[start]);
    int op = !strcmp(kind, "operador") && strchr("=!<>", text[start]) != NULL;
    if (((word || invalid_prefix) && start < cursor && cursor <= end) ||
        (op && start <= cursor && cursor < end)) {
        fragment_start = start; fragment_end = end;
    }
}

static int priority(const char *word) {
    if (isalpha((unsigned char) word[0])) return 0;
    if (strchr("[]():;,.", word[0])) return 1;
    return 2;
}
static int compare(const void *a, const void *b) {
    const char *x = a, *y = b;
    int p = priority(x) - priority(y);
    return p ? p : strcmp(x, y);
}

static int emit(FILE *out, const char *source, const char *status, const char *explanation) {
    int code = !strcmp(status, "error") || !strcmp(status, "limite") ? 1 : 0;
    fputs("{\"schema_version\":1,\"operation\":\"suggest\",\"offset_unit\":\"utf8\",\"source\":", out);
    diag_json_write_string(out, source);
    fprintf(out, ",\"cursor\":%zu,\"exit_code\":%d,\"status\":", cursor, code);
    diag_json_write_string(out, status);
    fputs(",\"explanation\":", out); diag_json_write_string(out, explanation);
    fputs(",\"suggestions\":[", out);
    if (!strcmp(status, "ok")) {
        size_t prefix = cursor - fragment_start;
        int emitted = 0;
        for (size_t i = 0; i < word_count; i++) {
            const char *word = words[i];
            if (prefix && (strlen(word) < prefix || memcmp(word, text + fragment_start, prefix))) continue;
            /* No presentar reemplazos idénticos al texto ya terminado. En el
             * medio de palabra sí se permite completar y mover al final. */
            if (fragment_end == cursor && prefix == strlen(word) && prefix) continue;
            if (emitted++) fputc(',', out);
            fputs("{\"label\":", out); diag_json_write_string(out, word);
            fputs(",\"text\":", out); diag_json_write_string(out, word);
            fprintf(out, ",\"start\":%zu,\"end\":%zu,\"category\":", fragment_start, fragment_end);
            diag_json_write_string(out, priority(word) == 0 ? "palabra" : priority(word) == 1 ? "delimitador" : "operador");
            fputs(",\"detail\":\"Continuacion permitida por el parser de ccom (Bison/LAC).\"}", out);
        }
    }
    fputs("]}\n", out);
    return fflush(out) == 0 && !ferror(out) ? code : 1;
}

int suggestions_run(FILE *out, const char *source, size_t requested_cursor) {
    cursor = requested_cursor;
    fragment_start = fragment_end = cursor;
    token_count = word_count = 0;
    in_comment = limited = probe = context_ok = internal_error = 0;
    first_error = (size_t) -1;
    FILE *file = fopen(source, "rb");
    if (!file) return emit(out, source, "error", "No se pudo abrir la fuente.");
    char *buffer = malloc(MAX_SOURCE + 1);
    if (!buffer) { fclose(file); return emit(out, source, "error", "Sin memoria para la consulta."); }
    length = fread(buffer, 1, MAX_SOURCE + 1, file);
    int read_error = ferror(file);
    if (read_error || length > MAX_SOURCE) {
        fclose(file); free(buffer);
        return emit(out, source, read_error ? "error" : "limite", "Entrada no disponible o superior a 200000 bytes.");
    }
    text = buffer;
    if (cursor > length || (cursor < length && ((unsigned char) text[cursor] & 0xc0) == 0x80)) {
        fclose(file); free(buffer);
        return emit(out, source, "error", "El cursor no es una frontera UTF-8 de la fuente.");
    }
    rewind(file); yyin = file;
    diag_set_observer(lexical_error, NULL, 0);
    diag_init(source);
    lexer_set_observer(observe);
    lexer_scan_only();
    lexer_set_observer(NULL);
    fclose(file);
    const char *status, *explanation;
    if (limited) { status = "limite"; explanation = "La consulta supera 50000 tokens."; }
    else if (in_comment) { status = "bloqueado"; explanation = "Cursor dentro de un comentario."; }
    else if (first_error < fragment_start) { status = "bloqueado"; explanation = "Un error lexico anterior impide obtener el contexto."; }
    else {
        probe = 1;
        int parsed = lexer_parse_text(text, fragment_start);
        probe = 0;
        if (internal_error || parsed == 2) { status = "error"; explanation = "No se pudo consultar el contexto del parser."; }
        else if (limited) { status = "limite"; explanation = "Demasiadas continuaciones del parser."; }
        else if (!context_ok) { status = "bloqueado"; explanation = "Un error sintactico anterior impide obtener el contexto."; }
        else {
            qsort(words, word_count, sizeof(words[0]), compare);
            size_t options = 0, prefix = cursor - fragment_start;
            for (size_t i = 0; i < word_count; i++) {
                if (prefix && (strlen(words[i]) < prefix || memcmp(words[i], text + fragment_start, prefix))) continue;
                if (fragment_end == cursor && prefix == strlen(words[i]) && prefix) continue;
                options++;
            }
            status = options ? "ok" : "sin_opciones";
            explanation = options ? "" : "No hay terminales concretos para este fragmento; no se inventan nombres ni numeros.";
        }
    }
    int code = emit(out, source, status, explanation);
    diag_set_observer(NULL, NULL, 1);
    diag_free();
    free(buffer); text = NULL;
    return code;
}
