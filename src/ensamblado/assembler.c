/*
 * assembler.c — Fase 5: ensamblado de etiquetas y bundles en dos pasadas.
 *
 * Separa etiquetas y bundles, asigna direcciones y codifica cada slot.
 * Resuelve destinos simbólicos como desplazamientos relativos al PC.
 *
 * Usa: assembler.h, isa_table.h; biblioteca C.
 * Lo usa: main.c, con líneas generadas o entrada -x.
 * Referencias: libro §1.1, §8.1.2; isa.md §3, §4, §6, §11.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "isa_table.h"

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

/* Unidades, formas y funct de las instrucciones (isa.md §6, §11.3): tabla única
 * en isa_table.h, compartida con el organizador de bundles. */

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

    char *text = isa_trim(copy);

    while (*text != '\0') {

        if (*text == '{') {
            char *close = strchr(text, '}');

            if (close == NULL) {
                asm_error(as, line, "falta '}' al final del bundle");
            }
            else if (*isa_trim(close + 1) != '\0') {
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

        char *name = isa_trim(text);

        if (!valid_label(name)) {
            asm_error(as, line, "nombre de etiqueta invalido: '%s'", name);
            break;
        }

        add_label(as, name, line);

        text = isa_trim(colon + 1);
    }

    free(copy);
}

/* ---- PASADA 2: OPERANDOS ---------------------------------------------- */

/* ==========================================================================
 * PASADA 2: OPERANDOS
 * ======================================================================= */


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
    int number = isa_parse_register(text);

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

    if (!isa_parse_integer(text, &value)) {
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

    if (!isa_parse_integer(text, &value) || value < 0 || value > 3) {
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

    if (!isa_parse_integer(text, &offset)) {

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

    return get_immediate(as, line, isa_trim(text), imm) &&
           get_register(as, line, isa_trim(open + 1), base);
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
static const char *unit_name(IsaUnit unit) {
    switch (unit) {
        case ISA_UNIT_ALU:    return "la ALU (slot 0 o 1)";
        case ISA_UNIT_CRYPTO: return "CRYPTO (slot 1)";
        case ISA_UNIT_BRU:    return "la BRU (slot 2)";
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
static int unit_fits(IsaUnit unit, int slot) {
    return (isa_unit_slots(unit) >> slot) & 1;
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
 * SlotOperands — operandos de un slot ya separados, con lo que hace falta
 * para diagnosticarlos y resolver destinos.
 */
typedef struct {
    Assembler *as;
    int line;                      /* línea del .asm, para los errores       */
    int pc;                        /* índice del bundle, para destinos       */
    char *operand[MAX_OPERANDS];   /* operandos recortados, en orden         */
} SlotOperands;

/*
 * split_operands — separa los operandos de una instrucción por comas.
 *
 * Entrada:
 *   rest  texto modificable posterior al mnemónico, sin espacios en los extremos.
 *   operand  arreglo de MAX_OPERANDS punteros escribibles.
 * Salida:
 *   cantidad de operandos encontrados; puede superar MAX_OPERANDS.
 *   Efectos: escribe NUL en cada coma y llena operand con los que caben.
 * Restricciones:
 *   texto vacío significa cero operandos; el llamador compara la cantidad
 *   con la que exige la forma antes de leer operand.
 */
static int split_operands(char *rest, char **operand) {
    int count = 0;

    if (*rest == '\0')
        return 0;

    char *start = rest;

    for (;;) {
        char *comma = strchr(start, ',');

        if (comma != NULL)
            *comma = '\0';

        if (count < MAX_OPERANDS)
            operand[count] = isa_trim(start);

        count++;

        if (comma == NULL)
            break;

        start = comma + 1;
    }

    return count;
}

/*
 * Codificadores por forma. Cada uno recibe los operandos ya contados y
 * completa *value con los campos de su forma (isa.md §4 y §6); si la forma
 * escribe un registro, deja su número en *written. Devuelven 1 si
 * codifican y 0 si reportaron un error; ante un error no tocan *written.
 */

/* ISA_FORM_R3: rd, rs1, rs2. */
static int encode_r3(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b, c;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_register(ops->as, ops->line, ops->operand[1], &b) ||
        !get_register(ops->as, ops->line, ops->operand[2], &c))
        return 0;

    *value |= FIELD_RD(a) | FIELD_RS1(b) | FIELD_RS2(c);
    *written = (int) a;

    return 1;
}

/* ISA_FORM_IMM: rd, rs1, imm. */
static int encode_imm(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b, c;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_register(ops->as, ops->line, ops->operand[1], &b) ||
        !get_immediate(ops->as, ops->line, ops->operand[2], &c))
        return 0;

    *value |= FIELD_RD(a) | FIELD_RS1(b) | c;
    *written = (int) a;

    return 1;
}

/* ISA_FORM_MOV: rd, imm. isa.md §6.1: el campo rs1 no se usa y vale cero. */
static int encode_mov(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_immediate(ops->as, ops->line, ops->operand[1], &b))
        return 0;

    *value |= FIELD_RD(a) | b;
    *written = (int) a;

    return 1;
}

/* ISA_FORM_BRANCH: rs1, rs2, destino. isa.md §4 (tipo B): rs2 en 25..22 y
 * rs1 en 21..18. */
static int encode_branch(SlotOperands *ops, uint32_t *value) {
    unsigned a, b, c;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_register(ops->as, ops->line, ops->operand[1], &b) ||
        !get_destination(ops->as, ops->line, ops->operand[2], ops->pc,
                         OFFSET_BITS, &c))
        return 0;

    *value |= FIELD_RD(b) | FIELD_RS1(a) | c;

    return 1;
}

/* ISA_FORM_JEFR: rd, rs1. */
static int encode_jefr(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_register(ops->as, ops->line, ops->operand[1], &b))
        return 0;

    *value |= FIELD_RD(a) | FIELD_RS1(b);
    *written = (int) a;

    return 1;
}

/* ISA_FORM_JEF: destino. isa.md §6.2: el campo rd no se usa y vale cero. */
static int encode_jef(SlotOperands *ops, uint32_t *value) {
    unsigned a;

    if (!get_destination(ops->as, ops->line, ops->operand[0], ops->pc,
                         TARGET_BITS, &a))
        return 0;

    *value |= a;

    return 1;
}

/* ISA_FORM_JAS: rd, destino. */
static int encode_jas(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_destination(ops->as, ops->line, ops->operand[1], ops->pc,
                         TARGET_BITS, &b))
        return 0;

    *value |= FIELD_RD(a) | b;
    *written = (int) a;

    return 1;
}

/* ISA_FORM_LOAD e ISA_FORM_STORE: registro, imm(rs1). Comparten campos. En
 * lw y lb el registro es el destino y se escribe. isa.md §6.3: en sw y sb la
 * fuente ocupa el campo 25..22 y no se escribe ningún registro. */
static int encode_memory(SlotOperands *ops, uint32_t *value, int *written,
                         int is_load) {
    unsigned a, b, c;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_memory(ops->as, ops->line, ops->operand[1], &b, &c))
        return 0;

    *value |= FIELD_RD(a) | FIELD_RS1(c) | b;

    if (is_load)
        *written = (int) a;

    return 1;
}

