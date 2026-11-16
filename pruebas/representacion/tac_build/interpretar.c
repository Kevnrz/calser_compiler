/*
 * interpretar.c — ejecución de TAC estructurado, exclusivamente para pruebas.
 * Usa: interpretar.h y biblioteca C. No ejecuta texto ni llama al generador.
 * Referencias: Aho §6.4 (valores), §6.4.3 (direcciones), §6.6 (flujo), §7.2.
 * Los símbolos viven en memoria estática; solo temporales y PARAM son por llamada.
 */
#include "interpretar.h"
#include <stdlib.h>
#include <string.h>

enum { TEST_MEMORY_SIZE = 65536 };

typedef struct PendingArgument {
    int64_t position;
    uint32_t value;
    struct PendingArgument *next;
} PendingArgument;

typedef struct Invocation {
    const TacFunction *function;
    const TacBlock *block;
    const TacInstruction *instruction;
    uint32_t *temporaries;
    unsigned char *defined;
    PendingArgument *arguments;
    TacOperand return_destination;
    struct Invocation *caller;
} Invocation;

typedef struct {
    const TacProgram *program;
    TacRunResult result;
    Invocation *current;
    const TacInstruction *executing;
} Machine;

/*
 * stop — conserva el primer fallo con su posición de origen.
 * Entrada: máquina viva, estado no normal y causa estática.
 * Salida: ninguna. Efectos: fija estado y diagnóstico.
 * Restricciones: no destruye las activaciones; el bucle principal las libera.
 */
static void stop(Machine *machine, TacRunStatus status, const char *message) {
    if (machine->result.status == TAC_RUN_OK) {
        machine->result.status = status;
        snprintf(machine->result.diagnostic, sizeof machine->result.diagnostic,
                 "linea %d: %s", machine->executing ? machine->executing->source_line : 0,
                 message);
    }
}

/*
 * allocate — reserva memoria comprobando el producto del tamaño.
 * Entrada: máquina, cantidad y ancho. Salida: bloque inicializado o NULL.
 * Efectos: informa NO_MEMORY ante desborde o agotamiento.
 * Restricciones: las reservas de cero elementos siguen siendo liberables.
 */
static void *allocate(Machine *machine, size_t count, size_t width) {
    if (width && count > SIZE_MAX / width) {
        stop(machine, TAC_RUN_NO_MEMORY, "tamano de reserva desbordado");
        return NULL;
    }
    void *memory = calloc(count ? count : 1, width);
    if (!memory) {
        stop(machine, TAC_RUN_NO_MEMORY, "sin memoria para interpretar");
    }
    return memory;
}

/* tac_run_read_word — véase el contrato en interpretar.h. */
int tac_run_read_word(const TacRunResult *result, uint32_t address, uint32_t *out) {
    if (!result || !result->memory || !out || result->memory_size < 4 ||
        (uint64_t)address > result->memory_size - 4) {
        return 0;
    }
    uint32_t value = 0;
    for (unsigned byte = 0; byte < 4; byte++) {
        value |= (uint32_t)result->memory[address + byte] << (8 * byte);
    }
    *out = value;
    return 1;
}

/*
 * load_word — lee una palabra desde la memoria compartida, sin límites de arreglo.
 * Entrada: máquina y dirección en bytes. Salida: palabra o cero al fallar.
 * Efectos: INVALID si el acceso sale del espacio de memoria del intérprete.
 * Restricciones: cero tras error no debe consumirse como resultado normal.
 */
static uint32_t load_word(Machine *machine, uint32_t address) {
    uint32_t value = 0;
    if (!tac_run_read_word(&machine->result, address, &value)) {
        stop(machine, TAC_RUN_INVALID, "lectura fuera de memoria");
    }
    return value;
}

/*
 * store_word — escribe cuatro bytes little endian en la memoria compartida.
 * Entrada: máquina, dirección en bytes y palabra. Salida: ninguna.
 * Efectos: escribe memoria o informa INVALID, sin escrituras parciales.
 * Restricciones: no añade comprobaciones de dimensiones que el generador no hace.
 */
