/* C3.1: capturas HIGH y presión ISA de la pasada real, diseño §4.5/11.4.
 * Usa frontend, gestor, selección e intérprete existentes; Aho §8.4.1/§9.6.
 */
#include "tac_passes.h"
#include "tac_loops.h"
#include "tac_lower.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include "../../representacion/tac_build/interpretar.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern ASTNode *root;

/* Entrada: HIGH e imagen. Salida: JSON de vivos/intervalos por función.
 * Efectos: baja una copia y libera sus análisis antes de editar el original.
 */
static void pressure(const TacProgram *high) {
    char error[256];
    TacProgram *isa = tac_lower(high, error, sizeof error);
    assert(isa);
    putchar('[');
    size_t functions = 0;
    for (const TacFunction *f = isa->first; f; f = f->next) {
        TacLiveAnalysis *live = tac_live_analyze(f, NULL, 0, error, sizeof error);
        assert(live);
        TacIntervalAnalysis *ranges = tac_intervals_analyze(f, live, error, sizeof error);
        assert(ranges);
        size_t peak = 0;
        for (size_t k = 0; k < tac_intervals_count(ranges); k++) {
            size_t start = tac_intervals_at(ranges, k)->start, active = 0;
            for (size_t j = 0; j < tac_intervals_count(ranges); j++) {
                const TacInterval *v = tac_intervals_at(ranges, j);
                active += v->start <= start && start <= v->end;
            }
            if (active > peak)
                peak = active;
        }
        printf("%s{\"funcion\":\"%s\",\"vivos\":%zu,\"intervalos\":%zu}",
               functions++ ? "," : "", f->symbol->name, tac_live_max_temporaries(live), peak);
        tac_intervals_free(ranges);
        tac_live_free(live);
    }
    putchar(']');
    tac_program_free(isa);
}

/* Entrada: ejecución original y nueva. Salida: ninguna.
 * Efectos: compara las raíces de 7.1 y estado ante fallo, no pasos ni privados.
 */
static void equivalent(const TacRunResult *before, const TacRunResult *after) {
    assert(before->status == after->status);
    if (before->status == TAC_RUN_UNSUPPORTED_CRYPTO)
        return;
    assert(!strcmp(before->diagnostic, after->diagnostic));
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

/* Entrada: fuente, antes, después y factor opcional. Salida: JSON de consulta.
 * Efectos: captura por tac_build_print, valida y compara el intérprete real;
 * no cambia la CLI -i ni imprime un pseudocódigo proyectado, §4.5.
 */
int main(int argc, char **argv) {
    assert(argc == 4 || argc == 5);
    FILE *input = fopen(argv[1], "r");
    assert(input);
    diag_init(argv[1]);
    assert(parser_accept(input, &root));
    fclose(input);
    assert(semantic_analyze(root));
    char error[256];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    assert(build);
    FILE *out = fopen(argv[2], "w");
    assert(out);
    tac_build_print(out, build);
    assert(!fclose(out));
    TacRunResult before = tac_interpret(build, 2000000);
    printf("{\"HIGH_antes\":%zu,\"marcas\":[", tac_metrics_instruction_count(build->program));
    size_t marks = 0;
    for (const TacFunction *f = build->program->first; f; f = f->next) {
        for (const TacLoopMark *m = f->loop_marks; m; m = m->next) {
            int64_t trips = -1;
            if (m->kind == TAC_LOOP_S && m->initial.kind == TAC_INTEGER && m->bound.kind == TAC_INTEGER) {
                int64_t d = m->step > 0 ? m->bound.value.integer - m->initial.value.integer :
                                          m->initial.value.integer - m->bound.value.integer;
                int64_t step = m->step > 0 ? m->step : -m->step;
                trips = d > 0 ? 1 + (d - 1) / step : 0;
            }
            printf("%s{\"funcion\":\"%s\",\"linea\":%d,\"tipo\":\"%s\",\"N\":%lld}",
                   marks++ ? "," : "", f->symbol->name, m->header->last->source_line,
                   m->kind == TAC_LOOP_S ? "S" : "L", (long long)trips);
        }
    }
    printf("],\"presion_antes\":");
    pressure(build->program);
    TacMetrics metrics = {0};
    TacPassContext context = {.program = build->program, .build = build,
        .scopes = scopes_first(), .source_name = argv[1], .enabled = 1u,
        .unroll_factor = argc == 5 ? (unsigned)strtoul(argv[4], NULL, 10) : 0, .metrics = &metrics};
    if (!tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    out = fopen(argv[3], "w");
    assert(out);
    tac_build_print(out, build);
    assert(!fclose(out));
    TacRunResult after = tac_interpret(build, 2000000);
    equivalent(&before, &after);
    printf(",\"HIGH_despues\":%zu,\"desenrollados\":[", tac_metrics_instruction_count(build->program));
    for (size_t k = 0; k < metrics.counters.unrolled_count; k++) {
        TacUnrolledLoop v = metrics.counters.unrolled[k];
        printf("%s{\"linea\":%d,\"factor\":%u}", k ? "," : "", v.line, v.factor);
    }
    printf("],\"presion_despues\":");
    pressure(build->program);
    puts("}");
    tac_run_free(&before);
    tac_run_free(&after);
    tac_metrics_free(&metrics);
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    diag_free();
    return 0;
}
