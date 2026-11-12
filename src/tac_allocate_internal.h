/* Estado privado C2: planes propios y asignaciones por ID; Aho §8.8. */
#ifndef TAC_ALLOCATE_INTERNAL_H
#define TAC_ALLOCATE_INTERNAL_H
#include "tac_allocate.h"
#include "tac_analysis_internal.h"
typedef struct {
    TacFunction *function;
    unsigned *registers;
    TacAllocationMetrics metrics;
} TacFunctionAllocation;
struct TacAllocationResult {
    TacFunctionAllocation *functions;
    size_t function_count;
    TacEmissionSite *sites;
    size_t site_count;
};
void tac_allocate_sites(TacAllocationResult *result, TacFunctionAllocation *function,
                        const TacLiveAnalysis *live);
#endif
