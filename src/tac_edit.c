/*
 * tac_edit.c — edición de listas de representación intermedia de tres direcciones.
 * Usa: tac_internal.h, biblioteca C. Lo usan futuras pasadas y pruebas/tac.
 * Referencias: Aho §6.2, §8.4.1 (bloques) y §10.2.3 (renombramiento).
 */
#include "tac_internal.h"
#include <stdlib.h>

/* Comprueba pertenencia sin desreferenciar el puntero buscado. */
int tac_has_block(const TacFunction *function, const TacBlock *block) {
    for (const TacBlock *current = function->first; current; current = current->next) {
        if (current == block) {
            return 1;
        }
    }
    return 0;
}

/* Comprueba pertenencia de una instrucción antes de editar sus enlaces. */
int tac_has_instruction(const TacBlock *block, const TacInstruction *instruction) {
    for (const TacInstruction *current = block->first; current;
         current = current->next) {
        if (current == instruction) {
            return 1;
        }
    }
    return 0;
}

/*
 * tac_block_insert_after — inserta sin cambiar identidades ni recalcular caídas.
 *
 * Entrada:
 *   function: función; previous: bloque propio o NULL para inicio.
 * Salida:
 *   bloque nuevo; NULL ante función NULL o previo ajeno.
 * Efectos:
 *   enlaza, actualiza first/last/entry; reserva etiqueta única.
 * Restricciones:
 *   no reconstruye aristas; revalidar después de editar (Aho §8.4.1).
 */
TacBlock *tac_block_insert_after(TacFunction *function, TacBlock *previous) {
    if (!function || (previous && !tac_has_block(function, previous))) {
        return NULL;
    }
    TacBlock *block = tac_allocate(sizeof *block);
    block->label = tac_next_identifier(&function->next_label);
    block->function = function;
    block->next = previous ? previous->next : function->first;
    if (previous) {
        previous->next = block;
    } else {
        function->first = block;
        function->entry = block;
    }
    if (!block->next) {
        function->last = block;
    }
    return block;
}

/*
 * tac_block_new — agrega un bloque al final de su función.
 *
 * Entrada:
 *   function: función o NULL.
 * Salida:
 *   bloque nuevo al final; NULL si function es NULL.
 * Efectos:
 *   reserva y enlaza; primer bloque se convierte en entrada.
 * Restricciones:
 *   no reconstruye caídas existentes (Aho §8.4.1).
 */
TacBlock *tac_block_new(TacFunction *function) {
    return function ? tac_block_insert_after(function, function->last) : NULL;
}

/*
 * tac_insert_after — copia cuádrupla e inicializa los metadatos opcionales.
 *
 * Entrada:
 *   block: bloque; previous: instrucción propia o NULL para inicio;
 *   op/dest/src1/src2: cuádrupla por valor.
 * Salida:
 *   instrucción nueva; NULL si block es NULL o previous es ajena.
 * Efectos:
 *   reserva, enlaza y actualiza first/last; selectores ausentes = -1.
 * Restricciones:
 *   puede dejar forma temporalmente inválida; validar luego (Aho §6.2.2).
 */
TacInstruction *tac_insert_after(TacBlock *block, TacInstruction *previous,
                               TacOpcode op, TacOperand dest,
                               TacOperand src1, TacOperand src2) {
    if (!block || (previous && !tac_has_instruction(block, previous))) {
        return NULL;
    }
    TacInstruction *instruction = tac_allocate(sizeof *instruction);
    instruction->op = op;
    instruction->dest = dest;
    instruction->src1 = src1;
    instruction->src2 = src2;
    instruction->selectors = (TacSelectors){-1, -1, -1};
    instruction->next = previous ? previous->next : block->first;
    if (previous) {
        previous->next = instruction;
    } else {
        block->first = instruction;
    }
    if (!instruction->next) {
        block->last = instruction;
    }
    return instruction;
}

/*
 * tac_append — agrega una cuádrupla al final.
 *
 * Entrada:
 *   block: bloque o NULL; op/dest/src1/src2: cuádrupla por valor.
 * Salida:
 *   instrucción nueva al final; NULL si block es NULL.
 * Efectos:
 *   reserva y actualiza extremos; no reconstruye el grafo.
 * Restricciones:
 *   los operandos prestados deben sobrevivir (Aho §6.2.2).
 */
TacInstruction *tac_append(TacBlock *block, TacOpcode op,
                          TacOperand dest, TacOperand src1, TacOperand src2) {
    return block ? tac_insert_after(block, block->last, op, dest, src1, src2) : NULL;
}

/*
 * tac_remove — quita y libera un nodo propio, manteniendo ambos extremos.
 *
 * Entrada:
 *   block: bloque o NULL; instruction: nodo propio, ajeno o NULL.
 * Salida:
 *   1 si lo quitó; 0 si no pertenece.
 * Efectos:
 *   libera el nodo y mantiene first/last.
 * Restricciones:
 *   guardar next antes de borrar durante recorrido; reconstruir aristas.
 */
int tac_remove(TacBlock *block, TacInstruction *instruction) {
    if (!block || !instruction) {
        return 0;
    }
    TacInstruction *previous = NULL;
    TacInstruction *current = block->first;
    while (current && current != instruction) {
        previous = current;
        current = current->next;
    }
    if (!current) {
        return 0;
    }
    if (previous) {
        previous->next = current->next;
    } else {
        block->first = current->next;
    }
    if (block->last == current) {
        block->last = previous;
    }
    free(current);
    return 1;
}
