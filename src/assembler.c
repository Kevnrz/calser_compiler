/*
 * assembler.c — Fase 5: ensamblado de etiquetas y bundles en dos pasadas.
 *
 * Separa etiquetas y bundles, asigna direcciones y codifica cada slot.
 * Resuelve destinos simbólicos como desplazamientos relativos al PC.
 *
 * Usa: assembler.h; biblioteca C.
 * Lo usa: main.c, con líneas generadas o entrada -x.
 * Referencias: libro §1.1, §8.1.2; isa.md §3, §4, §6, §11.
 */

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

/* ---- UNIDADES Y FORMAS DE LAS INSTRUCCIONES (ISA.MD §6, §11.3) -------- */

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

/* ---- ESTADO DE UN ENSAMBLADO ------------------------------------------ */

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

/* ==========================================================================
 * ERRORES Y LECTURA
 * ======================================================================= */


/*
 * out_of_memory — termina el ensamblado por falta de memoria.
 *
 * Entrada: ninguna
 * Salida:
 *   no retorna.
 *   Efectos: escribe en stderr y termina el proceso con EXIT_FAILURE.
 * Restricciones:
 *   ninguna.
 */
static void out_of_memory(void) {
    fprintf(stderr, "ccom: error interno: memoria insuficiente (ensamblado)\n");
    exit(EXIT_FAILURE);
}

/*
 * copy_text — copia un fragmento de texto y lo termina con NUL.
 *
 * Entrada:
 *   text  texto legible.
 *   length  cantidad de bytes a copiar.
 * Salida:
 *   cadena nueva; el receptor debe liberarla.
 *   Efectos: reserva memoria; termina el proceso si no hay memoria.
 * Restricciones:
 *   text contiene al menos length bytes.
 */
static char *copy_text(const char *text, size_t length) {
    char *copy = malloc(length + 1);

    if (copy == NULL)
        out_of_memory();

    memcpy(copy, text, length);
    copy[length] = '\0';

    return copy;
}

/*
 * asm_error — reporta un error de ensamblado.
 *
 * Entrada:
 *   as  estado de ensamblado no NULL.
 *   line  línea del .asm desde 1.
 *   fmt  formato printf no NULL.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe en stderr e incrementa as->errors.
 * Restricciones:
 *   ninguna.
 */
static void asm_error(Assembler *as, int line, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "ccom: %s:%d: error de ensamblado: ", as->name, line);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    as->errors++;
}

/*
 * trim — quita espacios de los extremos.
 *
 * Entrada:
 *   text  cadena modificable no NULL.
 * Salida:
 *   puntero prestado al primer carácter restante.
 *   Efectos: escribe NUL al final del texto útil.
 * Restricciones:
 *   ninguna.
 */
static char *trim(char *text) {
    while (isspace((unsigned char) *text))
        text++;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char) end[-1]))
        end--;

    *end = '\0';

    return text;
}

/*
 * valid_label — verifica la sintaxis de una etiqueta.
 *
 * Entrada:
 *   text  cadena terminada en NUL no NULL.
 * Salida:
 *   1 si acepta el nombre; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   acepta letras, dígitos, punto y guion bajo; el primer carácter no
 *   puede ser un dígito.
 */
static int valid_label(const char *text) {
    if (!(isalpha((unsigned char) *text) || *text == '_' || *text == '.'))
        return 0;

    for (text++; *text != '\0'; text++) {

        if (!(isalnum((unsigned char) *text) || *text == '_' || *text == '.'))
            return 0;
    }

    return 1;
}

/*
 * find_label — busca una etiqueta de la primera pasada.
 *
 * Entrada:
 *   as  estado no NULL.
 *   name  nombre no NULL.
 * Salida:
 *   entrada interna o NULL si no existe.
 *   Efectos: ninguno.
 * Restricciones:
 *   el puntero puede invalidarse al agregar etiquetas.
 */
static Label *find_label(Assembler *as, const char *name) {
    for (int i = 0; i < as->label_count; i++) {

        if (strcmp(as->labels[i].name, name) == 0)
            return &as->labels[i];
    }

    return NULL;
}

