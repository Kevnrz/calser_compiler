#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

/* Lista de líneas (etiquetas y bundles), en orden de emisión. */
static char **lines = NULL;
static int line_count = 0;
static int line_capacity = 0;
static int bundles = 0;

/* Para el .asm anotado, por cada línea: la línea del fuente que la generó
   (0 = ninguna) y su dirección (el bundle; en una etiqueta, el que la sigue). */
static int *source_lines = NULL;
static int *addresses = NULL;
static int current_source_line = 0;

/* Imagen de datos y bytes usados (mayor dirección escrita + 4). */
static uint32_t data[CG_DATA_BYTES / 4];
static int data_used = 0;

static void out_of_memory(void) {
    fprintf(stderr, "ccom: error interno: memoria insuficiente (codegen)\n");
    exit(EXIT_FAILURE);
}

// Agrega una línea (toma posesión de text).
static void push_line(char *text) {
    if (line_count == line_capacity) {
        int capacity = line_capacity == 0 ? 256 : line_capacity * 2;
        char **grown = realloc(lines, (size_t) capacity * sizeof(char *));
        int *grown_sources =
            realloc(source_lines, (size_t) capacity * sizeof(int));
        int *grown_addresses =
            realloc(addresses, (size_t) capacity * sizeof(int));

        if (grown == NULL || grown_sources == NULL || grown_addresses == NULL)
            out_of_memory();

        lines = grown;
        source_lines = grown_sources;
        addresses = grown_addresses;
        line_capacity = capacity;
    }

    source_lines[line_count] = current_source_line;
    addresses[line_count] = bundles;
    lines[line_count++] = text;
}

void cg_init(void) {
    cg_free();
    memset(data, 0, sizeof(data));
    data_used = 0;
}

void cg_free(void) {
    for (int i = 0; i < line_count; i++)
        free(lines[i]);

    free(lines);
    free(source_lines);
    free(addresses);

    lines = NULL;
    source_lines = NULL;
    addresses = NULL;
    current_source_line = 0;
    line_count = 0;
    line_capacity = 0;
    bundles = 0;
}

// isa.md §11.1: una etiqueta vale el índice del bundle que la sigue.
void cg_label(const char *name) {
    size_t size = strlen(name) + 2;
    char *text = malloc(size);

    if (text == NULL)
        out_of_memory();

    snprintf(text, size, "%s:", name);
    push_line(text);
}

// isa.md §11.1: cuatro slots siempre presentes, slot 0 a slot 3.
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3) {
    const char *slot[4] = {
        s0 ? s0 : "nopi",
        s1 ? s1 : "nopi",
        s2 ? s2 : "nopi",
        s3 ? s3 : "nopi"
    };

    size_t size = 8;   // "{ " " }" y los tres " | "

    for (int i = 0; i < 4; i++)
        size += strlen(slot[i]) + 3;

    char *text = malloc(size);

    if (text == NULL)
        out_of_memory();

    snprintf(text, size, "{ %s | %s | %s | %s }",
             slot[0], slot[1], slot[2], slot[3]);

    push_line(text);
    bundles++;
}

void cg_gap(void) {
    cg_bundle(NULL, NULL, NULL, NULL);
}

// Versión segura (isa.md §9.1): la operación, y un bundle vacío después.
void cg_op(Slot slot, const char *fmt, ...) {
    char op[256];
    va_list args;

    va_start(args, fmt);
    int length = vsnprintf(op, sizeof(op), fmt, args);
    va_end(args);

    if (length < 0 || (size_t) length >= sizeof(op)) {
        fprintf(stderr, "ccom: error interno: operacion demasiado larga\n");
        exit(EXIT_FAILURE);
    }

    const char *s[4] = { NULL, NULL, NULL, NULL };

    s[slot] = op;

    cg_bundle(s[0], s[1], s[2], s[3]);
    cg_gap();
}

void cg_write(FILE *out) {
    for (int i = 0; i < line_count; i++) {
        // Las etiquetas van pegadas al margen; los bundles con sangría.
        if (lines[i][0] == '{')
            fprintf(out, "  %s\n", lines[i]);
        else
            fprintf(out, "%s\n", lines[i]);
    }
}

void cg_set_line(int line) {
    current_source_line = line > 0 ? line : 0;
}

// Etiquetas internas de una expansión (no son de la tabla de la Fase 3).
static int internal_label(const char *name) {
    return strncmp(name, "__ccom_arith_", 13) == 0 ||
           strncmp(name, "__ccom_array_", 13) == 0 ||
           strncmp(name, ".L_crypto_", 10) == 0;
}

// Escribe text y rellena con espacios hasta width columnas.
static void write_padded(FILE *out, const char *text, int width) {
    int length = (int) strlen(text);

    fputs(text, out);

    for (int i = length; i < width; i++)
        fputc(' ', out);
}

void cg_write_listing(FILE *out) {
    static const char *const EMPTY = "{ nopi | nopi | nopi | nopi }";

    // Columna de los comentarios: el texto más largo, con un mínimo.
    int width = 44;

    for (int i = 0; i < line_count; i++) {
        int length = (int) strlen(lines[i]) + (lines[i][0] == '{' ? 2 : 0);

        if (length + 2 > width)
            width = length + 2;
    }

    // La línea del fuente solo se muestra cuando cambia. Los bundles vacíos
    // de separación no la llevan.
    int shown_line = 0;
    char text[512];

    for (int i = 0; i < line_count; i++) {

        if (lines[i][0] != '{') {
            write_padded(out, lines[i], width);
            fprintf(out, "# bundle %d\n", addresses[i]);
            continue;
        }

        snprintf(text, sizeof(text), "  %s", lines[i]);
        write_padded(out, text, width);
        fprintf(out, "# %04d", addresses[i]);

        if (source_lines[i] > 0 &&
            source_lines[i] != shown_line &&
            strcmp(lines[i], EMPTY) != 0) {

            fprintf(out, "  linea %d", source_lines[i]);
            shown_line = source_lines[i];
        }

        fputc('\n', out);
    }

    // Tabla de etiquetas en orden de dirección; en el empate, el de emisión.
    fprintf(out, "\n# Tabla de etiquetas (direccion = indice de bundle)\n");

    int previous = -1;

    for (;;) {
        int next = -1;

        for (int i = 0; i < line_count; i++) {

            if (lines[i][0] != '{' && addresses[i] > previous &&
                (next < 0 || addresses[i] < next))
                next = addresses[i];
        }

        if (next < 0)
            break;

        for (int i = 0; i < line_count; i++) {

            if (lines[i][0] == '{' || addresses[i] != next)
                continue;

            // La etiqueta sin los dos puntos.
            size_t length = strlen(lines[i]);
            snprintf(text, sizeof(text), "%.*s",
                     (int) (length > 0 ? length - 1 : 0), lines[i]);

            fprintf(out, "#   %-28s %5d%s\n", text, next,
                    internal_label(text) ? "  (interna)" : "");
        }

        previous = next;
    }
}

int cg_line_count(void) {
    return line_count;
}

const char *cg_line(int i) {
    return (i >= 0 && i < line_count) ? lines[i] : NULL;
}

int cg_bundle_count(void) {
    return bundles;
}

int cg_data_set(int addr, uint32_t value) {
    if (addr < 0 || addr + 4 > CG_DATA_BYTES || addr % 4 != 0)
        return -1;

    data[addr / 4] = value;

    if (addr + 4 > data_used)
        data_used = addr + 4;

    return 0;
}

int cg_data_size(void) {
    return data_used;
}

const uint32_t *cg_data(void) {
    return data;
}
