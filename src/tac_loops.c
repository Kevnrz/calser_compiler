/*
 * tac_loops.c — dominadores y lazos naturales desde entry, en HIGH o ISA.
 * Usa CFG validado; no confunde marcas sintácticas con lazos de R/copias.
 * Referencias: Aho §9.6.1, §9.6.4 y algoritmo 9.46 (§9.6.6).
 */
#include "tac_loops.h"
#include "tac_analysis_internal.h"
#include <stdlib.h>
#include <string.h>

struct TacLoopAnalysis {
    const TacFunction *function;
    const TacBlock **blocks;
    size_t count;
    unsigned char *reachable;
    unsigned char *dominators;  /* Fila n: dominadores de n. */
    unsigned char *members;     /* Fila h: unión de lazos naturales de cabecera h. */
    unsigned char *headers;
    size_t *depth;
    size_t loop_count;
};

/* Entrada: resultado y bloque. Salida: índice o SIZE_MAX. Efectos: ninguno.
 * Restricciones: identidad por puntero, no ID ni numeración de presentación.
 */
static size_t index_of(const TacLoopAnalysis *a, const TacBlock *block) {
    for (size_t i = 0; i < a->count; i++) {
        if (a->blocks[i] == block)
            return i;
    }
    return SIZE_MAX;
}

/* Entrada: índices preparados. Salida: ninguna. Efectos: marca alcanzables.
 * Restricciones: pila acotada por cantidad de bloques; cada uno entra una vez.
 */
static void reachability(TacLoopAnalysis *a) {
    size_t *stack = tac_analysis_array(a->count, sizeof *stack);
    size_t top = 0;
    a->reachable[0] = 1;
    stack[top++] = 0;
    while (top) {
        const TacBlock *block = a->blocks[stack[--top]];
        for (size_t s = 0; s < block->successor_count; s++) {
            size_t next = index_of(a, block->successors[s]);
            if (!a->reachable[next]) {
                a->reachable[next] = 1;
                stack[top++] = next;
            }
        }
    }
    free(stack);
}

/* Entrada: alcanzabilidad completa. Salida: ninguna. Efectos: punto fijo de dominadores.
 * Restricciones: entry solo se domina a sí misma; ignora predecesores inalcanzables.
 */
static void dominators(TacLoopAnalysis *a) {
    for (size_t n = 0; n < a->count; n++) {
        for (size_t d = 0; d < a->count; d++)
            a->dominators[n * a->count + d] = a->reachable[n] &&
                (n == 0 ? d == 0 : a->reachable[d]);
    }
    int changed;
    do {
        changed = 0;
        for (size_t n = 1; n < a->count; n++) {
            if (!a->reachable[n])
                continue;
            const TacBlock *block = a->blocks[n];
            for (size_t d = 0; d < a->count; d++) {
                unsigned char value = a->reachable[d];
                for (size_t p = 0; p < block->predecessor_count; p++) {
                    size_t predecessor = index_of(a, block->predecessors[p]);
                    if (a->reachable[predecessor])
                        value &= a->dominators[predecessor * a->count + d];
                }
                value |= n == d;
                size_t position = n * a->count + d;
                changed |= value != a->dominators[position];
                a->dominators[position] = value;
            }
        }
    } while (changed);
}

/* Entrada: dominadores. Salida: reducibilidad. Efectos: reserva recorrido temporal.
 * Restricciones: quitar aristas cuyo destino domina al origen debe dejar un DAG
 * alcanzable; si queda un ciclo, tiene entradas múltiples y no es reducible.
 */
static int reducible(const TacLoopAnalysis *a) {
    size_t *incoming = tac_analysis_array(a->count, sizeof *incoming);
    size_t *queue = tac_analysis_array(a->count, sizeof *queue);
    size_t reachable = 0;
    for (size_t n = 0; n < a->count; n++) {
        if (!a->reachable[n])
            continue;
        reachable++;
        const TacBlock *block = a->blocks[n];
        for (size_t s = 0; s < block->successor_count; s++) {
            size_t target = index_of(a, block->successors[s]);
            if (!a->dominators[n * a->count + target])
                incoming[target]++;
        }
    }
    size_t begin = 0;
    size_t end = 0;
    for (size_t n = 0; n < a->count; n++) {
        if (a->reachable[n] && !incoming[n])
            queue[end++] = n;
    }
    while (begin < end) {
        size_t n = queue[begin++];
        const TacBlock *block = a->blocks[n];
        for (size_t s = 0; s < block->successor_count; s++) {
            size_t target = index_of(a, block->successors[s]);
            if (!a->dominators[n * a->count + target] && --incoming[target] == 0)
                queue[end++] = target;
        }
    }
    free(incoming);
    free(queue);
    return begin == reachable;
}

