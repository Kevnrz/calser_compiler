/* Linear scan de intervalos: Poletto y Sarkar 1999 (fuera de Aho).
 * LRU de libres: Aho §10.2.3. No derrame, partición, conservación emitida ni CLI.
 */
#include "tac_allocate_internal.h"
#include <string.h>

/* Estado privado disponible en el punto de presión que extenderá C3.4. */
typedef struct {
    TacFunctionAllocation *function;
    const TacInterval *active[12];
    size_t active_count;
    size_t free_since[16];
    uint16_t busy;
} RegisterState;

/* Entrada: instrucción que necesita registro y fuente prestados.
 * Salida: registro recuperado, o cero con diagnóstico (siempre en C2).
 * Efectos: escribe error; recibe estado mutable del recorrido para C3.4.
 * Restricciones: punto interno del recorrido, antes de avanzar; C3.4 reemplazará
 * este cuerpo por derrame integrado, no por una pasada después del error.
 */
static unsigned no_register_available(RegisterState *state, const TacInterval *range,
    const char *source, char *error, size_t capacity) {
    (void)state; /* C3.4 retirará una víctima y devolverá su registro aquí. */
    const TacInstruction *i = range->origin;
    tac_error(error, capacity,
        "%s:%d:%d: error de generacion: sin registros temporales disponibles",
        source ? source : "ccom", i ? i->source_line : 0, i ? i->source_column : 0);
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
                              const char *source, char *error, size_t capacity) {
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
            chosen = no_register_available(&state, range, source, error, capacity);
        if (!chosen)
            return 0;
        f->registers[range->temp_id] = chosen;
        f->metrics.assigned_registers |= (uint16_t)(1u << chosen);
        state.busy |= (uint16_t)(1u << chosen);
        activate(state.active, &state.active_count, range);
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
                                 char *error, size_t capacity) {
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
        TacIntervalAnalysis *ranges = live ?
            tac_intervals_analyze(f, live, error, capacity) : NULL;
        int ok = ranges && allocate_function(assigned, ranges, source, error, capacity);
        if (ok)
            tac_allocate_sites(result, assigned, live);
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
                physical_operand(&i->dest, f->registers);
                physical_operand(&i->src1, f->registers);
                physical_operand(&i->src2, f->registers);
                physical_operand(&i->isa.target, f->registers);
                const TacEmissionSite *site = tac_allocation_site(result, i);
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
    for (size_t k = 0; k < a->function_count; k++)
        free(a->functions[k].registers);
    for (size_t k = 0; k < a->site_count; k++)
        free((void *)a->sites[k].preserved);
    free(a->functions);
    free(a->sites);
    free(a);
}
