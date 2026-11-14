/* Gestor de pasadas vacías E; validación y tiempos compartidos, Aho §9.1. */
#include "tac_passes.h"
#include "tac_loops.h"
#include <string.h>

static const TacPass passes[TAC_PASS_COUNT] = {
    {.name = "unroll", .point = TAC_PASS_HIGH, .run = tac_pass_unroll},
    {.name = "rename", .point = TAC_PASS_HIGH, .run = tac_pass_rename},
    {.name = "dce", .point = TAC_PASS_HIGH, .run = tac_pass_dce},
    {.name = "reorder", .point = TAC_PASS_VIRTUAL, .run = tac_pass_reorder},
    {.name = "spill", .point = TAC_PASS_ALLOCATION, .run = NULL}
};

/* Entrada: índice. Salida: descriptor prestado o NULL. Efectos: ninguno.
 * Restricciones: tabla fija, no se registra un recorrido adicional para spill.
 */
const TacPass *tac_pass_at(size_t index) {
    return index < TAC_PASS_COUNT ? &passes[index] : NULL;
}
/* Entrada: nombre y longitud. Salida: índice exacto o -1. Efectos: ninguno.
 * Restricciones: no acepta listas ni nombres parciales.
 */
int tac_pass_find(const char *name, size_t length) {
    for (size_t k = 0; k < TAC_PASS_COUNT; k++)
        if (strlen(passes[k].name) == length && !strncmp(name, passes[k].name, length))
            return (int)k;
    return -1;
}

/* Entrada: contexto HIGH. Salida: ninguna. Efectos: retira marcas antes de ediciones.
 * Restricciones: unroll las consume primero si está activo; Aho §9.6.
 */
static void discard_marks(TacPassContext *context) {
    for (TacFunction *f = context->program->first; f; f = f->next)
        tac_loop_marks_discard(f);
}
/* Entrada: contexto y descriptor válidos. Salida: éxito o error interno.
 * Efectos: ejecuta, valida y mide. Restricciones: índice de métrica 0..4.
 */
int tac_pass_execute(TacPassContext *context, const TacPass *pass, size_t index,
                       char *error, size_t capacity) {
    char detail[256] = "sin detalle";
    double start = tac_metrics_clock();
    int ok = pass->run(context, detail, sizeof detail);
    if (ok)
        ok = tac_validate(context->program, detail, sizeof detail);
    context->metrics->pass_ms[index] = tac_metrics_clock() - start;
    if (context->report)
        context->report(context->report_user, pass->name, ok);
    if (!ok) {
        if (capacity)
            snprintf(error, capacity, "%s: error interno en pasada %s: %s",
                     context->source_name, pass->name, detail);
        return 0;
    }
    context->metrics->executed |= 1u << index;
    if (pass->point == TAC_PASS_HIGH)
        context->metrics->high_after[index] = tac_metrics_instruction_count(context->program);
    return 1;
}
/* Entrada: contexto y punto del recorrido. Salida: éxito o primer error.
 * Efectos: ejecuta en orden fijo y descarta marcas. Restricciones: no transforma en E.
 */
int tac_passes_run(TacPassContext *context, TacPassPoint point, char *error, size_t capacity) {
    if (point == TAC_PASS_HIGH && !(context->enabled & 1u))
        discard_marks(context);
    for (size_t k = 0; k < TAC_PASS_COUNT; k++) {
        if (!passes[k].run || passes[k].point != point || !(context->enabled & (1u << k)))
            continue;
        if (!tac_pass_execute(context, &passes[k], k, error, capacity))
            return 0;
        if (k == 0)
            discard_marks(context);
    }
    return 1;
}
