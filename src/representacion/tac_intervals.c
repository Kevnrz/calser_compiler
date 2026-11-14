/* Intervalos conservadores derivados de C1; Aho §9.2.5.
 * Poletto y Sarkar 1999 (fuera de Aho): un intervalo por TEMP, sin partición.
 */
#include "tac_intervals.h"
#include "tac_loops.h"
#include "tac_analysis_internal.h"

typedef struct {
    const TacInstruction *instruction;
    size_t read;
} Position;
struct TacIntervalAnalysis {
    TacInterval *by_id;
    TacInterval **ordered;
    size_t count;
    size_t temp_count;
    Position *positions;
    size_t position_count;
    size_t current_depth;
};

/* Entrada: intervalos presentes. Salida: orden por inicio e ID.
 * Efectos: ninguno. Restricciones: comparación sin restas que desborden.
 */
static int compare_start(const void *a, const void *b) {
    const TacInterval *x = *(const TacInterval *const *)a;
    const TacInterval *y = *(const TacInterval *const *)b;
    if (x->start != y->start)
        return x->start < y->start ? -1 : 1;
    return (x->temp_id > y->temp_id) - (x->temp_id < y->temp_id);
}

/* Entrada: TEMP y posición. Salida: ninguna. Efectos: extiende su envolvente.
 * Restricciones: origin puede ser NULL en fronteras; IDs validados por C1.
 */
static void touch(TacIntervalAnalysis *a, TacOperand operand, size_t position,
                  const TacInstruction *origin) {
    if (operand.kind != TAC_TEMP)
        return;
    TacInterval *range = &a->by_id[operand.value.id];
    if (a->current_depth > range->loop_depth)
        range->loop_depth = a->current_depth;
    if (range->start == SIZE_MAX || position < range->start)
        range->start = position;
    if (position > range->end)
        range->end = position;
    if (!range->origin && origin)
        range->origin = origin;
}

/* Entrada: conjunto C1 y posición de frontera. Salida: ninguna.
 * Efectos: extiende TEMP vivos. Restricciones: PHYS nunca obtiene intervalo.
 */
static void touch_set(TacIntervalAnalysis *a, const TacLiveAnalysis *live,
                      const TacLiveSet *set, size_t position) {
    for (size_t k = 0; k < tac_live_location_count(live); k++) {
        TacOperand value = tac_live_location(live, k);
        if (tac_live_contains(live, set, value))
            touch(a, value, position, NULL);
    }
}

/* Entrada: instrucción y vista C1. Salida: ninguna.
 * Efectos: incorpora lecturas, definiciones y vivacidad a intervalos.
 * Restricciones: fuentes aritméticas se extienden hasta escritura; Aho §8.8.
 */
static void instruction_range(TacIntervalAnalysis *a, const TacLiveAnalysis *live,
                              const TacInstruction *i, size_t read) {
    const TacLiveInstruction *view = tac_live_instruction(live, i);
    TacOperand operands[3];
    TacOpInfo info;
    tac_instruction_info(i, &info);
    touch_set(a, live, view->before, read);
    touch_set(a, live, view->after, read + 1);
    size_t count = tac_uses(i, operands);
    for (size_t k = 0; k < count; k++) {
        touch(a, operands[k], read, i);
        if (info.scratch_count)
            touch(a, operands[k], read + 1, i);
    }
    count = tac_definitions(i, operands);
    for (size_t k = 0; k < count; k++)
        touch(a, operands[k], read + 1, i);
}

/* Entrada: función ISA virtual y análisis de ella. Salida: intervalos propios.
 * Efectos: reserva vistas y ordena; no modifica IR. Restricciones: contrato público.
 */
