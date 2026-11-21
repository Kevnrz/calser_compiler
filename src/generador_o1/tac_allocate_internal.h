/* Estado privado C2: planes propios y asignaciones por ID; Aho §8.8. */
#ifndef TAC_ALLOCATE_INTERNAL_H
#define TAC_ALLOCATE_INTERNAL_H
#include "tac_allocate.h"
#include "tac_analysis_internal.h"
typedef struct {
    TacFunction *function;
    unsigned *registers;
    int *spill_slot;                 /* -1 con registro; casilla propia en Chaitin. */
    size_t spill_count;
    TacAllocationMetrics metrics;
} TacFunctionAllocation;
struct TacAllocationResult {
    TacFunctionAllocation *functions;
    size_t function_count;
    TacEmissionSite *sites;
    size_t site_count;
};
int tac_allocate_color(TacFunctionAllocation *function, const TacIntervalAnalysis *ranges,
                       const char *source, char *error, size_t capacity);
int tac_allocate_sites(TacAllocationResult *result, TacFunctionAllocation *function,
                        const TacLiveAnalysis *live, char *error, size_t capacity);
#endif
