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

        if (grown == NULL)
            out_of_memory();

        lines = grown;
        line_capacity = capacity;
    }

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

    lines = NULL;
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