TacIntervalAnalysis *tac_intervals_analyze(const TacFunction *f,
    const TacLiveAnalysis *live, char *error, size_t capacity) {
    if (!f || !f->program || f->program->stage != TAC_STAGE_ISA_VIRTUAL || !live ||
        !tac_validate(f->program, error, capacity)) {
        tac_error(error, capacity, "intervalos requieren ISA virtual validada");
        return NULL;
    }
    size_t slots = 0;
    for (const TacBlock *b = f->first; b; b = b->next) {
        if (!tac_live_block(live, b)) {
            tac_error(error, capacity, "liveness no pertenece a la función");
            return NULL;
        }
        slots += !b->first;
        for (const TacInstruction *i = b->first; i; i = i->next)
            slots++;
    }
    if (slots > (SIZE_MAX - 1) / 2) {
        tac_error(error, capacity, "numeración de intervalos demasiado grande");
        return NULL;
    }
    TacLoopAnalysis *loops = tac_loops_analyze(f, error, capacity);
    if (!loops)
        return NULL;
    TacIntervalAnalysis *a = tac_analysis_array(1, sizeof *a);
    a->temp_count = f->next_temp;
    a->by_id = tac_analysis_array(a->temp_count, sizeof *a->by_id);
    a->ordered = tac_analysis_array(a->temp_count, sizeof *a->ordered);
    a->positions = tac_analysis_array(slots, sizeof *a->positions);
    for (size_t k = 0; k < a->temp_count; k++) {
        a->by_id[k].temp_id = k;
        a->by_id[k].start = SIZE_MAX;
    }
    size_t cursor = 0;
    for (const TacBlock *b = f->first; b; b = b->next) {
        a->current_depth = tac_loops_depth(loops, b);
        const TacLiveBlock *view = tac_live_block(live, b);
        touch_set(a, live, view->live_in, cursor);
        for (const TacInstruction *i = b->first; i; i = i->next) {
            a->positions[a->position_count++] = (Position){i, cursor};
            instruction_range(a, live, i, cursor);
            cursor += 2;
        }
        if (!b->first)
            cursor += 2;
        touch_set(a, live, view->live_out, cursor - 1);
    }
    for (size_t k = 0; k < a->temp_count; k++) {
        if (a->by_id[k].start != SIZE_MAX)
            a->ordered[a->count++] = &a->by_id[k];
    }
    tac_loops_free(loops);
    qsort(a->ordered, a->count, sizeof *a->ordered, compare_start);
    if (capacity)
        error[0] = '\0';
    return a;
}

/* Entrada: análisis vivo. Salida: cantidad/vistas prestadas o NULL fuera de rango.
 * Efectos: ninguno. Restricciones: las vistas caducan al liberar el análisis.
 */
size_t tac_intervals_count(const TacIntervalAnalysis *a) {
    return a ? a->count : 0;
}
const TacInterval *tac_intervals_at(const TacIntervalAnalysis *a, size_t index) {
    return a && index < a->count ? a->ordered[index] : NULL;
}
const TacInterval *tac_intervals_find(const TacIntervalAnalysis *a, size_t id) {
    return a && id < a->temp_count && a->by_id[id].start != SIZE_MAX ? &a->by_id[id] : NULL;
}

/* Entrada: instrucción del análisis, salidas escribibles. Salida: 1 encontrada.
 * Efectos: escribe posiciones solo al encontrar. Restricciones: punteros vivos.
 */
int tac_intervals_positions(const TacIntervalAnalysis *a, const TacInstruction *i,
                            size_t *read, size_t *write) {
    if (!a || !read || !write)
        return 0;
    for (size_t k = 0; k < a->position_count; k++) {
        if (a->positions[k].instruction == i) {
            *read = a->positions[k].read;
            *write = *read + 1;
            return 1;
        }
    }
    return 0;
}

/* Entrada: resultado propio o NULL. Salida: ninguna. Efectos: libera sus vectores.
 * Restricciones: no libera IR ni C1 prestados.
 */
void tac_intervals_free(TacIntervalAnalysis *a) {
    if (!a)
        return;
    free(a->by_id);
    free(a->ordered);
    free(a->positions);
    free(a);
}
