/* Estado privado D; planes y símbolos prestados, mapa de bloques propio. */
#ifndef TAC_EMIT_INTERNAL_H
#define TAC_EMIT_INTERNAL_H
#include "tac_emit.h"
#include "tac_analysis_internal.h"
#include "codegen_common.h"
#include "codegen.h"
typedef struct {
    const TacBlock *block;
    char name[48];
} TacEmitBlock;
typedef struct {
    const TacEmitInput *input;
    TacEmitBlock *blocks;
    size_t block_count;
    CGArithContext arithmetic;
    const char *end_label;
    const char *error_label;
    char *error;
    size_t capacity;
} TacEmitter;
int te_error(TacEmitter *emit, const TacInstruction *instruction, const char *message);
const char *te_function_label(const TacEmitter *emit, const Symbol *function);
int te_emit_special(TacEmitter *emit, const TacFunction *function,
                     const TacInstruction *instruction);
#endif
