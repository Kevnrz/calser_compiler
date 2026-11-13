/*
 * tac_lower.c — programa y correspondencia de bloques HIGH -> ISA virtual.
 * Usa: tac_lower_internal.h. No usa estado del generador ni de la CLI.
 * Lo usan pruebas y futuras pasadas O1; Aho §8.1–8.2, §8.4.1 y §8.6.
 */
#include "tac_lower_internal.h"
#include <stdlib.h>

/*
 * tl_fail — conserva el primer error con su posición fuente.
 * Entrada:
 *   lower vivo y mensaje no NULL.
 * Salida:
 *   ninguna.
 * Efectos:
 *   marca fallo y escribe error si hay capacidad.
 * Restricciones:
 *   no modifica el programa HIGH; columna cero si es desconocida.
 */
void tl_fail(TacLower *lower, const char *message) {
    if (!lower->failed && lower->capacity) {
        snprintf(lower->error, lower->capacity, "linea %d:%d: %s",
                 lower->source ? lower->source->source_line : 0,
                 lower->source ? lower->source->source_column : 0, message);
    }
    lower->failed = 1;
}

/*
 * tl_location — propaga posición a cada instrucción seleccionada.
 * Entrada:
 *   lower con fuente viva; instruction nueva o NULL.
 * Salida:
 *   ninguna.
 * Efectos:
 *   copia línea y columna, sin cambiar la fuente.
 * Restricciones:
 *   no asigna posición cuando no hay fuente.
 */
void tl_location(const TacLower *lower, TacInstruction *instruction) {
    if (instruction && lower->source) {
        instruction->source_line = lower->source->source_line;
        instruction->source_column = lower->source->source_column;
    }
}

/*
 * tl_emit — agrega una instrucción real con operandos normalizados.
 * Entrada:
 *   lower con bloque vivo, mnemónico y D/A/B.
 * Salida:
 *   instrucción propia o NULL si ya falló la selección.
 * Efectos:
 *   reserva y enlaza; copia posición; no emite ensamblador.
 * Restricciones:
 *   el mnemónico debe existir en isa_table; Aho §8.2.
 */
TacInstruction *tl_emit(TacLower *lower, const char *name,
                        TacOperand dest, TacOperand src1, TacOperand src2) {
    if (lower->failed) {
        return NULL;
    }
    const IsaInstruction *isa = isa_find(name);
    if (!isa) {
        tl_fail(lower, "instrucción ISA desconocida al seleccionar");
        return NULL;
    }
    TacInstruction *instruction = tac_append_isa(lower->block, isa, dest, src1, src2,
                                                 0, tac_none(), (TacSelectors){-1, -1, -1});
    tl_location(lower, instruction);
    return instruction;
}

/*
 * prepare_blocks — reserva todos los destinos antes de traducir saltos.
 * Entrada:
 *   función HIGH válida y lower con función de salida nueva.
 * Salida:
 *   ninguna.
 * Efectos:
 *   reserva mapa y bloques en el orden de lista original.
 * Restricciones:
 *   mapa por punteros, nunca por label ni número impreso; Aho §8.4.1.
 */
static void prepare_blocks(const TacFunction *high, TacLower *lower) {
    for (const TacBlock *block = high->first; block; block = block->next) {
        lower->block_count++;
    }
    lower->blocks = calloc(lower->block_count, sizeof *lower->blocks);
    if (!lower->blocks) {
        fputs("tac_lower: sin memoria\n", stderr);
        exit(EXIT_FAILURE);
    }
    size_t k = 0;
    for (const TacBlock *block = high->first; block; block = block->next) {
        lower->blocks[k].high = block;
        lower->blocks[k].lowered = tac_block_new(lower->function);
        k++;
    }
    /* TEMP es una identidad local por valor, no un puntero al programa HIGH. */
    lower->function->next_temp = high->next_temp;
}

/*
 * lower_function — selecciona una función y reconstruye su grafo.
 * Entrada:
 *   función HIGH y programa de salida vivos; buffer de diagnóstico.
 * Salida:
 *   1 si termina; 0 con diagnóstico ante fallo.
 * Efectos:
 *   agrega función, bloques y operaciones; libera el mapa temporal.
 * Restricciones:
 *   sin reordenamiento ni eliminación, incluidos bloques inalcanzables.
 */
static int lower_function(const TacFunction *high, TacProgram *program,
                           char *error, size_t capacity) {
    TacLower lower = {.function = tac_function_new(program, high->symbol),
                      .error = error, .capacity = capacity};
    prepare_blocks(high, &lower);
    for (size_t k = 0; k < lower.block_count && !lower.failed; k++) {
        lower.block = lower.blocks[k].lowered;
        for (lower.source = lower.blocks[k].high->first; lower.source && !lower.failed;
             lower.source = lower.source->next) {
            if (!tl_lower_value(&lower) && !tl_lower_control(&lower)) {
                tl_fail(&lower, "operación HIGH sin selección");
            }
        }
    }
    free(lower.blocks);
    return !lower.failed && tac_rebuild_edges(lower.function, error, capacity);
}

/*
 * tac_lower — véase el contrato público en tac_lower.h.
 * Entrada:
 *   HIGH y símbolos semánticos vivos; error/capacity.
 * Salida:
 *   programa ISA_VIRTUAL propio o NULL.
 * Efectos:
 *   valida entrada y salida, liberando una salida parcial ante error.
 * Restricciones:
 *   no toca HIGH, imagen, ámbitos ni generador; Aho §8.1–8.2 y §8.6.
 */
TacProgram *tac_lower(const TacProgram *high, char *error, size_t capacity) {
    if (capacity) {
        error[0] = '\0';
    }
    if (!high || high->stage != TAC_STAGE_HIGH) {
        if (capacity) {
            snprintf(error, capacity, "la selección requiere un programa HIGH");
        }
        return NULL;
    }
    if (!tac_validate(high, error, capacity)) {
        return NULL;
    }
    TacProgram *program = tac_program_new(high->global_scope);
    program->stage = TAC_STAGE_ISA_VIRTUAL;
    for (const TacFunction *function = high->first; function; function = function->next) {
        if (!lower_function(function, program, error, capacity)) {
            tac_program_free(program);
            return NULL;
        }
    }
    if (!tac_validate(program, error, capacity)) {
        tac_program_free(program);
        return NULL;
    }
    return program;
}
