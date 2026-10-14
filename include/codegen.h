/*
 * codegen.h — Fase 4: interfaz de emisión del IR, bundles e imagen de datos.
 *
 * Expone slots, instrucciones, IR, emisión, listados e imagen de datos.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: tipos de la biblioteca C y declaraciones propias.
 * Lo usa: codegen.c; generate.c, codegen_crypto.c, main.c y codegen_*.
 * Referencias: isa.md §3, §9, §11; fase4_contrato_comun.md §1, §4, §5.
 */

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
 
/* Representación previa al empaquetado de instrucciones. */
typedef enum {
    CG_UNIT_NONE, CG_UNIT_ALU, CG_UNIT_BRU, CG_UNIT_LSU,
    CG_UNIT_CRYPTO, CG_UNIT_UNKNOWN
} CGUnit;

typedef enum {
    CG_EFFECT_NONE = 0,
    CG_EFFECT_MEMORY_READ = 1u << 0,
    CG_EFFECT_MEMORY_WRITE = 1u << 1,
    CG_EFFECT_CONTROL = 1u << 2,
    CG_EFFECT_CALL = 1u << 3,
    CG_EFFECT_CRYPTO_STATE = 1u << 4,
    CG_EFFECT_CONDITIONAL_WRITE = 1u << 5,
    CG_EFFECT_PIPELINE_STALL = 1u << 6,
    CG_EFFECT_PC_RELATIVE = 1u << 7,
    CG_EFFECT_BARRIER = 1u << 8
} CGEffect;

typedef struct {
    const char *text;
    CGUnit unit;
    Slot original_slot;
    unsigned allowed_slots; /* Bit 0..3: ALU normal admite 0 y 1. */
    uint16_t reads, writes; /* Bits ISA 0..15; writes excluye rz. */
    unsigned effects;
    unsigned result_distance; /* Escritura en B: consumidor desde B+2. */
    int source_line;
    int known; /* 0: no mover; metadatos incompletos y barrera. */
} CGInstruction;

typedef enum {
    CG_IR_OPERATION, CG_IR_LABEL, CG_IR_FIXED_BUNDLE, CG_IR_GAP
} CGIRKind;
typedef enum { CG_GAP_AUTOMATIC, CG_GAP_EXPLICIT } CGGapReason;

typedef struct {
    CGIRKind kind;
    int source_line;
    int bundle_address; /* Dirección de la emisión conservadora actual. */
    union {
        CGInstruction instruction;
        CGInstruction slots[4];
        const char *label;
        CGGapReason gap_reason;
    } data;
} CGIRRecord;

/* ==========================================================================
 * DESCRIPCIÓN, IR Y ORGANIZACIÓN
 * ======================================================================= */

/*
 * cg_describe_instruction — describe unidad, registros y efectos de una instrucción.
 *
 * Entrada:
 *   text  texto prestado o NULL.
 *   slot  slot de emisión 0–3.
 *   source_line  línea fuente, 0 si ausente.
 *   out  salida escribible o NULL.
 * Salida:
 *   0 si conocida; 1 si desconocida/inválida.
 *   Efectos: llena *out sin tomar propiedad del texto.
 * Restricciones:
 *   texto menor de 256 bytes; desconocida queda como barrera; el
 *   ensamblador valida/codifica finalmente.
 */
int cg_describe_instruction(const char *text, Slot slot, int source_line,
                            CGInstruction *out);

/*
 * cg_ir_count — consulta la cantidad de registros del IR.
 *
 * Entrada: ninguna
 * Salida:
 *   cantidad de registros, incluidos etiquetas, bundles y huecos.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int cg_ir_count(void);
/*
 * cg_ir_record — consulta un registro del IR.
 *
 * Entrada:
 *   index  índice desde 0.
 * Salida:
 *   puntero interno de solo lectura o NULL fuera de rango.
 *   Efectos: ninguno.
 * Restricciones:
 *   válido hasta nueva emisión, cg_free o cg_init.
 */
const CGIRRecord *cg_ir_record(int index);

/*
 * cg_schedule — organiza regiones y reconstruye direcciones finales.
 *
 * Entrada: ninguna
 * Salida:
 *   0 si finaliza, incluso al conservar emisión; 1 si falla.
 *   Efectos: reconstruye líneas/bundles y marca finalized.
 * Restricciones:
 *   idempotente; no emitir después; efectos desconocidos o saltos
 *   numéricos conservan el programa completo.
 */
int cg_schedule(void);

/* ==========================================================================
 * EMISIÓN Y CONSULTAS
 * ======================================================================= */

/*
 * cg_init — reinicia el emisor y la imagen de datos.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera emisión anterior y pone imagen y tamaño a cero.
 * Restricciones:
 *   ninguna.
 */
void cg_init(void);
/*
 * cg_free — libera el IR y las líneas de salida.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera memoria, reinicia línea y finalized.
 * Restricciones:
 *   la imagen de datos se reinicia con cg_init.
 */
void cg_free(void);
 
