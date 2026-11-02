/*
 * codegen_instruction.c — Fase 4: descripción de instrucciones para dependencias.
 *
 * Obtiene unidades, slots, lecturas, escrituras y efectos de cada instrucción.
 * Las formas desconocidas se conservan como barreras del organizador.
 *
 * Usa: codegen.h, isa_table.h; biblioteca C.
 * Lo usa: codegen.c y pruebas de dependencias.
 * Referencias: isa.md §3, §4.2, §6, §9.
 */

#include <ctype.h>
#include <string.h>

#include "codegen.h"

/* Dependencias de isa.md §6. No resuelve destinos ni ensambla: una forma que
 * no se pueda describir queda como barrera, nunca como operación independiente.
 * Los mnemónicos, unidades y formas salen de la tabla única de isa_table.h,
 * la misma que usa el ensamblador. */

/* ==========================================================================
 * LECTURA DE OPERANDOS
 * ======================================================================= */

/*
 * bit — obtiene el bit de un registro ISA.
 *
 * Entrada:
 *   r  registro 0–15.
 * Salida:
 *   máscara uint16_t con un bit.
 *   Efectos: ninguno.
 * Restricciones:
 *   r ya validado.
 */
static uint16_t bit(int r) { return (uint16_t) (1u << r); }
/*
 * written — obtiene la máscara de un destino que puede escribirse.
 *
 * Entrada:
 *   r  registro 0–15.
 * Salida:
 *   cero para rz; bit de r para otros.
 *   Efectos: ninguno.
 * Restricciones:
 *   r ya validado.
 */
static uint16_t written(int r) { return r == 0 ? 0 : bit(r); }
/*
 * immediate — comprueba el rango de un inmediato.
 *
 * Entrada:
 *   p  texto no NULL.
 * Salida:
 *   1 si entero entre -131072 y 131071; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int immediate(const char *p) {
    long n;
    return isa_parse_integer(p, &n) && n >= -131072 && n <= 131071;
}
/*
 * selector — comprueba un selector de dos bits.
 *
 * Entrada:
 *   p  texto no NULL.
 * Salida:
 *   1 si entero entre 0 y 3; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int selector(const char *p) {
    long n;
    return isa_parse_integer(p, &n) && n >= 0 && n <= 3;
}
/*
 * target — describe un destino simbólico o relativo al PC.
 *
 * Entrada:
 *   p  texto no NULL.
 *   effects  puntero a efectos no NULL.
 * Salida:
 *   1 si entero o etiqueta válida; 0 si inválido.
 *   Efectos: agrega CG_EFFECT_PC_RELATIVE para enteros.
 * Restricciones:
 *   no resuelve etiquetas ni comprueba rango del destino numérico.
 */
static int target(const char *p, unsigned *effects) {
    long n;
    if (isa_parse_integer(p, &n)) { *effects |= CG_EFFECT_PC_RELATIVE; return 1; }
    if (!(isalpha((unsigned char) *p) || *p == '_' || *p == '.')) return 0;
    for (p++; *p; p++)
        if (!(isalnum((unsigned char) *p) || *p == '_' || *p == '.')) return 0;
    return 1;
}
/*
 * address — extrae el registro base de un operando de memoria.
 *
 * Entrada:
 *   p  operando modificable no NULL.
 * Salida:
 *   registro base 0–15 o -1 si inválido.
 *   Efectos: separa paréntesis con NUL.
 * Restricciones:
 *   inmediato de 18 bits con signo.
 */
static int address(char *p) {
    char *open = strchr(p, '('), *close = strchr(p, ')');
    if (!open || !close || close < open || *isa_trim(close + 1) != '\0') return -1;
    *open = '\0'; *close = '\0';
    if (!immediate(isa_trim(p))) return -1;
    return isa_parse_register(isa_trim(open + 1));
}

/* ==========================================================================
 * DESCRIPCIÓN DE INSTRUCCIONES
 * ======================================================================= */


