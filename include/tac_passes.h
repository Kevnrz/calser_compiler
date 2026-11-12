/* Gestor E: orden fijo y contrato único; Aho §8.1, §9.1 y §9.2. */
#ifndef TAC_PASSES_H
#define TAC_PASSES_H
#include "tac_build.h"
#include "tac_metrics.h"

typedef enum { TAC_PASS_HIGH, TAC_PASS_VIRTUAL, TAC_PASS_ALLOCATION } TacPassPoint;
typedef struct {
    TacProgram *program;               /* IR mutable del nivel correspondiente. */
    const TacBuildResult *build;        /* Imagen y símbolos auxiliares prestados. */
    const Scope *scopes;               /* Ámbitos semánticos prestados. */
    const char *source_name;           /* Ruta real para errores internos. */
    unsigned enabled;                 /* Selección de las cinco pasadas. */
    unsigned unroll_factor;           /* 0: heurística §11; explícito: 2..16. */
    TacMetrics *metrics;              /* Acumulador de esta compilación. */
    void (*report)(void *, const char *, int); /* Avance opcional de cada pasada. */
    void *report_user;                 /* Estado prestado del informador. */
} TacPassContext;
typedef int (*TacPassRun)(TacPassContext *, char *, size_t);
typedef struct {
    const char *name;                  /* Nombre estable de --opt y del JSON. */
    TacPassPoint point;                /* Momento fijo de ejecución. */
    TacPassRun run;                    /* 1 éxito; 0 con detalle en el buffer. */
} TacPass;
const TacPass *tac_pass_at(size_t index);
/* Entrada: nombre de longitud explícita. Salida: índice o -1. Efectos: ninguno.
 * Restricciones: comparación exacta, sin aliases ni reordenamiento del registro.
 */
int tac_pass_find(const char *name, size_t length);
/* Entrada: contexto vivo y pasada, índice de métricas 0..4.
 * Salida: 1 o error interno con archivo/pasada. Efectos: ejecuta, valida y mide.
 * Restricciones: mismo contrato para implementaciones futuras y pruebas dirigidas.
 */
int tac_pass_execute(TacPassContext *context, const TacPass *pass, size_t index,
                       char *error, size_t capacity);
/* Entrada: contexto y punto fijo. Salida: 1 o diagnóstico. Efectos: pasadas activas.
 * Restricciones: spill registra la política del recorrido C2; no derrama en E.
 */
int tac_passes_run(TacPassContext *context, TacPassPoint point, char *error, size_t capacity);
#endif
