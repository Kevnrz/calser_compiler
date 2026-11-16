/* C3.3a: tabla §11.3 sobre HIGH real y capturas de §14.2.
 * Usa frontend, gestor e intérprete; Aho §8.5 y §10.2.3; decisión §4.6.3.
 */
#include "tac_passes.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include "../../representacion/tac_build/interpretar.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern ASTNode *root;

/* Entrada: resultados interpretados. Salida: ninguna.
 * Efectos: compara todo el estado observable incluso al fallar, diseño §11.1.
 * Restricciones: omite locales de otras funciones; CRYPTO se prueba en ASM.
 */
static void equivalent(const TacRunResult *before, const TacRunResult *after) {
    assert(before->status == after->status);
    assert(before->status != TAC_RUN_UNSUPPORTED_CRYPTO);
    assert(before->memory_size == after->memory_size);
    for (const Scope *s = scopes_first(); s; s = s->next) {
        if (s->kind != SCOPE_GLOBAL && (!s->owner || strcmp(s->owner->name, "main")))
            continue;
        for (const Symbol *v = s->symbols; v; v = v->next) {
            if (v->kind == SYMBOL_FUNCTION)
                continue;
            for (int offset = 0; offset < v->width; offset += 4) {
                uint32_t a, b;
                assert(tac_run_read_word(before, (uint32_t)(v->address + offset), &a));
                assert(tac_run_read_word(after, (uint32_t)(v->address + offset), &b));
                assert(a == b);
            }
        }
    }
}

/* Entrada: fuente abierto y contador esperado. Salida: ninguna.
 * Efectos: aplica pasada, valida, compara estado y captura HIGH si se solicita.
 * Restricciones: captura antes de la segunda ejecución; no altera métricas HIGH.
 */
static void source(FILE *input, size_t expected, char **paths, int interpret) {
    diag_init("rename");
    assert(parser_accept(input, &root));
    assert(semantic_analyze(root));
    char error[256];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    assert(build);
    TacRunResult before = {0};
    if (interpret)
        before = tac_interpret(build, 1000000);
    if (paths) {
        FILE *out = fopen(paths[0], "w");
        assert(out);
        tac_build_print(out, build);
        assert(!fclose(out));
    }
    size_t high = tac_metrics_instruction_count(build->program);
    TacMetrics metrics = {0};
    TacPassContext context = {.program = build->program, .scopes = scopes_first(),
        .metrics = &metrics, .enabled = 1u << 1, .source_name = "rename"};
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    if (metrics.counters.renamed != expected) {
        fprintf(stderr, "renombradas: esperado %zu; medido %zu\n", expected,
                metrics.counters.renamed);
        assert(0);
    }
    assert(tac_metrics_instruction_count(build->program) == high);
    if (interpret) {
        TacRunResult after = tac_interpret(build, 1000000);
        equivalent(&before, &after);
        tac_run_free(&after);
        tac_run_free(&before);
    }
    if (paths) {
        FILE *out = fopen(paths[1], "w");
        assert(out);
        tac_build_print(out, build);
        assert(!fclose(out));
    }
    size_t temporaries = 0;
    for (const TacFunction *f = build->program->first; f; f = f->next)
        temporaries += f->next_temp;
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    assert(metrics.counters.renamed == expected);
    for (const TacFunction *f = build->program->first; f; f = f->next)
        temporaries -= f->next_temp;
    assert(!temporaries);
    tac_metrics_free(&metrics);
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    root = NULL;
    diag_free();
}

int main(int argc, char **argv) {
    if (argc == 5) {
        FILE *in = fopen(argv[1], "r");
        assert(in);
        source(in, (size_t)strtoul(argv[4], NULL, 10), &argv[2], 0);
        assert(!fclose(in));
        return 0;
    }
    assert(argc == 1);
    const struct { const char *name, *text; size_t renamed; } cases[] = {
        {"WAW/WAR", "F f [ int a , int b ] : int r ; Vm x : int = a + b ; "
         "r = x * 2 ; x = a - b ; r = r + x ; E ; .", 1},
        {"Sin inicializador", "F f [ int a , int b ] : int r ; Vm x : int ; "
         "x = a + b ; r = x * 2 ; x = a - b ; r = r + x ; E ; .", 2},
        {"Tres definiciones", "F f [ int a , int b ] : int r ; Vm x : int = a ; "
         "r = x ; x = b ; r = r + x ; x = a + b ; r = r + x ; E ; .", 2},
        {"Una definicion", "F f [ int a , int b ] : int r ; Vm x : int = a ; r = x ; E ; .", 0},
        {"Excluidas", "Vm g : int = 0 ; F f [ int a , int b ] : int r ; "
         "a = 1 ; a = 2 ; g = 1 ; g = 2 ; r = 1 ; r = 2 ; E ; .", 0},
        {"Entre bloques", "F f [ int a , int b ] : int r ; Vm x : int = a ; "
         "C [ a == b ] ; x = b ; . r = x ; E ; .", 0},
        {"Fallo main", "F main [] : void ; Vm a : int = 8 ; Vm b : int = 0 ; "
         "Vm x : int = 1 ; Vm y : int = a / b ; x = 2 ; E ; .", 0},
        {"Fallo lectura explicita", "F main [] : void ; Vm b : int = 0 ; "
         "Vm x : int = 1 ; Vm y : int = x / b ; x = 2 ; E ; .", 0},
        /* Casos adicionales para límites de la guarda y sustituciones, §11.3. */
        {"RMW y dos lecturas", "F main [] : void ; Vm x : int = 1 ; "
         "x = x + x ; x = x + x ; E ; .", 2},
        {"CALL main", "F tocar [] : void ; E ; . F main [] : void ; "
         "Vm x : int = 1 ; tocar [] ; x = 2 ; E ; .", 0},
        {"MOD main", "F main [] : void ; Vm b : int = 0 ; Vm x : int = 1 ; "
         "Vm y : int = x % b ; x = 2 ; E ; .", 0},
        {"POW main", "F main [] : void ; Vm b : int = -1 ; Vm x : int = 1 ; "
         "Vm y : int = x ^ b ; x = 2 ; E ; .", 0},
        {"Guarda no bloquea fuera main", "F f [ int a , int b ] : int r ; "
         "Vm x : int = 1 ; Vm y : int = x / b ; x = 2 ; r = x + y ; E ; .", 1},
        {"Inicio redundante", "F main [] : void ; Vm b : int = 0 ; Vm x : int ; "
         "x = 1 ; Vm y : int = x / b ; x = 2 ; E ; .", 1}
    };
    for (size_t k = 0; k < sizeof cases / sizeof *cases; k++) {
        FILE *in = tmpfile();
        assert(in);
        fputs(cases[k].text, in);
        if (!strstr(cases[k].text, "F main"))
            fputs(" F main [] : void ; f [ 8 , 2 ] ; E ; .", in);
        rewind(in);
        printf("%s: %zu\n", cases[k].name, cases[k].renamed);
        source(in, cases[k].renamed, NULL, 1);
        assert(!fclose(in));
    }
    puts("PASS rename: 8 casos de §11.3, 6 adicionales; estado incluso ante fallo e idempotencia");
    return 0;
}
