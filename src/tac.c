/*
 * tac.c — propiedad y consultas de representación intermedia de tres direcciones.
 * Usa: tac_internal.h, biblioteca C. Lo usan pruebas y futuras pasadas.
 * Referencias: Aho §6.2.1-2, §10.2.3; enunciado §4.1.
 */
#include "tac_internal.h"
#include <stdarg.h>
#include <stdlib.h>

#define VALUE_DEST (TAC_KIND(TAC_TEMP) | TAC_KIND(TAC_VARIABLE))
#define VALUE_SOURCE (VALUE_DEST | TAC_KIND(TAC_INTEGER))

/* Propiedades de alto nivel; ISA se consulta en isa_table, sin otra tabla. */
static const TacOpInfo operations[TAC_ISA] = {
    [TAC_COPY] = {
        .text = "=",
        .writes = TAC_DEST,
        .reads = TAC_SRC1,
        .format = TAC_FMT_COPY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_ADDRESS] = {
        .text = "address",
        .writes = TAC_DEST,
        .address_bases = TAC_SRC1,
        .format = TAC_FMT_ADDRESS,
        .allowed = {VALUE_DEST, TAC_KIND(TAC_VARIABLE), TAC_KIND(TAC_NONE)},
    },
    [TAC_ADD] = {
        .text = "+",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_SUB] = {
        .text = "-",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_AND] = {
        .text = "and",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_OR] = {
        .text = "or",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_XOR] = {
        .text = "xor",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_EQ] = {
        .text = "==",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_NE] = {
        .text = "!=",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_LT] = {
        .text = "<",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_LE] = {
        .text = "<=",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_GT] = {
        .text = ">",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_GE] = {
        .text = ">=",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_NEG] = {
        .text = "neg",
        .writes = TAC_DEST,
        .reads = TAC_SRC1,
        .format = TAC_FMT_UNARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_NOT] = {
        .text = "not",
        .writes = TAC_DEST,
        .reads = TAC_SRC1,
        .format = TAC_FMT_UNARY,
        .allowed = {VALUE_DEST, VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_MUL] = {
        .text = "*",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .may_create_blocks = 1,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_DIV] = {
        .text = "/",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .may_create_blocks = 1,
        .may_trap = 1,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_MOD] = {
        .text = "%",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .may_create_blocks = 1,
        .may_trap = 1,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_POW] = {
        .text = "^",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .format = TAC_FMT_BINARY,
        .may_create_blocks = 1,
        .may_trap = 1,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_LOAD] = {
        .text = "load",
        .writes = TAC_DEST,
        .reads = TAC_SRC1 | TAC_SRC2,
        .address_bases = TAC_SRC1,
        .memory_read = 1,
        .format = TAC_FMT_LOAD,
        .allowed = {VALUE_DEST, VALUE_DEST, VALUE_SOURCE},
    },
    [TAC_STORE] = {
        .text = "store",
        .reads = TAC_DEST | TAC_SRC1 | TAC_SRC2,
        .address_bases = TAC_DEST,
        .memory_write = 1,
        .nonremovable = 1,
        .format = TAC_FMT_STORE,
        .allowed = {VALUE_DEST, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_AUTH] = {
        .text = "autenticar",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .reads = TAC_SRC1,
        .allowed = {TAC_KIND(TAC_NONE), VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_LOGOUT] = {
        .text = "cerrar_sesion",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .allowed = {TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE)},
    },
    [TAC_KV_WRITE] = {
        .text = "cargar_llave",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .reads = TAC_SRC1,
        .selectors = TAC_SELECT_K | TAC_SELECT_W,
        .allowed = {TAC_KIND(TAC_NONE), VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_KV_LOCK] = {
        .text = "sellar",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .allowed = {TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE)},
    },
    [TAC_SANC] = {
        .text = "cambiar_ancla",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .reads = TAC_SRC1,
        .allowed = {TAC_KIND(TAC_NONE), VALUE_SOURCE, TAC_KIND(TAC_NONE)},
    },
    [TAC_CIFRAR] = {
        .text = "cifrar",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .reads = TAC_DEST | TAC_SRC1 | TAC_SRC2,
        .selectors = TAC_SELECT_K,
        .memory_read = 1,
        .memory_write = 1,
        .may_create_blocks = 1,
        .allowed = {VALUE_SOURCE, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_DESCIFRAR] = {
        .text = "descifrar",
        .barrier = 1,
        .nonremovable = 1,
        .crypto_state = 1,
        .format = TAC_FMT_CRYPTO,
        .reads = TAC_DEST | TAC_SRC1 | TAC_SRC2,
        .selectors = TAC_SELECT_K,
        .memory_read = 1,
        .memory_write = 1,
        .may_create_blocks = 1,
        .allowed = {VALUE_SOURCE, VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_PARAM] = {
        .text = "param",
        .reads = TAC_SRC1,
        .nonremovable = 1,
        .format = TAC_FMT_PARAM,
        .allowed = {TAC_KIND(TAC_NONE), VALUE_SOURCE, TAC_KIND(TAC_INTEGER)},
    },
    [TAC_CALL] = {
        .text = "call",
        .writes = TAC_DEST,
        .memory_read = 1,
        .memory_write = 1,
        .barrier = 1,
        .nonremovable = 1,
        .uses_globals = 1,
        .defines_globals = 1,
        .implicit_phys_reads = (1u << TAC_P1_RL) | (1u << TAC_P1_RX),
        .implicit_phys_writes = (1u << TAC_P1_RL) | (1u << TAC_P1_RX),
        .call_clobbers = TAC_P1_CALL_CLOBBERS,
        .format = TAC_FMT_CALL,
        .allowed = {VALUE_DEST | TAC_KIND(TAC_NONE), TAC_KIND(TAC_FUNCTION), TAC_KIND(TAC_INTEGER)},
    },
    [TAC_RETURN] = {
        .text = "return",
        .reads = TAC_SRC1,
        .barrier = 1,
        .nonremovable = 1,
        .uses_globals = 1,
        .flow = TAC_FLOW_RETURN,
        .format = TAC_FMT_RETURN,
        .allowed = {TAC_KIND(TAC_NONE), VALUE_SOURCE | TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE)},
    },
    [TAC_GOTO] = {
        .text = "goto",
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_GOTO,
        .format = TAC_FMT_GOTO,
        .allowed = {TAC_KIND(TAC_LABEL), TAC_KIND(TAC_NONE), TAC_KIND(TAC_NONE)},
    },
    [TAC_IF_EQ] = {
        .text = "==",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_IF_NE] = {
        .text = "!=",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_IF_LT] = {
        .text = "<",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_IF_LE] = {
        .text = "<=",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_IF_GT] = {
        .text = ">",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
    [TAC_IF_GE] = {
        .text = ">=",
        .reads = TAC_SRC1 | TAC_SRC2,
        .barrier = 1,
        .nonremovable = 1,
        .flow = TAC_FLOW_IF,
        .format = TAC_FMT_IF,
        .allowed = {TAC_KIND(TAC_LABEL), VALUE_SOURCE, VALUE_SOURCE},
    },
};

/* Reserva inicializada; termina si falta memoria. */
void *tac_allocate(size_t bytes) {
    void *memory = calloc(1, bytes);
    if (!memory) {
        fputs("tac: sin memoria\n", stderr);
        exit(EXIT_FAILURE);
    }
    return memory;
}

/* Los contadores virtuales nunca envuelven ni se limitan al banco físico. */
size_t tac_next_identifier(size_t *counter) {
    if (*counter == SIZE_MAX) {
        fputs("tac: identificadores agotados\n", stderr);
        exit(EXIT_FAILURE);
    }
    size_t id = *counter;
    (*counter)++;
    return id;
}

/* Diagnóstico acotado: capacity=0 permite error=NULL. */
int tac_error(char *error, size_t capacity, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    if (capacity) {
        vsnprintf(error, capacity, format, arguments);
    }
    va_end(arguments);
    return 0;
}

/*
 * tac_none — crea una posición ausente.
 *
 * Entrada:
 *   ninguna.
 * Salida:
 *   operando NONE por valor.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   ninguna.
 */
TacOperand tac_none(void) {
    TacOperand operand = {0};
    return operand;
}

/*
 * tac_variable — referencia un símbolo de datos prestado.
 *
 * Entrada:
 *   symbol: símbolo de datos prestado.
 * Salida:
 *   operando VARIABLE por valor.
 * Efectos:
 *   no copia ni libera el símbolo.
 * Restricciones:
 *   symbol debe sobrevivir al programa; la forma se valida después.
 */
TacOperand tac_variable(const Symbol *symbol) {
    TacOperand operand = {.kind = TAC_VARIABLE};
    operand.value.symbol = symbol;
    return operand;
}

/*
 * tac_integer — conserva una constante sin truncarla.
 *
 * Entrada:
 *   value: constante entera de 64 bits.
 * Salida:
 *   operando INTEGER sin truncar.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   el rango ISA se comprueba en la etapa ISA.
 */
TacOperand tac_integer(int64_t value) {
    TacOperand operand = {.kind = TAC_INTEGER};
    operand.value.integer = value;
    return operand;
}

/*
 * tac_label — referencia un bloque vivo.
 *
 * Entrada:
 *   block: referencia prestada a un bloque.
 * Salida:
 *   operando LABEL por referencia.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   debe pertenecer a la misma función cuando se valide (Aho §8.4.1).
 */
TacOperand tac_label(const TacBlock *block) {
    TacOperand operand = {.kind = TAC_LABEL};
    operand.value.block = block;
    return operand;
}

/*
 * tac_function_operand — referencia el símbolo de una función.
 *
 * Entrada:
 *   symbol: símbolo de función prestado.
 * Salida:
 *   operando FUNCTION por identidad.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   symbol debe sobrevivir al programa (Aho §6.2.1).
 */
TacOperand tac_function_operand(const Symbol *symbol) {
    TacOperand operand = {.kind = TAC_FUNCTION};
    operand.value.symbol = symbol;
    return operand;
}

/*
 * tac_physical — distingue registro arquitectónico de temporal virtual.
 *
 * Entrada:
 *   register_number: número arquitectónico.
 * Salida:
 *   operando PHYS, distinto de TEMP.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   validación posterior exige 0..15 y la etapa correspondiente.
 */
TacOperand tac_physical(unsigned register_number) {
    TacOperand operand = {.kind = TAC_PHYS};
    operand.value.id = register_number;
    return operand;
}

/*
 * tac_temp_new — reserva identidad virtual local a la función.
 *
 * Entrada:
 *   function: función viva no NULL.
 * Salida:
 *   temporal virtual nuevo, local a function.
 * Efectos:
 *   avanza next_temp; termina si se agotan identificadores.
 * Restricciones:
 *   no asigna registro físico ni impone límite de doce (Aho §10.2.3).
 */
TacOperand tac_temp_new(TacFunction *function) {
    TacOperand operand = {.kind = TAC_TEMP};
    operand.value.id = tac_next_identifier(&function->next_temp);
    return operand;
}

/*
 * tac_is_global — clasifica por identidad y clase, nunca por nombre.
 *
 * Entrada:
 *   program: programa o NULL; operand: operando por valor.
 * Salida:
 *   1 solo para VARIABLE global de clase SYMBOL_VARIABLE; 0 si no.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   compara punteros en global_scope; ignora funciones y homónimos.
 */
int tac_is_global(const TacProgram *program, TacOperand operand) {
    if (!program || !program->global_scope || operand.kind != TAC_VARIABLE ||
        !operand.value.symbol || operand.value.symbol->kind != SYMBOL_VARIABLE) {
        return 0;
    }
    for (const Symbol *symbol = program->global_scope->symbols;
         symbol; symbol = symbol->next) {
        if (symbol == operand.value.symbol) {
            return 1;
        }
    }
    return 0;
}

/*
 * tac_op_info — consulta propiedades inmutables de alto nivel.
 *
 * Entrada:
 *   op: código de operación.
 * Salida:
 *   fila inmutable; NULL para TAC_ISA o código inválido.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   ISA se consulta por instrucción, no por opcode (Aho §6.2.2).
 */
const TacOpInfo *tac_op_info(TacOpcode op) {
    return (unsigned)op < TAC_ISA ? &operations[op] : NULL;
}

/*
 * tac_instruction_info — resuelve propiedades de cualquiera de los niveles.
 *
 * Entrada:
 *   instruction: instrucción o NULL; out: salida escribible o NULL.
 * Salida:
 *   1 si obtiene propiedades; 0 si es inválida.
 * Efectos:
 *   inicializa out; incorpora resúmenes de globales y ABI.
 * Restricciones:
 *   no valida la forma completa; ISA usa isa_table (Aho §10.2.3).
 */
int tac_instruction_info(const TacInstruction *instruction, TacOpInfo *out) {
    if (!out) {
        return 0;
    }
    *out = (TacOpInfo){0};
    if (!instruction) {
        return 0;
    }
    if (instruction->op == TAC_ISA) {
        return tac_isa_info(instruction, out);
    }
    const TacOpInfo *info = tac_op_info(instruction->op);
    if (!info) {
        return 0;
    }
    *out = *info;
    return 1;
}

/* Identidad de un valor explícito, independiente de su nombre impreso. */
static int same_value(TacOperand left, TacOperand right) {
    if (left.kind != right.kind) {
        return 0;
    }
    if (left.kind == TAC_VARIABLE) {
        return left.value.symbol == right.value.symbol;
    }
    return left.value.id == right.value.id;
}

/* Proyecta posiciones sin duplicar valores; memoria/globales quedan implícitas. */
static size_t instruction_values(const TacInstruction *instruction,
                                  TacOperand out[3], int definitions) {
    TacOpInfo info;
    if (!tac_instruction_info(instruction, &info)) {
        return 0;
    }
    unsigned mask = definitions ? info.writes : info.reads;
    TacOperand operands[3] = {
        instruction->dest, instruction->src1, instruction->src2
    };
    size_t count = 0;
    for (unsigned position = 0; position < 3; position++) {
        TacOperand operand = operands[position];
        if (!(mask & (1u << position))) {
            continue;
        }
        if (operand.kind != TAC_VARIABLE && operand.kind != TAC_TEMP &&
            operand.kind != TAC_PHYS) {
            continue;
        }
        if (operand.kind == TAC_VARIABLE &&
            (info.address_bases & (1u << position))) {
            continue;
        }
        if (definitions && operand.kind == TAC_PHYS &&
            operand.value.id == TAC_P1_RZ) {
            continue;
        }
        size_t index = 0;
        while (index < count && !same_value(operand, out[index])) {
            index++;
        }
        if (index == count) {
            out[count] = operand;
            count++;
        }
    }
    return count;
}

/*
 * tac_definitions — obtiene definiciones explícitas en orden de posiciones.
 *
 * Entrada:
 *   instruction: instrucción o NULL; out: capacidad para tres operandos.
 * Salida:
 *   0..3 definiciones explícitas, sin duplicados.
 * Efectos:
 *   escribe out; no modifica instruction.
 * Restricciones:
 *   agregar globales/registros implícitos y call_clobbers (Aho §10.2.3).
 */
size_t tac_definitions(const TacInstruction *instruction, TacOperand out[3]) {
    return instruction_values(instruction, out, 1);
}

/*
 * tac_uses — obtiene usos explícitos, incluido el destino preservado ISA.
 *
 * Entrada:
 *   instruction: instrucción o NULL; out: capacidad para tres operandos.
 * Salida:
 *   0..3 usos explícitos, sin duplicados, incluido destino preservado ISA.
 * Efectos:
 *   escribe out; omite la base VARIABLE estática de LOAD/STORE.
 * Restricciones:
 *   agregar usos implícitos de TacOpInfo (Aho §6.2.1, §10.2.3).
 */
size_t tac_uses(const TacInstruction *instruction, TacOperand out[3]) {
    return instruction_values(instruction, out, 0);
}

/*
 * tac_program_new — crea un propietario en etapa de alto nivel.
 *
 * Entrada:
 *   global_scope: ámbito GLOBAL prestado, obligatorio aunque esté vacío.
 * Salida:
 *   programa vacío en TAC_STAGE_HIGH.
 * Efectos:
 *   reserva memoria; ámbito NULL/no GLOBAL o sin memoria: diagnóstico y termina.
 * Restricciones:
 *   el ámbito y los símbolos sobreviven al programa. El caso NULL no se
 *   prueba en el mismo proceso porque termina; no devuelve NULL.
 */
TacProgram *tac_program_new(const Scope *global_scope) {
    if (!global_scope || global_scope->kind != SCOPE_GLOBAL) {
        fputs("tac: se requiere un ámbito SCOPE_GLOBAL no NULL\n", stderr);
        exit(EXIT_FAILURE);
    }
    TacProgram *program = tac_allocate(sizeof *program);
    program->global_scope = global_scope;
    program->stage = TAC_STAGE_HIGH;
    return program;
}

/*
 * tac_function_new — agrega una función identificada por su Symbol.
 *
 * Entrada:
 *   program: programa vivo; symbol: símbolo FUNCTION prestado.
 * Salida:
 *   función nueva al final de program.
 * Efectos:
 *   reserva y enlaza; sin memoria termina.
 * Restricciones:
 *   no posee symbol; el primer bloque será entrada (Aho §8.4.1).
 */
TacFunction *tac_function_new(TacProgram *program, const Symbol *symbol) {
    TacFunction *function = tac_allocate(sizeof *function);
    function->symbol = symbol;
    function->program = program;
    if (program->last) {
        program->last->next = function;
    } else {
        program->first = function;
    }
    program->last = function;
    return function;
}

/*
 * tac_program_free — libera propietarios, conservando todas las referencias prestadas.
 *
 * Entrada:
 *   program: programa propietario o NULL.
 * Salida:
 *   ninguna.
 * Efectos:
 *   libera funciones, bloques, instrucciones y vectores de aristas.
 * Restricciones:
 *   conserva símbolos, ámbito global y filas ISA prestados.
 */
void tac_program_free(TacProgram *program) {
    if (!program) {
        return;
    }
    TacFunction *function = program->first;
    while (function) {
        TacFunction *next_function = function->next;
        TacBlock *block = function->first;
        while (block) {
            TacBlock *next_block = block->next;
            TacInstruction *instruction = block->first;
            while (instruction) {
                TacInstruction *next_instruction = instruction->next;
                free(instruction);
                instruction = next_instruction;
            }
            free(block->successors);
            free(block->predecessors);
            free(block);
            block = next_block;
        }
        free(function);
        function = next_function;
    }
    free(program);
}
