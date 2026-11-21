/* Coloreo optimista de Chaitin: presentación Spilling de Registros, Aho §8.8.4.
 * Usa intervalos conservadores y profundidad de lazos; decisión §4.6.5, §11.6.
 * No cambia el linear scan. Los temporales derramados reciben casilla, no rz.
 */
#include "tac_allocate_internal.h"
#include "tac_loops.h"
#include "data_area.h"

typedef struct {
    size_t count;
    unsigned char *adjacent;
    size_t *degree;
    double *cost;
} ColorGraph;
typedef struct { ColorGraph *graph; const size_t *index; double weight; } CostVisit;

/* Entrada: ubicación visitada. Salida: ninguna. Efectos: acumula costo por uso
 * y definición del valor en la instrucción; profundidad natural, §11.6.
 */
static void add_cost(void *user, TacOperand value, int definition) {
    CostVisit *visit = user;
    (void)definition;
    if (value.kind == TAC_TEMP && visit->index[value.value.id] != SIZE_MAX)
        visit->graph->cost[visit->index[value.value.id]] += visit->weight;
}

/* Entrada: función y sus intervalos. Salida: mapa/casillas o diagnóstico.
 * Efectos: simplifica, apila, colorea en orden inverso y reserva derrame.
 * Restricciones: k=12; costo/grado actual; empate por longitud y mayor ID.
 */
