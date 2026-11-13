/* Métricas comunes de E y futuras pasadas; Aho §9.2.5 y §10.2.3. */
#ifndef TAC_METRICS_H
#define TAC_METRICS_H
#include "tac_allocate.h"
#define TAC_PASS_COUNT 5

typedef struct {
    unsigned executed;                 /* Máscara de pasadas ejecutadas, orden fijo. */
    size_t high_initial;               /* Instrucciones HIGH antes de las pasadas. */
    size_t high_after[TAC_PASS_COUNT];  /* Conteos solo para pasadas HIGH ejecutadas. */
    double pass_ms[TAC_PASS_COUNT];     /* Tiempo de cada pasada, incluida validación. */
    size_t isa_before;                 /* ISA virtual antes de expandir PSEUDO. */
    size_t emitted;                    /* Operaciones no-NOP del IR de codegen. */
    size_t bundles;                    /* Bundles finales, organizados o conservadores. */
    size_t demand;                     /* Máximo TEMP vivos entre funciones. */
    size_t pressure;                   /* Máximo de intervalos activos asignados. */
    uint16_t registers;                /* Unión informativa de registros usados. */
} TacMetrics;

/* Entrada: programa vivo. Salida: instrucciones, sin etiquetas ni bloques.
 * Efectos: ninguno. Restricciones: no cuenta la futura expansión de PSEUDO.
 */
size_t tac_metrics_instruction_count(const TacProgram *program);
/* Entrada: métricas, IR físico y asignación vivos. Salida: ninguna.
 * Efectos: toma métricas C2 y del codegen final. Restricciones: antes de cg_free.
 */
void tac_metrics_collect(TacMetrics *metrics, const TacProgram *program,
                         const TacAllocationResult *allocation);
/* Entrada: destino, nombre real, métricas y tiempo total sin escritura JSON.
 * Salida: 1 éxito, 0 con diagnóstico. Efectos: escribe un único JSON.
 * Restricciones: solo después de completar todas las salidas de compilación.
 */
int tac_metrics_write(const char *path, const char *source, const TacMetrics *metrics,
                       double total_ms);
/* Entrada: ninguna. Salida: reloj monotónico en ms. Efectos: consulta reloj.
 * Restricciones: tiempos de una misma ejecución, no comparables entre procesos.
 */
double tac_metrics_clock(void);
#endif
