/*
 * tac_edit.c — edición de listas de representación intermedia de tres direcciones.
 * Usa: tac_internal.h, biblioteca C. Lo usan futuras pasadas y pruebas/tac.
 * Referencias: Aho §6.2, §8.4.1 (bloques) y §10.2.3 (renombramiento).
 */
#include "tac_internal.h"
#include <stdlib.h>

/* Entrada: programa y función buscada. Salida: pertenencia. Efectos: ninguno.
 * Restricciones: no desreferencia el puntero buscado, solo la lista del programa.
 */
int tac_has_function(const TacProgram *program, const TacFunction *function) {
    for (const TacFunction *f = program->first; f; f = f->next) {
        if (f == function)
            return 1;
    }
    return 0;
}

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

/* Entrada: bloque, previo y datos ISA. Salida: nodo propio o NULL por pertenencia.
 * Efectos: inserta e inicializa; no expande. Restricciones: validar forma después.
 */
TacInstruction *tac_insert_isa_after(TacBlock *block, TacInstruction *previous,
    const IsaInstruction *instruction, TacOperand dest, TacOperand src1, TacOperand src2,
    int64_t displacement, TacOperand target, TacSelectors selectors) {
    TacInstruction *result = tac_insert_after(block, previous, TAC_ISA, dest, src1, src2);
    if (result) {
        result->isa.instruction = instruction;
        result->isa.displacement = displacement;
        result->isa.target = target;
        result->selectors = selectors;
    }
    return result;
}

/* Entrada: bloque, previo y datos PSEUDO. Salida: nodo propio o NULL por pertenencia.
 * Efectos: inserta sin asignar auxiliares. Restricciones: validar forma después.
 */
TacInstruction *tac_insert_pseudo_after(TacBlock *block, TacInstruction *previous,
    TacPseudoKind kind, TacOperand dest, TacOperand src1, TacOperand src2,
    TacSelectors selectors, uint16_t scratch_clobbers) {
    TacInstruction *result = tac_insert_after(block, previous, TAC_PSEUDO, dest, src1, src2);
    if (result) {
        result->pseudo.kind = kind;
        result->pseudo.scratch_clobbers = scratch_clobbers;
        result->selectors = selectors;
    }
    return result;
}

/* Entrada: dos operandos. Salida: igualdad exacta. Efectos: ninguno.
 * Restricciones: valores por identidad; no usa la dirección de almacenamiento.
 */
static int equal_operand(TacOperand a, TacOperand b) {
    if (a.kind != b.kind)
        return 0;
    switch (a.kind) {
        case TAC_NONE: return 1;
        case TAC_INTEGER: return a.value.integer == b.value.integer;
        case TAC_VARIABLE:
        case TAC_FUNCTION: return a.value.symbol == b.value.symbol;
        case TAC_LABEL: return a.value.block == b.value.block;
        case TAC_TEMP:
        case TAC_PHYS: return a.value.id == b.value.id;
    }
    return 0;
}

/* Entrada: bloque, instrucción propia y sustitución. Salida: éxito y cantidad.
 * Efectos: valida una copia antes de modificar cualquier campo del original.
 * Restricciones: rechaza lecturas ligadas a definición; Aho §10.2.3.
 */
int tac_replace_reads(TacBlock *block, TacInstruction *instruction,
    TacOperand from, TacOperand to, size_t *replaced, char *error, size_t capacity) {
    if (capacity)
        error[0] = '\0';
    if (replaced)
        *replaced = 0;
    if (!block || !instruction || !tac_has_instruction(block, instruction) ||
        !tac_validate_instruction(block->function, instruction, error, capacity))
        return tac_error(error, capacity, "reemplazo requiere instrucción propia válida");
    TacOpInfo info;
    tac_instruction_info(instruction, &info);
    TacInstruction candidate = *instruction;
    TacOperand *operands[] = {&candidate.dest, &candidate.src1, &candidate.src2};
    size_t count = 0;
    for (unsigned k = 0; k < 3; k++) {
        unsigned mask = 1u << k;
        if (!(info.reads & mask) || !equal_operand(*operands[k], from))
            continue;
        if (operands[k]->kind == TAC_VARIABLE && (info.address_bases & mask))
            continue;
        if (info.writes & mask)
            return tac_error(error, capacity, "lectura ligada a definición: reemplazo rechazado");
        *operands[k] = to;
        count++;
    }
    if (!tac_validate_instruction(block->function, &candidate, error, capacity))
        return 0;
    instruction->dest = candidate.dest;
    instruction->src1 = candidate.src1;
    instruction->src2 = candidate.src2;
    if (replaced)
        *replaced = count;
    return 1;
}
