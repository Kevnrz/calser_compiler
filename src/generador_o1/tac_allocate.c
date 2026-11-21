#define _POSIX_C_SOURCE 200809L
/* Linear scan de intervalos: Poletto y Sarkar 1999 (fuera de Aho).
 * LRU de libres: Aho §10.2.3. Con spill se elige Chaitin (§4.6.5), no se
 * modifica el recorrido linear scan ni se intenta recuperar su error.
 */
#include "tac_allocate_internal.h"
#include <string.h>
#include <time.h>

/* Entrada: ninguna. Salida: ms monotónicos, cero si falla el reloj.
 * Efectos: consulta el tiempo solo con spill; no añade dependencia del escritor
 * JSON al asignador independiente, diseño §7.6 y anexo A.3.
 */
static double spill_clock(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;
    return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1000000.0;
}

/* Estado privado del linear scan, sin derrame (decisión §4.6.5). */
typedef struct {
    TacFunctionAllocation *function;
    const TacInterval *active[12];
    size_t active_count;
    size_t free_since[16];
    uint16_t busy;
} RegisterState;

/* Entrada: intervalo que necesita registro y nombre del fuente.
 * Salida: cero con diagnóstico. Efectos: escribe error.
 * Restricciones: solo linear scan; Chaitin es otra estrategia, §4.6.5.
 */
static unsigned no_register_available(RegisterState *state, const TacInterval *range,
    const char *source, const TacAllocationOptions *options, char *error, size_t capacity) {
    (void)options; /* La selección de estrategia ocurre antes de este recorrido. */
    (void)state; /* El linear scan conserva su error bajo presión. */
    const TacInstruction *i = range->origin;
    tac_error(error, capacity,
        "%s:%d:%d: error de generacion: sin registros temporales disponibles",
        source, i ? i->source_line : 0, i ? i->source_column : 0);
    return 0;
}

/* Entrada: intervalos activos, orden fin/ID. Salida: ninguna.
 * Efectos: inserta en vector de capacidad doce. Restricciones: hay un registro libre.
 */
static void activate(const TacInterval **active, size_t *count, const TacInterval *range) {
    size_t at = *count;
    while (at && (active[at - 1]->end > range->end ||
           (active[at - 1]->end == range->end && active[at - 1]->temp_id > range->temp_id))) {
        active[at] = active[at - 1];
        at--;
    }
    active[at] = range;
    (*count)++;
}

/* Entrada: función, intervalos ordenados y nombre. Salida: 1 o diagnóstico.
 * Efectos: completa mapa/métrica; no toca IR. Restricciones: r4-r15, sin derrame.
 */
static int allocate_function(TacFunctionAllocation *f, const TacIntervalAnalysis *ranges,
                              const char *source, const TacAllocationOptions *options, char *error, size_t capacity) {
    RegisterState state = {.function = f};
    for (size_t k = 0; k < tac_intervals_count(ranges); k++) {
        const TacInterval *range = tac_intervals_at(ranges, k);
        while (state.active_count && state.active[0]->end < range->start) {
            unsigned reg = f->registers[state.active[0]->temp_id];
            state.free_since[reg] = state.active[0]->end + 1;
            state.busy &= (uint16_t)~(1u << reg);
            state.active_count--;
            memmove(state.active, state.active + 1, state.active_count * sizeof *state.active);
        }
        unsigned chosen = 0;
        for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++) {
            if (!(state.busy & (1u << r)) &&
                (!chosen || state.free_since[r] < state.free_since[chosen]))
                chosen = r;
        }
        if (!chosen)
            chosen = no_register_available(&state, range, source, options, error, capacity);
        if (!chosen)
            return 0;
        f->registers[range->temp_id] = chosen;
        f->metrics.assigned_registers |= (uint16_t)(1u << chosen);
        state.busy |= (uint16_t)(1u << chosen);
        activate(state.active, &state.active_count, range);
        if (state.active_count > f->metrics.pressure)
            f->metrics.pressure = state.active_count;
    }
    for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++)
        f->metrics.register_count += !!(f->metrics.assigned_registers & (1u << r));
    return 1;
}

/* Entrada: operando válido y mapa completo. Salida: ninguna.
 * Efectos: sustituye TEMP por su PHYS. Restricciones: incluye destinos ligados.
 */
static void physical_operand(TacOperand *operand, const unsigned *registers) {
    if (operand->kind == TAC_TEMP)
        *operand = tac_physical(registers[operand->value.id]);
}

/* Entrada: programa virtual válido. Salida: planes propios o NULL.
 * Efectos: analiza todas las funciones antes de reescribir; finalmente valida físico.
 * Restricciones: contrato público; liveness debe ver todavía el programa virtual.
 */