/* cg_describe_instruction — véase el contrato en codegen.h. */
int cg_describe_instruction(const char *text, Slot slot, int source_line,
                            CGInstruction *out) {
    if (!out) return 1;
    *out = (CGInstruction) {.text = text, .unit = ISA_UNIT_UNKNOWN,
        .original_slot = slot, .source_line = source_line > 0 ? source_line : 0,
        .effects = CG_EFFECT_BARRIER};
    if (!text || slot < SLOT_ALU0 || slot > SLOT_LSU || strlen(text) >= 256)
        return 1;
    char buffer[256];
    strcpy(buffer, text);
    char *name = isa_trim(buffer), *rest = name;
    while (*rest && !isspace((unsigned char) *rest)) rest++;
    if (*rest) *rest++ = '\0';
    rest = isa_trim(rest);
    const IsaInstruction *desc = isa_find(name);
    if (!desc) return 1;
    CGInstruction ins = *out;
    ins.unit = desc->unit;
    ins.effects = 0;
    // Los slots de cada unidad salen de la tabla de la ISA; aquí solo los
    // efectos que el organizador debe respetar.
    ins.allowed_slots = isa_unit_slots(ins.unit);
    switch (ins.unit) {
        case ISA_UNIT_BRU: ins.effects = CG_EFFECT_CONTROL; break;
        case ISA_UNIT_CRYPTO:
            ins.effects = CG_EFFECT_CRYPTO_STATE | CG_EFFECT_BARRIER;
            break;
        default: break;
    }
    if (!(ins.allowed_slots & (1u << slot))) return 1;
    char *args[5];
    int count = 0;
    if (*rest) for (;;) {
        if (count == 5) return 1;
        char *comma = strchr(rest, ',');
        if (comma) *comma = '\0';
        args[count] = isa_trim(rest);
        if (!*args[count++]) return 1;
        if (!comma) break;
        rest = comma + 1;
    }
    if (count != isa_operand_count(desc->form)) return 1;
    int d, a, b;
    switch (desc->form) {
        case ISA_FORM_R3: case ISA_FORM_FEIS:
            d = isa_parse_register(args[0]); a = isa_parse_register(args[1]); b = isa_parse_register(args[2]);
            if (d < 0 || a < 0 || b < 0) return 1;
            ins.reads = bit(a) | bit(b); ins.writes = written(d);
            if (desc->form == ISA_FORM_FEIS) {
                if (!selector(args[3]) || !selector(args[4])) return 1;
                ins.effects |= CG_EFFECT_CONDITIONAL_WRITE;
            }
            if (strcmp(name, "ifmov") == 0) ins.effects |= CG_EFFECT_CONDITIONAL_WRITE;
            break;
        case ISA_FORM_IMM: case ISA_FORM_MOV:
            d = isa_parse_register(args[0]);
            if (d < 0 || !immediate(args[count - 1])) return 1;
            if (desc->form == ISA_FORM_IMM) {
                a = isa_parse_register(args[1]); if (a < 0) return 1;
                ins.reads = bit(a);
            }
            ins.writes = written(d);
            break;
        case ISA_FORM_BRANCH:
            a = isa_parse_register(args[0]); b = isa_parse_register(args[1]);
            if (a < 0 || b < 0 || !target(args[2], &ins.effects)) return 1;
            ins.reads = bit(a) | bit(b);
            break;
        case ISA_FORM_JEFR:
            d = isa_parse_register(args[0]); a = isa_parse_register(args[1]);
            if (d < 0 || a < 0) return 1;
            ins.writes = written(d); ins.reads = bit(a);
            if (d != 0) ins.effects |= CG_EFFECT_CALL;
            break;
        case ISA_FORM_JAS:
            d = isa_parse_register(args[0]);
            if (d < 0 || !target(args[1], &ins.effects)) return 1;
            ins.writes = written(d); ins.effects |= CG_EFFECT_CALL;
            break;
        case ISA_FORM_JEF:
            if (!target(args[0], &ins.effects)) return 1;
            break;
        case ISA_FORM_LOAD: case ISA_FORM_STORE:
            d = isa_parse_register(args[0]); a = address(args[1]);
            if (d < 0 || a < 0) return 1;
            ins.reads = bit(a);
            if (desc->form == ISA_FORM_LOAD) {
                ins.writes = written(d); ins.effects |= CG_EFFECT_MEMORY_READ;
            } else {
                ins.reads |= bit(d); ins.effects |= CG_EFFECT_MEMORY_WRITE;
            }
            break;
        case ISA_FORM_RS1: case ISA_FORM_KV_WRITE:
            a = isa_parse_register(args[0]); if (a < 0) return 1;
            ins.reads = bit(a);
            if (desc->form == ISA_FORM_KV_WRITE && (!selector(args[1]) || !selector(args[2]))) return 1;
            if (strcmp(name, "auth") == 0) ins.effects |= CG_EFFECT_PIPELINE_STALL;
            break;
        case ISA_FORM_NONE: break;
    }
    ins.result_distance = ins.writes ? 2 : 0;
    ins.known = 1;
    *out = ins;
    return 0;
}
