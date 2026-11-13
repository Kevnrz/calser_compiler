/*
 * diagnostics.c — Fases 1–3: presentación de errores con posición del fuente.
 *
 * Carga líneas del archivo y mantiene un contador de errores. Los mensajes
 * incluyen el texto y un señalador que conserva las tabulaciones.
 *
 * Usa: diagnostics.h; biblioteca C.
 * Lo usa: lexer.l, parser.y, semantic.c, semantic_expr.c y main.c.
 * Referencias: enunciado §8.2 (referencia ya citada en el código).
 */

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
/* Offsets de inicio antes de quitar CR/LF: el protocolo cuenta bytes reales. */
static size_t *diag_offsets = NULL;
static DiagObserver diag_observer = NULL;
static void *diag_observer_data = NULL;
static int diag_emit_text = 1;

void diag_set_observer(DiagObserver observer, void *data, int emit_text) {
    diag_observer = observer;
    diag_observer_data = data;
    diag_emit_text = emit_text;
}


/* ==========================================================================
 * CARGA Y REPORTE DE ERRORES
 * ======================================================================= */


/* diag_init — véase el contrato en diagnostics.h. */
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
    size_t offset = 0;

    while ((length = getline(&buffer, &size, file)) != -1) {

        size_t raw_length = (size_t) length;

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
            size_t *offsets = realloc(diag_offsets, capacity * sizeof(size_t));
            if (!offsets) {
                fprintf(stderr, "Error: out of memory\n");
                exit(EXIT_FAILURE);
            }
            diag_offsets = offsets;
        }

        char *copy = malloc((size_t) length + 1);

        if (copy == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        memcpy(copy, buffer, (size_t) length + 1);

        diag_offsets[diag_line_count] = offset;
        offset += raw_length;
        diag_lines[diag_line_count] = copy;
        diag_line_count++;
    }

    free(buffer);
    fclose(file);
}

/* diag_verror — véase el contrato en diagnostics.h. */
void diag_verror(const char *category,
                 int line, int column, int length,
                 const char *fmt, va_list args) {
    const char *filename = diag_filename ? diag_filename : "ccom";

    // Solo se muestra la posición si la línea existe en el archivo.
    const char *source =
        (line > 0 && line <= diag_line_count) ? diag_lines[line - 1] : NULL;

    diag_errors++;
    if (diag_observer) {
        DiagPosition position = {0};
        /* No inventar posiciones para diagnósticos generales. Al final de una
         * línea se conserva una inserción vacía, sin subrayar el siguiente token. */
        if (source && column > 0 && (size_t) column <= strlen(source) + 1) {
            size_t local = (size_t) column - 1;
            size_t count = length > 0 ? (size_t) length : 1;
            size_t available = strlen(source) - local;
            if (count > available) count = available;
            position = (DiagPosition) {1, line, column,
                diag_offsets[line - 1] + local, diag_offsets[line - 1] + local + count};
        }
        va_list copy;
        va_copy(copy, args);
        diag_observer(category, position, fmt, copy, diag_observer_data);
        va_end(copy);
    }
    if (!diag_emit_text) return;

    /*
     * §8.2:
     *
     * <archivo>:<linea>:<columna>: <categoria>: <mensaje>
     * <linea> | <texto de la línea>
     *         | <señalador>
     */
    if (source == NULL && strcmp(category, "error sintactico") != 0)
        fprintf(stderr, "%s: %s: ", filename, category);
    else
        fprintf(stderr, "%s:%d:%d: %s: ", filename, line, column, category);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    if (source == NULL)
        return;

    fprintf(stderr, "%6d |", line);
    if (source[0] != '\0')
        fprintf(stderr, " %s", source);
    fputc('\n', stderr);
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

/* diag_error — véase el contrato en diagnostics.h. */
void diag_error(const char *category,
                int line, int column, int length,
                const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    diag_verror(category, line, column, length, fmt, args);
    va_end(args);
}

/* ==========================================================================
 * CONSULTAS Y LIBERACIÓN
 * ======================================================================= */


/* diag_last_line — véase el contrato en diagnostics.h. */
int diag_last_line(void) {
    return diag_line_count > 0 ? diag_line_count : 1;
}

/* diag_count — véase el contrato en diagnostics.h. */
int diag_count(void) {
    return diag_errors;
}

/* diag_free — véase el contrato en diagnostics.h. */
void diag_free(void) {
    for (int i = 0; i < diag_line_count; i++)
        free(diag_lines[i]);

    free(diag_lines);
    free(diag_offsets);
    diag_offsets = NULL;

    diag_lines = NULL;
    diag_line_count = 0;
}
