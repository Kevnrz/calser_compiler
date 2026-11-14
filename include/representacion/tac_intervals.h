/* Intervalos reutilizables desde C1, ISA virtual; Aho §9.2.5.
 * Linear scan: Poletto y Sarkar 1999 (fuera de Aho).
 */
#ifndef TAC_INTERVALS_H
#define TAC_INTERVALS_H
#include "tac_live.h"

typedef struct TacIntervalAnalysis TacIntervalAnalysis;
typedef struct {
    size_t temp_id;                  /* Identidad local a la función. */
    size_t start;                    /* Primera posición inclusiva. */
    size_t loop_depth;               /* Mayor profundidad donde aparece; §9.13. */
    size_t end;                      /* Última posición inclusiva. */
    const TacInstruction *origin;    /* Origen prestado para diagnóstico. */
} TacInterval;

/* ISA_VIRTUAL validado y live de esa función, vivos hasta free.
 * No modifica entrada; NULL con diagnóstico ante contrato inválido.
 * Orden de lista: lectura 2*i, escritura 2*i+1; bloque vacío consume un índice.
 * Fuentes PSEUDO aritméticas ocupan también escritura. Sin partición de huecos.
 */
TacIntervalAnalysis *tac_intervals_analyze(const TacFunction *function,
    const TacLiveAnalysis *live, char *error, size_t capacity);
size_t tac_intervals_count(const TacIntervalAnalysis *analysis);
const TacInterval *tac_intervals_at(const TacIntervalAnalysis *analysis, size_t index);
const TacInterval *tac_intervals_find(const TacIntervalAnalysis *analysis, size_t temp_id);
int tac_intervals_positions(const TacIntervalAnalysis *analysis,
    const TacInstruction *instruction, size_t *read, size_t *write);
void tac_intervals_free(TacIntervalAnalysis *analysis);
#endif
