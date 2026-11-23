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
#include "modules.h"
#include "diagnostics_json.h"

// Nombre del archivo tal como se pasó a ccom (no es dueño de la cadena).
static const char *diag_filename = NULL;

// Líneas del archivo fuente, sin el salto de línea final.
static char **diag_lines = NULL;
static int diag_line_count = 0;

static int diag_errors = 0;
/* §12: contexto de procedencia; diag_select directo corresponde al principal. */
static int diag_file;
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
    diag_errors = 0;
    diag_select(filename);
}

/* §12: cambiar de fuente conserva todos los errores ya reportados. */
void diag_select(const char *filename) {
    diag_free();
    diag_filename = filename;
    diag_file = 0;

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

/* §12: seleccionar una fuente registrada también fija su contexto. */
void diag_select_file(int file) {
    const char *path = modules_file_name(file);
    if (path) {
        diag_select(path);
        diag_file = file;
    }
}

DiagPosition diag_range(int line, int column, int end_line, int end_column) {
    if (line < 1 || end_line < line || end_line > diag_line_count || column < 1 ||
        end_column < 1 || (size_t)column > strlen(diag_lines[line - 1]) ||
        (size_t)end_column > strlen(diag_lines[end_line - 1])) return (DiagPosition) {0};
    size_t start = diag_offsets[line - 1] + (size_t)column - 1;
    size_t end = diag_offsets[end_line - 1] + (size_t)end_column;
    if (end <= start) return (DiagPosition) {0};
    return (DiagPosition) {1, line, column, start, end};
}

/* §12: la semántica solo selecciona procedencia; no adapta mensajes/rangos. */
void diag_semantic_verror(int file, int line, int column, int length,
                          const char *fmt, va_list args) {
    diag_select_file(file);
    diag_verror("error semantico", line, column, length, fmt, args);
}

static void notify_observer(const char *category, DiagPosition position,
                             const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    diag_observer(category, position, fmt, args, diag_observer_data);
    va_end(args);
}

/* §12: ÚNICA regla de anclaje de módulos en JSON, común a todas las fases.
 * No cambia la fuente seleccionada: el lexer/parser puede seguir reportando
 * errores del mismo módulo. El contador se incrementa una sola vez. */
static void observe_error(const char *category, DiagPosition position,
                           int line, int column, const char *fmt, va_list args) {
    const char *name = modules_import_name(diag_file);
    if (diag_json_active() && name) {
        va_list copy;
        va_copy(copy, args);
        int size = vsnprintf(NULL, 0, fmt, copy);
        va_end(copy);
        if (size < 0) { fprintf(stderr, "Error: formato de diagnostico invalido\n"); exit(EXIT_FAILURE); }
        char *message = malloc((size_t)size + 1);
        if (!message) { fprintf(stderr, "Error: out of memory\n"); exit(EXIT_FAILURE); }
        va_copy(copy, args);
        vsnprintf(message, (size_t)size + 1, fmt, copy);
        va_end(copy);
        notify_observer(category, modules_import_range(diag_file),
                        "error en el modulo '%s': %s (%s.cls:%d:%d)",
                        name, message, name, line, column);
        free(message);
    } else {
        va_list copy;
        va_copy(copy, args);
        diag_observer(category, position, fmt, copy, diag_observer_data);
        va_end(copy);
    }
}

/* Presentación común con rango opcional aportado por el analizador. */
static void report_error(const char *category,
                         int line, int column, int length,
                         const char *fmt, va_list args, const DiagPosition *given) {
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
        if (given) position = *given;
        observe_error(category, position, line, column, fmt, args);
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

/* Conserva las posiciones calculadas por el reconocedor, incluido EOF después
 * de un salto de línea que no tiene texto cargado en diag_lines.
 */
void diag_error_at(const char *category, DiagPosition position, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    report_error(category, position.line, position.column,
                 (int)(position.end - position.start), fmt, args, &position);
    va_end(args);
}

void diag_verror(const char *category, int line, int column, int length,
                 const char *fmt, va_list args) {
    report_error(category, line, column, length, fmt, args, NULL);
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
