#define _POSIX_C_SOURCE 200809L   // getline

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"

// Nombre del archivo tal como se pasó a ccom (no es dueño de la cadena).
static const char *diag_filename = NULL;

// Líneas del archivo fuente, sin el salto de línea final.
static char **diag_lines = NULL;
static int diag_line_count = 0;

static int diag_errors = 0;

void diag_init(const char *filename) {
    diag_free();

    diag_filename = filename;
    diag_errors = 0;

    // Si no se puede abrir, los errores salen solo con la primera línea.
    FILE *file = filename ? fopen(filename, "r") : NULL;

    if (file == NULL)
        return;

    char *buffer = NULL;
    size_t size = 0;
    ssize_t length;
    int capacity = 0;

    while ((length = getline(&buffer, &size, file)) != -1) {

        // Quitar el salto de línea (y el \r de los archivos de Windows).
        while (length > 0 &&
               (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
            buffer[--length] = '\0';

        if (diag_line_count == capacity) {

            capacity = capacity == 0 ? 64 : capacity * 2;

            char **lines = realloc(diag_lines, capacity * sizeof(char *));

            if (lines == NULL) {
                fprintf(stderr, "Error: out of memory\n");
                exit(EXIT_FAILURE);
            }

            diag_lines = lines;
        }

        char *copy = malloc((size_t) length + 1);

        if (copy == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        memcpy(copy, buffer, (size_t) length + 1);

        diag_lines[diag_line_count] = copy;
        diag_line_count++;
    }

    free(buffer);
    fclose(file);
}

void diag_verror(const char *category,
                 int line, int column, int length,
                 const char *fmt, va_list args) {
    const char *filename = diag_filename ? diag_filename : "ccom";

    // Solo se muestra la posición si la línea existe en el archivo.
    const char *source =
        (line > 0 && line <= diag_line_count) ? diag_lines[line - 1] : NULL;

    diag_errors++;

    /*
     * §8.2:
     *
     * <archivo>:<linea>:<columna>: <categoria>: <mensaje>
     * <linea> | <texto de la línea>
     *         | <señalador>
     */
    if (source == NULL)
        fprintf(stderr, "%s: %s: ", filename, category);
    else
        fprintf(stderr, "%s:%d:%d: %s: ", filename, line, column, category);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    if (source == NULL)
        return;

    fprintf(stderr, "%6d | %s\n", line, source);
    fprintf(stderr, "%6s | ", "");

    // El señalador copia las tabulaciones de la línea original, para quedar
    // alineado sin importar el ancho de tabulación de la terminal.
    size_t source_length = strlen(source);

    for (int i = 0; i < column - 1; i++) {

        if ((size_t) i < source_length && source[i] == '\t')
            fputc('\t', stderr);
        else
            fputc(' ', stderr);
    }

    // Un ^ por cada carácter del lexema (mínimo 1).
    if (length < 1)
        length = 1;

    for (int i = 0; i < length; i++)
        fputc('^', stderr);

    fprintf(stderr, "\n");
}

void diag_error(const char *category,
                int line, int column, int length,
                const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    diag_verror(category, line, column, length, fmt, args);
    va_end(args);
}

int diag_count(void) {
    return diag_errors;
}

void diag_free(void) {
    for (int i = 0; i < diag_line_count; i++)
        free(diag_lines[i]);

    free(diag_lines);

    diag_lines = NULL;
    diag_line_count = 0;
}
