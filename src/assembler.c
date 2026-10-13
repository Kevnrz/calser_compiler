#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"

/*
 * Ensamblado en dos pasadas sobre la sintaxis de isa.md §11.
 *
 * El programa objeto en ensamblador usa nombres simbólicos para los destinos
 * de salto (libro §8.1.2); el ensamblador los traduce a desplazamientos
 * (libro §1.1, Fig. 1.5). Como una etiqueta puede usarse antes de definirse,
 * hacen falta dos pasadas:
 *
 *   Pasada 1: quita comentarios, separa etiquetas y bundles, y asigna a cada
 *             etiqueta el índice del bundle que la sigue (isa.md §11.1).
 *   Pasada 2: codifica cada slot (isa.md §6) y resuelve los saltos como
 *             etiqueta − PC (isa.md §6.2, §11.2).
 */

/* ---- Unidades y formas de las instrucciones (isa.md §6, §11.3) ---------- */

typedef enum { UNIT_ALU, UNIT_CRYPTO, UNIT_BRU, UNIT_LSU } Unit;

typedef enum {
    FORM_R3,       /* rd, rs1, rs2                 ALU formato R            */
    FORM_IMM,      /* rd, rs1, imm                 saddi, xori              */
    FORM_MOV,      /* rd, imm                      mov (rs1 = 0)            */
    FORM_BRANCH,   /* rs1, rs2, destino            beq, bne, blt, bge       */
    FORM_JEFR,     /* rd, rs1                                               */
    FORM_JEF,      /* destino                      (rd = 0)                 */
    FORM_JAS,      /* rd, destino                                           */
    FORM_LOAD,     /* rd, imm(rs1)                 lw, lb                   */
    FORM_STORE,    /* rs2, imm(rs1)                sw, sb: fuente en 25..22 */
    FORM_FEIS,     /* rd, rs1, rs2, k, i                                    */
    FORM_RS1,      /* rs1                          auth, sanc               */
    FORM_NONE,     /* sin operandos                logout, kv_lock          */
    FORM_KV_WRITE  /* rs1, k, w                    w va en el campo i       */
} Form;

typedef struct {
    const char *name;
    Unit unit;
    unsigned funct;
    Form form;
} Instruction;

/* Tablas de funct de isa.md §6.1 (ALU), §6.2 (BRU), §6.3 (LSU), §6.4 (CRYPTO). */
static const Instruction INSTRUCTIONS[] = {
    { "add",      UNIT_ALU,    0x01, FORM_R3 },
    { "sub",      UNIT_ALU,    0x02, FORM_R3 },
    { "and",      UNIT_ALU,    0x03, FORM_R3 },
    { "or",       UNIT_ALU,    0x04, FORM_R3 },
    { "xor",      UNIT_ALU,    0x05, FORM_R3 },
    { "sll",      UNIT_ALU,    0x06, FORM_R3 },
    { "srl",      UNIT_ALU,    0x07, FORM_R3 },
    { "sra",      UNIT_ALU,    0x08, FORM_R3 },
    { "slt",      UNIT_ALU,    0x09, FORM_R3 },
    { "sle",      UNIT_ALU,    0x0A, FORM_R3 },
    { "seq",      UNIT_ALU,    0x0B, FORM_R3 },
    { "sne",      UNIT_ALU,    0x0C, FORM_R3 },
    { "ifmov",    UNIT_ALU,    0x0D, FORM_R3 },
    { "saddi",    UNIT_ALU,    0x10, FORM_IMM },
    { "xori",     UNIT_ALU,    0x11, FORM_IMM },
    { "mov",      UNIT_ALU,    0x12, FORM_MOV },

    { "beq",      UNIT_BRU,    0x01, FORM_BRANCH },
    { "bne",      UNIT_BRU,    0x02, FORM_BRANCH },
    { "blt",      UNIT_BRU,    0x03, FORM_BRANCH },
    { "bge",      UNIT_BRU,    0x04, FORM_BRANCH },
    { "jefR",     UNIT_BRU,    0x05, FORM_JEFR },
    { "jef",      UNIT_BRU,    0x10, FORM_JEF },
    { "jas",      UNIT_BRU,    0x11, FORM_JAS },

    { "lw",       UNIT_LSU,    0x01, FORM_LOAD },
    { "sw",       UNIT_LSU,    0x02, FORM_STORE },
    { "lb",       UNIT_LSU,    0x03, FORM_LOAD },
    { "sb",       UNIT_LSU,    0x04, FORM_STORE },

    { "feis",     UNIT_CRYPTO, 0x01, FORM_FEIS },
    { "auth",     UNIT_CRYPTO, 0x02, FORM_RS1 },
    { "logout",   UNIT_CRYPTO, 0x03, FORM_NONE },
    { "kv_write", UNIT_CRYPTO, 0x04, FORM_KV_WRITE },
    { "kv_lock",  UNIT_CRYPTO, 0x05, FORM_NONE },
    { "sanc",     UNIT_CRYPTO, 0x06, FORM_RS1 }
};

