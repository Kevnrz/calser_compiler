#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdarg.h>

// Guarda el nombre del archivo y carga sus líneas para mostrarlas.
void diag_init(const char *filename);

// Imprime un error en el formato de §8.2 y suma 1 al contador.
// line == 0: sin posición ni línea del fuente.
void diag_error(const char *category,
                int line, int column, int length,
                const char *fmt, ...);

// Igual, con va_list (para envolverla desde otros módulos).
void diag_verror(const char *category,
                 int line, int column, int length,
                 const char *fmt, va_list args);

// Última línea disponible del fuente (1 para un archivo vacío).
int  diag_last_line(void);

int  diag_count(void);   // cuántos errores se imprimieron
void diag_free(void);    // libera las líneas guardadas

#endif
