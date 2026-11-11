/*
 * tac_lower_value.c — selección de valores, direcciones y operaciones pendientes.
 * Usa: tac_lower_internal.h e isa_constant.h; no usa GenCtx ni emite bundles.
 * Referencias: Aho §8.1–8.2 y §8.6; memoria en bytes, como HIGH.
 */
#include "tac_lower_internal.h"
#include "isa_constant.h"

/*
 * tl_immediate — reconoce el rango exacto de un inmediato con signo de 18 bits.
 * Entrada:
 *   valor entero.
 * Salida:
 *   1 si cabe; 0 si no.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   no trunca ni normaliza el valor.
 */
int tl_immediate(int64_t value) {
    return value >= -131072 && value <= 131071;
}

/*
 * tl_constant — materializa el plan compartido en registros virtuales.
 * Entrada:
 *   lower, destino TEMP y entero aceptable por isa_constant_plan.
 * Salida:
 *   ninguna; marca fallo si la constante está fuera de 32 bits.
 * Efectos:
 *   agrega MOV/SLL/SADDI con la posición fuente actual.
 * Restricciones:
 *   DEST es dest; SCRATCH es otro TEMP, sin límite físico; Aho §8.2.
 */
void tl_constant(TacLower *lower, TacOperand dest, int64_t value) {
    IsaConstantPlan plan;
    if (!isa_constant_plan(value, &plan)) {
        tl_fail(lower, "constante fuera de 32 bits");
        return;
    }
    TacOperand roles[2] = {dest, tac_none()};
    if (plan.scratch_count) {
        roles[ISA_CONST_SCRATCH] = tac_temp_new(lower->function);
    }
    for (unsigned k = 0; k < plan.count; k++) {
        const IsaConstantStep *step = &plan.steps[k];
        switch (step->op) {
            case ISA_CONST_MOV:
                tl_emit(lower, "mov", roles[step->target], tac_none(),
                        tac_integer(step->immediate));
                break;
            case ISA_CONST_SLL:
                tl_emit(lower, "sll", dest, dest, roles[ISA_CONST_SCRATCH]);
                break;
            case ISA_CONST_SADDI:
                tl_emit(lower, "saddi", dest, dest, tac_integer(step->immediate));
                break;
        }
    }
}

/*
 * symbol_address — consulta una dirección estática ya asignada.
 * Entrada:
 *   lower y símbolo semántico.
 * Salida:
 *   dirección o cero tras marcar fallo.
 * Efectos:
 *   diagnóstico ante dirección ausente.
 * Restricciones:
 *   no asigna casillas ni limita a desplazamientos de 18 bits.
 */
static int symbol_address(TacLower *lower, const Symbol *symbol) {
    if (!symbol || symbol->address < 0) {
        tl_fail(lower, "símbolo sin dirección estática");
        return 0;
    }
    return symbol->address;
}

/*
 * address — selecciona base y desplazamiento, materializando lo que no cabe.
 * Entrada:
 *   base VARIABLE (dirección) o TEMP (valor), offset en bytes.
 * Salida:
 *   base física rz o TEMP; escribe displacement.
 * Efectos:
 *   agrega cálculo de dirección; nunca lee el contenido de una base simbólica.
 * Restricciones:
 *   solo combina dirección simbólica y desplazamiento inmediato; Aho §8.2.
 */
static TacOperand address(TacLower *lower, TacOperand base, TacOperand offset,
                           int64_t *displacement) {
    if (base.kind == TAC_VARIABLE) {
        int location = symbol_address(lower, base.value.symbol);
        if (offset.kind == TAC_INTEGER && tl_immediate(offset.value.integer)) {
            int64_t combined = (int64_t)location + offset.value.integer;
            if (tl_immediate(combined)) {
                *displacement = combined;
                return tac_physical(TAC_P1_RZ);
            }
        }
        base = tl_value(lower, tac_integer(location), TL_SOURCE_FIXED_ALLOWED);
    }
    if (offset.kind == TAC_INTEGER && tl_immediate(offset.value.integer)) {
        *displacement = offset.value.integer;
        return base;
    }
    TacOperand value = tl_value(lower, offset, TL_SOURCE_FIXED_ALLOWED);
    TacOperand result = tac_temp_new(lower->function);
    tl_emit(lower, "add", result, base, value);
    *displacement = 0;
    return result;
}

