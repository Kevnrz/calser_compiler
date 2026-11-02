/*
 * isa_table.c — Fases 4–5: tabla de instrucciones de la ISA y lectura de operandos.
 *
 * Contiene la única tabla de mnemónicos y las funciones de lectura de texto
 * que comparten el organizador de bundles y el ensamblador.
 *
 * Usa: isa_table.h; biblioteca C.
 * Lo usa: codegen_instruction.c y assembler.c.
 * Referencias: isa.md §3, §4.2, §6.
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "isa_table.h"

/* ==========================================================================
 * TABLA DE INSTRUCCIONES
 * ======================================================================= */


/* Tablas de funct de isa.md §6.1 (ALU), §6.2 (BRU), §6.3 (LSU), §6.4 (CRYPTO).
 * El funct de nopi no se codifica: es la instrucción vacía, todo ceros. */
static const IsaInstruction INSTRUCTIONS[] = {
    { "add",      ISA_UNIT_ALU,    0x01, ISA_FORM_R3 },
    { "sub",      ISA_UNIT_ALU,    0x02, ISA_FORM_R3 },
    { "and",      ISA_UNIT_ALU,    0x03, ISA_FORM_R3 },
    { "or",       ISA_UNIT_ALU,    0x04, ISA_FORM_R3 },
    { "xor",      ISA_UNIT_ALU,    0x05, ISA_FORM_R3 },
    { "sll",      ISA_UNIT_ALU,    0x06, ISA_FORM_R3 },
    { "srl",      ISA_UNIT_ALU,    0x07, ISA_FORM_R3 },
    { "sra",      ISA_UNIT_ALU,    0x08, ISA_FORM_R3 },
    { "slt",      ISA_UNIT_ALU,    0x09, ISA_FORM_R3 },
    { "sle",      ISA_UNIT_ALU,    0x0A, ISA_FORM_R3 },
    { "seq",      ISA_UNIT_ALU,    0x0B, ISA_FORM_R3 },
    { "sne",      ISA_UNIT_ALU,    0x0C, ISA_FORM_R3 },
    { "ifmov",    ISA_UNIT_ALU,    0x0D, ISA_FORM_R3 },
    { "saddi",    ISA_UNIT_ALU,    0x10, ISA_FORM_IMM },
    { "xori",     ISA_UNIT_ALU,    0x11, ISA_FORM_IMM },
    { "mov",      ISA_UNIT_ALU,    0x12, ISA_FORM_MOV },

    { "beq",      ISA_UNIT_BRU,    0x01, ISA_FORM_BRANCH },
    { "bne",      ISA_UNIT_BRU,    0x02, ISA_FORM_BRANCH },
    { "blt",      ISA_UNIT_BRU,    0x03, ISA_FORM_BRANCH },
    { "bge",      ISA_UNIT_BRU,    0x04, ISA_FORM_BRANCH },
    { "jefR",     ISA_UNIT_BRU,    0x05, ISA_FORM_JEFR },
    { "jef",      ISA_UNIT_BRU,    0x10, ISA_FORM_JEF },
    { "jas",      ISA_UNIT_BRU,    0x11, ISA_FORM_JAS },

    { "lw",       ISA_UNIT_LSU,    0x01, ISA_FORM_LOAD },
    { "sw",       ISA_UNIT_LSU,    0x02, ISA_FORM_STORE },
    { "lb",       ISA_UNIT_LSU,    0x03, ISA_FORM_LOAD },
    { "sb",       ISA_UNIT_LSU,    0x04, ISA_FORM_STORE },

    { "feis",     ISA_UNIT_CRYPTO, 0x01, ISA_FORM_FEIS },
    { "auth",     ISA_UNIT_CRYPTO, 0x02, ISA_FORM_RS1 },
    { "logout",   ISA_UNIT_CRYPTO, 0x03, ISA_FORM_NONE },
    { "kv_write", ISA_UNIT_CRYPTO, 0x04, ISA_FORM_KV_WRITE },
    { "kv_lock",  ISA_UNIT_CRYPTO, 0x05, ISA_FORM_NONE },
    { "sanc",     ISA_UNIT_CRYPTO, 0x06, ISA_FORM_RS1 },

    { "nopi",     ISA_UNIT_NONE,   0x00, ISA_FORM_NONE }
};