/* Entrada: retroceso tail -> header válido. Salida: ninguna.
 * Efectos: une el lazo natural a la fila de su cabecera, algoritmo 9.46.
 * Restricciones: no recorre hacia atrás más allá de header ni desde inalcanzables.
 */
static void natural_loop(TacLoopAnalysis *a, size_t tail, size_t header) {
    unsigned char *members = a->members + header * a->count;
    unsigned char *visited = tac_analysis_array(a->count, sizeof *visited);
    size_t *stack = tac_analysis_array(a->count, sizeof *stack);
    size_t top = 0;
    visited[header] = 1;
    visited[tail] = 1;
    if (tail != header)
        stack[top++] = tail;
    while (top) {
        const TacBlock *block = a->blocks[stack[--top]];
        for (size_t p = 0; p < block->predecessor_count; p++) {
            size_t n = index_of(a, block->predecessors[p]);
            if (a->reachable[n] && !visited[n]) {
                visited[n] = 1;
                stack[top++] = n;
            }
        }
    }
    for (size_t n = 0; n < a->count; n++)
        members[n] |= visited[n];
    a->headers[header] = 1;
    free(visited);
    free(stack);
}

/* Entrada: grafo reducible y dominadores. Salida: ninguna.
 * Efectos: une lazos de igual cabecera y calcula profundidad por bloque.
 * Restricciones: profundidad natural independiente de la profundidad sintáctica.
 */
static void find_loops(TacLoopAnalysis *a) {
    for (size_t n = 0; n < a->count; n++) {
        if (!a->reachable[n])
            continue;
        const TacBlock *block = a->blocks[n];
        for (size_t s = 0; s < block->successor_count; s++) {
            size_t h = index_of(a, block->successors[s]);
            if (a->dominators[n * a->count + h])
                natural_loop(a, n, h);
        }
    }
    for (size_t h = 0; h < a->count; h++) {
        a->loop_count += a->headers[h];
        for (size_t n = 0; n < a->count; n++)
            a->depth[n] += a->members[h * a->count + n] != 0;
    }
}

/* Entrada: función validable. Salida: resultado propio o NULL con diagnóstico.
 * Efectos: reserva análisis sin modificar CFG ni marcas. Restricciones: Aho §9.6.
 */
TacLoopAnalysis *tac_loops_analyze(const TacFunction *function, char *error, size_t capacity) {
    if (capacity)
        error[0] = '\0';
    if (!function || !function->program || !tac_has_function(function->program, function) || !tac_validate(function->program, error, capacity)) {
        tac_error(error, capacity, "lazos requieren función/programa válidos");
        return NULL;
    }
    TacLoopAnalysis *a = tac_allocate(sizeof *a);
    a->function = function;
    for (const TacBlock *b = function->first; b; b = b->next)
        a->count++;
    a->blocks = tac_analysis_array(a->count, sizeof *a->blocks);
    a->reachable = tac_analysis_array(a->count, 1);
    a->dominators = tac_analysis_array(a->count, a->count);
    a->members = tac_analysis_array(a->count, a->count);
    a->headers = tac_analysis_array(a->count, 1);
    a->depth = tac_analysis_array(a->count, sizeof *a->depth);
    size_t n = 0;
    for (const TacBlock *b = function->first; b; b = b->next)
        a->blocks[n++] = b;
    reachability(a);
    dominators(a);
    if (!reducible(a)) {
        tac_error(error, capacity, "grafo irreducible en función %s", function->symbol->name);
        tac_loops_free(a);
        return NULL;
    }
    find_loops(a);
    return a;
}

/* Entrada: análisis/bloque. Salida: alcanzabilidad. Efectos: ninguno.
 * Restricciones: bloque ajeno o análisis NULL produce cero.
 */
int tac_loops_reachable(const TacLoopAnalysis *a, const TacBlock *block) {
    size_t n = a ? index_of(a, block) : SIZE_MAX;
    return n != SIZE_MAX && a->reachable[n];
}

/* Entrada: análisis y bloques. Salida: dominancia. Efectos: ninguno.
 * Restricciones: falsa para bloques ajenos o inalcanzables.
 */
int tac_loops_dominates(const TacLoopAnalysis *a, const TacBlock *dominator, const TacBlock *block) {
    size_t n = a ? index_of(a, block) : SIZE_MAX;
    size_t d = a ? index_of(a, dominator) : SIZE_MAX;
    return n != SIZE_MAX && d != SIZE_MAX && a->dominators[n * a->count + d];
}

/* Entrada: análisis/bloque. Salida: profundidad natural o cero. Efectos: ninguno.
 * Restricciones: no usa marcas ni cuenta dos veces una cabecera.
 */
