/*
 * isa_table.c — Fases 4–5: tabla de instrucciones de la ISA y lectura de operandos.
 *
 * Contiene la única tabla de mnemónicos y las funciones de lectura de texto
 * que comparten el organizador de bundles y el ensamblador. La semántica
 * común separa lecturas físicas y preservación del destino (Aho §10.2.3).
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
    {
        .name = "add",
        .unit = ISA_UNIT_ALU,
        .funct = 0x01,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "sub",
        .unit = ISA_UNIT_ALU,
        .funct = 0x02,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "and",
        .unit = ISA_UNIT_ALU,
        .funct = 0x03,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "or",
        .unit = ISA_UNIT_ALU,
        .funct = 0x04,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "xor",
        .unit = ISA_UNIT_ALU,
        .funct = 0x05,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "sll",
        .unit = ISA_UNIT_ALU,
        .funct = 0x06,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "srl",
        .unit = ISA_UNIT_ALU,
        .funct = 0x07,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "sra",
        .unit = ISA_UNIT_ALU,
        .funct = 0x08,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "slt",
        .unit = ISA_UNIT_ALU,
        .funct = 0x09,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "sle",
        .unit = ISA_UNIT_ALU,
        .funct = 0x0A,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "seq",
        .unit = ISA_UNIT_ALU,
        .funct = 0x0B,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "sne",
        .unit = ISA_UNIT_ALU,
        .funct = 0x0C,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "ifmov",
        .unit = ISA_UNIT_ALU,
        .funct = 0x0D,
        .form = ISA_FORM_R3,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .preserves = ISA_REG_DEST,
            .effects = ISA_EFFECT_CONDITIONAL_WRITE,
            .result_distance = 2,
        }
    },
    {
        .name = "saddi",
        .unit = ISA_UNIT_ALU,
        .funct = 0x10,
        .form = ISA_FORM_IMM,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "xori",
        .unit = ISA_UNIT_ALU,
        .funct = 0x11,
        .form = ISA_FORM_IMM,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "mov",
        .unit = ISA_UNIT_ALU,
        .funct = 0x12,
        .form = ISA_FORM_MOV,
        .semantics = {
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_NONE,
            .result_distance = 2,
        }
    },
    {
        .name = "beq",
        .unit = ISA_UNIT_BRU,
        .funct = 0x01,
        .form = ISA_FORM_BRANCH,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_CONTROL,
        }
    },
    {
        .name = "bne",
        .unit = ISA_UNIT_BRU,
        .funct = 0x02,
        .form = ISA_FORM_BRANCH,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_CONTROL,
        }
    },
    {
        .name = "blt",
        .unit = ISA_UNIT_BRU,
        .funct = 0x03,
        .form = ISA_FORM_BRANCH,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_CONTROL,
        }
    },
    {
        .name = "bge",
        .unit = ISA_UNIT_BRU,
        .funct = 0x04,
        .form = ISA_FORM_BRANCH,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_CONTROL,
        }
    },
    {
        .name = "jefR",
        .unit = ISA_UNIT_BRU,
        .funct = 0x05,
        .form = ISA_FORM_JEFR,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_CONTROL,
            .result_distance = 2,
            .call_if_link_nonzero = 1,
        }
    },
    {
        .name = "jef",
        .unit = ISA_UNIT_BRU,
        .funct = 0x10,
        .form = ISA_FORM_JEF,
        .semantics = {
            .effects = ISA_EFFECT_CONTROL,
        }
    },
    {
        .name = "jas",
        .unit = ISA_UNIT_BRU,
        .funct = 0x11,
        .form = ISA_FORM_JAS,
        .semantics = {
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_CONTROL |
                       ISA_EFFECT_CALL,
            .result_distance = 2,
        }
    },
    {
        .name = "lw",
        .unit = ISA_UNIT_LSU,
        .funct = 0x01,
        .form = ISA_FORM_LOAD,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_MEMORY_READ,
            .result_distance = 2,
        }
    },
    {
        .name = "sw",
        .unit = ISA_UNIT_LSU,
        .funct = 0x02,
        .form = ISA_FORM_STORE,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_MEMORY_WRITE,
        }
    },
    {
        .name = "lb",
        .unit = ISA_UNIT_LSU,
        .funct = 0x03,
        .form = ISA_FORM_LOAD,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .writes = ISA_REG_DEST,
            .effects = ISA_EFFECT_MEMORY_READ,
            .result_distance = 2,
        }
    },
    {
        .name = "sb",
        .unit = ISA_UNIT_LSU,
        .funct = 0x04,
        .form = ISA_FORM_STORE,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .effects = ISA_EFFECT_MEMORY_WRITE,
        }
    },
    {
        .name = "feis",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x01,
        .form = ISA_FORM_FEIS,
        .semantics = {
            .reads = ISA_REG_SRC1 | ISA_REG_SRC2,
            .writes = ISA_REG_DEST,
            .preserves = ISA_REG_DEST,
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER |
                       ISA_EFFECT_CONDITIONAL_WRITE,
            .result_distance = 2,
        }
    },
    {
        .name = "auth",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x02,
        .form = ISA_FORM_RS1,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER |
                       ISA_EFFECT_PIPELINE_STALL,
        }
    },
    {
        .name = "logout",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x03,
        .form = ISA_FORM_NONE,
        .semantics = {
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER,
        }
    },
    {
        .name = "kv_write",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x04,
        .form = ISA_FORM_KV_WRITE,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER,
        }
    },
    {
        .name = "kv_lock",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x05,
        .form = ISA_FORM_NONE,
        .semantics = {
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER,
        }
    },
    {
        .name = "sanc",
        .unit = ISA_UNIT_CRYPTO,
        .funct = 0x06,
        .form = ISA_FORM_RS1,
        .semantics = {
            .reads = ISA_REG_SRC1,
            .effects = ISA_EFFECT_CRYPTO_STATE |
                       ISA_EFFECT_BARRIER,
        }
    },
    {
        .name = "nopi",
        .unit = ISA_UNIT_NONE,
        .funct = 0x00,
        .form = ISA_FORM_NONE,
        .semantics = {
            .effects = ISA_EFFECT_NONE,
        }
    },
};

#define INSTRUCTION_COUNT (int) (sizeof(INSTRUCTIONS) / sizeof(INSTRUCTIONS[0]))

/*
 * register_mask — proyecta posiciones normalizadas a registros físicos.
 *
 * Entrada:
 *   positions  máscara de DEST/SRC1/SRC2.
 *   registers  tres números 0..15 o -1 en posiciones no seleccionadas.
 * Salida:
 *   unión de bits de los registros seleccionados, incluidos registros iguales.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   el llamador ya comprobó los registros y las posiciones.
 */
