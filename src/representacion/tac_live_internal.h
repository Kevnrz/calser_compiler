/* Ayudas privadas de vivacidad; ubicaciones densas y conjuntos de bits. Aho §9.2.5. */
#ifndef TAC_LIVE_INTERNAL_H
#define TAC_LIVE_INTERNAL_H
#include "tac_live.h"
#include "tac_internal.h"
struct TacLiveSet {
    size_t width;
    uint64_t *words;
};
struct TacLiveAnalysis {
    const TacFunction *function;
    TacOperand *locations;
    size_t count;
    size_t capacity;
    TacLiveBlock *blocks;
    size_t block_count;
    TacLiveInstruction *instructions;
    size_t instruction_count;
    size_t *offsets;
};
void tac_locations_physical(const TacInstruction *instruction, TacLiveInstruction *out);
#endif