#define INSTRUCTION_COUNT (int) (sizeof(INSTRUCTIONS) / sizeof(INSTRUCTIONS[0]))

/* Rangos de isa.md §4.2. */
#define IMM_BITS    18
#define OFFSET_BITS 18
#define TARGET_BITS 22

#define MAX_OPERANDS 5

/* ---- Estado de un ensamblado -------------------------------------------- */

typedef struct {
    char *name;
    int value;      /* índice del bundle que sigue a la etiqueta */
} Label;

typedef struct {
    char *body;     /* texto entre las llaves */
    int line;
} RawBundle;

typedef struct {
    const char *name;   /* nombre para los mensajes */
    int errors;

    Label *labels;
    int label_count;
    int label_capacity;

    RawBundle *raw;
    int raw_count;
    int raw_capacity;
} Assembler;

static void out_of_memory(void) {
    fprintf(stderr, "ccom: error interno: memoria insuficiente (ensamblado)\n");
    exit(EXIT_FAILURE);
}

static char *copy_text(const char *text, size_t length) {
    char *copy = malloc(length + 1);

    if (copy == NULL)
        out_of_memory();

    memcpy(copy, text, length);
    copy[length] = '\0';

    return copy;
}

// Único punto de salida de los errores de ensamblado.
static void asm_error(Assembler *as, int line, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "ccom: %s:%d: error de ensamblado: ", as->name, line);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    as->errors++;
}

// Quita los espacios de los extremos (modifica text) y devuelve el inicio.
static char *trim(char *text) {
    while (isspace((unsigned char) *text))
        text++;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char) end[-1]))
        end--;

    *end = '\0';

    return text;
}

// Etiqueta: [A-Za-z_.][A-Za-z0-9_.]*
static int valid_label(const char *text) {
    if (!(isalpha((unsigned char) *text) || *text == '_' || *text == '.'))
        return 0;

    for (text++; *text != '\0'; text++) {

        if (!(isalnum((unsigned char) *text) || *text == '_' || *text == '.'))
            return 0;
    }

    return 1;
}

static Label *find_label(Assembler *as, const char *name) {
    for (int i = 0; i < as->label_count; i++) {

        if (strcmp(as->labels[i].name, name) == 0)
            return &as->labels[i];
    }

    return NULL;
}

/* ---- Pasada 1 ----------------------------------------------------------- */

static void add_label(Assembler *as, const char *name, int line) {
    // isa.md §11.4: una etiqueta definida dos veces es un error.
    if (find_label(as, name) != NULL) {
        asm_error(as, line, "etiqueta definida dos veces: '%s'", name);
        return;
    }

    if (as->label_count == as->label_capacity) {
        int capacity = as->label_capacity == 0 ? 64 : as->label_capacity * 2;
        Label *grown = realloc(as->labels, (size_t) capacity * sizeof(Label));

        if (grown == NULL)
            out_of_memory();

        as->labels = grown;
        as->label_capacity = capacity;
    }

    // isa.md §11.1: vale el índice del bundle que la sigue.
    as->labels[as->label_count].name = copy_text(name, strlen(name));
    as->labels[as->label_count].value = as->raw_count;
    as->label_count++;
}

static void add_bundle(Assembler *as, const char *body, size_t length,
                       int line) {
    if (as->raw_count == as->raw_capacity) {
        int capacity = as->raw_capacity == 0 ? 256 : as->raw_capacity * 2;
        RawBundle *grown =
            realloc(as->raw, (size_t) capacity * sizeof(RawBundle));

        if (grown == NULL)
            out_of_memory();

        as->raw = grown;
        as->raw_capacity = capacity;
    }

    as->raw[as->raw_count].body = copy_text(body, length);
    as->raw[as->raw_count].line = line;
    as->raw_count++;
}