#define INSTRUCTION_COUNT (int) (sizeof(INSTRUCTIONS) / sizeof(INSTRUCTIONS[0]))

/* isa_find — véase el contrato en isa_table.h. */
const IsaInstruction *isa_find(const char *name) {
    for (int i = 0; i < INSTRUCTION_COUNT; i++) {

        if (strcmp(INSTRUCTIONS[i].name, name) == 0)
            return &INSTRUCTIONS[i];
    }

    return NULL;
}

/* isa_operand_count — véase el contrato en isa_table.h. */
int isa_operand_count(IsaForm form) {
    switch (form) {
        case ISA_FORM_R3:       return 3;
        case ISA_FORM_IMM:      return 3;
        case ISA_FORM_MOV:      return 2;
        case ISA_FORM_BRANCH:   return 3;
        case ISA_FORM_JEFR:     return 2;
        case ISA_FORM_JEF:      return 1;
        case ISA_FORM_JAS:      return 2;
        case ISA_FORM_LOAD:     return 2;
        case ISA_FORM_STORE:    return 2;
        case ISA_FORM_FEIS:     return 5;
        case ISA_FORM_RS1:      return 1;
        case ISA_FORM_NONE:     return 0;
        default:                return 3;   // ISA_FORM_KV_WRITE
    }
}

/* isa_unit_slots — véase el contrato en isa_table.h. */
unsigned isa_unit_slots(IsaUnit unit) {
    switch (unit) {
        case ISA_UNIT_ALU:    return 3;    // slots 0 y 1
        case ISA_UNIT_CRYPTO: return 2;    // slot 1
        case ISA_UNIT_BRU:    return 4;    // slot 2
        case ISA_UNIT_LSU:    return 8;    // slot 3
        case ISA_UNIT_NONE:   return 15;   // nopi cabe en cualquier slot
        default:              return 0;
    }
}

/* ==========================================================================
 * LECTURA DE OPERANDOS
 * ======================================================================= */


/* isa_trim — véase el contrato en isa_table.h. */
char *isa_trim(char *text) {
    while (isspace((unsigned char) *text))
        text++;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char) end[-1]))
        end--;

    *end = '\0';

    return text;
}

/* isa_parse_register — véase el contrato en isa_table.h. */
int isa_parse_register(const char *text) {
    static const char *const ALIASES[] = { "rz", "rl", "rx", "rg" };

    for (int i = 0; i < 4; i++) {

        if (strcmp(text, ALIASES[i]) == 0)
            return i;
    }

    if ((text[0] != 'r' && text[0] != 't') || !isdigit((unsigned char) text[1]))
        return -1;

    // Sin ceros a la izquierda: r07 no es un registro.
    if (text[1] == '0' && text[2] != '\0')
        return -1;

    char *end;
    long number = strtol(text + 1, &end, 10);

    if (*end != '\0')
        return -1;

    if (text[0] == 'r')
        return number <= 15 ? (int) number : -1;

    // tN es el registro N + 4.
    return number <= 11 ? (int) number + 4 : -1;
}

/* isa_parse_integer — véase el contrato en isa_table.h. */
int isa_parse_integer(const char *text, long *value) {
    const char *digits = text;

    if (*digits == '+' || *digits == '-')
        digits++;

    int hex = digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X');

    if (hex ? !isxdigit((unsigned char) digits[2])
            : !isdigit((unsigned char) digits[0]))
        return 0;

    char *end;

    errno = 0;
    *value = strtol(text, &end, hex ? 16 : 10);

    return errno == 0 && *end == '\0';
}
