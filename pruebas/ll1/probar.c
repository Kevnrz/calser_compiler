/* Pruebas nativas y consulta de desarrollo del reconocedor completo.
 * yyparse está interceptado al enlazar este ejecutable. Cualquier llamada
 * accidental a Bison aborta la prueba en vez de dar un resultado aparente.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ll1_parser.h"
#include "lexer_service.h"
#include "json_text.h"
#include "suggestions.h"
#include "repairs.h"
#include "syntax_service.h"
#include "diagnostics_json.h"
#include "diagnostics.h"

int __wrap_yyparse(void) {
    fputs("El reconocedor LL(1) intentó llamar a Bison\n", stderr);
    abort();
}

#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "Falló la comprobación de la línea %d\n", __LINE__); exit(1); \
} } while (0)

static LL1Result analyze(const char *text) {
    return ll1_analyze(text, strlen(text), NULL, NULL, NULL);
}

static int cancel_after(void *data) {
    int *remaining = data;
    return (*remaining)-- <= 0;
}

static LL1Context context_at_end(const char *text) {
    size_t length = strlen(text);
    return ll1_context(text, length, length, length, NULL, NULL, NULL);
}

static void context_tests(void) {
    LL1Context c = context_at_end("");
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && !c.can_end);
    REQUIRE(ll1_expects(&c.analysis, "F") && ll1_expects(&c.analysis, "Vm"));
    c = context_at_end("F main");
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && ll1_expects(&c.analysis, "["));
    c = context_at_end("F main [] :");
    REQUIRE(ll1_expects(&c.analysis, "bool") && ll1_expects(&c.analysis, "int") && ll1_expects(&c.analysis, "void"));
    const char *partial = "F main [] : vo";
    size_t point = (size_t)(strstr(partial, "vo") - partial);
    c = ll1_context(partial, strlen(partial), strlen(partial), point, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && ll1_expects(&c.analysis, "void"));
    REQUIRE(c.analysis.offset == point && c.point == point && c.cursor == strlen(partial));
    c = context_at_end(partial);
    REQUIRE(c.analysis.status == LL1_SYNTAX_ERROR);
    c = ll1_context("V", 1, 1, 0, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && c.analysis.lexical_errors == 0);
    c = context_at_end("F main [] : void ; Vm x : int = ");
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && ll1_expects(&c.analysis, "num") && ll1_expects(&c.analysis, "["));
    c = context_at_end("F main [] : void ; x = ");
    REQUIRE(ll1_expects(&c.analysis, "num") && !ll1_expects(&c.analysis, "["));
    c = context_at_end("F main [] : void ; x = datos[1");
    REQUIRE(ll1_expects(&c.analysis, "]") && !ll1_expects(&c.analysis, ";"));
    const char *program = "F main [] : void ; E ; .";
    c = context_at_end(program);
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && c.can_end);
    REQUIRE(ll1_expects(&c.analysis, "$") && ll1_expects(&c.analysis, "F"));
    REQUIRE(analyze(program).status == LL1_ACCEPTED);
    const char *suffix = "F main [] : void ; x = @@@";
    size_t cursor = (size_t)(strchr(suffix, '@') - suffix);
    c = ll1_context(suffix, strlen(suffix), cursor, cursor, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && c.analysis.lexical_errors == 0);
    c = context_at_end("F main [] : void ; Vm x : int = ; x = ");
    REQUIRE(c.analysis.status == LL1_SYNTAX_ERROR && !c.can_end);
    c = context_at_end("@ F main");
    REQUIRE(c.analysis.status == LL1_LEXICAL_ERROR && !c.can_end);
    c = context_at_end("# texto");
    REQUIRE(c.analysis.status == LL1_IN_COMMENT && c.comment == LL1_LINE_COMMENT);
    REQUIRE(c.analysis.expected == 0 && !c.can_end && c.comment_start == 0);
    c = context_at_end("#[ comentario abierto");
    REQUIRE(c.analysis.status == LL1_IN_COMMENT && c.comment == LL1_BLOCK_COMMENT);
    REQUIRE(c.analysis.lexical_errors == 0 && c.analysis.expected == 0);
    REQUIRE(analyze("#[ comentario abierto").status == LL1_LEXICAL_ERROR);
    c = context_at_end("#[ cerrado ]#");
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && ll1_expects(&c.analysis, "F"));
    c = context_at_end("# comentario\n");
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && c.comment == LL1_NO_COMMENT);
    const char *inside = "F main [] : void ; #[ texto ]# E ; . @";
    cursor = (size_t)(strstr(inside, "texto") - inside) + 3;
    c = ll1_context(inside, strlen(inside), cursor, cursor, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_IN_COMMENT && c.comment == LL1_BLOCK_COMMENT);
    const char *unicode = "# á😀\r\nF main [] : void ; x = ";
    c = context_at_end(unicode);
    REQUIRE(c.analysis.status == LL1_PREFIX_VALID && c.analysis.offset == strlen(unicode));
    REQUIRE(c.analysis.line == 2 && c.analysis.column == 24);
    c = ll1_context("# á", strlen("# á"), 3, 3, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_INVALID_CONTEXT && c.analysis.expected == 0);
    c = ll1_context("F main", 6, 6, 4, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_INVALID_CONTEXT && c.analysis.expected == 0);
    c = ll1_context("F main", 6, 7, 7, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_INVALID_CONTEXT);
    c = ll1_context("F main", 6, 4, 5, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_INVALID_CONTEXT);
    c = ll1_context("a b", 3, 3, 0, NULL, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_INVALID_CONTEXT);
    LL1Limits limits = ll1_default_limits(); limits.steps = 2;
    c = ll1_context(program, strlen(program), strlen(program), strlen(program), &limits, NULL, NULL);
    REQUIRE(c.analysis.status == LL1_LIMIT && c.analysis.limit == LL1_STEP_LIMIT && !c.can_end);
    int remaining = 4;
    c = ll1_context(program, strlen(program), strlen(program), strlen(program), NULL, cancel_after, &remaining);
    REQUIRE(c.analysis.status == LL1_CANCELLED && !c.can_end);
    for (int i = 0; i < 100; i++) {
        REQUIRE(context_at_end("#[ abierto").analysis.status == LL1_IN_COMMENT);
        REQUIRE(context_at_end(program).can_end);
    }
}

static void self_test(void) {
    LL1GrammarInfo info;
    REQUIRE(ll1_grammar_info(&info));
    REQUIRE(info.terminals == 49 && info.nonterminals == 61 && info.productions == 120);
    REQUIRE(info.cells == 341 && info.nullable == 22 && info.conflicts == 0);
    const char *valid[] = {
        "F main [] : void ; E ; .",
        "F f [] : int r ; r = desconocida + 1 ; E ; .",
        "Vm g : int = 3 ; F main [] : void ; g = 4 ; .",
        "F main [] : void ; Vm a : int [2][2] = [[1,2],[3,4]] ; a[0][1] = 8 ; E ; .",
        "F main [] : void ; C [verdadero] ; E ; A [falso] ; E ; A ; E ; . .",
        "F main [] : void ; Vm x : int = -2^2 + 2^-3 + 2^3^4 ; E ; .",
        "#[ comentario ]#\r\nF main [] : void ; # comentario final\nE ; .\n",
    };
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++)
        REQUIRE(analyze(valid[i]).status == LL1_ACCEPTED);
    const char *invalid[] = {"", "# comentario", "F main [] : void ; E ;", "F main [] : void ; . sobra",
                             "F main [] : void ; Vm x : int = ; E ; .", "F main () : void ; E ; .",
                             "F main [] : void ; f(1) ; E ; ."};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        REQUIRE(analyze(invalid[i]).status == LL1_SYNTAX_ERROR);
    LL1Result r = analyze("");
    REQUIRE(r.offset == 0 && r.end == 0 && r.line == 1 && r.column == 1);
    REQUIRE(ll1_expects(&r, "F") && ll1_expects(&r, "Vm") && ll1_expects(&r, "Vi"));
    REQUIRE(!ll1_expects(&r, "$") && !ll1_expects(&r, "TOKEN_CURSOR"));
    r = analyze("F main [] : void ;\r\nE ;\r\n");
    REQUIRE(r.status == LL1_SYNTAX_ERROR && r.line == 3 && r.column == 1);
    REQUIRE(r.offset == r.end && ll1_expects(&r, "."));
    const char *missing = "F main [] : void ; Vm x : int = ; E ; .";
    r = analyze(missing);
    REQUIRE(r.offset == (size_t)(strstr(missing, "= ;") - missing) + 2);
    REQUIRE(ll1_expects(&r, "num") && ll1_expects(&r, "id") && !ll1_expects(&r, ";"));
    r = analyze("F main [] : void ;\nvs\nE ; .");
    REQUIRE(r.status == LL1_SYNTAX_ERROR && r.line == 3 && r.column == 1);
    REQUIRE(ll1_expects(&r, "=") && ll1_expects(&r, "["));
    REQUIRE(!ll1_expects(&r, ";") && !ll1_expects(&r, "]"));
    REQUIRE(analyze("F nombre_excesivamente_largo_para_el_lexer [] : void ; .").status == LL1_LEXICAL_ERROR);
    /* Una profundidad real excesiva debe informar un límite sin desbordar
     * la pila de C. La consulta siguiente debe seguir funcionando. */
    const char *prefix = "F main [] : void ; Vm v : int = ";
    size_t prefix_length = strlen(prefix);
    char *deep = malloc(prefix_length + 2 * LL1_STACK_MAX + 20);
    REQUIRE(deep != NULL);
    memcpy(deep, prefix, prefix_length);
    memset(deep + prefix_length, '(', LL1_STACK_MAX);
    deep[prefix_length + LL1_STACK_MAX] = '1';
    memset(deep + prefix_length + LL1_STACK_MAX + 1, ')', LL1_STACK_MAX);
    strcpy(deep + prefix_length + 2 * LL1_STACK_MAX + 1, " ; E ; .");
    r = analyze(deep);
    free(deep);
    REQUIRE(r.status == LL1_LIMIT && r.limit == LL1_STACK_LIMIT);
    const char *lexical = "# á😀\r\nF main [] : void ;\r\n@ E ; .";
    r = analyze(lexical);
    REQUIRE(r.status == LL1_LEXICAL_ERROR && r.lexical_errors == 1);
    REQUIRE(r.offset == (size_t)(strchr(lexical, '@') - lexical) && r.end == r.offset + 1);
    REQUIRE(r.line == 3 && r.column == 1);
    const char *comment = "F main [] : void ;\n  #[ sin cerrar";
    r = analyze(comment);
    REQUIRE(r.status == LL1_LEXICAL_ERROR && r.line == 2 && r.column == 3);
    REQUIRE(r.offset == (size_t)(strstr(comment, "#[") - comment) && r.end == r.offset + 2);
    REQUIRE(analyze("F main [] : void ; Vm x : int = 2147483649 ; .").status == LL1_LEXICAL_ERROR);
    const char nul[] = {'F', ' ', '\0', 'x'};
    REQUIRE(ll1_analyze(nul, sizeof(nul), NULL, NULL, NULL).status == LL1_LEXICAL_ERROR);
    const char *program = valid[0];
    LL1Limits limits = ll1_default_limits();
    limits.source = 2;
    r = ll1_analyze(program, strlen(program), &limits, NULL, NULL);
    REQUIRE(r.status == LL1_LIMIT && r.limit == LL1_SOURCE_LIMIT);
    limits = ll1_default_limits(); limits.tokens = 3;
    r = ll1_analyze(program, strlen(program), &limits, NULL, NULL);
    REQUIRE(r.status == LL1_LIMIT && r.limit == LL1_TOKEN_LIMIT && r.consumed == 3);
    limits = ll1_default_limits(); limits.steps = 2;
    r = ll1_analyze(program, strlen(program), &limits, NULL, NULL);
    REQUIRE(r.status == LL1_LIMIT && r.limit == LL1_STEP_LIMIT && r.steps == 2);
    limits = ll1_default_limits(); limits.stack = 2;
    r = ll1_analyze(program, strlen(program), &limits, NULL, NULL);
    REQUIRE(r.status == LL1_LIMIT && r.limit == LL1_STACK_LIMIT);
    limits = ll1_default_limits(); limits.stack = 1;
    REQUIRE(ll1_analyze(program, strlen(program), &limits, NULL, NULL).status == LL1_INTERNAL_ERROR);
    REQUIRE(ll1_analyze(NULL, 1, NULL, NULL, NULL).status == LL1_INTERNAL_ERROR);
    REQUIRE(ll1_analyze(NULL, 0, NULL, NULL, NULL).status == LL1_SYNTAX_ERROR);
    int remaining = 0;
    r = ll1_analyze(program, strlen(program), NULL, cancel_after, &remaining);
    REQUIRE(r.status == LL1_CANCELLED && r.steps == 0);
    remaining = 8;
    r = ll1_analyze(program, strlen(program), NULL, cancel_after, &remaining);
    REQUIRE(r.status == LL1_CANCELLED && r.steps > 0);
    for (int i = 0; i < 100; i++) {
        REQUIRE(analyze(comment).status == LL1_LEXICAL_ERROR);
        REQUIRE(analyze(missing).status == LL1_SYNTAX_ERROR);
        REQUIRE(analyze(program).status == LL1_ACCEPTED);
    }
    REQUIRE(lexer_visit_text(NULL, 1, NULL, NULL, NULL) == -1);
    context_tests();
    puts("PASS LL(1) nativo con 300 consultas completas y 200 de contexto sin Bison");
}