// Una línea: comentario fuera, y luego  etiqueta:*  [ { bundle } ]
static void scan_line(Assembler *as, const char *source, int line) {
    char *copy = copy_text(source, strlen(source));

    // isa.md §11.1: comentarios con #, hasta el final de la línea.
    char *comment = strchr(copy, '#');

    if (comment != NULL)
        *comment = '\0';

    char *text = trim(copy);

    while (*text != '\0') {

        if (*text == '{') {
            char *close = strchr(text, '}');

            if (close == NULL) {
                asm_error(as, line, "falta '}' al final del bundle");
            }
            else if (*trim(close + 1) != '\0') {
                asm_error(as, line, "texto despues del bundle");
            }
            else {
                add_bundle(as, text + 1, (size_t) (close - text - 1), line);
            }

            break;
        }

        // Lo que no es un bundle debe ser una etiqueta  nombre:
        char *colon = strchr(text, ':');

        if (colon == NULL) {
            asm_error(as, line,
                      "se esperaba una etiqueta o un bundle entre llaves");
            break;
        }

        *colon = '\0';

        char *name = trim(text);

        if (!valid_label(name)) {
            asm_error(as, line, "nombre de etiqueta invalido: '%s'", name);
            break;
        }

        add_label(as, name, line);

        text = trim(colon + 1);
    }

    free(copy);
}

/* ---- Pasada 2: operandos ------------------------------------------------ */

