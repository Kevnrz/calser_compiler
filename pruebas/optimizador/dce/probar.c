/* DCE C3.2: tabla §11.2 sobre HIGH real y constantes inseguras unitarias.
 * Usa frontend y gestor públicos; Aho §9.2.5. Sin generador ni simulador.
 */
#include "tac_passes.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include "../../representacion/tac_analysis/fixture.h"
#include <stdlib.h>
extern ASTNode *root;

/* Entrada: HIGH válido y contadores esperados. Salida: ninguna.
 * Efectos: ejecuta gestor, valida e impone idempotencia; diseño §11.2.
 */
static void check(TacProgram *program, const Scope *scopes, size_t removed, size_t blocks) {
    char error[256];
    TacMetrics metrics = {0};
    TacPassContext context = {.program = program, .scopes = scopes,
        .metrics = &metrics, .enabled = 1u << 2, .source_name = "dce"};
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    if (metrics.counters.removed != removed || metrics.counters.removed_blocks != blocks) {
        fprintf(stderr, "esperado %zu/%zu; medido %zu/%zu\n", removed, blocks,
                metrics.counters.removed, metrics.counters.removed_blocks);
        assert(0);
    }
    size_t count = tac_metrics_instruction_count(program);
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    assert(tac_metrics_instruction_count(program) == count);
    assert(metrics.counters.removed == removed && metrics.counters.removed_blocks == blocks);
    tac_metrics_free(&metrics);
}

/* Entrada: fuente abierto, esperados y rutas opcionales. Salida: ninguna.
 * Efectos: construye HIGH real y guarda antes/después si se piden (§4.5).
 * Restricciones: libera TAC antes de símbolos y AST; no normaliza listados.
 */
static void source(FILE *input, size_t removed, size_t blocks, char **paths) {
    diag_init("dce");
    assert(parser_accept(input, &root));
    assert(semantic_analyze(root));
    char error[256];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    assert(build);
    if (paths) {
        FILE *out = fopen(paths[0], "w");
        assert(out);
        tac_build_print(out, build);
        assert(!fclose(out));
    }
    check(build->program, scopes_first(), removed, blocks);
    if (paths) {
        FILE *out = fopen(paths[1], "w");
        assert(out);
        tac_build_print(out, build);
        assert(!fclose(out));
    }
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    root = NULL;
    diag_free();
}

int main(int argc, char **argv) {
    if (argc == 6) {
        FILE *in = fopen(argv[1], "r");
        assert(in);
        source(in, (size_t)strtoul(argv[4], NULL, 10),
               (size_t)strtoul(argv[5], NULL, 10), &argv[2]);
        assert(!fclose(in));
        return 0;
    }
    assert(argc == 1);
    /* Fuentes de la tabla: se cuentan también copias y prólogos implícitos. */
    const struct { const char *text; size_t removed, blocks; } cases[] = {
        {"F f [ int a ] : void ; Vm x : int = a * 2 ; E ; .", 2, 0},
        {"F f [ int a ] : int r ; Vm x : int = a * 2 ; r = a ; E ; .", 3, 0},
        {"Vm g : int = 0 ; F tocar [ int a ] : void ; g = a ; E ; . "
         "F f [ int a ] : void ; Vm x : int = a * 2 ; tocar [ a ] ; E ; .", 2, 0},
        {"F f [ int a ] : void ; Vm x : int = a / 4 ; E ; .", 2, 0},
        {"F f [ int a , int b ] : void ; Vm x : int = a / b ; E ; .", 1, 0},
        {"F f [ int a ] : void ; Vm x : int = a % 3 ; Vm y : int = a ^ 2 ; E ; .", 4, 0},
        {"F main [] : void ; Vm x : int = 7 ; E ; .", 0, 0},
        {"F main [] : void ; Vm a : int = 8 ; Vm b : int = 0 ; "
         "Vm x : int = 1 ; Vm y : int = a / b ; x = 2 ; E ; .", 0, 0},
        {"F f [ int a , int b ] : void ; C [ a == b ] ; E ; A ; E ; . E ; .", 0, 1},
        {"Vm datos : int [ 2 ] ; F f [ int a ] : void ; "
         "autenticar [ a ] ; datos [ 0 ] = a ; E ; .", 0, 0}
    };
    for (size_t k = 0; k < sizeof cases / sizeof *cases; k++) {
        FILE *in = tmpfile();
        assert(in);
        fputs(cases[k].text, in);
        if (!strstr(cases[k].text, "F main"))
            fputs(" F main [] : void ; E ; .", in);
        rewind(in);
        source(in, cases[k].removed, cases[k].blocks, NULL);
        assert(!fclose(in));
    }
    /* HIGH artificial: el semántico rechaza estas tres constantes (§4.5). */
    const TacOpcode ops[] = {TAC_DIV, TAC_MOD, TAC_POW};
    for (size_t k = 0; k < 3; k++) {
        Fixture f;
        fixture_init(&f, TAC_STAGE_HIGH);
        TacInstruction *unsafe = tac_append(f.entry, ops[k], f.d,
            tac_integer(8), tac_integer(k == 2 ? -1 : 0));
        fixture_return(f.entry);
        fixture_validate(&f);
        check(f.program, &f.scope, 0, 0);
        assert(f.entry->first == unsafe);
        tac_program_free(f.program);
    }
    /* Ciclo inalcanzable de etiquetas artificial: la decisión 4.6.1 lo deja. */
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    fixture_return(f.entry);
    TacBlock *a = tac_block_new(f.function), *b = tac_block_new(f.function);
    tac_append(a, TAC_GOTO, tac_label(b), tac_none(), tac_none());
    tac_append(b, TAC_GOTO, tac_label(a), tac_none(), tac_none());
    fixture_validate(&f);
    check(f.program, &f.scope, 0, 0);
    tac_program_free(f.program);
    /* Referencia en orden inverso: primero falla a, se quita b y luego a. */
    fixture_init(&f, TAC_STAGE_HIGH);
    fixture_return(f.entry);
    a = tac_block_new(f.function);
    b = tac_block_new(f.function);
    fixture_return(a);
    tac_append(b, TAC_GOTO, tac_label(a), tac_none(), tac_none());
    fixture_validate(&f);
    check(f.program, &f.scope, 0, 2);
    tac_program_free(f.program);
    puts("PASS DCE: 10 fuentes de §11.2, 3 constantes inseguras, ciclo retenido, borrado iterativo e idempotencia");
    return 0;
}