static int query(const char *path, int context_mode, size_t cursor, size_t point) {
    FILE *file = fopen(path, "rb");
    if (!file) return 2;
    char *text = malloc(LL1_SOURCE_MAX + 1);
    if (!text) { fclose(file); return 2; }
    size_t length = fread(text, 1, LL1_SOURCE_MAX + 1, file);
    int failed = ferror(file);
    fclose(file);
    if (failed) { free(text); return 2; }
    LL1Context context = {0};
    LL1Result r;
    if (context_mode) {
        context = ll1_context(text, length, cursor, point, NULL, NULL, NULL);
        r = context.analysis;
    } else r = ll1_analyze(text, length, NULL, NULL, NULL);
    free(text);
    fprintf(stdout, "{\"status\":\"%s\",\"accepted\":%s,\"offset\":%zu,\"end\":%zu,"
                    "\"line\":%d,\"column\":%d,\"consumed\":%zu,\"steps\":%zu,\"expected\":[",
            ll1_status_name(r.status), r.status == LL1_ACCEPTED ? "true" : "false",
            r.offset, r.end, r.line, r.column, r.consumed, r.steps);
    int separator = 0;
    for (size_t i = 0; ll1_terminal_name(i); i++) {
        if (!(r.expected & (UINT64_C(1) << i))) continue;
        if (separator++) fputc(',', stdout);
        json_write_string(stdout, ll1_terminal_name(i));
    }
    fputc(']', stdout);
    if (context_mode) fprintf(stdout, ",\"can_end\":%s,\"cursor\":%zu,\"point\":%zu,\"comment\":%d,\"comment_start\":%zu",
                              context.can_end ? "true" : "false", context.cursor, context.point,
                              context.comment, context.comment_start);
    puts("}");
    return r.status == LL1_ACCEPTED || r.status == LL1_PREFIX_VALID || r.status == LL1_IN_COMMENT ? 0 : 1;
}

