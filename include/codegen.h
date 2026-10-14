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

/* Describe la sintaxis generada sin resolver etiquetas ni codificar palabras.
 * 0 = descripción conocida; 1 = desconocida/inválida, marcada como barrera.
 * text se presta: el llamador conserva su propiedad. El ensamblador sigue
 * siendo el responsable de la validación y de los mensajes de error. */
int cg_describe_instruction(const char *text, Slot slot, int source_line,
                            CGInstruction *out);

/* Flujo en orden de emisión, independiente de las líneas de salida.
 * cg_op registra operación + separación AUTOMATIC; cg_bundle conserva un
 * bundle fijo; cg_gap es separación EXPLICIT. Los textos pertenecen al emisor.
 * Los registros son de solo lectura y sus punteros valen hasta la siguiente
 * emisión o cg_free/cg_init. Una etiqueta o bundle fijo delimita regiones que
 * el organizador mantiene como fronteras de calendarización. */
int cg_ir_count(void);
const CGIRRecord *cg_ir_record(int index);

/* Organiza regiones del IR y reconstruye líneas/direcciones. Llamar una vez
 * al terminar la emisión, antes de consultar etiquetas o ensamblar. Idempotente;
 * después no admite nueva emisión. 0 = éxito (incluido fallback conservador),
 * 1 = no se pudo organizar. No cambia la imagen de datos ni el IR original. */
int cg_schedule(void);

void cg_init(void);
void cg_free(void);
 
/* Etiqueta: vale el índice del bundle que la sigue (isa.md §11.1). */
void cg_label(const char *name);
 
/* Bundle completo. NULL en un slot = nopi. Las cadenas se copian. */
void cg_bundle(const char *s0, const char *s1, const char *s2, const char *s3);
 
/* Registra una operación y su separación conservadora. cg_schedule podrá
   agruparla con otras operaciones y reemplazar huecos automáticos. El slot es
   preferido: una ALU normal admite 0 o 1. Formato tipo printf. */
void cg_op(Slot slot, const char *fmt, ...);
 
/* Bundle vacío. */
void cg_gap(void);
 
/* Escribe las líneas actuales: llamar cg_schedule antes para organizarlas.
 * Formato isa.md §11: un bundle por línea, cuatro slots. */
void cg_write(FILE *out);

/* Línea del fuente vigente: cada bundle que se emita desde ahora la guarda,
   para el .asm anotado. 0 = sin línea. */
void cg_set_line(int line);

/* Como cg_write, pero anotado (enunciado CE1108 §6.2): la dirección (índice
   de bundle) y la línea del fuente de cada bundle, y al final la tabla de
   etiquetas con su dirección. Todo lo agregado va en comentarios con #
   (isa.md §11.1): el archivo da los mismos bundles con ccom -x. */
void cg_write_listing(FILE *out);

/* Dirección final después de cg_schedule (conservadora antes de llamarlo).
 * Índice de bundle de la etiqueta con ese nombre, o -1 si no
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