static void store_word(Machine *machine, uint32_t address, uint32_t value) {
    if (!machine->result.memory || machine->result.memory_size < 4 ||
        (uint64_t)address > machine->result.memory_size - 4) {
        stop(machine, TAC_RUN_INVALID, "escritura fuera de memoria");
        return;
    }
    for (unsigned byte = 0; byte < 4; byte++) {
        machine->result.memory[address + byte] = (unsigned char)(value >> (8 * byte));
    }
}

/*
 * read_value — obtiene literales, temporales o el contenido de casillas Symbol.
 * Entrada: máquina, invocación y operando de valor. Salida: palabra de 32 bits.
 * Efectos: puede informar INVALID por temporal no definido u operando incorrecto.
 * Restricciones: VARIABLE arreglo parámetro lee su dirección recibida, no su base.
 */
static uint32_t read_value(Machine *machine, Invocation *frame, TacOperand operand) {
    switch (operand.kind) {
        case TAC_INTEGER:
            return (uint32_t)operand.value.integer;
        case TAC_VARIABLE:
            return load_word(machine, (uint32_t)operand.value.symbol->address);
        case TAC_TEMP:
            if (operand.value.id < frame->function->next_temp &&
                frame->defined[operand.value.id]) {
                return frame->temporaries[operand.value.id];
            }
            stop(machine, TAC_RUN_INVALID, "lectura de temporal no definido");
            return 0;
        default:
            stop(machine, TAC_RUN_INVALID, "operando no es un valor alto nivel");
            return 0;
    }
}

/*
 * write_value — define temporal o casilla estática, incluidos parámetros arreglo.
 * Entrada: máquina, invocación, destino y palabra. Salida: ninguna.
 * Efectos: escribe solo mientras el estado siga normal.
 * Restricciones: no crea variables ni áreas de función por invocación.
 */
static void write_value(Machine *machine, Invocation *frame,
                         TacOperand operand, uint32_t value) {
    if (machine->result.status != TAC_RUN_OK) {
        return;
    }
    if (operand.kind == TAC_VARIABLE) {
        store_word(machine, (uint32_t)operand.value.symbol->address, value);
    } else if (operand.kind == TAC_TEMP && operand.value.id < frame->function->next_temp) {
        frame->temporaries[operand.value.id] = value;
        frame->defined[operand.value.id] = 1;
    } else {
        stop(machine, TAC_RUN_INVALID, "destino no escribible");
    }
}

/*
 * signed_word — interpreta el patrón de 32 bits sin conversión signed ambigua.
 * Entrada: palabra. Salida: entero entre INT32_MIN e INT32_MAX en int64_t.
 * Efectos: ninguno. Restricciones: usarlo para comparaciones, DIV/MOD y exponente.
 */
static int64_t signed_word(uint32_t value) {
    return value <= INT32_MAX ? (int64_t)value : (int64_t)value - INT64_C(4294967296);
}

/*
 * binary — aritmética modular; DIV trunca hacia cero y MOD conserva signo izquierdo.
 * Entrada: máquina, operación y palabras. Salida: palabra o cero al fallar.
 * Efectos: informa error terminal para división por cero o exponente negativo.
 * Restricciones: INT32_MIN / -1 se calcula en 64 bits y se trunca sin UB.
 */
static uint32_t binary(Machine *machine, TacOpcode op, uint32_t a, uint32_t b) {
    int64_t x = signed_word(a);
    int64_t y = signed_word(b);
    switch (op) {
        case TAC_ADD: return a + b;
        case TAC_SUB: return a - b;
        case TAC_MUL: return (uint32_t)((uint64_t)a * b);
        case TAC_AND: return a & b;
        case TAC_OR: return a | b;
        case TAC_XOR: return a ^ b;
        case TAC_EQ: return a == b;
        case TAC_NE: return a != b;
        case TAC_LT: return x < y;
        case TAC_LE: return x <= y;
        case TAC_GT: return x > y;
        case TAC_GE: return x >= y;
        case TAC_DIV:
        case TAC_MOD:
            if (!b) {
                stop(machine, TAC_RUN_DIVISION_BY_ZERO, "division o modulo por cero");
                return 0;
            }
            return (uint32_t)(op == TAC_DIV ? x / y : x % y);
        case TAC_POW: {
            if (y < 0) {
                stop(machine, TAC_RUN_NEGATIVE_EXPONENT, "exponente negativo");
                return 0;
            }
            uint32_t value = 1;
            while (b) {
                if (b & 1) {
                    value = (uint32_t)((uint64_t)value * a);
                }
                a = (uint32_t)((uint64_t)a * a);
                b >>= 1;
            }
            return value;
        }
        default:
            stop(machine, TAC_RUN_INVALID, "operacion binaria desconocida");
            return 0;
    }
}