size_t tac_loops_depth(const TacLoopAnalysis *a, const TacBlock *block) {
    size_t n = a ? index_of(a, block) : SIZE_MAX;
    return n == SIZE_MAX ? 0 : a->depth[n];
}

/* Entrada: análisis o NULL. Salida: cantidad de cabeceras. Efectos: ninguno.
 * Restricciones: las aristas de retroceso de una misma cabecera cuentan una vez.
 */
size_t tac_loops_count(const TacLoopAnalysis *a) {
    return a ? a->loop_count : 0;
}

/* Entrada: análisis/índice de lazo. Salida: cabecera o NULL. Efectos: ninguno.
 * Restricciones: orden determinista por posición en la función.
 */
const TacBlock *tac_loops_header(const TacLoopAnalysis *a, size_t loop) {
    if (a) {
        for (size_t n = 0; n < a->count; n++) {
            if (a->headers[n] && loop-- == 0)
                return a->blocks[n];
        }
    }
    return NULL;
}

/* Entrada: análisis/lazo/bloque. Salida: pertenencia. Efectos: ninguno.
 * Restricciones: falsa fuera de rango.
 */
int tac_loops_contains(const TacLoopAnalysis *a, size_t loop, const TacBlock *block) {
    const TacBlock *header = tac_loops_header(a, loop);
    size_t h = header ? index_of(a, header) : SIZE_MAX;
    size_t n = a ? index_of(a, block) : SIZE_MAX;
    return h != SIZE_MAX && n != SIZE_MAX && a->members[h * a->count + n];
}

/* Entrada: análisis y marca válida perteneciente a su función. Salida: estado.
 * Efectos: ninguno. Restricciones: validar las marcas antes de consultar.
 */
TacLoopMarkState tac_loop_mark_state(const TacLoopAnalysis *a, const TacLoopMark *mark) {
    if (!tac_loops_reachable(a, mark->header))
        return TAC_LOOP_MARK_UNREACHABLE;
    if (!mark->latch || !tac_loops_reachable(a, mark->latch))
        return TAC_LOOP_MARK_WITHOUT_BACKEDGE;
    return TAC_LOOP_MARK_WITH_BACKEDGE;
}

/* Entrada: operando capturado y función. Salida: si es literal o TEMP existente.
 * Efectos: ninguno. Restricciones: nunca acepta AST, LABEL ni lecturas diferidas de VARIABLE.
 */
static int captured_value(const TacFunction *function, TacOperand value) {
    return value.kind == TAC_INTEGER ||
           (value.kind == TAC_TEMP && value.value.id < function->next_temp);
}

/* Entrada: análisis de CFG y marcas prestadas. Salida: 1 válidas, 0 con diagnóstico.
 * Efectos: solo error. Restricciones: consistencia en un solo sentido; R/copias sin marca.
 */
int tac_loop_marks_validate(const TacLoopAnalysis *a, char *error, size_t capacity) {
    if (capacity)
        error[0] = '\0';
    if (!a)
        return tac_error(error, capacity, "análisis de lazos ausente");
    for (const TacLoopMark *m = a->function->loop_marks; m; m = m->next) {
        if (a->function->program->stage != TAC_STAGE_HIGH ||
            (m->kind != TAC_LOOP_S && m->kind != TAC_LOOP_L) || !m->structural_depth ||
            index_of(a, m->header) == SIZE_MAX || index_of(a, m->body) == SIZE_MAX ||
            (m->latch && index_of(a, m->latch) == SIZE_MAX))
            return tac_error(error, capacity, "marca de lazo con referencias inválidas");
        if (m->kind == TAC_LOOP_S && (!m->control || !m->limit || !m->step ||
            !captured_value(a->function, m->initial) || !captured_value(a->function, m->bound)))
            return tac_error(error, capacity, "marca S sin valores capturados");
        if (tac_loop_mark_state(a, m) != TAC_LOOP_MARK_WITH_BACKEDGE)
            continue;
        size_t h = index_of(a, m->header);
        size_t tail = index_of(a, m->latch);
        if (!tac_has_edge(m->latch->successors, m->latch->successor_count, m->header) ||
            !a->dominators[tail * a->count + h] || !a->headers[h] ||
            !a->members[h * a->count + index_of(a, m->body)])
            return tac_error(error, capacity, "marca sin lazo natural correspondiente en B%zu",
                             tac_block_presentation_number(m->header));
    }
    return 1;
}

/* Entrada: resultado propio o NULL. Salida: ninguna. Efectos: libera matrices e índices.
 * Restricciones: no libera función, marcas, bloques ni símbolos.
 */
void tac_loops_free(TacLoopAnalysis *a) {
    if (!a)
        return;
    free(a->blocks);
    free(a->reachable);
    free(a->dominators);
    free(a->members);
    free(a->headers);
    free(a->depth);
    free(a);
}
