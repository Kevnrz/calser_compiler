/*
 * tac_loops.h — marcas estructuradas HIGH y análisis recalculable de lazos (C1).
 * Usa tac.h. Referencias: Aho §9.6.1, §9.6.4 y algoritmo 9.46 (§9.6.6).
 */
#ifndef TAC_LOOPS_H
#define TAC_LOOPS_H
#include "tac.h"

typedef enum { TAC_LOOP_S, TAC_LOOP_L } TacLoopKind;
struct TacLoopMark {
    TacLoopKind kind;                  /* Origen estructurado; no cuenta R ni copias. */
    const TacBlock *header;            /* Primera prueba, incluido corto circuito. */
    const TacBlock *body;              /* Entrada al cuerpo. */
    const TacBlock *latch;             /* Retorno estructural, NULL o inalcanzable tras E. */
    size_t structural_depth;           /* Profundidad sintáctica, desde uno. */
    const Symbol *control;             /* Control existente del S; NULL en L. */
    const Symbol *limit;               /* Auxiliar existente del S; NULL en L. */
    TacOperand initial;               /* Inicio ya evaluado, no AST para reevaluar. */
    TacOperand bound;                 /* Fin ya evaluado, exclusivo, solo S. */
    int64_t step;                     /* Paso con signo, distinto de cero en S. */
    TacLoopMark *next;                /* Colección propia de TacFunction. */
};
typedef enum {
    TAC_LOOP_MARK_WITH_BACKEDGE,
    TAC_LOOP_MARK_WITHOUT_BACKEDGE,   /* Válida; el unrolling la ignora. */
    TAC_LOOP_MARK_UNREACHABLE         /* Sin entrada alcanzable; el unrolling la ignora. */
} TacLoopMarkState;
typedef struct TacLoopAnalysis TacLoopAnalysis;

/* Copia descripción HIGH y presta bloques/símbolos. next se administra internamente.
 * Devuelve NULL por argumentos ajenos/etapa inválida; sin memoria termina como TAC.
 * Las marcas no se imprimen. Descartarlas antes de que una pasada invalide referencias.
 */
TacLoopMark *tac_loop_mark_add(TacFunction *function, const TacLoopMark *description);
void tac_loop_marks_discard(TacFunction *function);

/* Requiere programa validado; analiza solo ciclos alcanzables desde entry.
 * NULL con diagnóstico en grafo irreducible/contrato inválido. Resultado propio,
 * presta función/bloques/marcas; liberar antes de editar o descartar marcas.
 * Inalcanzables: reachable=0, dominates=0, depth=0. No modifica el programa.
 */
TacLoopAnalysis *tac_loops_analyze(const TacFunction *function, char *error, size_t capacity);
int tac_loops_reachable(const TacLoopAnalysis *analysis, const TacBlock *block);
int tac_loops_dominates(const TacLoopAnalysis *analysis, const TacBlock *dominator,
                        const TacBlock *block);
size_t tac_loops_depth(const TacLoopAnalysis *analysis, const TacBlock *block);
size_t tac_loops_count(const TacLoopAnalysis *analysis);
const TacBlock *tac_loops_header(const TacLoopAnalysis *analysis, size_t loop);
int tac_loops_contains(const TacLoopAnalysis *analysis, size_t loop, const TacBlock *block);
/* Un solo sentido: solo marcas con retroceso alcanzable requieren lazo natural.
 * Una marca ajena/inválida se rechaza antes de consultar su estado.
 */
int tac_loop_marks_validate(const TacLoopAnalysis *analysis, char *error, size_t capacity);
TacLoopMarkState tac_loop_mark_state(const TacLoopAnalysis *analysis, const TacLoopMark *mark);
void tac_loops_free(TacLoopAnalysis *analysis);
#endif
