/*
 * diagnostics.h — Fases 1–3: interfaz de presentación de errores con posición del fuente.
 *
 * Expone carga del fuente, reporte de errores y contador compartido.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: diagnostics.c; lexer.l, parser.y, semantic.c, semantic_expr.c y main.c.
 * Referencias: enunciado §8.2 (referencia ya citada en el código).
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdarg.h>
#include <stddef.h>

/* Ubicación estructurada: offsets UTF-8 [start,end), línea/columna base 1.
 * located=0 indica que no hay una ubicación representable. Un rango vacío puede señalar EOF o
 * un terminador faltante. Los datos se calculan sobre las líneas cargadas. */
typedef struct {
    int located, line, column;
    size_t start, end;
} DiagPosition;

/* El observador recibe argumentos prestados solo durante la llamada. Debe
 * usar va_copy si formatea el mensaje y copiar cualquier dato que conserve.
 * Se instala antes de analizar; diag_init/diag_free no lo desconectan. */
typedef void (*DiagObserver)(const char *category, DiagPosition position,
                             const char *format, va_list args, void *data);
void diag_set_observer(DiagObserver observer, void *data, int emit_text);

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
/* §12: carga/selecciona otra fuente sin reiniciar el contador ni el observador.
 * La ruta es prestada y debe vivir hasta la siguiente selección o diag_free. */
void diag_select(const char *filename);
/* §12: selecciona una ruta registrada y conserva su índice como contexto de
 * los siguientes diagnósticos. Un índice no registrado no cambia la fuente. */
void diag_select_file(int file);
/* §12: rango UTF-8 de la fuente seleccionada, último carácter inclusivo.
 * Admite varias líneas; located=0 si los extremos no existen en la fuente. */
DiagPosition diag_range(int line, int column, int end_line, int end_column);
/* §12: selecciona la fuente del nodo; la regla común adapta el JSON. */
void diag_semantic_verror(int file, int line, int column, int length,
                          const char *fmt, va_list args);

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
/* Posición explícita calculada por el analizador, también para EOF vacío.
 * Conserva el rango original al notificar al observador del protocolo.
 */
void diag_error_at(const char *category, DiagPosition position, const char *fmt, ...);

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