/*
 * release_arguments — libera capturas PARAM de una activación.
 * Entrada: frame vivo. Salida: ninguna.
 * Efectos: vacía su lista sin modificar parámetros del programa.
 * Restricciones: cada argumento pertenece exclusivamente a esta lista.
 */
static void release_arguments(Invocation *frame) {
    PendingArgument *argument = frame->arguments;
    while (argument) {
        PendingArgument *next = argument->next;
        free(argument);
        argument = next;
    }
    frame->arguments = NULL;
}

/*
 * release_frame — destruye temporales y PARAM de una invocación.
 * Entrada: frame propio. Salida: ninguna.
 * Efectos: libera frame; no toca memoria estática ni el caller.
 * Restricciones: guardar caller antes de liberar.
 */
static void release_frame(Invocation *frame) {
    release_arguments(frame);
    free(frame->temporaries);
    free(frame->defined);
    free(frame);
}

/*
 * enter — crea solo temporales por llamada y comienza en entry.
 * Entrada: máquina y función prestada. Salida: frame propio o NULL.
 * Efectos: reserva arrays; informa NO_MEMORY si falla.
 * Restricciones: el prólogo TAC, no el intérprete, inicializa el retorno a cero.
 */
static Invocation *enter(Machine *machine, const TacFunction *function) {
    Invocation *frame = allocate(machine, 1, sizeof *frame);
    if (!frame) {
        return NULL;
    }
    frame->function = function;
    frame->block = function->entry;
    frame->instruction = function->entry->first;
    frame->temporaries = allocate(machine, function->next_temp, sizeof *frame->temporaries);
    frame->defined = allocate(machine, function->next_temp, sizeof *frame->defined);
    if (!frame->temporaries || !frame->defined) {
        release_frame(frame);
        return NULL;
    }
    return frame;
}

/*
 * find_function — resuelve funciones por identidad del Symbol, no por texto.
 * Entrada: programa y símbolo prestados. Salida: función o NULL.
 * Efectos: ninguno. Restricciones: listas válidas y acíclicas.
 */
static const TacFunction *find_function(const TacProgram *program, const Symbol *symbol) {
    for (const TacFunction *function = program->first; function; function = function->next) {
        if (function->symbol == symbol) {
            return function;
        }
    }
    return NULL;
}

/*
 * call — transfiere PARAM capturados a casillas estáticas y apila temporales.
 * Entrada: máquina, caller y CALL válido. Salida: ninguna.
 * Efectos: escribe parámetros del callee, crea una activación e informa errores.
 * Restricciones: el PC del caller ya apunta después del CALL; no usa recursión C.
 */
static void call(Machine *machine, Invocation *caller, const TacInstruction *instruction) {
    const Symbol *symbol = instruction->src1.value.symbol;
    const TacFunction *function = find_function(machine->program, symbol);
    if (!function || instruction->src2.value.integer != symbol->parameter_count) {
        stop(machine, TAC_RUN_INVALID, "CALL sin funcion o aridad semantica correcta");
        return;
    }
    for (PendingArgument *arg = caller->arguments; arg; arg = arg->next) {
        if (arg->position < 0 || arg->position >= symbol->parameter_count || !symbol->param_list) {
            stop(machine, TAC_RUN_INVALID, "PARAM fuera de la firma");
            return;
        }
        store_word(machine, (uint32_t)symbol->param_list[arg->position]->address, arg->value);
    }
    release_arguments(caller);
    if (machine->result.status != TAC_RUN_OK) {
        return;
    }
    Invocation *callee = enter(machine, function);
    if (callee) {
        callee->caller = caller;
        callee->return_destination = instruction->dest;
        machine->current = callee;
    }
}

/*
 * address_base — distingue dirección de arreglo declarado y valor TEMP.
 * Entrada: máquina, frame y base LOAD/STORE. Salida: dirección de 32 bits.
 * Efectos: puede informar INVALID al leer una base temporal inválida.
 * Restricciones: el validador ya rechazó un parámetro arreglo como base VARIABLE.
 */
