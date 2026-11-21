/* Desenrollado HIGH: notas §5.2.2, Aho §9.6 y diseño §7.4/11.4.
 * Entrada: contexto validado con marcas. Salida: éxito o detalle sin imprimir.
 * Efectos: clona, ajusta el control y fusiona solo bloques tocados; API pública.
 */
#include "tac_passes.h"
#include "tac_loops.h"
#include <limits.h>
#include <stdlib.h>

typedef struct {
    TacLoopMark mark;
    const TacBlock **blocks;
    size_t block_count, size;
    TacBlock *previous, *exit;
    int line;
} LoopPlan;
typedef struct { TacBlock **blocks; size_t count; } Touched;
enum { UNROLL_FULL_LIMIT = 8, UNROLL_DEFAULT_FACTOR = 4 };
#define UNROLL_BUDGET (ISA_ROM_BUNDLES / 4)

/* Entrada: buffer y texto. Salida: cero. Efectos: informa fallo del contrato. */
static int fail(char *detail, size_t capacity, const char *text) {
    if (capacity)
        snprintf(detail, capacity, "%s", text);
    return 0;
}

/* Entrada: planes propios. Salida: ninguna. Efectos: libera sus vectores. */
static void free_plans(LoopPlan *plans, size_t count) {
    for (size_t k = 0; k < count; k++)
        free(plans[k].blocks);
    free(plans);
}

/* Entrada: función HIGH. Salida: planes independientes del análisis y marcas.
 * Efectos: consulta lazos naturales; no edita, §4.6.2 y §11.4.
 */
static int collect_plans(TacFunction *f, LoopPlan **out, size_t *count,
                          char *detail, size_t capacity) {
    *out = NULL;
    *count = 0;
    TacLoopAnalysis *loops = tac_loops_analyze(f, detail, capacity);
    if (!loops)
        return 0;
    size_t marks = 0, blocks = 0;
    for (const TacLoopMark *m = f->loop_marks; m; m = m->next)
        marks++;
    for (const TacBlock *b = f->first; b; b = b->next)
        blocks++;
    LoopPlan *plans = marks ? calloc(marks, sizeof *plans) : NULL;
    if (marks && !plans) {
        tac_loops_free(loops);
        return fail(detail, capacity, "sin memoria para planes de unrolling");
    }
    for (const TacLoopMark *m = f->loop_marks; m; m = m->next) {
        if (m->kind != TAC_LOOP_S ||
            tac_loop_mark_state(loops, m) != TAC_LOOP_MARK_WITH_BACKEDGE)
            continue;
        size_t loop = 0;
        while (loop < tac_loops_count(loops) && tac_loops_header(loops, loop) != m->header)
            loop++;
        if (loop == tac_loops_count(loops)) {
            free_plans(plans, *count);
            tac_loops_free(loops);
            return fail(detail, capacity, "marca sin lazo natural");
        }
        int eligible = 1;
        for (const TacBlock *b = f->first; b && eligible; b = b->next) {
            if (b == m->header || !tac_loops_contains(loops, loop, b))
                continue;
            for (const TacLoopMark *other = f->loop_marks; other; other = other->next)
                if (other != m && other->header == b)
                    eligible = 0;
            for (size_t k = 0; k < b->successor_count; k++)
                if (!tac_loops_contains(loops, loop, b->successors[k]))
                    eligible = 0;
        }
        TacBlock *previous = NULL;
        size_t exterior = 0;
        for (size_t k = 0; k < m->header->predecessor_count; k++) {
            TacBlock *b = m->header->predecessors[k];
            if (!tac_loops_contains(loops, loop, b)) {
                previous = b;
                exterior++;
            }
        }
        if (!eligible || exterior != 1)
            continue;
        LoopPlan *p = &plans[*count];
        p->blocks = malloc(blocks * sizeof *p->blocks);
        if (!p->blocks) {
            free_plans(plans, *count);
            tac_loops_free(loops);
            return fail(detail, capacity, "sin memoria para bloques del lazo");
        }
        p->mark = *m;
        p->mark.next = NULL;
        p->previous = previous;
        p->exit = (TacBlock *)m->header->last->dest.value.block;
        p->line = m->header->last->source_line;
        for (const TacBlock *b = f->first; b; b = b->next) {
            if (b != m->header && tac_loops_contains(loops, loop, b)) {
                p->blocks[p->block_count++] = b;
                for (const TacInstruction *i = b->first; i; i = i->next)
                    p->size++;
            }
        }
        (*count)++;
    }
    tac_loops_free(loops);
    *out = plans;
    return 1;
}