TacAllocationResult *tac_allocate_registers(TacProgram *program, const char *source,
                                 const TacAllocationOptions *options,
                                 char *error, size_t capacity) {
    if (!source) {
        tac_error(error, capacity, "asignación requiere source_name no NULL");
        return NULL;
    }
    if (!program || program->stage != TAC_STAGE_ISA_VIRTUAL ||
        !tac_validate(program, error, capacity)) {
        tac_error(error, capacity, "asignación requiere ISA virtual validada");
        return NULL;
    }
    TacAllocationResult *result = tac_analysis_array(1, sizeof *result);
    size_t instruction_count = 0;
    for (TacFunction *f = program->first; f; f = f->next) {
        result->function_count++;
        for (TacBlock *b = f->first; b; b = b->next)
            for (TacInstruction *i = b->first; i; i = i->next)
                instruction_count++;
    }
    result->functions = tac_analysis_array(result->function_count, sizeof *result->functions);
    result->sites = tac_analysis_array(instruction_count, sizeof *result->sites);
    size_t index = 0;
    for (TacFunction *f = program->first; f; f = f->next) {
        TacFunctionAllocation *assigned = &result->functions[index++];
        assigned->function = f;
        assigned->registers = tac_analysis_array(f->next_temp, sizeof *assigned->registers);
        TacLiveAnalysis *live = tac_live_analyze(f, NULL, 0, error, capacity);
        assigned->metrics.demand = tac_live_max_temporaries(live);
        TacIntervalAnalysis *ranges = live ?
            tac_intervals_analyze(f, live, error, capacity) : NULL;
        int spill = options && options->spill;
        double start = spill ? spill_clock() : 0;
        int ok = ranges && (spill ? tac_allocate_color(assigned, ranges, source, error, capacity) :
            allocate_function(assigned, ranges, source, options, error, capacity));
        if (ok)
            ok = tac_allocate_sites(result, assigned, live, error, capacity);
        if (spill)
            assigned->metrics.spill_ms = spill_clock() - start;
        tac_intervals_free(ranges);
        tac_live_free(live);
        if (!ok) {
            tac_allocation_free(result);
            return NULL;
        }
    }
    for (size_t k = 0; k < result->function_count; k++) {
        TacFunctionAllocation *f = &result->functions[k];
        for (TacBlock *b = f->function->first; b; b = b->next) {
            for (TacInstruction *i = b->first; i; i = i->next) {
                const TacEmissionSite *site = tac_allocation_site(result, i);
                TacOperand *operands[] = {&i->dest, &i->src1, &i->src2};
                for (size_t j = 0; j < 3; j++) {
                    if (site && site->spill_registers[j])
                        *operands[j] = tac_physical(site->spill_registers[j]);
                    else {
                        if (operands[j]->kind == TAC_TEMP &&
                            !f->registers[operands[j]->value.id]) {
                            tac_error(error, capacity, "temporal sin registro ni plan de derrame");
                            tac_allocation_free(result);
                            return NULL;
                        }
                        physical_operand(operands[j], f->registers);
                    }
                }
                physical_operand(&i->isa.target, f->registers);
                if (site && site->scratch_mask)
                    i->pseudo.scratch_clobbers = site->scratch_mask;
            }
        }
    }
    program->stage = TAC_STAGE_ISA_PHYSICAL;
    if (!tac_validate(program, error, capacity)) {
        tac_allocation_free(result);
        return NULL;
    }
    return result;
}

/* Entrada: resultado e identidad prestada. Salida: vista propia del resultado o NULL.
 * Efectos: ninguno. Restricciones: mantener IR vivo hasta liberar resultado.
 */
const TacEmissionSite *tac_allocation_site(const TacAllocationResult *a,
                                          const TacInstruction *instruction) {
    if (a)
        for (size_t k = 0; k < a->site_count; k++)
            if (a->sites[k].instruction == instruction)
                return &a->sites[k];
    return NULL;
}
const TacAllocationMetrics *tac_allocation_metrics(const TacAllocationResult *a,
                                                   const TacFunction *function) {
    if (a)
        for (size_t k = 0; k < a->function_count; k++)
            if (a->functions[k].function == function)
                return &a->functions[k].metrics;
    return NULL;
}

/* Entrada: resultado propio o NULL. Salida: ninguna. Efectos: libera mapas y planes.
 * Restricciones: no libera programa, símbolos ni datos estáticos.
 */
void tac_allocation_free(TacAllocationResult *a) {
    if (!a)
        return;
    for (size_t k = 0; k < a->function_count; k++) {
        free(a->functions[k].registers);
        free(a->functions[k].spill_slot);
    }
    for (size_t k = 0; k < a->site_count; k++)
        free((void *)a->sites[k].preserved);
    free(a->functions);
    free(a->sites);
    free(a);
}
