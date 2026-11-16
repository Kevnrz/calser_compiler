/*
 * tac_loop_marks.c — propiedad de marcas S/L, independientes de los análisis.
 * Usa tac_internal.h y tac_loops.h. Lo usan construcción, edición y liberación.
 * Referencias: Aho §8.4.1 y §9.6.4; vida útil del unrolling en el contrato C1.
 */
#include "tac_internal.h"
#include "tac_loops.h"
#include <stdlib.h>

/* Entrada: función HIGH y descripción prestada. Salida: copia propia o NULL.
 * Efectos: agrega al final sin alterar bloques ni CFG.
 * Restricciones: referencias de la misma función; los símbolos sobreviven a la marca.
 */
TacLoopMark *tac_loop_mark_add(TacFunction *function, const TacLoopMark *description) {
    if (!function || !description || function->program->stage != TAC_STAGE_HIGH ||
        (description->kind != TAC_LOOP_S && description->kind != TAC_LOOP_L) ||
        !tac_has_block(function, description->header) ||
        !tac_has_block(function, description->body) ||
        (description->latch && !tac_has_block(function, description->latch)))
        return NULL;
    TacLoopMark *mark = tac_allocate(sizeof *mark);
    *mark = *description;
    mark->next = NULL;
    TacLoopMark **tail = &function->loop_marks;
    while (*tail)
        tail = &(*tail)->next;
    *tail = mark;
    return mark;
}

/* Entrada: función o NULL. Salida: ninguna. Efectos: libera toda la colección.
 * Restricciones: no libera bloques ni símbolos; liberar análisis prestados primero.
 */
void tac_loop_marks_discard(TacFunction *function) {
    if (!function)
        return;
    TacLoopMark *mark = function->loop_marks;
    while (mark) {
        TacLoopMark *next = mark->next;
        free(mark);
        mark = next;
    }
    function->loop_marks = NULL;
}
