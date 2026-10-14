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

/* Comprueba el calendario contra el orden y efectos originales del IR,
 * independientemente del grafo usado por el organizador. Solo regiones sin
 * etiquetas/plantillas/barreras. Distancia RAW=2, WAR/WAW/memoria=1.
 * 0 = válido; 1 = inválido, con detalle en issue si no es NULL. */
int cg_verify_schedule(const CGInstruction *const *operations, int count,
                        const CGScheduledBundle *bundles, int bundle_count,
                        uint16_t previous_writes, CGScheduleIssue *issue);
const char *cg_verify_kind_name(CGVerifyKind kind);

/* Lista de operaciones de una región, sin etiquetas/plantillas/barreras.
 * Un salto solo puede aparecer al final. previous_writes son los registros
 * potencialmente escritos por el bundle físico inmediatamente anterior.
 * Callback sin propiedad: los punteros pertenecen a las operaciones de entrada.
 * Construye y verifica el calendario entero antes de emitir el primer bundle.
 * 0 = éxito, 1 = entrada/calendario inválido o memoria insuficiente.
 * No añade la separación de frontera:
 * el emisor del programa la mantiene al cerrar la región. */
int cg_schedule_block(const CGInstruction *const *operations, int count,
                      uint16_t previous_writes, CGScheduleEmit emit, void *user);
#endif
