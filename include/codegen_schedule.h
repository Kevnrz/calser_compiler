/*
 * codegen_schedule.h — Fase 4: interfaz de planificación de operaciones en bundles.
 *
 * Expone bundles planificados, problemas, organizador y verificador.
 * Los contratos indican entradas, resultados, efectos y límites de uso.
 *
 * Usa: codegen.h.
 * Lo usa: codegen_schedule.c; codegen.c, codegen_schedule.c, codegen_verify.c y pruebas/bundles.
 * Referencias: isa.md §3, §9; docs/organizacion_bundles.md.
 */

#ifndef CODEGEN_SCHEDULE_H
#define CODEGEN_SCHEDULE_H
#include "codegen.h"
#define CG_SCHEDULE_MAX_OPERATIONS 256

typedef struct { const CGInstruction *slots[4]; } CGScheduledBundle;
typedef void (*CGScheduleEmit)(void *user, const CGScheduledBundle *bundle);

#define CG_SCHEDULE_MAX_BUNDLES (2 * CG_SCHEDULE_MAX_OPERATIONS + 1)

typedef enum {
    CG_VERIFY_OK, CG_VERIFY_INPUT, CG_VERIFY_MISSING, CG_VERIFY_DUPLICATE,
    CG_VERIFY_FOREIGN, CG_VERIFY_SLOT, CG_VERIFY_DESTINATION,
    CG_VERIFY_RAW, CG_VERIFY_WAR, CG_VERIFY_WAW, CG_VERIFY_MEMORY,
    CG_VERIFY_CONTROL
} CGVerifyKind;

typedef struct {
    CGVerifyKind kind;
    int operation, related_operation; /* Índices en la región; -1 si no aplica. */
    int bundle, slot; /* Relativos al calendario de la región; -1 si no aplica. */
    uint16_t registers;
} CGScheduleIssue;

/* ==========================================================================
 * VERIFICACIÓN Y PLANIFICACIÓN
 * ======================================================================= */

/*
 * cg_verify_schedule — verifica calendario contra orden y efectos originales.
 *
 * Entrada:
 *   operations  operaciones originales.
 *   count  cantidad 0–256.
 *   bundles  bundles planificados.
 *   bundle_count  cantidad hasta CG_SCHEDULE_MAX_BUNDLES.
 *   previous_writes  escrituras del bundle anterior.
 *   issue  detalle opcional del fallo.
 * Salida:
 *   0 si válido; 1 si falla.
 *   Efectos: escribe issue si está presente.
 * Restricciones:
 *   sin barreras ni destinos relativos numéricos; RAW=2,
 *   WAR/WAW/memoria=1; operación única y salto solo al final.
 */
int cg_verify_schedule(const CGInstruction *const *operations, int count,
                        const CGScheduledBundle *bundles, int bundle_count,
                        uint16_t previous_writes, CGScheduleIssue *issue);
/*
 * cg_verify_kind_name — nombra una clase de problema del calendario.
 *
 * Entrada:
 *   kind  clase de verificación.
 * Salida:
 *   cadena literal; error desconocido si no reconoce kind.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
const char *cg_verify_kind_name(CGVerifyKind kind);

/*
 * cg_schedule_block — planifica y verifica una región de operaciones.
 *
 * Entrada:
 *   operations  arreglo de count instrucciones.
 *   count  cantidad 0–256.
 *   previous_writes  máscara de escrituras del bundle anterior.
 *   emit  callback no NULL que recibe bundles.
 *   user  dato opaco para callback.
 * Salida:
 *   0 si emite calendario válido; 1 por entrada, memoria o verificación inválida.
 *   Efectos: reserva grafo/calendario, reporta fallos y llama emit después de verificar.
 * Restricciones:
 *   sin etiquetas, barreras ni destinos numéricos; salto solo al final; no
 *   añade hueco de frontera.
 */
int cg_schedule_block(const CGInstruction *const *operations, int count,
                      uint16_t previous_writes, CGScheduleEmit emit, void *user);
#endif
