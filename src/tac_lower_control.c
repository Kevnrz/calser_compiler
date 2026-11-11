/*
 * tac_lower_control.c — saltos y convención de casillas de parámetros/retorno.
 * Usa: tac_lower_internal.h y anotaciones AST prestadas por Symbol.
 * No guarda vivos ni emite código; Aho §7.2, §8.2, §8.4.1 y §8.6.
 */
#include "tac_lower_internal.h"
#include "ast.h"
#include <string.h>

/*
 * target — traduce un objetivo de bloque por identidad.
 * Entrada:
 *   lower con mapa completo y bloque HIGH propio.
 * Salida:
 *   LABEL del bloque de salida o NONE tras marcar fallo.
 * Efectos:
 *   ninguno salvo el diagnóstico si falta correspondencia.
 * Restricciones:
 *   no usa IDs internos ni números de presentación; Aho §8.4.1.
 */
static TacOperand target(TacLower *lower, const TacBlock *high) {
    for (size_t k = 0; k < lower->block_count; k++) {
        if (lower->blocks[k].high == high) {
            return tac_label(lower->blocks[k].lowered);
        }
    }
    tl_fail(lower, "salto sin bloque de destino");
    return tac_none();
}

/*
 * jump — agrega un salto directo al bloque correspondiente.
 * Entrada:
 *   lower con GOTO válido.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega JEF con objetivo LABEL y posición de origen.
 * Restricciones:
 *   nunca entrega texto Bn al organizador; Aho §8.6.
 */
static void jump(TacLower *lower) {
    TacOperand label = target(lower, lower->source->dest.value.block);
    TacInstruction *instruction = tl_emit(lower, "jef", tac_none(), tac_none(), tac_none());
    if (instruction) {
        instruction->isa.target = label;
    }
}

/*
 * conditional — selecciona comparaciones de salto conservando tomada/caída.
 * Entrada:
 *   lower con instrucción HIGH.
 * Salida:
 *   1 si es IF; 0 si no.
 * Efectos:
 *   materializa fuentes y agrega BRANCH con LABEL.
 * Restricciones:
 *   LE/GT intercambian fuentes; no intercambian aristas; Aho §8.2.
 */
static int conditional(TacLower *lower) {
    static const struct {
        TacOpcode op;
        const char *name;
        int swap;
    } selection[] = {
        {TAC_IF_EQ, "beq", 0}, {TAC_IF_NE, "bne", 0},
        {TAC_IF_LT, "blt", 0}, {TAC_IF_LE, "bge", 1},
        {TAC_IF_GT, "blt", 1}, {TAC_IF_GE, "bge", 0},
    };
    for (size_t k = 0; k < sizeof selection / sizeof selection[0]; k++) {
        if (lower->source->op != selection[k].op) {
            continue;
        }
        TacOperand left = tl_value(lower, lower->source->src1, TL_SOURCE_FIXED_ALLOWED);
        TacOperand right = tl_value(lower, lower->source->src2, TL_SOURCE_FIXED_ALLOWED);
        TacOperand label = target(lower, lower->source->dest.value.block);
        TacInstruction *instruction = tl_emit(lower, selection[k].name, tac_none(),
            selection[k].swap ? right : left, selection[k].swap ? left : right);
        if (instruction) {
            instruction->isa.target = label;
        }
        return 1;
    }
    return 0;
}

/*
 * callee — comprueba los datos semánticos que usa el grupo PARAM/CALL.
 * Entrada:
 *   lower y CALL de forma válida.
 * Salida:
 *   símbolo del callee o NULL.
 * Efectos:
 *   diagnóstico si la firma no corresponde a la aridad o faltan parámetros.
 * Restricciones:
 *   no resuelve nombres ni construye firmas nuevas; Aho §7.2.
 */
static const Symbol *callee(TacLower *lower, const TacInstruction *call) {
    const Symbol *symbol = call->src1.value.symbol;
    if (symbol->parameter_count < 0 || symbol->parameter_count != call->src2.value.integer ||
        (symbol->parameter_count && !symbol->param_list)) {
        tl_fail(lower, "CALL sin firma semántica compatible");
        return NULL;
    }
    for (int k = 0; k < symbol->parameter_count; k++) {
        if (!symbol->param_list[k]) {
            tl_fail(lower, "CALL sin símbolo de parámetro");
            return NULL;
        }
    }
    return symbol;
}

/*
 * parameter — escribe el valor en la casilla indicada por posición y callee.
 * Entrada:
 *   lower con PARAM de grupo validado.
 * Salida:
 *   ninguna.
 * Efectos:
 *   materializa el valor y agrega SW a la casilla del callee.
 * Restricciones:
 *   no supone PARAM ordenados físicamente; Aho §7.2 y §8.6.
 */