static int position(const char *text, size_t *result) {
    if (!*text) return 0;
    for (const char *p = text; *p; p++) if (*p < '0' || *p > '9') return 0;
    errno = 0;
    char *end;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno || *end || value > SIZE_MAX) return 0;
    *result = (size_t)value;
    return 1;
}

int main(int argc, char **argv) {
    /* Los servicios reales quedan bajo la misma prohibición de llamar a Bison. */
    if (argc == 4 && !strcmp(argv[1], "--suggest")) {
        size_t cursor;
        if (!position(argv[2], &cursor)) return 2;
        return suggestions_run(stdout, argv[3], cursor);
    }
    if (argc == 3 && !strcmp(argv[1], "--repair")) return repairs_run(stdout, argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--check-ll1")) {
        int code = syntax_check_ll1(argv[2]);
        code = diag_json_finish(stdout, argv[2], code);
        diag_free();
        return code;
    }
    if (argc == 3 && !strcmp(argv[1], "--syntax")) return syntax_json_run(stdout, argv[2]);
    if (argc == 5 && !strcmp(argv[1], "--context")) {
        size_t cursor, point;
        if (!position(argv[2], &cursor) || !position(argv[3], &point)) return 2;
        return query(argv[4], 1, cursor, point);
    }
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "--self-test")) { self_test(); return 0; }
    return query(argv[1], 0, 0, 0);
}