/*
 * cg_label — registra una etiqueta en IR y salida.
 *
 * Entrada:
 *   name  nombre no NULL sin dos puntos.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia nombre y agrega línea de etiqueta.
 * Restricciones:
 *   antes de cg_schedule; su dirección es el índice del bundle siguiente.
 */
void cg_label(const char *name);
 
/*
 * cg_bundle — registra un bundle fijo de cuatro slots.
 *
 * Entrada:
 *   s0  texto de slot 0 o NULL.
 *   s1  texto de slot 1 o NULL.
 *   s2  texto de slot 2 o NULL.
 *   s3  texto de slot 3 o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia/describe slots en IR y emite una línea.
 * Restricciones:
 *   antes de cg_schedule; los bundles fijos delimitan regiones.
 */
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3);
 
/*
 * cg_op — registra una operación con separación conservadora.
 *
 * Entrada:
 *   slot  slot preferido 0–3.
 *   fmt  formato printf de la instrucción.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: copia operación al IR y emite operación más hueco automático.
 * Restricciones:
 *   texto menor de 256 bytes; antes de cg_schedule; slot/texto inválido
 *   termina el proceso.
 */
void cg_op(Slot slot, const char *fmt, ...);
 
/*
 * cg_gap — registra una separación explícita.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: agrega IR y bundle de cuatro nopi.
 * Restricciones:
 *   se conserva al organizar; antes de cg_schedule.
 */
void cg_gap(void);
 
/*
 * cg_write — escribe las líneas actuales del emisor.
 *
 * Entrada:
 *   out  flujo no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe etiquetas y bundles.
 * Restricciones:
 *   organizar antes si se desea calendario compacto; comprobar errores del
 *   flujo en el llamador.
 */
void cg_write(FILE *out);

/*
 * cg_set_line — selecciona la línea fuente de la emisión posterior.
 *
 * Entrada:
 *   line  línea fuente, valores no positivos indican sin línea.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza current_source_line.
 * Restricciones:
 *   ninguna.
 */
void cg_set_line(int line);

/*
 * cg_write_listing — escribe ensamblador anotado y tabla de etiquetas.
 *
 * Entrada:
 *   out  flujo no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe direcciones, líneas por bundle/slot y tabla de etiquetas.
 * Restricciones:
 *   calendario actual; las anotaciones con # no cambian el ensamblado.
 */
void cg_write_listing(FILE *out);

/*
 * cg_label_address — consulta la dirección de una etiqueta.
 *
 * Entrada:
 *   name  nombre no NULL sin dos puntos.
 * Salida:
 *   índice de bundle o -1 si no está definida.
 *   Efectos: ninguno.
 * Restricciones:
 *   después de cg_schedule entrega dirección final; antes, conservadora.
 */
int cg_label_address(const char *name);
 
/*
 * cg_line_count — consulta la cantidad de líneas de salida.
 *
 * Entrada: ninguna
 * Salida:
 *   cantidad de etiquetas y bundles textuales.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int         cg_line_count(void);
/*
 * cg_line — consulta una línea del ensamblador.
 *
 * Entrada:
 *   i  índice desde 0.
 * Salida:
 *   cadena interna o NULL fuera de rango.
 *   Efectos: ninguno.
 * Restricciones:
 *   no liberar; cg_schedule y nuevas emisiones pueden invalidarla.
 */
const char *cg_line(int i);
 
/*
 * cg_bundle_count — consulta el índice del próximo bundle.
 *
 * Entrada: ninguna
 * Salida:
 *   cantidad de bundles emitidos.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int cg_bundle_count(void);
 
/* ==========================================================================
 * IMAGEN DE DATOS
 * ======================================================================= */

/* Palabras de 32 bits para 0x0000–0x0FFF, cero por defecto (contrato §4). */
#define CG_DATA_BYTES 0x1000
 
/*
 * cg_data_set — escribe una palabra en la imagen de datos.
 *
 * Entrada:
 *   addr  dirección en bytes.
 *   value  palabra de 32 bits.
 * Salida:
 *   0 si escribe; -1 si la dirección no es válida.
 *   Efectos: actualiza imagen y máximo de bytes usados.
 * Restricciones:
 *   addr múltiplo de 4 entre 0 y 4092.
 */
int cg_data_set(int addr, uint32_t value);
 
/*
 * cg_data_size — consulta los bytes usados de la imagen.
 *
 * Entrada: ninguna
 * Salida:
 *   máxima dirección escrita + 4, o cero; múltiplo de 4.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
int             cg_data_size(void);   /* bytes usados, múltiplo de 4 */

/*
 * cg_data — consulta las palabras de la imagen.
 *
 * Entrada: ninguna
 * Salida:
 *   puntero interno de solo lectura.
 *   Efectos: ninguno.
 * Restricciones:
 *   usar cg_data_size para el tamaño; cg_init reinicia su contenido.
 */
const uint32_t *cg_data(void);        /* palabras desde la dirección 0 */
 
#endif
