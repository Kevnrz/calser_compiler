/* Pasada DCE: barrido HIGH hasta punto fijo y bloques inalcanzables.
 * Usa liveness y edición compartidas; Aho §9.2.5, diseño §7.2 y §11.2.
 * Entrada: contexto HIGH válido. Salida: éxito o detalle, sin imprimir.
 * Efectos: retira instrucciones/bloques y acumula contadores separados.
 */
#include "tac_passes.h"
#include "tac_live.h"
#include "tac_loops.h"
#include <stdlib.h>

/* Entrada: operación que puede fallar. Salida: prueba literal de seguridad.
 * Efectos: ninguno. Restricciones: no propaga constantes, §11.2.
 */
static int trap_is_impossible(const TacInstruction *instruction) {
    if (instruction->src2.kind != TAC_INTEGER)
        return 0;
    if (instruction->op == TAC_DIV || instruction->op == TAC_MOD)
        return instruction->src2.value.integer != 0;
    if (instruction->op == TAC_POW)
        return instruction->src2.value.integer >= 0;
    return 0;
}

/* Entrada: instrucción, propiedades y análisis vigente al iniciar el barrido.
 * Salida: si todas sus definiciones están muertas y no hay efecto protegido.
 * Efectos: ninguno. Restricciones: estado observable ante fallo, §11.1.
 */
static int is_removable(const TacInstruction *instruction, const TacOpInfo *info,
                        const TacLiveAnalysis *live) {
    if (info->nonremovable || info->memory_write || info->crypto_state || info->barrier ||
        info->flow != TAC_FLOW_NEXT)
        return 0;
    if (info->may_trap && !trap_is_impossible(instruction))
        return 0;
    TacOperand definitions[3];
    size_t count = tac_definitions(instruction, definitions);
    if (!count)
        return 0;
    const TacLiveInstruction *view = tac_live_instruction(live, instruction);
    for (size_t k = 0; k < count; k++)
        if (tac_live_contains(live, view->after, definitions[k]))
            return 0;
    return 1;
}

/* Entrada: función y raíces prestadas. Salida: instrucciones retiradas o fallo.
 * Efectos: libera nodos, conserva siguiente antes de editar; Aho §9.2.5.
 * Restricciones: la información vieja solo sobreestima vivos; recalcular después.
 */
static size_t sweep(TacFunction *function, const TacOperand *roots, size_t root_count,
                    char *detail, size_t capacity, int *failed) {
    TacLiveAnalysis *live = tac_live_analyze(function, roots, root_count, detail, capacity);
    if (!live) {
        *failed = 1;
        return 0;
    }
    size_t removed = 0;
    for (TacBlock *block = function->first; block; block = block->next) {
        TacInstruction *instruction = block->first;
        while (instruction) {
            TacInstruction *next = instruction->next;
            TacOpInfo info;
            tac_instruction_info(instruction, &info);
            if (is_removable(instruction, &info, live)) {
                tac_remove(block, instruction);
                removed++;
            }
            instruction = next;
        }
    }
    tac_live_free(live);
    return removed;
}

/* Entrada: función sin marcas (contrato del gestor). Salida: bloques retirados.
 * Efectos: reconstruye y retira como máximo un bloque por análisis, §4.6.1.
 * Restricciones: deja los que la API no permite borrar; no es error.
 */
static size_t remove_unreachable(TacFunction *function, char *detail, size_t capacity,
                                 int *failed) {
    size_t removed = 0;
    for (;;) {
        if (!tac_rebuild_edges(function, detail, capacity)) {
            *failed = 1;
            return removed;
        }
        TacLoopAnalysis *loops = tac_loops_analyze(function, detail, capacity);
        if (!loops) {
            *failed = 1;
            return removed;
        }
        size_t count = 0;
        for (TacBlock *b = function->first; b; b = b->next)
            if (!tac_loops_reachable(loops, b))
                count++;
        TacBlock **blocks = count ? malloc(count * sizeof *blocks) : NULL;
        if (count && !blocks) {
            tac_loops_free(loops);
            if (capacity)
                snprintf(detail, capacity, "sin memoria para bloques inalcanzables");
            *failed = 1;
            return removed;
        }
        size_t k = 0;
        for (TacBlock *b = function->first; b; b = b->next)
            if (!tac_loops_reachable(loops, b))
                blocks[k++] = b;
        tac_loops_free(loops);
        int changed = 0;
        for (k = 0; k < count && !changed; k++) {
            char reason[256];
            if (tac_block_remove(function, blocks[k], reason, sizeof reason)) {
                changed = 1;
                removed++;
            }
        }
        free(blocks);
        if (!changed)
            return removed;
    }
}

int tac_pass_dce(TacPassContext *context, char *detail, size_t capacity) {
    size_t root_count = 0;
    TacOperand *roots = tac_observable_roots(context->scopes, &root_count);
    for (TacFunction *f = context->program->first; f; f = f->next) {
        int failed = 0;
        size_t removed;
        do {
            removed = sweep(f, roots, root_count, detail, capacity, &failed);
            context->metrics->counters.removed += removed;
            if (failed) {
                free(roots);
                return 0;
            }
        } while (removed);
        context->metrics->counters.removed_blocks +=
            remove_unreachable(f, detail, capacity, &failed);
        if (failed) {
            free(roots);
            return 0;
        }
    }
    free(roots);
    return 1;
}