/*
 * memory — construye LW/SW con base y desplazamiento normalizados.
 * Entrada:
 *   lower, mnemónico lw/sw, destino, base, offset y dato.
 * Salida:
 *   ninguna.
 * Efectos:
 *   selecciona dirección y agrega la operación LSU.
 * Restricciones:
 *   dest es NONE al guardar; value es NONE al cargar; Aho §8.6.
 */
static void memory(TacLower *lower, const char *name, TacOperand dest,
                    TacOperand base, TacOperand offset, TacOperand value) {
    int64_t displacement;
    TacOperand selected = address(lower, base, offset, &displacement);
    TacInstruction *instruction = tl_emit(lower, name, dest, selected, value);
    if (instruction) {
        instruction->isa.displacement = displacement;
    }
}

/*
 * tl_load_symbol — lee el valor de una casilla estática.
 * Entrada:
 *   lower, destino TEMP y símbolo anotado.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega LW y cálculo de dirección si hace falta.
 * Restricciones:
 *   para parámetros arreglo carga su dirección recibida, no sus elementos.
 */
void tl_load_symbol(TacLower *lower, TacOperand dest, const Symbol *symbol) {
    memory(lower, "lw", dest, tac_variable(symbol), tac_integer(0), tac_none());
}

/*
 * tl_store_symbol — escribe un valor en su casilla estática.
 * Entrada:
 *   lower, símbolo anotado y valor materializado.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega SW y cálculo de dirección si hace falta.
 * Restricciones:
 *   no modifica Symbol.address ni crea variables; Aho §8.6.
 */
void tl_store_symbol(TacLower *lower, const Symbol *symbol, TacOperand value) {
    memory(lower, "sw", tac_none(), tac_variable(symbol), tac_integer(0), value);
}

/*
 * tl_value — obtiene un registro fuente para un valor HIGH.
 * Entrada:
 *   TEMP, INTEGER o VARIABLE válida y requisito de registro del consumidor.
 * Salida:
 *   TEMP, o PHYS rz para INTEGER exactamente cero si la política lo admite.
 * Efectos:
 *   materializa constantes o carga casillas; conserva el ID de un TEMP.
 *   El cero admitido como rz no reserva un temporal.
 * Restricciones:
 *   no propaga constantes de TEMP ni VARIABLE. Las plantillas aritméticas P1
 *   requieren fuentes asignables a registros 4–15; Aho §8.1–8.2.
 */
TacOperand tl_value(TacLower *lower, TacOperand operand, TacLowerSourcePolicy policy) {
    if (operand.kind == TAC_TEMP) {
        return operand;
    }
    if (policy == TL_SOURCE_FIXED_ALLOWED && operand.kind == TAC_INTEGER &&
        operand.value.integer == 0) {
        return tac_physical(TAC_P1_RZ);
    }
    TacOperand result = tac_temp_new(lower->function);
    if (operand.kind == TAC_INTEGER) {
        tl_constant(lower, result, operand.value.integer);
    } else if (operand.kind == TAC_VARIABLE) {
        tl_load_symbol(lower, result, operand.value.symbol);
    } else {
        tl_fail(lower, "operando HIGH no es un valor");
    }
    return result;
}

/*
 * destination — reserva resultado si HIGH escribe una VARIABLE.
 * Entrada:
 *   destino HIGH válido.
 * Salida:
 *   TEMP de resultado.
 * Efectos:
 *   reserva una identidad si corresponde.
 * Restricciones:
 *   la escritura a memoria se realiza después de calcular el resultado.
 */
static TacOperand destination(TacLower *lower) {
    TacOperand dest = lower->source->dest;
    return dest.kind == TAC_TEMP ? dest : tac_temp_new(lower->function);
}

