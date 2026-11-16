/* C3.3b: fuente -> HIGH -> ISA virtual antes/después, diseño §4.5 y §11.5.
 * Mide vivos por función sin cambiar la pasada ni limitar su presión.
 * Referencias: Aho §9.2.5 y §10.3–10.4.
 */
#include "tac_passes.h"
#include "tac_lower.h"
#include "tac_live.h"
#include "tac_allocate.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern ASTNode *root;

/* Entrada: programa y flujo vivos. Salida: ninguna.
 * Efectos: JSON con máximo de TEMP vivos por función, antes de asignar.
 * Restricciones: libera cada análisis antes de editar; diseño §7.1.
 */
static void pressure(FILE *out, TacProgram *program) {
    char error[256];
    int comma = 0;
    fputc('[', out);
    for (TacFunction *f = program->first; f; f = f->next) {
        TacLiveAnalysis *live = tac_live_analyze(f, NULL, 0, error, sizeof error);
        assert(live);
        TacIntervalAnalysis *intervals = tac_intervals_analyze(f, live, error, sizeof error);
        assert(intervals);
        size_t overlap = 0;
        for (size_t k = 0; k < tac_intervals_count(intervals); k++) {
            size_t at = tac_intervals_at(intervals, k)->start, active = 0;
            for (size_t j = 0; j < tac_intervals_count(intervals); j++) {
                const TacInterval *v = tac_intervals_at(intervals, j);
                active += v->start <= at && at <= v->end;
            }
            if (active > overlap)
                overlap = active;
        }
        fprintf(out, "%s{\"funcion\":\"%s\",\"vivos\":%zu,\"solapamiento\":%zu}",
                comma ? "," : "", f->symbol->name, tac_live_max_temporaries(live), overlap);
        tac_intervals_free(intervals);
        comma = 1;
        tac_live_free(live);
    }
    fputc(']', out);
}

/* Entrada: fuente, prefijo de capturas y pasadas HIGH opcionales (rename/dce).
 * Salida: 0 y JSON de contadores/presión. Efectos: guarda .antes/.despues.isa.
 * Restricciones: captura antes de asignar; mide después planes de preservación
 * sin emitir código. Permite inspeccionar los fallos del backend. Conserva identidad, metadatos y bloque de cada nodo.
 */
int main(int argc, char **argv) {
    if (argc < 3 || argc > 4)
        return 2;
    FILE *input = fopen(argv[1], "r");
    assert(input);
    diag_init(argv[1]);
    assert(parser_accept(input, &root));
    assert(!fclose(input));
    assert(semantic_analyze(root));
    char error[256], path[4096];
    TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
    assert(build);
    unsigned high = 0;
    int do_reorder = 1;
    if (argc == 4) {
        if (!strcmp(argv[3], "sin_reorder"))
            do_reorder = 0;
        else if (!strcmp(argv[3], "rename"))
            high = 1u << 1;
        else if (!strcmp(argv[3], "rename,dce"))
            high = (1u << 1) | (1u << 2);
        else
            return 2;
    }
    TacMetrics metrics = {0};
    TacPassContext context = {.program = build->program, .scopes = scopes_first(),
        .metrics = &metrics, .enabled = high, .source_name = argv[1]};
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    TacProgram *isa = tac_lower(build->program, error, sizeof error);
    assert(isa);
    snprintf(path, sizeof path, "%s.antes.isa", argv[2]);
    FILE *out = fopen(path, "w");
    assert(out);
    tac_print(out, isa);
    assert(!fclose(out));
    size_t count = tac_metrics_instruction_count(isa);
    typedef struct { TacBlock *block; TacInstruction *node; TacInstruction saved; } Original;
    Original *original = calloc(count ? count : 1, sizeof *original);
    assert(original);
    size_t k = 0;
    for (TacFunction *f = isa->first; f; f = f->next)
        for (TacBlock *b = f->first; b; b = b->next)
            for (TacInstruction *i = b->first; i; i = i->next)
                original[k++] = (Original){b, i, *i};
    printf("{\"antes\":");
    pressure(stdout, isa);
    context.program = isa;
    context.enabled = do_reorder ? 1u << 3 : 0;
    assert(tac_passes_run(&context, TAC_PASS_VIRTUAL, error, sizeof error));
    printf(",\"despues\":");
    pressure(stdout, isa);
    printf(",\"reordenadas\":%zu", metrics.counters.reordered);
    assert(tac_metrics_instruction_count(isa) == count);
    for (k = 0; k < count; k++) {
        Original *o = &original[k];
        TacInstruction *i = o->block->first;
        while (i && i != o->node)
            i = i->next;
        assert(i);
        TacInstruction copy = *i;
        copy.next = o->saved.next;
        assert(!memcmp(&copy, &o->saved, sizeof copy));
    }
    free(original);
    snprintf(path, sizeof path, "%s.despues.isa", argv[2]);
    out = fopen(path, "w");
    assert(out);
    tac_print(out, isa);
    assert(!fclose(out));
    /* Las capturas/los vivos anteriores no dependen de que la asignación termine. */
    TacAllocationResult *allocation = tac_allocate_registers(isa, argv[1], NULL, error, sizeof error);
    printf(",\"asignacion\":{\"ok\":%s,\"sitios\":[", allocation ? "true" : "false");
    int comma = 0;
    if (allocation)
        for (TacFunction *f = isa->first; f; f = f->next)
            for (TacBlock *b = f->first; b; b = b->next) {
                size_t position = 0;
                for (TacInstruction *i = b->first; i; i = i->next, position++) {
                    const TacEmissionSite *site = tac_allocation_site(allocation, i);
                    if (site && site->preserved_count) {
                        TacOpInfo info;
                        tac_instruction_info(i, &info);
                        printf("%s{\"funcion\":\"%s\",\"bloque\":%zu,\"posicion\":%zu,"
                               "\"operacion\":\"%s\",\"preservados\":%zu}", comma ? "," : "",
                               f->symbol->name, b->label, position, info.text, site->preserved_count);
                        comma = 1;
                    }
                }
            }
    puts("]}}");
    tac_allocation_free(allocation);
    tac_metrics_free(&metrics);
    tac_program_free(isa);
    tac_build_free(build);
    semantic_free();
    ast_free(root);
    diag_free();
    return 0;
}
