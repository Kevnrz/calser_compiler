/*
 * tac_validate.c — forma de representación intermedia de tres direcciones.
 * Usa: tac_internal.h. Lo usan pruebas y futuras pasadas.
 * Referencias: Aho §6.2, §8.4.1, §10.2.3. No exige SSA ni definición previa.
 */
#include "tac_internal.h"

/* Datos prestados válidos; TEMP y PHYS se verifican según la etapa. */
static int valid_operand(const TacFunction *function, TacOperand operand) {
    TacStage stage = function->program->stage;
    switch (operand.kind) {
        case TAC_NONE:
        case TAC_INTEGER:
            return 1;
        case TAC_TEMP:
            return stage != TAC_STAGE_ISA_PHYSICAL &&
                   operand.value.id < function->next_temp;
        case TAC_PHYS:
            return stage != TAC_STAGE_HIGH && operand.value.id <= 15 &&
                   (stage != TAC_STAGE_ISA_VIRTUAL || operand.value.id <= TAC_P1_RX);
        case TAC_LABEL:
            return tac_has_block(function, operand.value.block);
        case TAC_FUNCTION:
            return operand.value.symbol && operand.value.symbol->name &&
                   operand.value.symbol->kind == SYMBOL_FUNCTION;
        case TAC_VARIABLE:
            return stage == TAC_STAGE_HIGH && operand.value.symbol &&
                   operand.value.symbol->name &&
                   (operand.value.symbol->kind == SYMBOL_VARIABLE ||
                    operand.value.symbol->kind == SYMBOL_PARAMETER ||
                    operand.value.symbol->kind == SYMBOL_RETURN);
    }
    return 0;
}

/* Selectores presentes son literales 0..3; todos los demás deben ser -1. */
static int valid_selectors(const TacInstruction *instruction, unsigned mask) {
    int selectors[3] = {
        instruction->selectors.k, instruction->selectors.w,
        instruction->selectors.round
    };
    for (unsigned k = 0; k < 3; k++) {
        if (mask & (1u << k)) {
            if (selectors[k] < 0 || selectors[k] > 3) {
                return 0;
            }
        } else if (selectors[k] != -1) {
            return 0;
        }
    }
    return 1;
}

/* La base simbólica debe ser arreglo declarado, nunca parámetro ni escalar. */
static int valid_array_roles(const TacInstruction *instruction, const TacOpInfo *info) {
    TacOperand operands[3] = {
        instruction->dest, instruction->src1, instruction->src2
    };
    for (unsigned k = 0; k < 3; k++) {
        TacOperand operand = operands[k];
        if (operand.kind != TAC_VARIABLE) {
            continue;
        }
        const Symbol *symbol = operand.value.symbol;
        if (info->address_bases & (1u << k)) {
            if (symbol->kind != SYMBOL_VARIABLE || symbol->dimensions <= 0) {
                return 0;
            }
        } else if (symbol->dimensions > 0) {
            if (symbol->kind != SYMBOL_PARAMETER ||
                instruction->op != TAC_COPY || k > 1) {
                return 0;
            }
        }
    }
    return 1;
}

/* Valida una operación antes de inspeccionar sus campos dependientes de forma. */
int tac_validate_instruction(const TacFunction *function,
                             const TacInstruction *instruction,
                             char *error, size_t cap) {
    TacOpInfo info;
    int isa_stage = function->program->stage != TAC_STAGE_HIGH;
    int target_op = instruction->op == TAC_ISA || instruction->op == TAC_PSEUDO;
    if (target_op != isa_stage) {
        return tac_error(error, cap, "operación no corresponde a la etapa");
    }
    if (!tac_instruction_info(instruction, &info)) {
        return tac_error(error, cap, "operación o fila ISA inválida");
    }
    TacOperand operands[3] = {
        instruction->dest, instruction->src1, instruction->src2
    };
    for (unsigned k = 0; k < 3; k++) {
        TacOperand operand = operands[k];
        if ((unsigned)operand.kind > TAC_PHYS ||
            !(info.allowed[k] & TAC_KIND(operand.kind)) ||
            !valid_operand(function, operand)) {
            return tac_error(error, cap, "operando %u inválido para %s o etapa",
                             k, info.text);
        }
    }
    if (!valid_selectors(instruction, info.selectors)) {
        return tac_error(error, cap, "selector inválido para %s", info.text);
    }
    if (instruction->op == TAC_PSEUDO) {
        if (instruction->isa.instruction || instruction->isa.displacement ||
            instruction->isa.target.kind != TAC_NONE) {
            return tac_error(error, cap, "metadatos ISA en PSEUDO");
        }
        return tac_pseudo_validate(function, instruction, error, cap);
    }
    if (instruction->pseudo.kind != TAC_PSEUDO_NONE ||
        instruction->pseudo.scratch_clobbers) {
        return tac_error(error, cap, "metadatos PSEUDO en otra operación");
    }
    if (isa_stage) {
        if (!valid_operand(function, instruction->isa.target)) {
            return tac_error(error, cap, "objetivo ISA inválido");
        }
        return tac_isa_validate(instruction, error, cap);
    }
    if (instruction->isa.instruction || instruction->isa.displacement ||
        instruction->isa.target.kind != TAC_NONE) {
        return tac_error(error, cap, "metadatos ISA en alto nivel");
    }
    if (!valid_array_roles(instruction, &info)) {
        return tac_error(error, cap, "base o uso de arreglo inválido");
    }
    if (instruction->op == TAC_CALL && instruction->src2.value.integer < 0) {
        return tac_error(error, cap, "aridad negativa");
    }
    return 1;
}