/*
 * finish — transfiere a memoria un resultado cuyo destino es VARIABLE.
 * Entrada:
 *   lower con fuente y valor calculado.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega SW solo cuando HIGH define una casilla.
 * Restricciones:
 *   no elimina operaciones ni escrituras; Aho §8.6.
 */
static void finish(TacLower *lower, TacOperand result) {
    if (lower->source->dest.kind == TAC_VARIABLE) {
        tl_store_symbol(lower, lower->source->dest.value.symbol, result);
    }
}

/*
 * copy — selecciona copia, lectura de casilla o materialización directa.
 * Entrada:
 *   lower con COPY válido.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega instrucciones reales y la escritura de resultado necesaria.
 * Restricciones:
 *   COPY de parámetro arreglo copia el puntero; Aho §8.2.
 */
static void copy(TacLower *lower) {
    const TacInstruction *source = lower->source;
    if (source->dest.kind == TAC_VARIABLE) {
        finish(lower, tl_value(lower, source->src1, TL_SOURCE_FIXED_ALLOWED));
        return;
    }
    if (source->src1.kind == TAC_INTEGER) {
        tl_constant(lower, source->dest, source->src1.value.integer);
    } else if (source->src1.kind == TAC_VARIABLE) {
        tl_load_symbol(lower, source->dest, source->src1.value.symbol);
    } else {
        tl_emit(lower, "add", source->dest, source->src1, tac_physical(TAC_P1_RZ));
    }
}

/*
 * binary — selecciona ALU sin plegar operaciones ni reducir su fuerza.
 * Entrada:
 *   lower con operación HIGH.
 * Salida:
 *   1 si pertenece a la tabla; 0 si no.
 * Efectos:
 *   materializa fuentes, agrega ALU y escribe el resultado.
 * Restricciones:
 *   ADD admite inmediato derecho; GT/GE intercambian fuentes; Aho §8.2.
 */
static int binary(TacLower *lower) {
    static const struct {
        TacOpcode op;
        const char *name;
        int swap;
    } selection[] = {
        {TAC_ADD, "add", 0}, {TAC_SUB, "sub", 0},
        {TAC_AND, "and", 0}, {TAC_OR, "or", 0}, {TAC_XOR, "xor", 0},
        {TAC_EQ, "seq", 0}, {TAC_NE, "sne", 0},
        {TAC_LT, "slt", 0}, {TAC_LE, "sle", 0},
        {TAC_GT, "slt", 1}, {TAC_GE, "sle", 1},
    };
    for (size_t k = 0; k < sizeof selection / sizeof selection[0]; k++) {
        if (lower->source->op != selection[k].op) {
            continue;
        }
        TacOperand left = tl_value(lower, lower->source->src1, TL_SOURCE_FIXED_ALLOWED);
        TacOperand result = destination(lower);
        if (selection[k].op == TAC_ADD && lower->source->src2.kind == TAC_INTEGER &&
            tl_immediate(lower->source->src2.value.integer)) {
            tl_emit(lower, "saddi", result, left, lower->source->src2);
        } else {
            TacOperand right = tl_value(lower, lower->source->src2, TL_SOURCE_FIXED_ALLOWED);
            tl_emit(lower, selection[k].name, result,
                    selection[k].swap ? right : left, selection[k].swap ? left : right);
        }
        finish(lower, result);
        return 1;
    }
    return 0;
}

/*
 * pseudo — conserva una operación compleja como una sola instrucción de destino.
 * Entrada:
 *   lower con operación HIGH.
 * Salida:
 *   1 si la selecciona; 0 si no.
 * Efectos:
 *   materializa operandos, copia selectores y agrega TAC_PSEUDO.
 * Restricciones:
 *   sin auxiliares físicos, expansión ni conservación de vivos; Aho §8.2.
 */
