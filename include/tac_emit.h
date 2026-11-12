/* D: ISA física -> codegen finalizado, sin CLI ni reserva de derrame.
 * Usa TAC, planes C2 y etiquetas P1; Aho §7.2, §8.6, §8.8 y §10.2.3.
 */
#ifndef TAC_EMIT_H
#define TAC_EMIT_H
#include "tac_allocate.h"
#include "label_table.h"

typedef struct {
    const TacProgram *program;             /* ISA_PHYSICAL prestado. */
    const TacAllocationResult *allocation; /* Plan C2 correspondiente, sin editar. */
    const uint32_t *initial_data;           /* Imagen prestada de TacBuildResult. */
    size_t initial_data_size;               /* Extensión en bytes. */
    const LabelTable *labels;               /* Etiquetas P1 prestadas. */
    const char *source_name;                /* Nombre real obligatorio y prestado. */
    int no_schedule;                       /* 0 organiza; 1 conserva bundles iniciales. */
} TacEmitInput;

/* Entrada: IR físico y plan vivos; error NULL solo con capacity=0.
 * Salida: 1 éxito, 0 con diagnóstico. Efectos: reinicia codegen, carga imagen,
 * emite y organiza salvo no_schedule; comprueba pila y ROM. No modifica IR, símbolos ni planes.
 * Restricciones: descartar emisión parcial al fallar; no emitir tras éxito.
 * Mantener datos prestados durante la llamada; ensamblar/consultar cg_ir_record
 * y cg_label_address antes de cg_free/cg_init. No hay tabla de etiquetas propia.
 */
int tac_emit_program(const TacEmitInput *input, char *error, size_t capacity);

/* Entrada: cantidad final y nombre obligatorio. Salida: 1 si <=4096, 0 con error.
 * Efectos: solo diagnóstico. Restricciones: llamar sobre la salida final, también conservadora.
 */
int tac_emit_check_rom(size_t bundle_count, const char *source_name,
                       char *error, size_t capacity);
#endif