static void parameter(TacLower *lower) {
    const TacInstruction *call = lower->source->next;
    while (call && call->op != TAC_CALL) {
        call = call->next;
    }
    if (!call) {
        tl_fail(lower, "PARAM sin CALL");
        return;
    }
    const Symbol *symbol = callee(lower, call);
    if (symbol) {
        size_t position = (size_t)lower->source->src2.value.integer;
        TacOperand value = tl_value(lower, lower->source->src1, TL_SOURCE_FIXED_ALLOWED);
        tl_store_symbol(lower, symbol->param_list[position], value);
    }
}

/*
 * return_symbol — consulta el resultado anotado en la declaración del callee.
 * Entrada:
 *   símbolo FUNCTION semántico vivo.
 * Salida:
 *   símbolo de retorno o NULL.
 * Efectos:
 *   ninguno; no altera el AST ni vuelve a buscar nombres.
 * Restricciones:
 *   el productor conserva decl_node y sus anotaciones; Aho §7.2.
 */
static const Symbol *return_symbol(const Symbol *function) {
    ASTNode *declaration = function->decl_node;
    ASTNode *type = ast_find_child(declaration, AST_RETURN_TYPE);
    ASTNode *identifier = ast_find_child(type, AST_IDENTIFIER);
    return identifier ? identifier->symbol : NULL;
}

/*
 * call — selecciona JAS y lee inmediatamente el resultado estático solicitado.
 * Entrada:
 *   lower con CALL válido.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega llamada y LW/COPY del retorno, sin modificar HIGH.
 * Restricciones:
 *   no conserva vivos ni rl; esa secuencia pertenece a commits siguientes.
 */
static void call(TacLower *lower) {
    const Symbol *symbol = callee(lower, lower->source);
    if (!symbol) {
        return;
    }
    const Symbol *result = NULL;
    if (lower->source->dest.kind != TAC_NONE) {
        result = return_symbol(symbol);
        if (symbol->type == TYPE_VOID || !result) {
            tl_fail(lower, "CALL sin casilla de retorno");
            return;
        }
    }
    TacInstruction *instruction = tl_emit(lower, "jas", tac_physical(TAC_P1_RL),
                                           tac_none(), tac_none());
    if (instruction) {
        instruction->isa.target = tac_function_operand(symbol);
    }
    if (result) {
        TacOperand dest = lower->source->dest;
        TacOperand value = dest.kind == TAC_TEMP ? dest : tac_temp_new(lower->function);
        tl_load_symbol(lower, value, result);
        if (dest.kind == TAC_VARIABLE) {
            tl_store_symbol(lower, dest.value.symbol, value);
        }
    }
}

/*
 * return_value — selecciona retorno por rl o salida terminal de main.
 * Entrada:
 *   lower con RETURN y símbolo de función vivos.
 * Salida:
 *   ninguna.
 * Efectos:
 *   agrega EXIT o transferencia a casilla de resultado y JEFR.
 * Restricciones:
 *   no reinicializa el retorno ni implementa R como llamada; Aho §7.2.
 */
static void return_value(TacLower *lower) {
    if (!strcmp(lower->function->symbol->name, "main")) {
        TacInstruction *instruction = tac_append_pseudo(lower->block, TAC_PSEUDO_EXIT,
            tac_none(), tac_none(), tac_none(), (TacSelectors){-1, -1, -1}, 0);
        tl_location(lower, instruction);
        return;
    }
    TacOperand value = lower->source->src1;
    if (value.kind != TAC_NONE) {
        const Symbol *result = return_symbol(lower->function->symbol);
        if (!result) {
            tl_fail(lower, "RETURN sin casilla de retorno");
            return;
        }
        if (value.kind != TAC_VARIABLE || value.value.symbol != result) {
            tl_store_symbol(lower, result, tl_value(lower, value, TL_SOURCE_FIXED_ALLOWED));
        }
    }
    tl_emit(lower, "jefR", tac_physical(TAC_P1_RZ), tac_physical(TAC_P1_RL), tac_none());
}

/*
 * tl_lower_control — selecciona transferencias y protocolo de casillas.
 * Entrada:
 *   lower con instrucción HIGH válida.
 * Salida:
 *   1 si la maneja; 0 si no.
 * Efectos:
 *   agrega instrucciones manteniendo orden de argumentos y aristas.
 * Restricciones:
 *   sin pila ni conservación de vivos; Aho §7.2, §8.2 y §8.6.
 */
int tl_lower_control(TacLower *lower) {
    switch (lower->source->op) {
        case TAC_GOTO:
            jump(lower);
            return 1;
        case TAC_PARAM:
            parameter(lower);
            return 1;
        case TAC_CALL:
            call(lower);
            return 1;
        case TAC_RETURN:
            return_value(lower);
            return 1;
        default:
            return conditional(lower);
    }
}