static uint32_t address_base(Machine *machine, Invocation *frame, TacOperand base) {
    return base.kind == TAC_VARIABLE ? (uint32_t)base.value.symbol->address :
           read_value(machine, frame, base);
}

/*
 * jump — selecciona destino y primera instrucción de un bloque.
 * Entrada: frame y bloque prestados. Salida: ninguna.
 * Efectos: cambia PC; conserva temporales, incluso al volver por R.
 * Restricciones: destino de la misma función, validado por tac_validate.
 */
static void jump(Invocation *frame, const TacBlock *target) {
    frame->block = target;
    frame->instruction = target->first;
}

/*
 * execute — ejecuta una instrucción cuyo PC de continuación ya fue avanzado.
 * Entrada: máquina, frame e instrucción de alto nivel. Salida: ninguna.
 * Efectos: valores, memoria, control y pila de activaciones; informa primer fallo.
 * Restricciones: no consume el valor de una operación que haya terminado en error.
 */
static void execute(Machine *machine, Invocation *frame, const TacInstruction *instruction) {
    TacOperand dest = instruction->dest;
    TacOperand first = instruction->src1;
    TacOperand second = instruction->src2;
    uint32_t a;
    uint32_t b;
    switch (instruction->op) {
        case TAC_COPY:
            a = read_value(machine, frame, first);
            write_value(machine, frame, dest, a);
            break;
        case TAC_ADDRESS:
            write_value(machine, frame, dest, (uint32_t)first.value.symbol->address);
            break;
        case TAC_NEG:
        case TAC_NOT:
            a = read_value(machine, frame, first);
            write_value(machine, frame, dest, instruction->op == TAC_NEG ? 0u - a : !a);
            break;
        case TAC_LOAD:
            a = address_base(machine, frame, first);
            b = read_value(machine, frame, second);
            a = load_word(machine, a + b);
            write_value(machine, frame, dest, a);
            break;
        case TAC_STORE:
            a = address_base(machine, frame, dest);
            b = read_value(machine, frame, first);
            a += b;
            b = read_value(machine, frame, second);
            if (machine->result.status == TAC_RUN_OK) {
                store_word(machine, a, b);
            }
            break;
        case TAC_PARAM: {
            a = read_value(machine, frame, first);
            PendingArgument *argument = allocate(machine, 1, sizeof *argument);
            if (argument) {
                argument->position = second.value.integer;
                argument->value = a;
                argument->next = frame->arguments;
                frame->arguments = argument;
            }
            break;
        }
        case TAC_CALL:
            call(machine, frame, instruction);
            break;
        case TAC_RETURN: {
            a = first.kind == TAC_NONE ? 0 : read_value(machine, frame, first);
            Invocation *caller = frame->caller;
            if (caller && frame->return_destination.kind != TAC_NONE) {
                write_value(machine, caller, frame->return_destination, a);
            }
            machine->current = caller;
            release_frame(frame);
            break;
        }
        case TAC_GOTO:
            jump(frame, dest.value.block);
            break;
        case TAC_IF_EQ:
        case TAC_IF_NE:
        case TAC_IF_LT:
        case TAC_IF_LE:
        case TAC_IF_GT:
        case TAC_IF_GE: {
            static const TacOpcode comparisons[] = {
                TAC_EQ, TAC_NE, TAC_LT, TAC_LE, TAC_GT, TAC_GE
            };
            a = read_value(machine, frame, first);
            b = read_value(machine, frame, second);
            if (binary(machine, comparisons[instruction->op - TAC_IF_EQ], a, b)) {
                jump(frame, dest.value.block);
            } else {
                jump(frame, frame->block->next);
            }
            break;
        }
        default:
            a = read_value(machine, frame, first);
            b = read_value(machine, frame, second);
            a = binary(machine, instruction->op, a, b);
            write_value(machine, frame, dest, a);
            break;
    }
}

/*
 * preflight — valida forma HIGH y rechaza CRYPTO sin simularla ni ejecutarla.
 * Entrada: máquina y build prestados. Salida: función main o NULL.
 * Efectos: fija diagnóstico ante forma inválida, CRYPTO o main ausente.
 * Restricciones: un programa con CRYPTO se rechaza incluso si no la alcanzaría.
 */