/* Recorre el grupo previo a CALL, buscando unicidad y rango sin reservar memoria. */
static int valid_parameters(const TacInstruction *start,
                            const TacInstruction *call, size_t count,
                            char *error, size_t cap) {
    int64_t arity = call->src2.value.integer;
    if ((uint64_t)arity != count) {
        return tac_error(error, cap, "PARAM faltantes o sobrantes para CALL");
    }
    for (const TacInstruction *current = start; current != call;
         current = current->next) {
        if (current->op != TAC_PARAM) {
            continue;
        }
        int64_t position = current->src2.value.integer;
        if (position < 0 || position >= arity) {
            return tac_error(error, cap, "posición PARAM fuera de rango");
        }
        for (const TacInstruction *previous = start; previous != current;
             previous = previous->next) {
            if (previous->op == TAC_PARAM &&
                previous->src2.value.integer == position) {
                return tac_error(error, cap, "posición PARAM duplicada");
            }
        }
    }
    return 1;
}

/* Comprueba instrucciones y argumentos de un bloque; no examina aristas viejas. */
static int valid_block_form(const TacFunction *function, const TacBlock *block,
                            char *error, size_t cap) {
    const TacInstruction *group_start = block->first;
    size_t pending = 0;
    for (const TacInstruction *instruction = block->first; instruction;
         instruction = instruction->next) {
        if (!tac_validate_instruction(function, instruction, error, cap)) {
            return 0;
        }
        if (instruction->op == TAC_PARAM) {
            pending++;
        } else if (instruction->op == TAC_CALL) {
            if (!valid_parameters(group_start, instruction, pending, error, cap)) {
                return 0;
            }
            pending = 0;
            group_start = instruction->next;
        }
    }
    if (pending) {
        return tac_error(error, cap, "PARAM sin CALL en el mismo bloque");
    }
    TacBlock *expected[2];
    size_t count;
    return tac_expected_successors(block, expected, &count, error, cap);
}

/* Parte compartida: valida la función completa sin exigir aristas preexistentes. */
int tac_validate_function_form(const TacFunction *function, char *error, size_t cap) {
    if (!function || !function->program || !function->symbol ||
        function->symbol->kind != SYMBOL_FUNCTION || !function->symbol->name) {
        return tac_error(error, cap, "función sin Symbol o programa válido");
    }
    if ((unsigned)function->program->stage > TAC_STAGE_ISA_PHYSICAL) {
        return tac_error(error, cap, "etapa inválida");
    }
    if (!function->program->global_scope ||
        function->program->global_scope->kind != SCOPE_GLOBAL) {
        return tac_error(error, cap, "ámbito global inválido");
    }
    if (!function->first || function->entry != function->first) {
        return tac_error(error, cap, "función sin bloque de entrada inicial");
    }
    const TacBlock *last = NULL;
    for (const TacBlock *block = function->first; block; block = block->next) {
        if (block->function != function || block->label >= function->next_label) {
            return tac_error(error, cap, "dueño o identificador de bloque inválido");
        }
        for (const TacBlock *previous = function->first; previous != block;
             previous = previous->next) {
            if (previous->label == block->label) {
                return tac_error(error, cap, "etiqueta de bloque duplicada");
            }
        }
        if (!valid_block_form(function, block, error, cap)) {
            return 0;
        }
        last = block;
    }
    return last == function->last ? 1 : tac_error(error, cap, "last de función inválido");
}

