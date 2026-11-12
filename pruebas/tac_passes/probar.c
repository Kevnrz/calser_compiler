/* E: contrato del gestor y métricas; no repite la emisión D. Aho §9.1. */
#include "tac_passes.h"
#include "tac_live.h"
#include "../tac_analysis/fixture.h"
#include "codegen.h"
#include <string.h>

/* Entrada: contexto y buffer. Salida: fallo deliberado. Efectos: detalle.
 * Restricciones: solo prueba el mismo ejecutor público usado por la tabla fija.
 */
static int fail_pass(TacPassContext *context, char *error, size_t capacity) {
    (void)context;
    snprintf(error, capacity, "fallo dirigido");
    return 0;
}
/* Entrada: orden esperado y nombre. Salida: ninguna. Efectos: comprueba avance.
 * Restricciones: las pasadas pertenecen a los puntos HIGH/virtual/asignación.
 */
static void report(void *user, const char *name, int ok) {
    size_t *next = user;
    assert(ok);
    assert(!strcmp(name, tac_pass_at((*next)++)->name));
}
int main(void) {
    Fixture f;
    fixture_init(&f, TAC_STAGE_HIGH);
    tac_append(f.entry, TAC_RETURN, tac_none(), tac_none(), tac_none());
    fixture_validate(&f);
    TacMetrics metrics = {0};
    size_t next = 0;
    TacPassContext context = {.program = f.program, .source_name = "ruta/prueba.cls",
        .enabled = 31, .metrics = &metrics, .report = report, .report_user = &next};
    char error[512];
    assert(tac_passes_run(&context, TAC_PASS_HIGH, error, sizeof error));
    assert(next == 3 && metrics.executed == 7);
    tac_program_free(f.program);
    fixture_init(&f, TAC_STAGE_ISA_VIRTUAL);
    fixture_exit(f.entry);
    context.program = f.program;
    assert(tac_passes_run(&context, TAC_PASS_VIRTUAL, error, sizeof error));
    assert(tac_passes_run(&context, TAC_PASS_ALLOCATION, error, sizeof error));
    assert(next == 5 && metrics.executed == 31);
    context.report = NULL;
    TacPass failure = {.name = "dce", .point = TAC_PASS_HIGH, .run = fail_pass};
    assert(!tac_pass_execute(&context, &failure, 2, error, sizeof error));
    assert(!strcmp(error, "ruta/prueba.cls: error interno en pasada dce: fallo dirigido"));
    /* También se comprueba que la validación posterior rechaza un IR alterado. */
    f.program->global_scope = NULL;
    assert(!tac_pass_execute(&context, tac_pass_at(3), 3, error, sizeof error));
    assert(strstr(error, "ruta/prueba.cls: error interno en pasada reorder:"));
    f.program->global_scope = &f.scope;
    tac_program_free(f.program);
    /* Gaps/etiquetas y slots NOP no son operaciones emitidas. */
    cg_init();
    cg_label("prueba");
    cg_op(SLOT_ALU0, "nopi");
    cg_gap();
    cg_bundle("mov t0, 1", "nopi", "nopi", "sw t0, 0(rz)");
    TacProgram empty = {0};
    tac_metrics_collect(&metrics, &empty, NULL);
    assert(metrics.emitted == 2);
    cg_free();
    puts("PASS gestor E: orden fijo, errores con pasada, validacion y operaciones no-NOP");
    return 0;
}
