/*
 * diagnostics.h — Fases 1–3: interfaz de presentación de errores con posición del fuente.
 *
 * Expone carga del fuente, reporte de errores y contador compartido.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: diagnostics.c; lexer.l, parser.y, semantic.c y main.c.
 * Referencias: enunciado §8.2 (referencia ya citada en el código).
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdarg.h>

/* ==========================================================================
 * CARGA Y REPORTE DE ERRORES
 * ======================================================================= */

/*
 * diag_init — carga las líneas del fuente para diagnósticos.
 *
 * Entrada:
 *   filename  nombre de archivo prestado o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera fuente anterior, reinicia contador y carga líneas.
 * Restricciones:
 *   filename vive hasta diag_free; si no abre muestra diagnósticos sin
 *   texto.
 */
void diag_init(const char *filename);

/*
 * diag_error — reporta un error con posición y formato.
 *
 * Entrada:
 *   category  categoría no NULL.
 *   line  línea desde 1 o 0 sin posición.
 *   column  columna desde 1.
 *   length  longitud del señalador.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe stderr e incrementa contador.
 * Restricciones:
 *   diag_init previo; usa diag_verror.
 */
void diag_error(const char *category,
                int line, int column, int length,
                const char *fmt, ...);

/*
 * diag_verror — reporta un error con argumentos ya agrupados.
 *
 * Entrada:
 *   category  categoría no NULL.
 *   line  línea desde 1 o 0 sin posición.
 *   column  columna desde 1.
 *   length  longitud del señalador, mínimo efectivo 1.
 *   fmt  formato printf.
 *   args  lista de argumentos válida.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe stderr e incrementa contador; consume args.
 * Restricciones:
 *   diag_init previo; una línea inexistente no muestra texto del fuente.
 */
void diag_verror(const char *category,
                 int line, int column, int length,
                 const char *fmt, va_list args);

/* ==========================================================================
 * CONSULTAS Y LIBERACIÓN
 * ======================================================================= */

/*
 * diag_last_line — consulta la última línea disponible.
 *
 * Entrada: ninguna
 * Salida:
 *   cantidad de líneas; 1 si el archivo está vacío.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int  diag_last_line(void);

/*
 * diag_count — consulta la cantidad de errores reportados.
 *
 * Entrada: ninguna
 * Salida:
 *   contador desde el último diag_init.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int  diag_count(void);   /* cuántos errores se imprimieron */

/*
 * diag_free — libera las líneas del fuente de diagnóstico.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera líneas y reinicia su cantidad; conserva nombre y contador de errores.
 * Restricciones:
 *   no reinicia el contador de errores.
 */
void diag_free(void);    // libera las líneas guardadas

#endif