/* Entrada: marca con operandos de 32 bits. Salida: vueltas o -1 desconocido.
 * Efectos: ninguno; techo sin redondeos flotantes, diseño §11.4/P15 contrato B.
 */
static int64_t trip_count(const LoopPlan *p) {
    if (p->mark.initial.kind != TAC_INTEGER || p->mark.bound.kind != TAC_INTEGER)
        return -1;
    int64_t a = p->mark.initial.value.integer, b = p->mark.bound.value.integer;
    int64_t distance = p->mark.step > 0 ? b - a : a - b;
    int64_t step = p->mark.step > 0 ? p->mark.step : -p->mark.step;
    return distance > 0 ? 1 + (distance - 1) / step : 0;
}

/* Entrada: conjunto y bloque vivo. Salida: éxito. Efectos: agrega identidad. */
static int touch(Touched *set, TacBlock *block, char *detail, size_t capacity) {
    for (size_t k = 0; k < set->count; k++)
        if (set->blocks[k] == block)
            return 1;
    TacBlock **more = realloc(set->blocks, (set->count + 1) * sizeof *more);
    if (!more)
        return fail(detail, capacity, "sin memoria para bloques tocados");
    set->blocks = more;
    set->blocks[set->count++] = block;
    return 1;
}

/* Entrada: copias completas. Salida: éxito. Efectos: i+j*paso en las lecturas,
 * un solo incremento por ronda; no cambia operandos de dirección, §4.6.7.
 */
static int adjust_control(TacFunction *f, const LoopPlan *p, TacBlockCopy *copies,
                           unsigned count, char *detail, size_t capacity) {
    TacOperand control = tac_variable(p->mark.control);
    for (unsigned j = 0; j < count; j++) {
        TacBlockCopy *round = copies + j * p->block_count;
        TacInstruction *added = NULL;
        TacOperand value = control;
        if (j) {
            value = tac_temp_new(f);
            added = tac_insert_after(round[0].copy, NULL, TAC_ADD, value, control,
                                     tac_integer((int64_t)j * p->mark.step));
            added->source_line = p->line;
        }
        TacInstruction *increment = NULL;
        TacBlock *increment_block = NULL;
        for (size_t k = 0; k < p->block_count; k++) {
            TacBlock *block = round[k].copy;
            for (TacInstruction *i = block->first; i; i = i->next) {
                if (i == added)
                    continue;
                if (j && !tac_replace_reads(block, i, control, value, NULL, detail, capacity))
                    return 0;
                if (i->dest.kind == TAC_VARIABLE && i->dest.value.symbol == p->mark.control) {
                    increment = i;
                    increment_block = block;
                }
            }
        }
        if (!increment)
            return fail(detail, capacity, "copia sin incremento de control");
        if (j + 1 < count && !tac_remove(increment_block, increment))
            return fail(detail, capacity, "no se pudo quitar incremento de copia");
    }
    return 1;
}

/* Entrada: conjunto original ya inalcanzable. Salida: éxito.
 * Efectos: retira cualquiera permitido y repite; retiene el resto, §4.6.1.
 */