/* ---- PASADA 1 --------------------------------------------------------- */

/* ==========================================================================
 * PASADA 1: ETIQUETAS Y BUNDLES
 * ======================================================================= */


/*
 * add_label — registra la dirección simbólica de una etiqueta.
 *
 * Entrada:
 *   as  estado no NULL.
 *   name  nombre no NULL.
 *   line  línea para el diagnóstico.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia el nombre y actualiza la tabla; reporta duplicados.
 * Restricciones:
 *   la dirección es as->raw_count, índice del próximo bundle.
 */
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

/*
 * add_bundle — guarda un bundle para la segunda pasada.
 *
 * Entrada:
 *   as  estado no NULL.
 *   body  texto entre llaves.
 *   length  cantidad de bytes del texto.
 *   line  línea del .asm.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia body e incrementa as->raw_count.
 * Restricciones:
 *   body contiene al menos length bytes.
 */
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

/*
 * scan_line — separa comentarios, etiquetas y un bundle.
 *
 * Entrada:
 *   as  estado no NULL.
 *   source  línea terminada en NUL.
 *   line  línea del .asm desde 1.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: agrega etiquetas y bundles; reporta errores de estructura.
 * Restricciones:
 *   comentarios con #; solo admite etiquetas antes del bundle.
 */
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

/* ---- PASADA 2: OPERANDOS ---------------------------------------------- */

/* ==========================================================================
 * PASADA 2: OPERANDOS
 * ======================================================================= */


/*
 * parse_register — reconoce un registro o su alias.
 *
 * Entrada:
 *   text  operando no NULL.
 * Salida:
 *   número ISA 0–15 o -1 si es inválido.
 *   Efectos: ninguno.
 * Restricciones:
 *   no admite ceros iniciales; t0–t11 equivalen a r4–r15.
 */
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

/*
 * parse_integer — lee un entero decimal o hexadecimal.
 *
 * Entrada:
 *   text  operando no NULL.
 *   value  puntero no NULL para el valor.
 * Salida:
 *   1 si convierte todo el operando sin error; 0 si falla.
 *   Efectos: escribe *value al intentar strtol.
 * Restricciones:
 *   usa el rango de long del anfitrión.
 */
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

/*
 * fits_signed — comprueba un rango con signo.
 *
 * Entrada:
 *   value  entero a verificar.
 *   bits  ancho del campo en bits.
 * Salida:
 *   1 si value cabe; 0 si no cabe.
 *   Efectos: ninguno.
 * Restricciones:
 *   bits positivo y menor que el ancho de long; aquí se usan 18 y 22.
 */
static int fits_signed(long value, int bits) {
    long limit = 1L << (bits - 1);

    return value >= -limit && value < limit;
}

/*
 * get_register — valida un registro y obtiene su campo ISA.
 *
 * Entrada:
 *   as  estado no NULL.
 *   line  línea del diagnóstico.
 *   text  operando no NULL.
 *   reg  puntero no NULL para el registro.
 * Salida:
 *   1 si acepta; 0 si reporta error.
 *   Efectos: escribe *reg al aceptar; reporta errores en as.
 * Restricciones:
 *   ninguna.
 */
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

/*
 * get_immediate — valida un inmediato de 18 bits con signo.
 *
 * Entrada:
 *   as  estado no NULL.
 *   line  línea del diagnóstico.
 *   text  operando no NULL.
 *   field  puntero no NULL para el campo.
 * Salida:
 *   1 si acepta; 0 si reporta error.
 *   Efectos: escribe *field al aceptar; reporta errores en as.
 * Restricciones:
 *   rango -131072–131071.
 */
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

/*
 * get_selector — valida un selector de cifrado.
 *
 * Entrada:
 *   as  estado no NULL.
 *   line  línea del diagnóstico.
 *   what  nombre k, i o w para el mensaje.
 *   text  operando no NULL.
 *   field  puntero no NULL para el campo.
 * Salida:
 *   1 si acepta; 0 si reporta error.
 *   Efectos: escribe *field al aceptar; reporta errores en as.
 * Restricciones:
 *   valor entre 0 y 3.
 */
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

