/*
 * tac_cfg.c — grafo de representación intermedia de tres direcciones.
 * Usa: tac_internal.h y biblioteca C. Lo usan validación, pasadas y pruebas.
 * Referencias: Aho §8.4.1; CALL/jas conservan continuación dentro del bloque.
 */
#include "tac_internal.h"
#include <stdlib.h>
#include <string.h>

/*
 * tac_has_edge — busca un bloque por identidad en un vector de aristas.
 *
 * Entrada:
 *   edges: vector con count elementos; puede ser NULL si count es cero.
 *   block: puntero buscado, que no se desreferencia.
 * Salida:
 *   1 si aparece; 0 si no.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   el vector debe tener count elementos legibles.
 */
int tac_has_edge(TacBlock *const *edges, size_t count, const TacBlock *block) {
    for (size_t k = 0; k < count; k++) {
        if (edges[k] == block) {
            return 1;
        }
    }
    return 0;
}

/* Agrega al final; termina ante agotamiento de memoria o tamaño. */
static void add_edge(TacBlock ***edges, size_t *count, TacBlock *block) {
    if (*count >= SIZE_MAX / sizeof **edges - 1) {
        fputs("tac: demasiadas aristas\n", stderr);
        exit(EXIT_FAILURE);
    }
    TacBlock **resized = realloc(*edges, (*count + 1) * sizeof *resized);
    if (!resized) {
        fputs("tac: sin memoria\n", stderr);
        exit(EXIT_FAILURE);
    }
    *edges = resized;
    resized[*count] = block;
    (*count)++;
}

/* Quita una arista conservando el orden de las restantes. */
static int remove_edge(TacBlock **edges, size_t *count, const TacBlock *block) {
    for (size_t k = 0; k < *count; k++) {
        if (edges[k] == block) {
            memmove(edges + k, edges + k + 1, (*count - k - 1) * sizeof *edges);
            (*count)--;
            return 1;
        }
    }
    return 0;
}

/*
 * tac_connect — conecta ambos sentidos sin ordenar ni duplicar.
 *
 * Entrada:
 *   from/to: bloques vivos o NULL.
 * Salida:
 *   1 si conecta; 0 si son NULL o de funciones distintas.
 * Efectos:
 *   actualiza sucesores/predecesores sin duplicar.
 * Restricciones:
 *   conserva orden de llamadas; rebuild produce el canónico (Aho §8.4.1).
 */
int tac_connect(TacBlock *from, TacBlock *to) {
    if (!from || !to || from->function != to->function) {
        return 0;
    }
    if (!tac_has_edge(from->successors, from->successor_count, to)) {
        add_edge(&from->successors, &from->successor_count, to);
    }
    if (!tac_has_edge(to->predecessors, to->predecessor_count, from)) {
        add_edge(&to->predecessors, &to->predecessor_count, from);
    }
    return 1;
}

/*
 * tac_disconnect — quita ambos sentidos, también en una autoarista.
 *
 * Entrada:
 *   from/to: bloques vivos o NULL.
 * Salida:
 *   1 si quitó alguna dirección; 0 si no existía o cruza funciones.
 * Efectos:
 *   compacta ambos vectores, conservando el orden restante.
 * Restricciones:
 *   no cambia los terminadores; revalidar (Aho §8.4.1).
 */
int tac_disconnect(TacBlock *from, TacBlock *to) {
    if (!from || !to || from->function != to->function) {
        return 0;
    }
    int forward = remove_edge(from->successors, &from->successor_count, to);
    int backward = remove_edge(to->predecessors, &to->predecessor_count, from);
    return forward || backward;
}

/* Única regla de sucesores, compartida con el validador (Aho §8.4.1). */
int tac_expected_successors(const TacBlock *block, TacBlock *out[2],
                            size_t *count, char *error, size_t cap) {
    *count = 0;
    TacFlow flow = TAC_FLOW_NEXT;
    const TacInstruction *last = NULL;
    for (const TacInstruction *instruction = block->first; instruction;
         instruction = instruction->next) {
        TacOpInfo info;
        if (!tac_instruction_info(instruction, &info)) {
            return tac_error(error, cap, "B%zu: operación inválida", block->label);
        }
        if (info.flow != TAC_FLOW_NEXT && instruction->next) {
            return tac_error(error, cap, "B%zu: terminador no está al final",
                             block->label);
        }
        flow = info.flow;
        last = instruction;
    }
    if (last != block->last) {
        return tac_error(error, cap, "B%zu: last incorrecto", block->label);
    }
    if (!block->next && (flow == TAC_FLOW_NEXT || flow == TAC_FLOW_IF)) {
        return tac_error(error, cap, "B%zu: último bloque sin RETURN/GOTO o caída",
                         block->label);
    }
    if (flow == TAC_FLOW_GOTO || flow == TAC_FLOW_IF) {
        TacOperand label = last->op == TAC_ISA ? last->isa.target : last->dest;
        if (label.kind != TAC_LABEL ||
            !tac_has_block(block->function, label.value.block)) {
            return tac_error(error, cap, "B%zu: etiqueta fuera de la función",
                             block->label);
        }
        out[(*count)++] = (TacBlock *)label.value.block;
    }
    if ((flow == TAC_FLOW_NEXT || flow == TAC_FLOW_IF) && block->next &&
        (*count == 0 || out[0] != block->next)) {
        out[(*count)++] = block->next;
    }
    return 1;
}

/*
 * tac_rebuild_edges — valida la forma antes de reemplazar cualquier arista.
 *
 * Entrada:
 *   function: función o NULL; error/capacity: buffer o NULL con cero.
 * Salida:
 *   1 éxito; 0 con diagnóstico legible.
 * Efectos:
 *   verifica toda la forma; luego reemplaza aristas en ambos sentidos.
 * Restricciones:
 *   ante forma inválida conserva aristas; listas vivas/acíclicas (Aho §8.4.1).
 */
int tac_rebuild_edges(TacFunction *function, char *error, size_t capacity) {
    if (capacity) {
        error[0] = '\0';
    }
    if (!tac_validate_function_form(function, error, capacity)) {
        return 0;
    }
    for (TacBlock *block = function->first; block; block = block->next) {
        free(block->successors);
        free(block->predecessors);
        block->successors = NULL;
        block->predecessors = NULL;
        block->successor_count = 0;
        block->predecessor_count = 0;
    }
    for (TacBlock *block = function->first; block; block = block->next) {
        TacBlock *expected[2];
        size_t count;
        tac_expected_successors(block, expected, &count, NULL, 0);
        for (size_t k = 0; k < count; k++) {
            tac_connect(block, expected[k]);
        }
    }
    return 1;
}
