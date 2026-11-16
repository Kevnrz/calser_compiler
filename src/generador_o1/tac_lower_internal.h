/* Ayudas privadas de selección; Aho §8.1–8.2 y §8.6. */
#ifndef TAC_LOWER_INTERNAL_H
#define TAC_LOWER_INTERNAL_H
#include "tac_lower.h"

typedef struct {
    const TacBlock *high;
    TacBlock *lowered;
} TacLowerBlock;

typedef struct {
    TacFunction *function;
    TacBlock *block;
    const TacInstruction *source;
    TacLowerBlock *blocks;
    size_t block_count;
    char *error;
    size_t capacity;
    int failed;
} TacLower;

/* Requisito del consumidor para sus registros fuente. */
typedef enum {
    TL_SOURCE_FIXED_ALLOWED, /* Admite rz para el literal cero. */
    TL_SOURCE_ASSIGNABLE    /* Requiere TEMP para la plantilla aritmética P1. */
} TacLowerSourcePolicy;

void tl_fail(TacLower *lower, const char *message);
TacInstruction *tl_emit(TacLower *lower, const char *name,
                        TacOperand dest, TacOperand src1, TacOperand src2);
void tl_location(const TacLower *lower, TacInstruction *instruction);
void tl_constant(TacLower *lower, TacOperand dest, int64_t value);
TacOperand tl_value(TacLower *lower, TacOperand operand, TacLowerSourcePolicy policy);
void tl_load_symbol(TacLower *lower, TacOperand dest, const Symbol *symbol);
void tl_store_symbol(TacLower *lower, const Symbol *symbol, TacOperand value);
int tl_lower_value(TacLower *lower);
int tl_lower_control(TacLower *lower);
#endif
