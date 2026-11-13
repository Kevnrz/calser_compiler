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
static size_t first_error, return_end;
static int in_comment, limited, probe, context_ok, internal_error, complete_fragment;
static char words[MAX_OPTIONS][64];
static size_t word_count;
static int function_main, previous_f, main_name_at_cursor, previous_e, return_ended_at_cursor;
extern FILE *yyin;

int suggestions_active(void) { return probe; }
void suggestions_context(int at_cursor) { context_ok = at_cursor; }
void suggestions_parser_error(void) { internal_error = 1; }
void suggestions_function(const char *name) { function_main = name && !strcmp(name, "main"); }

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
    /* Flex ya identificó los tokens. Distinguir el nombre de una declaración
     * F main de un '[' dentro de su cuerpo, sin extender la plantilla allí. */
    if (end <= cursor && strcmp(kind, "invalido")) {
        main_name_at_cursor = previous_f && !strcmp(kind, "identificador") &&
                              end - start == 4 && !memcmp(text + start, "main", 4);
        previous_f = !strcmp(kind, "reservada") && end - start == 1 && text[start] == 'F';
        return_ended_at_cursor = previous_e && end - start == 1 && text[start] == ';';
        if (return_ended_at_cursor) return_end = end;
        previous_e = !strcmp(kind, "reservada") && end - start == 1 && text[start] == 'E';
    }
    int word = !strcmp(kind, "reservada") || !strcmp(kind, "tipo") ||
               !strcmp(kind, "booleano") || !strcmp(kind, "identificador") || !strcmp(kind, "numero");
    int invalid_prefix = !strcmp(kind, "invalido") && end == start + 1 &&
                         isalpha((unsigned char) text[start]);
    int op = !strcmp(kind, "operador") && strchr("=!<>", text[start]) != NULL;
    if (((word || invalid_prefix) && start < cursor && cursor <= end) ||
        (op && start <= cursor && cursor < end)) {
        fragment_start = start; fragment_end = end;
        complete_fragment = word;
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

/* Filtrado común para contar y emitir exactamente las mismas propuestas. */
static int offer(const char *word) {
    size_t prefix = cursor - fragment_start, n = strlen(word);
    if (prefix && (n < prefix || memcmp(word, text + fragment_start, prefix))) return 0;
    if (fragment_end == cursor && prefix == n && prefix) return 0;
    /* No duplicar un delimitador que ya está inmediatamente a la derecha. */
    size_t next = cursor;
    while (next < length && isspace((unsigned char) text[next])) next++;
    if (!prefix && n == 1 && !isalpha((unsigned char) word[0]) && next < length && text[next] == word[0]) return 0;
    return 1;
}
static size_t option_count(void) {
    size_t count = 0;
    for (size_t i = 0; i < word_count; i++) count += offer(words[i]);
    return count;
}
static int query_context(size_t position) {
    word_count = 0; context_ok = internal_error = function_main = 0;
    probe = 1;
    int result = lexer_parse_text(text, position);
    probe = 0;
    return result;
}

/* Una continuación sintáctica no implica intención de cerrar una función
 * o iniciar otra. La lista conserva todas las opciones; el gris solo guía
 * prefijos, tipos y puntuación local. Este criterio pertenece al motor C. */
static int inline_preferred(const char *word, size_t count) {
    size_t prefix = cursor - fragment_start;
    if (prefix) return 1;
    /* El cierre se propone en una línea posterior a E;, nunca pegado a él. */
    if (!strcmp(word, ".")) return return_ended_at_cursor &&
        memchr(text + return_end, '\n', cursor - return_end) != NULL;
    if (!strcmp(word, "F")) return 0;
    if (!strcmp(word, ";")) return 1;
    int types = 1, return_types = 0;
    for (size_t i = 0; i < word_count; i++) {
        if (!offer(words[i])) continue;
        if (!strcmp(words[i], "void")) return_types = 1;
        if (strcmp(words[i], "bool") && strcmp(words[i], "int") && strcmp(words[i], "void")) types = 0;
    }
    if (types) return !function_main || !return_types || !strcmp(word, "void");
    return count == 1 && priority(word) == 1;
}

static int emit(FILE *out, const char *source, const char *status, const char *explanation) {
    int code = !strcmp(status, "error") || !strcmp(status, "limite") ? 1 : 0;
    fputs("{\"schema_version\":1,\"operation\":\"suggest\",\"offset_unit\":\"utf8\",\"source\":", out);
    json_write_string(out, source);
    fprintf(out, ",\"cursor\":%zu,\"exit_code\":%d,\"status\":", cursor, code);
    json_write_string(out, status);
    fputs(",\"explanation\":", out); json_write_string(out, explanation);
    fputs(",\"suggestions\":[", out);
    if (!strcmp(status, "ok")) {
        size_t prefix = cursor - fragment_start, count = option_count();
        int emitted = 0;
        for (size_t i = 0; i < word_count; i++) {
            const char *word = words[i];
            if (!offer(word)) continue;
            /* Un '[' inequívoco puede presentarse como par editable, sin
             * inventar nombres/valores. No duplicar un cierre ya existente. */
            size_t next = cursor;
            while (next < length && isspace((unsigned char) text[next])) next++;
            int pair = !prefix && count == 1 && !strcmp(word, "[") &&
                       !(next < length && text[next] == ']');
            int main_header = pair && function_main && main_name_at_cursor;
            int existing_colon = next < length && text[next] == ':';
            const char *insert = main_header && !existing_colon ? "[]: " : pair ? "[]" : word;
            if (emitted++) fputc(',', out);
            fputs("{\"label\":", out); json_write_string(out, insert);
            fputs(",\"text\":", out); json_write_string(out, insert);
            fprintf(out, ",\"start\":%zu,\"end\":%zu,\"category\":", fragment_start, fragment_end);
            json_write_string(out, priority(word) == 0 ? "palabra" : priority(word) == 1 ? "delimitador" : "operador");
            fprintf(out, ",\"inline_preferred\":%s", inline_preferred(word, count) ? "true" : "false");
            if (pair) fprintf(out, ",\"cursor_relative\":%d", main_header ? (existing_colon ? 2 : 4) : 1);
            fputs(",\"detail\":", out);
            json_write_string(out, main_header ? "Cabecera de main sin parametros; continuar con retorno void." : pair ? "Par de corchetes esperado; el cursor queda dentro para escribir parametros." : "Continuacion permitida por el parser de ccom (Bison/LAC).");
            fputc('}', out);
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
    complete_fragment = 0;
    previous_f = main_name_at_cursor = previous_e = return_ended_at_cursor = 0;
    return_end = 0;
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
    const char *status = NULL, *explanation = NULL;
    if (limited) { status = "limite"; explanation = "La consulta supera 50000 tokens."; }
    else if (in_comment) { status = "bloqueado"; explanation = "Cursor dentro de un comentario."; }
    else if (first_error < fragment_start) { status = "bloqueado"; explanation = "Un error lexico anterior impide obtener el contexto."; }
    else {
        int parsed = query_context(fragment_start);
        if (internal_error || parsed == 2) { status = "error"; explanation = "No se pudo consultar el contexto del parser."; }
        else if (limited) { status = "limite"; explanation = "Demasiadas continuaciones del parser."; }
        else if (!context_ok) { status = "bloqueado"; explanation = "Un error sintactico anterior impide obtener el contexto."; }
        else {
            qsort(words, word_count, sizeof(words[0]), compare);
            size_t options = option_count();
            /* Un identificador ya escrito (main) no es un prefijo de una
             * reservada. Consultar después del token permite ofrecer '['.
             * El mismo paso avanza después de tipos/palabras ya completos.
             * Un prefijo como 'vo' conserva primero su reemplazo por 'void'. */
            if (!options && complete_fragment && fragment_start < cursor &&
                    fragment_end == cursor && first_error >= cursor) {
                int next = query_context(cursor);
                if (internal_error || next == 2) {
                    status = "error"; explanation = "No se pudo consultar la continuacion del token.";
                } else if (limited) {
                    status = "limite"; explanation = "Demasiadas continuaciones del parser.";
                } else if (context_ok) {
                    fragment_start = fragment_end = cursor;
                    qsort(words, word_count, sizeof(words[0]), compare);
                    options = option_count();
                }
            }
            if (!status) {
                status = options ? "ok" : "sin_opciones";
                explanation = options ? "" : "No hay terminales concretos para este fragmento; no se inventan nombres ni numeros.";
            }
        }
    }
    int code = emit(out, source, status, explanation);
    diag_set_observer(NULL, NULL, 1);
    diag_free();
    free(buffer); text = NULL;
    return code;
}