/* Valida sucesores canónicos, predecesores únicos y reciprocidad. */
static int valid_edges(const TacBlock *block, char *error, size_t cap) {
    TacBlock *expected[2];
    size_t count;
    if (!tac_expected_successors(block, expected, &count, error, cap)) {
        return 0;
    }
    if ((block->successor_count && !block->successors) ||
        (block->predecessor_count && !block->predecessors)) {
        return tac_error(error, cap, "B%zu: vector de aristas ausente",
                         tac_block_presentation_number(block));
    }
    if (count != block->successor_count) {
        return tac_error(error, cap, "B%zu: sucesores incorrectos",
                         tac_block_presentation_number(block));
    }
    for (size_t k = 0; k < count; k++) {
        if (block->successors[k] != expected[k] ||
            !tac_has_edge(expected[k]->predecessors, expected[k]->predecessor_count, block)) {
            return tac_error(error, cap, "B%zu: orden/recíproca incorrectos",
                             tac_block_presentation_number(block));
        }
    }
    for (size_t k = 0; k < block->predecessor_count; k++) {
        TacBlock *previous = block->predecessors[k];
        if (!tac_has_block(block->function, previous) ||
            !tac_has_edge(previous->successors, previous->successor_count, block)) {
            return tac_error(error, cap, "B%zu: predecesor inválido",
                             tac_block_presentation_number(block));
        }
        if (tac_has_edge(block->predecessors, k, previous)) {
            return tac_error(error, cap, "B%zu: predecesor duplicado",
                             tac_block_presentation_number(block));
        }
    }
    return 1;
}

/*
 * tac_validate — comprueba forma y grafo completos, sin exigir SSA.
 *
 * Entrada:
 *   program: programa o NULL; error/capacity: buffer o NULL con cero.
 * Salida:
 *   1 válido; 0 y diagnóstico de forma, etapa, argumentos o grafo.
 * Efectos:
 *   solo escribe error, nunca modifica el programa.
 * Restricciones:
 *   listas vivas/acíclicas; no verifica SSA ni tipos CLS (Aho §6.2, §8.4.1).
 */
int tac_validate(const TacProgram *program, char *error, size_t capacity) {
    if (capacity) {
        error[0] = '\0';
    }
    if (!program || (unsigned)program->stage > TAC_STAGE_ISA_PHYSICAL ||
        (!program->global_scope || program->global_scope->kind != SCOPE_GLOBAL)) {
        return tac_error(error, capacity, "programa, etapa o ámbito global inválido");
    }
    const TacFunction *last = NULL;
    for (const TacFunction *function = program->first; function; function = function->next) {
        if (function->program != program) {
            return tac_error(error, capacity, "dueño de función incorrecto");
        }
        if (!tac_validate_function_form(function, error, capacity)) {
            return 0;
        }
        for (const TacBlock *block = function->first; block; block = block->next) {
            if (!valid_edges(block, error, capacity)) {
                return 0;
            }
        }
        last = function;
    }
    return last == program->last ? 1 : tac_error(error, capacity, "last del programa inválido");
}

/*
 * tac_validate_for_emission — exige instrucciones físicas reales antes de emitir.
 * Entrada:
 *   program o NULL; error escribible o NULL solo si capacity es cero.
 * Salida:
 *   1 si puede entregar ISA al emisor; 0 con diagnóstico.
 * Efectos:
 *   valida todo el programa sin modificarlo ni expandir operaciones.
 * Restricciones:
 *   el futuro emisor debe llamar antes de emitir; Aho §8.6 y §8.8.
 */
int tac_validate_for_emission(const TacProgram *program, char *error, size_t capacity) {
    if (!tac_validate(program, error, capacity)) {
        return 0;
    }
    if (program->stage != TAC_STAGE_ISA_PHYSICAL) {
        return tac_error(error, capacity, "emisión requiere etapa ISA_PHYSICAL");
    }
    for (const TacFunction *function = program->first; function; function = function->next) {
        for (const TacBlock *block = function->first; block; block = block->next) {
            for (const TacInstruction *instruction = block->first; instruction;
                 instruction = instruction->next) {
                if (instruction->op == TAC_PSEUDO) {
                    return tac_error(error, capacity, "B%zu: PSEUDO sin expandir antes de emitir",
                                     tac_block_presentation_number(block));
                }
            }
        }
    }
    return 1;
}