static const TacFunction *preflight(Machine *machine, const TacBuildResult *build) {
    if (!build || !build->program || build->program->stage != TAC_STAGE_HIGH) {
        stop(machine, TAC_RUN_INVALID, "se requiere un programa de alto nivel");
        return NULL;
    }
    if (!tac_validate(build->program, machine->result.diagnostic,
                       sizeof machine->result.diagnostic)) {
        machine->result.status = TAC_RUN_INVALID;
        return NULL;
    }
    machine->program = build->program;
    const TacFunction *main_function = NULL;
    for (const TacFunction *function = machine->program->first; function;
         function = function->next) {
        if (!strcmp(function->symbol->name, "main")) {
            main_function = function;
        }
        for (const TacBlock *block = function->first; block; block = block->next) {
            for (const TacInstruction *instruction = block->first; instruction;
                 instruction = instruction->next) {
                if (instruction->op >= TAC_AUTH && instruction->op <= TAC_DESCIFRAR) {
                    machine->executing = instruction;
                    stop(machine, TAC_RUN_UNSUPPORTED_CRYPTO, "CRYPTO no soportado");
                    return NULL;
                }
            }
        }
    }
    if (!main_function || main_function->symbol->parameter_count) {
        stop(machine, TAC_RUN_INVALID, "se requiere main sin parametros");
        return NULL;
    }
    return main_function;
}

/* tac_interpret — véase el contrato en interpretar.h. */
TacRunResult tac_interpret(const TacBuildResult *build, uint64_t max_steps) {
    Machine machine = {0};
    const TacFunction *entry = preflight(&machine, build);
    if (!entry) {
        return machine.result;
    }
    machine.result.memory_size = TEST_MEMORY_SIZE;
    machine.result.memory = allocate(&machine, TEST_MEMORY_SIZE, 1);
    if (build->initial_data_size > TEST_MEMORY_SIZE || build->initial_data_size % 4 ||
        (build->initial_data_size && !build->initial_data)) {
        stop(&machine, TAC_RUN_INVALID, "imagen inicial invalida");
    }
    if (machine.result.status == TAC_RUN_OK) {
        for (size_t offset = 0; offset < build->initial_data_size; offset += 4) {
            store_word(&machine, (uint32_t)offset, build->initial_data[offset / 4]);
        }
        machine.current = enter(&machine, entry);
    }
    while (machine.current && machine.result.status == TAC_RUN_OK) {
        Invocation *frame = machine.current;
        if (!frame->instruction) {
            if (!frame->block->next) {
                stop(&machine, TAC_RUN_INVALID, "funcion sin retorno");
                break;
            }
            jump(frame, frame->block->next);
            continue;
        }
        machine.executing = frame->instruction;
        if (machine.result.steps >= max_steps) {
            stop(&machine, TAC_RUN_STEP_LIMIT, "limite de pasos alcanzado");
            break;
        }
        machine.result.steps++;
        const TacInstruction *instruction = frame->instruction;
        frame->instruction = instruction->next;
        execute(&machine, frame, instruction);
    }
    while (machine.current) {
        Invocation *caller = machine.current->caller;
        release_frame(machine.current);
        machine.current = caller;
    }
    return machine.result;
}

/* tac_run_status_name — véase el contrato en interpretar.h. */
const char *tac_run_status_name(TacRunStatus status) {
    switch (status) {
        case TAC_RUN_OK: return "ok";
        case TAC_RUN_DIVISION_BY_ZERO: return "division_by_zero";
        case TAC_RUN_NEGATIVE_EXPONENT: return "negative_exponent";
        case TAC_RUN_UNSUPPORTED_CRYPTO: return "unsupported_crypto";
        case TAC_RUN_STEP_LIMIT: return "step_limit";
        case TAC_RUN_NO_MEMORY: return "no_memory";
        default: return "invalid";
    }
}

/* tac_run_free — véase el contrato en interpretar.h. */
void tac_run_free(TacRunResult *result) {
    if (result) {
        free(result->memory);
        result->memory = NULL;
        result->memory_size = 0;
    }
}
