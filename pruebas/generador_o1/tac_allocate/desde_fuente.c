/* Medición y asignación por función del corpus, sin emisor ni CLI de ccom.
 * Aho §9.2.5; intervalos y linear scan: Poletto/Sarkar 1999, fuera de Aho.
 */
#include "tac_allocate.h"
#include "tac_lower.h"
#include "tac_build.h"
#include "semantic.h"
#include "semantic_scope.h"
#include "diagnostics.h"
#include "parser_service.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern FILE *yyin;
extern ASTNode *root;

/* Entrada: función virtual, nombre y buffer de métricas. Salida: ninguna.
 * Efectos: calcula máximos y conteos sin añadirlos a la API pública.
 * Restricciones: incluye inalcanzables conservados; cuenta vivos antes/después.
 */
static void measure(TacFunction *f, size_t stats[6]) {
    char error[256];
    TacLiveAnalysis *live = tac_live_analyze(f, NULL, 0, error, sizeof error);
    TacIntervalAnalysis *ranges = tac_intervals_analyze(f, live, error, sizeof error);
    assert(ranges);
    stats[0] = tac_live_max_temporaries(live);
    for (TacBlock *b = f->first; b; b = b->next) {
        for (TacInstruction *i = b->first; i; i = i->next) {
            if (i->op == TAC_PSEUDO && i->pseudo.kind >= TAC_PSEUDO_MUL &&
                i->pseudo.kind <= TAC_PSEUDO_POW)
                stats[2 + i->pseudo.kind - TAC_PSEUDO_MUL]++;
        }
    }
    for (size_t k = 0; k < tac_intervals_count(ranges); k++) {
        size_t position = tac_intervals_at(ranges, k)->start;
        size_t overlap = 0;
        for (size_t n = 0; n < tac_intervals_count(ranges); n++) {
            const TacInterval *r = tac_intervals_at(ranges, n);
            overlap += r->start <= position && r->end >= position;
        }
        if (overlap > stats[1])
            stats[1] = overlap;
    }
    tac_intervals_free(ranges);
    tac_live_free(live);
}

/* Entrada: programa ISA válido y fuente. Salida: cantidad de funciones fallidas.
 * Efectos: asigna cada función, imprime métricas y continúa tras fallos.
 * Restricciones: vista de una función temporal; restaura enlaces/programa siempre.
 */
static size_t allocate_all(TacProgram *program, const char *source) {
    size_t failures = 0;
    for (TacFunction *f = program->first; f;) {
        TacFunction *next = f->next;
        TacProgram single = {.stage = TAC_STAGE_ISA_VIRTUAL,
            .global_scope = program->global_scope, .first = f, .last = f};
        f->program = &single;
        f->next = NULL;
        size_t stats[6] = {0};
        measure(f, stats);
        char error[512];
        TacAllocationResult *a = tac_allocate_registers(&single, source, NULL, error, sizeof error);
        size_t sites = 0;
        unsigned registers = 0;
        if (a) {
            assert(tac_validate(&single, error, sizeof error));
            registers = tac_allocation_metrics(a, f)->register_count;
            for (TacBlock *b = f->first; b; b = b->next) {
                for (TacInstruction *i = b->first; i; i = i->next) {
                    const TacEmissionSite *site = tac_allocation_site(a, i);
                    sites += site && site->preserved_count;
                    assert(i->dest.kind != TAC_TEMP && i->src1.kind != TAC_TEMP &&
                           i->src2.kind != TAC_TEMP);
                }
            }
        } else {
            assert(strstr(error, "sin registros temporales disponibles"));
            failures++;
        }
        printf("%s\t%s\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%u\t%s\n",
            source, f->symbol->name, stats[0], stats[1], stats[2], stats[3], stats[4],
            stats[5], sites, registers, a ? "OK" : error);
        tac_allocation_free(a);
        f->next = next;
        f->program = program;
        f = next;
    }
    if (!failures) {
        program->stage = TAC_STAGE_ISA_PHYSICAL;
        char error[256];
        assert(tac_validate(program, error, sizeof error));
    }
    return failures;
}

/* Entrada: fuente. Salida: 0 válido asignado, 4 presión, 3 rechazo frontend.
 * Efectos: analiza, baja y mide; no genera archivos ni reserva datos.
 * Restricciones: continúa entre funciones; libera planes antes de sus símbolos.
 */
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    yyin = fopen(argv[1], "r");
    assert(yyin);
    diag_init(argv[1]);
    int accepted = parser_accept(yyin, &root);
    assert(!fclose(yyin));
    int status = 3;
    if (accepted && semantic_analyze(root)) {
        char error[256];
        TacBuildResult *build = tac_build(root, scopes_global(), error, sizeof error);
        assert(build);
        TacProgram *isa = tac_lower(build->program, error, sizeof error);
        assert(isa);
        status = allocate_all(isa, argv[1]) ? 4 : 0;
        assert(tac_validate(build->program, error, sizeof error));
        tac_program_free(isa);
        tac_build_free(build);
    }
    semantic_free();
    ast_free(root);
    diag_free();
    return status;
}