static int pseudo(TacLower *lower) {
    static const struct {
        TacOpcode high;
        TacPseudoKind kind;
    } selection[] = {
        {TAC_MUL, TAC_PSEUDO_MUL}, {TAC_DIV, TAC_PSEUDO_DIV},
        {TAC_MOD, TAC_PSEUDO_MOD}, {TAC_POW, TAC_PSEUDO_POW},
        {TAC_AUTH, TAC_PSEUDO_AUTH}, {TAC_LOGOUT, TAC_PSEUDO_LOGOUT},
        {TAC_KV_WRITE, TAC_PSEUDO_KV_WRITE}, {TAC_KV_LOCK, TAC_PSEUDO_KV_LOCK},
        {TAC_SANC, TAC_PSEUDO_SANC}, {TAC_CIFRAR, TAC_PSEUDO_CIFRAR},
        {TAC_DESCIFRAR, TAC_PSEUDO_DESCIFRAR},
    };
    for (size_t k = 0; k < sizeof selection / sizeof selection[0]; k++) {
        if (lower->source->op != selection[k].high) {
            continue;
        }
        const TacOpInfo *info = tac_op_info(lower->source->op);
        TacInstruction target = {.op = TAC_PSEUDO, .pseudo = {.kind = selection[k].kind}};
        TacOpInfo target_info;
        if (!tac_instruction_info(&target, &target_info)) {
            tl_fail(lower, "propiedades PSEUDO no disponibles");
            return 1;
        }
        TacLowerSourcePolicy policy = target_info.scratch_count ?
            TL_SOURCE_ASSIGNABLE : TL_SOURCE_FIXED_ALLOWED;
        TacOperand high[] = {lower->source->dest, lower->source->src1, lower->source->src2};
        TacOperand operands[3] = {tac_none(), tac_none(), tac_none()};
        for (unsigned i = 0; i < 3; i++) {
            if (info->reads & (1u << i)) {
                operands[i] = tl_value(lower, high[i], policy);
            }
        }
        if (info->writes & TAC_DEST) {
            operands[0] = destination(lower);
        }
        if (!lower->failed) {
            TacInstruction *instruction = tac_append_pseudo(lower->block, selection[k].kind,
                operands[0], operands[1], operands[2], lower->source->selectors, 0);
            tl_location(lower, instruction);
        }
        if (info->writes & TAC_DEST) {
            finish(lower, operands[0]);
        }
        return 1;
    }
    return 0;
}

/*
 * tl_lower_value — selecciona operaciones de valor, memoria y CRYPTO.
 * Entrada:
 *   lower con instrucción HIGH válida.
 * Salida:
 *   1 si la maneja; 0 para control.
 * Efectos:
 *   agrega instrucciones en el bloque actual y conserva posición.
 * Restricciones:
 *   la copia de arreglos ya llega como lazo HIGH; Aho §8.1–8.2 y §8.6.
 */
int tl_lower_value(TacLower *lower) {
    const TacInstruction *source = lower->source;
    TacOperand result;
    TacOperand value;
    switch (source->op) {
        case TAC_COPY:
            copy(lower);
            return 1;
        case TAC_ADDRESS:
            result = destination(lower);
            tl_constant(lower, result, symbol_address(lower, source->src1.value.symbol));
            finish(lower, result);
            return 1;
        case TAC_LOAD:
            result = destination(lower);
            memory(lower, "lw", result, source->src1, source->src2, tac_none());
            finish(lower, result);
            return 1;
        case TAC_STORE:
            value = tl_value(lower, source->src2, TL_SOURCE_FIXED_ALLOWED);
            memory(lower, "sw", tac_none(), source->dest, source->src1, value);
            return 1;
        case TAC_NEG:
        case TAC_NOT:
            value = tl_value(lower, source->src1, TL_SOURCE_FIXED_ALLOWED);
            result = destination(lower);
            tl_emit(lower, source->op == TAC_NEG ? "sub" : "seq", result,
                    source->op == TAC_NEG ? tac_physical(TAC_P1_RZ) : value,
                    source->op == TAC_NEG ? value : tac_physical(TAC_P1_RZ));
            finish(lower, result);
            return 1;
        default:
            return binary(lower) || pseudo(lower);
    }
}