int tac_allocate_color(TacFunctionAllocation *f, const TacIntervalAnalysis *ranges,
                        const char *source, char *error, size_t capacity) {
    size_t n = tac_intervals_count(ranges), ids = f->function->next_temp;
    ColorGraph graph = {.count = n};
    if (n && n > SIZE_MAX / n)
        return tac_error(error, capacity, "%s: grafo de interferencia demasiado grande", source);
    graph.adjacent = tac_analysis_array(n * n, sizeof *graph.adjacent);
    graph.degree = tac_analysis_array(n, sizeof *graph.degree);
    graph.cost = tac_analysis_array(n, sizeof *graph.cost);
    size_t *index = tac_analysis_array(ids, sizeof *index);
    size_t *stack = tac_analysis_array(n, sizeof *stack);
    unsigned char *removed = tac_analysis_array(n, sizeof *removed);
    f->spill_slot = tac_analysis_array(ids, sizeof *f->spill_slot);
    for (size_t k = 0; k < ids; k++) {
        index[k] = SIZE_MAX;
        f->spill_slot[k] = -1;
    }
    for (size_t k = 0; k < n; k++) {
        const TacInterval *a = tac_intervals_at(ranges, k);
        index[a->temp_id] = k;
        for (size_t j = 0; j < k; j++) {
            const TacInterval *b = tac_intervals_at(ranges, j);
            if (a->start <= b->end && b->start <= a->end) {
                graph.adjacent[k * n + j] = graph.adjacent[j * n + k] = 1;
                graph.degree[k]++;
                graph.degree[j]++;
            }
        }
    }
    TacLoopAnalysis *loops = tac_loops_analyze(f->function, error, capacity);
    int ok = loops != NULL;
    if (ok) {
        CostVisit visit = {.graph = &graph, .index = index};
        for (const TacBlock *b = f->function->first; b; b = b->next) {
            visit.weight = 1;
            for (size_t d = 0; d < tac_loops_depth(loops, b); d++)
                visit.weight *= 10;
            for (const TacInstruction *i = b->first; i; i = i->next)
                tac_locations_visit(f->function, i, add_cost, &visit);
        }
        tac_loops_free(loops);
        for (size_t top = 0; top < n; top++) {
            size_t chosen = SIZE_MAX;
            for (size_t k = 0; k < n; k++)
                if (!removed[k] && graph.degree[k] < 12 &&
                    (chosen == SIZE_MAX || tac_intervals_at(ranges, k)->temp_id <
                                           tac_intervals_at(ranges, chosen)->temp_id))
                    chosen = k;
            if (chosen == SIZE_MAX) {
                for (size_t k = 0; k < n; k++) {
                    if (removed[k])
                        continue;
                    const TacInterval *a = tac_intervals_at(ranges, k);
                    const TacInterval *b = chosen == SIZE_MAX ? NULL : tac_intervals_at(ranges, chosen);
                    double ratio = graph.cost[k] / graph.degree[k];
                    double best = b ? graph.cost[chosen] / graph.degree[chosen] : 0;
                    if (!b || ratio < best || (ratio == best &&
                        (a->end - a->start > b->end - b->start ||
                         (a->end - a->start == b->end - b->start && a->temp_id > b->temp_id))))
                        chosen = k;
                }
            }
            stack[top] = chosen;
            removed[chosen] = 1;
            for (size_t k = 0; k < n; k++)
                if (!removed[k] && graph.adjacent[chosen * n + k])
                    graph.degree[k]--;
        }
        for (size_t top = n; top-- > 0;) {
            size_t k = stack[top], id = tac_intervals_at(ranges, k)->temp_id;
            uint16_t busy = 0;
            for (size_t j = 0; j < n; j++)
                if (graph.adjacent[k * n + j])
                    busy |= (uint16_t)(1u << f->registers[tac_intervals_at(ranges, j)->temp_id]);
            unsigned chosen = 12;
            size_t farthest = 0;
            const TacInterval *v = tac_intervals_at(ranges, k);
            /* §11.6: distancia mínima a todo intervalo que ya tiene el color.
             * SIZE_MAX representa infinito; empate por menor color (Aho §8.8).
             * Un color libre no tiene intervalos solapados con v.
             */
            for (unsigned color = 0; color < 12; color++) {
                unsigned reg = TAC_P1_TEMP_FIRST + color;
                if (busy & (1u << reg))
                    continue;
                size_t distance = SIZE_MAX;
                for (size_t j = 0; j < n; j++) {
                    const TacInterval *u = tac_intervals_at(ranges, j);
                    if (f->registers[u->temp_id] != reg)
                        continue;
                    size_t gap = u->end < v->start ? v->start - u->end : u->start - v->end;
                    if (gap < distance)
                        distance = gap;
                }
                if (chosen == 12 || distance > farthest) {
                    chosen = color;
                    farthest = distance;
                }
            }
            if (chosen == 12) {
                f->spill_slot[id] = (int)f->spill_count++;
            } else {
                f->registers[id] = TAC_P1_TEMP_FIRST + chosen;
                f->metrics.assigned_registers |= (uint16_t)(1u << f->registers[id]);
            }
        }
        for (size_t k = 0; k < n; k++) {
            size_t at = tac_intervals_at(ranges, k)->start, active = 0;
            for (size_t j = 0; j < n; j++) {
                const TacInterval *v = tac_intervals_at(ranges, j);
                active += f->registers[v->temp_id] && v->start <= at && at <= v->end;
            }
            if (active > f->metrics.pressure)
                f->metrics.pressure = active;
        }
        f->metrics.spilled = f->spill_count;
        for (unsigned r = TAC_P1_TEMP_FIRST; r <= TAC_P1_TEMP_LAST; r++)
            f->metrics.register_count += !!(f->metrics.assigned_registers & (1u << r));
        if (f->spill_count && (f->spill_count > DATA_LIMIT / 4 ||
            !data_area_reserve_spill((Symbol *)f->function->symbol, (int)(4 * f->spill_count))))
            ok = tac_error(error, capacity, "%s: error de generacion: sin espacio de datos para derrame", source);
        if (ok && data_area_stack_short())
            ok = tac_error(error, capacity, "%s: error de generacion: no queda espacio para la pila de llamadas: se necesitan %d bytes", source, STACK_MIN_BYTES);
    }
    free(graph.adjacent);
    free(graph.degree);
    free(graph.cost);
    free(index);
    free(stack);
    free(removed);
    return ok;
}