static int remove_original(TacFunction *f, const LoopPlan *p, char *detail, size_t capacity) {
    size_t n = p->block_count + 1;
    const TacBlock **pending = malloc(n * sizeof *pending);
    if (!pending)
        return fail(detail, capacity, "sin memoria para retirar originales");
    pending[0] = p->mark.header;
    for (size_t k = 0; k < p->block_count; k++)
        pending[k + 1] = p->blocks[k];
    int changed;
    do {
        if (!tac_rebuild_edges(f, detail, capacity)) {
            free(pending);
            return 0;
        }
        changed = 0;
        for (size_t k = 0; k < n; k++) {
            char reason[256];
            if (pending[k] && tac_block_remove(f, (TacBlock *)pending[k], reason, sizeof reason)) {
                pending[k] = NULL;
                changed = 1;
                break;
            }
        }
    } while (changed);
    free(pending);
    return 1;
}

/* Entrada: plan y factor. Salida: éxito. Efectos: total o guarda+residuo,
 * redirige retrocesos y prepara ajuste; sin fusión anticipada, §11.4.
 */
static int unroll(TacFunction *f, const LoopPlan *p, unsigned count, int total,
                    Touched *touched, char *detail, size_t capacity) {
    TacBlockCopy *copies = count ? calloc(count * p->block_count, sizeof *copies) : NULL;
    if (count && !copies)
        return fail(detail, capacity, "sin memoria para copias de unrolling");
    TacBlock *guard = NULL, *previous = p->previous;
    if (!total) {
        guard = tac_block_insert_after(f, previous);
        TacOperand t = tac_temp_new(f);
        TacInstruction *i = tac_append(guard, TAC_ADD, t, tac_variable(p->mark.control),
                                       tac_integer((int64_t)(count - 1) * p->mark.step));
        i->source_line = p->line;
        i = tac_append(guard, p->mark.step > 0 ? TAC_IF_GE : TAC_IF_LE,
                       tac_label(p->mark.header), t, tac_variable(p->mark.limit));
        i->source_line = p->line;
        previous = guard;
    }
    int ok = 1;
    for (unsigned j = 0; j < count && ok; j++) {
        TacBlockCopy *round = copies + j * p->block_count;
        ok = tac_blocks_clone(f, p->blocks, p->block_count, previous, round, detail, capacity);
        if (ok)
            previous = round[p->block_count - 1].copy;
    }
    for (unsigned j = 0; j < count && ok; j++) {
        TacBlockCopy *round = copies + j * p->block_count;
        for (size_t k = 0; k < p->block_count; k++) {
            if (round[k].original == p->mark.latch)
                round[k].copy->last->dest = tac_label(j + 1 < count ?
                    copies[(j + 1) * p->block_count].copy : total ? p->exit : guard);
        }
    }
    if (ok)
        ok = adjust_control(f, p, copies, count, detail, capacity);
    if (ok) {
        p->previous->last->dest = tac_label(total ? (count ? copies[0].copy : p->exit) : guard);
        if (total)
            ok = remove_original(f, p, detail, capacity);
    }
    if (ok)
        ok = touch(touched, p->previous, detail, capacity);
    for (size_t k = 0; k < count * p->block_count && ok; k++)
        ok = touch(touched, copies[k].copy, detail, capacity);
    free(copies);
    return ok;
}

/* Entrada: bloque y nodo prestado. Salida: copia propia con metadatos.
 * Efectos: agrega conservando selectores, origen y operandos, Aho §8.4.1.
 */
static TacInstruction *append_copy(TacBlock *block, const TacInstruction *i) {
    TacInstruction *copy = tac_append(block, i->op, i->dest, i->src1, i->src2);
    *copy = *i;
    copy->next = NULL;
    return copy;
}

/* Entrada: función y conjunto tocado. Salida: éxito.
 * Efectos: fusiona solo X->siguiente con predecesor único; decisión §4.6.6.
 * La API valida antes de borrar: se copian los nodos antes de retirar Y, que
 * libera sus originales. Para una caída se estaciona un goto a Y.next y se
 * restaura el terminador tras borrar; nunca se pide borrar con CFG inválido.
 */