static uint16_t register_mask(unsigned positions, const int registers[3]) {
    uint16_t mask = 0;
    for (unsigned k = 0; k < 3; k++) {
        if (positions & (1u << k)) {
            mask |= (uint16_t)(1u << registers[k]);
        }
    }
    return mask;
}

/*
 * isa_physical_effects — aplica la tabla sin interpretar texto de ensamblador.
 *
 * Entrada:
 *   instruction  descriptor o NULL; dest/src1/src2 0..15 o -1 si ausentes.
 *   pc_relative  destino numérico; out  resultado escribible o NULL.
 * Salida:
 *   1 en éxito; 0 si los argumentos no corresponden al descriptor.
 * Efectos:
 *   inicializa out; reúne máscaras sin agregar preserves a reads.
 * Restricciones:
 *   valida posiciones y registros, no sintaxis, inmediatos, selectores o slots.
 *   La distancia se anula cuando no escribe registros, incluido destino rz.
 *   Véase preserves en isa_table.h (Aho §10.2.3; isa.md §9).
 */
int isa_physical_effects(const IsaInstruction *instruction,
                         int dest, int src1, int src2, int pc_relative,
                         IsaPhysicalEffects *out) {
    if (!out) {
        return 0;
    }
    *out = (IsaPhysicalEffects){0};
    if (!instruction) {
        return 0;
    }
    const IsaSemantics *semantics = &instruction->semantics;
    unsigned positions = semantics->reads | semantics->writes |
                         semantics->preserves;
    int registers[3] = {dest, src1, src2};
    if (positions & ~7u) {
        return 0;
    }
    for (unsigned k = 0; k < 3; k++) {
        if (positions & (1u << k)) {
            if (registers[k] < 0 || registers[k] > 15) {
                return 0;
            }
        } else if (registers[k] != -1) {
            return 0;
        }
    }
    if (pc_relative && instruction->form != ISA_FORM_BRANCH &&
        instruction->form != ISA_FORM_JEF && instruction->form != ISA_FORM_JAS) {
        return 0;
    }
    out->reads = register_mask(semantics->reads, registers);
    out->writes = register_mask(semantics->writes, registers) & (uint16_t)~1u;
    out->preserves = register_mask(semantics->preserves, registers) &
                     (uint16_t)~1u;
    out->effects = semantics->effects;
    if (semantics->call_if_link_nonzero && dest != 0) {
        out->effects |= ISA_EFFECT_CALL;
    }
    if (pc_relative) {
        out->effects |= ISA_EFFECT_PC_RELATIVE;
    }
    out->result_distance = out->writes ? semantics->result_distance : 0;
    return 1;
}

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

static const char *const REGISTER_NAMES[] = {
    "rz", "rl", "rx", "rg", "t0", "t1", "t2", "t3",
    "t4", "t5", "t6", "t7", "t8", "t9", "t10", "t11"
};

/* isa_register_name — véase el contrato en isa_table.h. */
const char *isa_register_name(int reg) {
    return REGISTER_NAMES[reg < REG_ZERO || reg > REG_TEMP_LAST ? REG_ZERO : reg];
}

/* isa_parse_register — véase el contrato en isa_table.h. */
int isa_parse_register(const char *text) {

    for (int i = 0; i < 4; i++) {

        if (strcmp(text, REGISTER_NAMES[i]) == 0)
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
