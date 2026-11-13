/*
 * codegen_instruction.c — Fase 4: descripción de instrucciones para dependencias.
 *
 * Parsea y valida texto; isa_table aporta lecturas, escrituras y efectos.
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
 * Los mnemónicos, unidades, formas y semántica salen de isa_table.h,
 * la misma que usa el ensamblador. */

/* ==========================================================================
 * LECTURA DE OPERANDOS
 * ======================================================================= */

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
    return isa_parse_integer(p, &n) && isa_immediate_fits(n);
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
 *   pc_relative  marca escribible no NULL.
 * Salida:
 *   1 si entero o etiqueta válida; 0 si inválido.
 *   Efectos: marca pc_relative si el destino es un entero.
 * Restricciones:
 *   no resuelve etiquetas ni comprueba rango del destino numérico.
 */
static int target(const char *p, int *pc_relative) {
    long n;
    if (isa_parse_integer(p, &n)) {
        *pc_relative = 1;
        return 1;
    }
    if (!(isalpha((unsigned char)*p) || *p == '_' || *p == '.')) {
        return 0;
    }
    for (p++; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '.')) {
            return 0;
        }
    }
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
    char *open = strchr(p, '(');
    char *close = strchr(p, ')');
    if (!open || !close || close < open || *isa_trim(close + 1) != '\0') return -1;
    *open = '\0';
    *close = '\0';
    if (!immediate(isa_trim(p))) return -1;
    return isa_parse_register(isa_trim(open + 1));
}

/* ==========================================================================
 * DESCRIPCIÓN DE INSTRUCCIONES
 * ======================================================================= */


/*
 * split_arguments — separa hasta cinco argumentos, sin interpretar semántica.
 *
 * Entrada:
 *   rest  texto modificable recortado; args  espacio para cinco punteros.
 * Salida:
 *   cantidad de argumentos o -1 si hay un argumento vacío o sobran argumentos.
 * Efectos:
 *   reemplaza comas por NUL y entrega punteros prestados dentro de rest.
 * Restricciones:
 *   el texto debe seguir vivo mientras se usan args.
 */
static int split_arguments(char *rest, char *args[5]) {
    int count = 0;
    if (!*rest) {
        return 0;
    }
    for (;;) {
        if (count == 5) {
            return -1;
        }
        char *comma = strchr(rest, ',');
        if (comma) {
            *comma = '\0';
        }
        args[count] = isa_trim(rest);
        if (!*args[count]) {
            return -1;
        }
        count++;
        if (!comma) {
            return count;
        }
        rest = comma + 1;
    }
}

/*
 * parse_operands — valida valores textuales y normaliza posiciones de registros.
 *
 * Entrada:
 *   form  forma ISA; args/count  argumentos de la cantidad ya comprobada.
 *   registers  salida DEST/SRC1/SRC2 inicializada a -1.
 *   pc_relative  marca inicializada a cero.
 * Salida:
 *   1 si inmediatos, selectores y destinos son válidos; 0 si falla alguno.
 * Efectos:
 *   escribe registros (incluido -1 ante registro inválido) y marca de destino.
 * Restricciones:
 *   isa_physical_effects valida los registros según las posiciones de tabla.
 *   No calcula dependencias ni asigna efectos (isa.md §4.2, §6).
 */
static int parse_operands(IsaForm form, char *args[5], int count,
                          int registers[3], int *pc_relative) {
    switch (form) {
        case ISA_FORM_R3:
        case ISA_FORM_FEIS:
            registers[0] = isa_parse_register(args[0]);
            registers[1] = isa_parse_register(args[1]);
            registers[2] = isa_parse_register(args[2]);
            return form != ISA_FORM_FEIS ||
                   (selector(args[3]) && selector(args[4]));
        case ISA_FORM_IMM:
        case ISA_FORM_MOV:
            registers[0] = isa_parse_register(args[0]);
            if (form == ISA_FORM_IMM) {
                registers[1] = isa_parse_register(args[1]);
            }
            return immediate(args[count - 1]);
        case ISA_FORM_BRANCH:
            registers[1] = isa_parse_register(args[0]);
            registers[2] = isa_parse_register(args[1]);
            return target(args[2], pc_relative);
        case ISA_FORM_JEFR:
            registers[0] = isa_parse_register(args[0]);
            registers[1] = isa_parse_register(args[1]);
            return 1;
        case ISA_FORM_JAS:
            registers[0] = isa_parse_register(args[0]);
            return target(args[1], pc_relative);
        case ISA_FORM_JEF:
            return target(args[0], pc_relative);
        case ISA_FORM_LOAD:
            registers[0] = isa_parse_register(args[0]);
            registers[1] = address(args[1]);
            return 1;
        case ISA_FORM_STORE:
            registers[1] = address(args[1]);
            registers[2] = isa_parse_register(args[0]);
            return 1;
        case ISA_FORM_RS1:
        case ISA_FORM_KV_WRITE:
            registers[1] = isa_parse_register(args[0]);
            return form != ISA_FORM_KV_WRITE ||
                   (selector(args[1]) && selector(args[2]));
        case ISA_FORM_NONE:
            return 1;
    }
    return 0;
}

/*
 * cg_describe_instruction — describe texto usando la semántica única del ISA.
 *
 * Entrada:
 *   text  texto prestado o NULL; slot  slot 0..3; source_line  línea o cero.
 *   out  resultado escribible o NULL.
 * Salida:
 *   0 si describe; 1 si el texto, slot o argumento es inválido/desconocido.
 * Efectos:
 *   llena out; ante error conserva la descripción desconocida como barrera.
 * Restricciones:
 *   texto menor de 256 bytes. No suma preserves a lecturas físicas: mantiene
 *   las distancias RAW/WAW actuales (isa_table.h; Aho §10.2.3; isa.md §9).
 */
int cg_describe_instruction(const char *text, Slot slot, int source_line,
                            CGInstruction *out) {
    if (!out) {
        return 1;
    }
    *out = (CGInstruction){
        .text = text,
        .unit = ISA_UNIT_UNKNOWN,
        .original_slot = slot,
        .source_line = source_line > 0 ? source_line : 0,
        .effects = CG_EFFECT_BARRIER
    };
    if (!text || slot < SLOT_ALU0 || slot > SLOT_LSU || strlen(text) >= 256) {
        return 1;
    }
    char buffer[256];
    strcpy(buffer, text);
    char *name = isa_trim(buffer);
    char *rest = name;
    while (*rest && !isspace((unsigned char)*rest)) {
        rest++;
    }
    if (*rest) {
        *rest++ = '\0';
    }
    const IsaInstruction *desc = isa_find(name);
    if (!desc || !(isa_unit_slots(desc->unit) & (1u << slot))) {
        return 1;
    }
    char *args[5];
    int count = split_arguments(isa_trim(rest), args);
    if (count != isa_operand_count(desc->form)) {
        return 1;
    }
    int registers[3] = {-1, -1, -1};
    int pc_relative = 0;
    if (!parse_operands(desc->form, args, count, registers, &pc_relative)) {
        return 1;
    }
    IsaPhysicalEffects physical;
    if (!isa_physical_effects(desc, registers[0], registers[1], registers[2],
                              pc_relative, &physical)) {
        return 1;
    }
    out->unit = desc->unit;
    out->allowed_slots = isa_unit_slots(desc->unit);
    out->reads = physical.reads;
    out->writes = physical.writes;
    out->effects = physical.effects;
    out->result_distance = physical.result_distance;
    out->known = 1;
    return 0;
}