static int merge_blocks(TacFunction *f, Touched *touched, char *detail, size_t capacity) {
    for (;;) {
        if (!tac_rebuild_edges(f, detail, capacity))
            return 0;
        int changed = 0;
        for (TacBlock *x = f->first; x && !changed; x = x->next) {
            int selected = 0;
            for (size_t k = 0; k < touched->count; k++)
                selected |= touched->blocks[k] == x;
            TacBlock *y = x->next;
            if (!selected || !y || y == f->entry || !x->last || x->last->op != TAC_GOTO ||
                x->last->dest.value.block != y || y->predecessor_count != 1 || y->predecessors[0] != x)
                continue;
            tac_remove(x, x->last);
            for (const TacInstruction *i = y->first; i; i = i->next)
                append_copy(x, i);
            TacOpInfo info = {.flow = TAC_FLOW_NEXT};
            if (x->last)
                tac_instruction_info(x->last, &info);
            int fall = info.flow == TAC_FLOW_NEXT || info.flow == TAC_FLOW_IF;
            TacInstruction terminal = {0};
            if (fall) {
                if (info.flow == TAC_FLOW_IF) {
                    terminal = *x->last;
                    tac_remove(x, x->last);
                }
                tac_append(x, TAC_GOTO, tac_label(y->next), tac_none(), tac_none());
            }
            if (!tac_rebuild_edges(f, detail, capacity) || !tac_block_remove(f, y, detail, capacity))
                return 0;
            for (size_t k = 0; k < touched->count; k++)
                if (touched->blocks[k] == y)
                    touched->blocks[k] = NULL;
            if (fall) {
                tac_remove(x, x->last);
                if (info.flow == TAC_FLOW_IF)
                    append_copy(x, &terminal);
            }
            changed = 1;
        }
        if (!changed)
            return 1;
    }
}

int tac_pass_unroll(TacPassContext *context, char *detail, size_t capacity) {
    size_t budget = UNROLL_BUDGET;
    for (TacFunction *f = context->program->first; f; f = f->next) {
        LoopPlan *plans;
        size_t count;
        if (!collect_plans(f, &plans, &count, detail, capacity))
            return 0;
        tac_loop_marks_discard(f);
        Touched touched = {0};
        int ok = 1;
        for (size_t k = 0; k < count && ok; k++) {
            const LoopPlan *p = &plans[k];
            int64_t n = trip_count(p);
            int total = n >= 0 && n <= UNROLL_FULL_LIMIT;
            unsigned copies = total ? (unsigned)n : context->unroll_factor ?
                context->unroll_factor : UNROLL_DEFAULT_FACTOR;
            uint64_t step = p->mark.step < 0 ? (uint64_t)(-(p->mark.step + 1)) + 1 : (uint64_t)p->mark.step;
            size_t multiplier = total ? (copies ? copies - 1 : 0) : copies;
            if ((copies > 1 && step > INT32_MAX / (copies - 1)) ||
                (multiplier && p->size > budget / multiplier))
                continue;
            budget -= p->size * multiplier;
            ok = unroll(f, p, copies, total, &touched, detail, capacity);
            if (ok) {
                TacPassCounters *c = &context->metrics->counters;
                TacUnrolledLoop *more = realloc(c->unrolled, (c->unrolled_count + 1) * sizeof *more);
                if (!more)
                    ok = fail(detail, capacity, "sin memoria para contador de unrolling");
                else {
                    c->unrolled = more;
                    c->unrolled[c->unrolled_count++] = (TacUnrolledLoop){p->line, copies};
                }
            }
        }
        if (ok)
            ok = merge_blocks(f, &touched, detail, capacity);
        free(touched.blocks);
        free_plans(plans, count);
        if (!ok || !tac_rebuild_edges(f, detail, capacity))
            return 0;
    }
    return 1;
}
