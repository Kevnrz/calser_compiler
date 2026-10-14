#ifndef CODEGEN_H
#define CODEGEN_H
 
#include <stdint.h>
#include <stdio.h>
 
/* Emisor de código de la Fase 4. Contrato: fase4_contrato_comun.md §1 y §4.
 *
 * La Fase 4 no escribe texto directamente: arma en memoria una lista de
 * líneas (etiquetas y bundles) y al final la escribe al .asm o la entrega
 * al ensamblado. Sintaxis de salida: isa.md §11. */
 
/* Slots del bundle (isa.md §3). */
typedef enum { SLOT_ALU0 = 0, SLOT_ALU1 = 1, SLOT_BRU = 2, SLOT_LSU = 3 } Slot;
 
void cg_init(void);
void cg_free(void);
 
/* Etiqueta: vale el índice del bundle que la sigue (isa.md §11.1). */
void cg_label(const char *name);
 
/* Bundle completo. NULL en un slot = nopi. Las cadenas se copian. */
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3);
 
/* Una sola operación en su slot, y a continuación un bundle vacío de
   separación (versión segura, isa.md §9.1). Formato tipo printf. */
void cg_op(Slot slot, const char *fmt, ...);
 
/* Bundle vacío. */
void cg_gap(void);
 
/* Escribe todo en formato isa.md §11 (un bundle por línea, 4 slots). */
void cg_write(FILE *out);

/* Línea del fuente vigente: cada bundle que se emita desde ahora la guarda,
   para el .asm anotado. 0 = sin línea. */
void cg_set_line(int line);

/* Como cg_write, pero anotado (enunciado CE1108 §6.2): la dirección (índice
   de bundle) y la línea del fuente de cada bundle, y al final la tabla de
   etiquetas con su dirección. Todo lo agregado va en comentarios con #
   (isa.md §11.1): el archivo da los mismos bundles con ccom -x. */
void cg_write_listing(FILE *out);

/* Dirección (índice de bundle) de la etiqueta con ese nombre, o -1 si no
   está definida. Es la misma que muestra el .asm anotado. */
int cg_label_address(const char *name);
 
/* Acceso para el ensamblado: cantidad de líneas y cada línea ya en texto.
   Las etiquetas aparecen como líneas "nombre:" (no son bundles); un bundle
   empieza con '{'. */
int         cg_line_count(void);
const char *cg_line(int i);
 
/* Cantidad de bundles emitidos hasta ahora (índice del próximo bundle). */
int cg_bundle_count(void);
 
/* ---- Imagen de datos (contrato §4) --------------------------------------
 * Palabras de 32 bits que representan 0x0000-0x0FFF, en cero por defecto. */
#define CG_DATA_BYTES 0x1000
 
/* AGREGADO por Kevin: el contrato solo exponía la lectura. Escribe la
   palabra en la dirección de byte addr (múltiplo de 4, < CG_DATA_BYTES).
   Devuelve 0 si ok, -1 si la dirección es inválida. */
int cg_data_set(int addr, uint32_t value);
 
int             cg_data_size(void);   /* bytes usados, múltiplo de 4 */
const uint32_t *cg_data(void);        /* palabras desde la dirección 0 */
 
#endif