/* ISA_FORM_FEIS: rd, rs1, rs2, k, i. */
static int encode_feis(SlotOperands *ops, uint32_t *value, int *written) {
    unsigned a, b, c, d, e;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_register(ops->as, ops->line, ops->operand[1], &b) ||
        !get_register(ops->as, ops->line, ops->operand[2], &c) ||
        !get_selector(ops->as, ops->line, "k", ops->operand[3], &d) ||
        !get_selector(ops->as, ops->line, "i", ops->operand[4], &e))
        return 0;

    *value |= FIELD_RD(a) | FIELD_RS1(b) | FIELD_RS2(c) |
              FIELD_K(d) | FIELD_I(e);
    *written = (int) a;

    return 1;
}

/* ISA_FORM_RS1: rs1. auth y sanc: rd, rs2, k, i en cero (isa.md §6.4). */
static int encode_rs1(SlotOperands *ops, uint32_t *value) {
    unsigned a;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a))
        return 0;

    *value |= FIELD_RS1(a);

    return 1;
}

/* ISA_FORM_KV_WRITE: rs1, k, w. isa.md §6.4: la palabra w va en los bits del
 * campo i. */
static int encode_kv_write(SlotOperands *ops, uint32_t *value) {
    unsigned a, b, c;

    if (!get_register(ops->as, ops->line, ops->operand[0], &a) ||
        !get_selector(ops->as, ops->line, "k", ops->operand[1], &b) ||
        !get_selector(ops->as, ops->line, "w", ops->operand[2], &c))
        return 0;

    *value |= FIELD_RS1(a) | FIELD_K(b) | FIELD_I(c);

    return 1;
}

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

    text = isa_trim(text);

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

    const IsaInstruction *ins = isa_find(text);

    // isa.md §11.4: mnemónico desconocido. nopi está en la tabla para el
    // organizador, pero aquí ya se codificó arriba: con operandos no es una
    // instrucción válida.
    if (ins == NULL || ins->unit == ISA_UNIT_NONE) {
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
    SlotOperands ops = { .as = as, .line = line, .pc = pc };
    int count = split_operands(isa_trim(rest), ops.operand);

    if (count != isa_operand_count(ins->form)) {
        asm_error(as, line, "'%s' espera %d operandos, se recibieron %d",
                  ins->name, isa_operand_count(ins->form), count);
        return 0;
    }

    // isa.md §11.3: en el slot 1, sel = 1 para CRYPTO y 0 para la ALU.
    uint32_t value =
        FIELD_SEL(ins->unit == ISA_UNIT_CRYPTO) | FIELD_FUNCT(ins->funct);

    int ok = 1;

    switch (ins->form) {
        case ISA_FORM_R3:       ok = encode_r3(&ops, &value, written); break;
        case ISA_FORM_IMM:      ok = encode_imm(&ops, &value, written); break;
        case ISA_FORM_MOV:      ok = encode_mov(&ops, &value, written); break;
        case ISA_FORM_BRANCH:   ok = encode_branch(&ops, &value); break;
        case ISA_FORM_JEFR:     ok = encode_jefr(&ops, &value, written); break;
        case ISA_FORM_JEF:      ok = encode_jef(&ops, &value); break;
        case ISA_FORM_JAS:      ok = encode_jas(&ops, &value, written); break;
        case ISA_FORM_LOAD:     ok = encode_memory(&ops, &value, written, 1); break;
        case ISA_FORM_STORE:    ok = encode_memory(&ops, &value, written, 0); break;
        case ISA_FORM_FEIS:     ok = encode_feis(&ops, &value, written); break;
        case ISA_FORM_RS1:      ok = encode_rs1(&ops, &value); break;
        case ISA_FORM_KV_WRITE: ok = encode_kv_write(&ops, &value); break;
        case ISA_FORM_NONE:     break;
    }

    if (!ok)
        return 0;

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
        slot[i] = isa_trim(slot[i]);
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