// isa.md §8 y §11.2: r0–r15, o los alias rz, rl, rx, rg, t0–t11.
static int parse_register(const char *text) {
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

// isa.md §11.2: decimal con signo (-5) o hexadecimal (0x1F).
static int parse_integer(const char *text, long *value) {
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

static int fits_signed(long value, int bits) {
    long limit = 1L << (bits - 1);

    return value >= -limit && value < limit;
}

// Registro en un operando; reporta y devuelve 0 si no lo es.
static int get_register(Assembler *as, int line, const char *text,
                        unsigned *reg) {
    int number = parse_register(text);

    if (number < 0) {
        asm_error(as, line, "registro invalido: '%s'", text);
        return 0;
    }

    *reg = (unsigned) number;

    return 1;
}

// Inmediato de 18 bits con signo (isa.md §4.2).
static int get_immediate(Assembler *as, int line, const char *text,
                         unsigned *field) {
    long value;

    if (!parse_integer(text, &value)) {
        asm_error(as, line, "inmediato invalido: '%s'", text);
        return 0;
    }

    if (!fits_signed(value, IMM_BITS)) {
        asm_error(as, line,
                  "inmediato fuera de rango: %ld (de -131072 a 131071)",
                  value);
        return 0;
    }

    *field = (unsigned) value & ((1u << IMM_BITS) - 1);

    return 1;
}

// k, i o w: entero de 0 a 3 (isa.md §4.2, §11.2).
static int get_selector(Assembler *as, int line, const char *what,
                        const char *text, unsigned *field) {
    long value;

    if (!parse_integer(text, &value) || value < 0 || value > 3) {
        asm_error(as, line, "'%s' debe ser un entero de 0 a 3: '%s'",
                  what, text);
        return 0;
    }

    *field = (unsigned) value;

    return 1;
}

// Destino de salto: una etiqueta (etiqueta − PC) o un desplazamiento entero
// en bundles (isa.md §11.2). PC es el índice del bundle del salto (§6.2).
static int get_destination(Assembler *as, int line, const char *text,
                           int pc, int bits, unsigned *field) {
    long offset;

    if (!parse_integer(text, &offset)) {

        if (!valid_label(text)) {
            asm_error(as, line, "destino de salto invalido: '%s'", text);
            return 0;
        }

        Label *label = find_label(as, text);

        // isa.md §11.4: etiqueta no definida.
        if (label == NULL) {
            asm_error(as, line, "etiqueta no definida: '%s'", text);
            return 0;
        }

        offset = (long) label->value - pc;
    }

    if (!fits_signed(offset, bits)) {
        asm_error(as, line,
                  "destino fuera de rango: %ld bundles (%d bits con signo)",
                  offset, bits);
        return 0;
    }

    *field = (unsigned) offset & ((1u << bits) - 1);

    return 1;
}

// Operando de memoria  imm(rs1)  (isa.md §11.3).
static int get_memory(Assembler *as, int line, char *text,
                      unsigned *imm, unsigned *base) {
    char *open = strchr(text, '(');
    size_t length = strlen(text);

    if (open == NULL || length == 0 || text[length - 1] != ')') {
        asm_error(as, line, "se esperaba imm(registro): '%s'", text);
        return 0;
    }

    text[length - 1] = '\0';
    *open = '\0';

    return get_immediate(as, line, trim(text), imm) &&
           get_register(as, line, trim(open + 1), base);
}

/* ---- Pasada 2: codificación --------------------------------------------- */

static const char *unit_name(Unit unit) {
    switch (unit) {
        case UNIT_ALU:    return "la ALU (slot 0 o 1)";
        case UNIT_CRYPTO: return "CRYPTO (slot 1)";
        case UNIT_BRU:    return "la BRU (slot 2)";
        default:          return "la LSU (slot 3)";
    }
}

// isa.md §3.1: slot 0 ALU0, slot 1 ALU1 o CRYPTO, slot 2 BRU, slot 3 LSU.
static int unit_fits(Unit unit, int slot) {
    switch (unit) {
        case UNIT_ALU:    return slot == 0 || slot == 1;
        case UNIT_CRYPTO: return slot == 1;
        case UNIT_BRU:    return slot == 2;
        default:          return slot == 3;
    }
}

static int operand_count(Form form) {
    switch (form) {
        case FORM_R3:       return 3;
        case FORM_IMM:      return 3;
        case FORM_MOV:      return 2;
        case FORM_BRANCH:   return 3;
        case FORM_JEFR:     return 2;
        case FORM_JEF:      return 1;
        case FORM_JAS:      return 2;
        case FORM_LOAD:     return 2;
        case FORM_STORE:    return 2;
        case FORM_FEIS:     return 5;
        case FORM_RS1:      return 1;
        case FORM_NONE:     return 0;
        default:            return 3;
    }
}

/* Posición de los campos (isa.md §4): sel 31, funct 30..26, rd 25..22,
 * rs1 21..18, rs2 17..14, k 13..12, i 11..10; imm/off 17..0; target 21..0. */
#define FIELD_SEL(x)   ((uint32_t) (x) << 31)
#define FIELD_FUNCT(x) ((uint32_t) (x) << 26)
#define FIELD_RD(x)    ((uint32_t) (x) << 22)
#define FIELD_RS1(x)   ((uint32_t) (x) << 18)
#define FIELD_RS2(x)   ((uint32_t) (x) << 14)
#define FIELD_K(x)     ((uint32_t) (x) << 12)
#define FIELD_I(x)     ((uint32_t) (x) << 10)

/* Codifica un slot. text se modifica. Devuelve 1 si ok y deja la palabra en
 * *word y en *written el registro que escribe (-1 si ninguno). */
static int encode_slot(Assembler *as, int line, char *text, int slot, int pc,
                       uint32_t *word, int *written) {
    *word = 0;
    *written = -1;

    text = trim(text);

    // isa.md §3.2: la instrucción vacía (nopi) es todo ceros en cualquier
    // slot. Un slot sin texto se codifica igual.
    if (*text == '\0' || strcmp(text, "nopi") == 0)
        return 1;

    // Mnemónico: hasta el primer espacio.
    char *rest = text;

    while (*rest != '\0' && !isspace((unsigned char) *rest))
        rest++;

    if (*rest != '\0')
        *rest++ = '\0';

    const Instruction *ins = NULL;

    for (int i = 0; i < INSTRUCTION_COUNT; i++) {

        if (strcmp(INSTRUCTIONS[i].name, text) == 0) {
            ins = &INSTRUCTIONS[i];
            break;
        }
    }

    // isa.md §11.4: mnemónico desconocido.
    if (ins == NULL) {
        asm_error(as, line, "mnemonico desconocido: '%s'", text);
        return 0;
    }

    // isa.md §11.4: instrucción en un slot que no es el de su unidad.
    if (!unit_fits(ins->unit, slot)) {
        asm_error(as, line, "'%s' no puede ir en el slot %d: es de %s",
                  ins->name, slot, unit_name(ins->unit));
        return 0;
    }

    // Operandos separados por comas.
    char *operand[MAX_OPERANDS];
    int count = 0;

    rest = trim(rest);

    if (*rest != '\0') {
        char *start = rest;

        for (;;) {
            char *comma = strchr(start, ',');

            if (comma != NULL)
                *comma = '\0';

            if (count < MAX_OPERANDS)
                operand[count] = trim(start);

            count++;

            if (comma == NULL)
                break;

            start = comma + 1;
        }
    }

    if (count != operand_count(ins->form)) {
        asm_error(as, line, "'%s' espera %d operandos, se recibieron %d",
                  ins->name, operand_count(ins->form), count);
        return 0;
    }

    // isa.md §11.3: en el slot 1, sel = 1 para CRYPTO y 0 para la ALU.
    uint32_t value =
        FIELD_SEL(ins->unit == UNIT_CRYPTO) | FIELD_FUNCT(ins->funct);

    unsigned a = 0, b = 0, c = 0, d = 0, e = 0;

    switch (ins->form) {

        case FORM_R3:
            if (!get_register(as, line, operand[0], &a) ||
                !get_register(as, line, operand[1], &b) ||
                !get_register(as, line, operand[2], &c))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(b) | FIELD_RS2(c);
            *written = (int) a;
            break;

        case FORM_IMM:
            if (!get_register(as, line, operand[0], &a) ||
                !get_register(as, line, operand[1], &b) ||
                !get_immediate(as, line, operand[2], &c))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(b) | c;
            *written = (int) a;
            break;

        // isa.md §6.1: en mov el campo rs1 no se usa y vale cero.
        case FORM_MOV:
            if (!get_register(as, line, operand[0], &a) ||
                !get_immediate(as, line, operand[1], &b))
                return 0;

            value |= FIELD_RD(a) | b;
            *written = (int) a;
            break;

        // isa.md §4 (tipo B): rs2 en 25..22 y rs1 en 21..18.
        case FORM_BRANCH:
            if (!get_register(as, line, operand[0], &a) ||
                !get_register(as, line, operand[1], &b) ||
                !get_destination(as, line, operand[2], pc, OFFSET_BITS, &c))
                return 0;

            value |= FIELD_RD(b) | FIELD_RS1(a) | c;
            break;

        case FORM_JEFR:
            if (!get_register(as, line, operand[0], &a) ||
                !get_register(as, line, operand[1], &b))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(b);
            *written = (int) a;
            break;

        // isa.md §6.2: en jef el campo rd no se usa y vale cero.
        case FORM_JEF:
            if (!get_destination(as, line, operand[0], pc, TARGET_BITS, &a))
                return 0;

            value |= a;
            break;

        case FORM_JAS:
            if (!get_register(as, line, operand[0], &a) ||
                !get_destination(as, line, operand[1], pc, TARGET_BITS, &b))
                return 0;

            value |= FIELD_RD(a) | b;
            *written = (int) a;
            break;

        case FORM_LOAD:
            if (!get_register(as, line, operand[0], &a) ||
                !get_memory(as, line, operand[1], &b, &c))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(c) | b;
            *written = (int) a;
            break;

        // isa.md §6.3: en sw y sb la fuente ocupa el campo 25..22 y no se
        // escribe ningún registro.
        case FORM_STORE:
            if (!get_register(as, line, operand[0], &a) ||
                !get_memory(as, line, operand[1], &b, &c))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(c) | b;
            break;

        case FORM_FEIS:
            if (!get_register(as, line, operand[0], &a) ||
                !get_register(as, line, operand[1], &b) ||
                !get_register(as, line, operand[2], &c) ||
                !get_selector(as, line, "k", operand[3], &d) ||
                !get_selector(as, line, "i", operand[4], &e))
                return 0;

            value |= FIELD_RD(a) | FIELD_RS1(b) | FIELD_RS2(c) |
                     FIELD_K(d) | FIELD_I(e);
            *written = (int) a;
            break;

        // auth y sanc: rd, rs2, k, i en cero (isa.md §6.4).
        case FORM_RS1:
            if (!get_register(as, line, operand[0], &a))
                return 0;

            value |= FIELD_RS1(a);
            break;

        case FORM_NONE:
            break;

        // isa.md §6.4: la palabra w va en los bits del campo i.
        case FORM_KV_WRITE:
            if (!get_register(as, line, operand[0], &a) ||
                !get_selector(as, line, "k", operand[1], &b) ||
                !get_selector(as, line, "w", operand[2], &c))
                return 0;

            value |= FIELD_RS1(a) | FIELD_K(b) | FIELD_I(c);
            break;
    }

    *word = value;

    return 1;
}

// Un bundle: exactamente 4 slots separados por | (isa.md §11.1).
static void encode_bundle(Assembler *as, const RawBundle *raw, int pc,
                          uint32_t word[4], char **text) {
    char *body = copy_text(raw->body, strlen(raw->body));
    char *slot[4];
    int count = 0;

    char *start = body;

    for (;;) {
        char *bar = strchr(start, '|');

        if (bar != NULL)
            *bar = '\0';

        if (count < 4)
            slot[count] = start;

        count++;

        if (bar == NULL)
            break;

        start = bar + 1;
    }

    for (int i = 0; i < 4; i++)
        word[i] = 0;

    // isa.md §11.4: un bundle sin exactamente cuatro slots.
    if (count != 4) {
        asm_error(as, raw->line,
                  "un bundle debe tener exactamente 4 slots (tiene %d)",
                  count);
        free(body);
        return;
    }

    // Texto normalizado para -p, antes de que la codificación lo modifique.
    size_t size = sizeof("{  }") + 3 * 3;

    for (int i = 0; i < 4; i++) {
        slot[i] = trim(slot[i]);
        size += strlen(slot[i]) + sizeof("nopi");
    }

    *text = malloc(size);

    if (*text == NULL)
        out_of_memory();

    snprintf(*text, size, "{ %s | %s | %s | %s }",
             *slot[0] ? slot[0] : "nopi", *slot[1] ? slot[1] : "nopi",
             *slot[2] ? slot[2] : "nopi", *slot[3] ? slot[3] : "nopi");

    int written[4];

    for (int i = 0; i < 4; i++) {

        if (!encode_slot(as, raw->line, slot[i], i, pc, &word[i],
                         &written[i]))
            written[i] = -1;
    }

    // isa.md §9.3 y §11.4: dos slots del mismo bundle no deben escribir el
    // mismo registro, salvo rz (sus escrituras se descartan, §8).
    for (int i = 0; i < 4; i++) {

        for (int j = i + 1; j < 4; j++) {

            if (written[i] > 0 && written[i] == written[j])
                asm_error(as, raw->line,
                          "los slots %d y %d escriben el mismo registro r%d",
                          i, j, written[i]);
        }
    }

    free(body);
}

/* ---- Interfaz ----------------------------------------------------------- */

void asm_free(AsmProgram *program) {
    if (program == NULL)
        return;

    if (program->text != NULL) {

        for (int i = 0; i < program->count; i++)
            free(program->text[i]);
    }

    free(program->text);
    free(program->bundles);

    program->text = NULL;
    program->bundles = NULL;
    program->count = 0;
}

int asm_assemble(const char *const *lines, int nlines, const char *name,
                 AsmProgram *out) {
    Assembler as = { 0 };

    as.name = name;

    out->count = 0;
    out->bundles = NULL;
    out->text = NULL;

    // Pasada 1: etiquetas y bundles.
    for (int i = 0; i < nlines; i++)
        scan_line(&as, lines[i], i + 1);

    // Pasada 2: codificación, con todas las etiquetas ya conocidas.
    size_t slots = as.raw_count > 0 ? (size_t) as.raw_count : 1;

    out->bundles = calloc(slots, sizeof(*out->bundles));
    out->text = calloc(slots, sizeof(char *));

    if (out->bundles == NULL || out->text == NULL)
        out_of_memory();

    out->count = as.raw_count;

    for (int i = 0; i < as.raw_count; i++)
        encode_bundle(&as, &as.raw[i], i, out->bundles[i], &out->text[i]);

    for (int i = 0; i < as.label_count; i++)
        free(as.labels[i].name);

    for (int i = 0; i < as.raw_count; i++)
        free(as.raw[i].body);

    free(as.labels);
    free(as.raw);

    if (as.errors > 0) {
        asm_free(out);
        return 1;
    }

    return 0;
}