/*
 * get_destination — resuelve un destino como desplazamiento relativo al PC.
 *
 * Entrada:
 *   as  estado con tabla de etiquetas.
 *   line  línea del diagnóstico.
 *   text  entero o nombre de etiqueta.
 *   pc  índice del bundle de salto.
 *   bits  ancho con signo, 18 o 22.
 *   field  puntero no NULL para el campo.
 * Salida:
 *   1 si resuelve y cabe; 0 si reporta error.
 *   Efectos: escribe *field al aceptar; reporta errores en as.
 * Restricciones:
 *   la primera pasada ya registró todas las etiquetas.
 * Referencia: isa.md §6.2, §11.2.
 */
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

/*
 * get_memory — descompone un operando imm(registro).
 *
 * Entrada:
 *   as  estado no NULL.
 *   line  línea del diagnóstico.
 *   text  operando modificable no NULL.
 *   imm  puntero no NULL para el inmediato.
 *   base  puntero no NULL para el registro base.
 * Salida:
 *   1 si ambos campos son válidos; 0 si falla.
 *   Efectos: separa text con NUL; escribe campos y reporta errores.
 * Restricciones:
 *   inmediato de 18 bits con signo.
 */
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

/* ---- PASADA 2: CODIFICACIÓN ------------------------------------------- */

/*
 * unit_name — nombra una unidad y sus slots.
 *
 * Entrada:
 *   unit  unidad de la instrucción.
 * Salida:
 *   cadena literal; no liberar.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *unit_name(Unit unit) {
    switch (unit) {
        case UNIT_ALU:    return "la ALU (slot 0 o 1)";
        case UNIT_CRYPTO: return "CRYPTO (slot 1)";
        case UNIT_BRU:    return "la BRU (slot 2)";
        default:          return "la LSU (slot 3)";
    }
}

/*
 * unit_fits — verifica la unidad admitida en un slot.
 *
 * Entrada:
 *   unit  unidad de la instrucción.
 *   slot  índice de slot 0–3.
 * Salida:
 *   1 si es compatible; 0 si no lo es.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int unit_fits(Unit unit, int slot) {
    switch (unit) {
        case UNIT_ALU:    return slot == 0 || slot == 1;
        case UNIT_CRYPTO: return slot == 1;
        case UNIT_BRU:    return slot == 2;
        default:          return slot == 3;
    }
}

/*
 * operand_count — obtiene la cantidad de operandos de una forma.
 *
 * Entrada:
 *   form  forma de instrucción.
 * Salida:
 *   cantidad esperada, entre 0 y 5.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
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

/* ==========================================================================
 * PASADA 2: CODIFICACIÓN
 * ======================================================================= */


/*
 * encode_slot — codifica una instrucción en su slot.
 *
 * Entrada:
 *   as  estado con etiquetas resueltas.
 *   line  línea del .asm.
 *   text  texto modificable no NULL.
 *   slot  slot 0–3.
 *   pc  índice del bundle actual.
 *   word  puntero no NULL para la palabra.
 *   written  puntero no NULL para el destino escrito.
 * Salida:
 *   1 si codifica; 0 si reporta error.
 *   Efectos: modifica text, *word y *written; acumula errores.
 * Restricciones:
 *   *written empieza en -1; nopi y slot vacío se codifican como cero.
 */
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

/*
 * encode_bundle — codifica cuatro slots y verifica destinos repetidos.
 *
 * Entrada:
 *   as  estado con etiquetas registradas.
 *   raw  bundle leído en la pasada 1.
 *   pc  índice del bundle.
 *   word  arreglo de cuatro palabras escribibles.
 *   text  puntero escribible para texto normalizado.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe word y reserva *text si hay cuatro slots; reporta errores.
 * Restricciones:
 *   el llamador inicializa *text a NULL; no comprueba dependencias entre
 *   bundles.
 */
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

/* ---- INTERFAZ --------------------------------------------------------- */

/* ==========================================================================
 * INTERFAZ DEL ENSAMBLADOR
 * ======================================================================= */


/* asm_free — véase el contrato en assembler.h. */
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

/* asm_assemble — véase el contrato en assembler.h. */
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
