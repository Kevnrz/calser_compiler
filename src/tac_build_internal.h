/* Ayudas privadas de construcción; Aho §6.4, §6.6 y §8.4.1. */
#ifndef TAC_BUILD_INTERNAL_H
#define TAC_BUILD_INTERNAL_H
#include "tac_build.h"

typedef struct {
    TacBuildResult *result;
    TacFunction *function;
    TacBlock *block;
    TacBlock *body;
    const Symbol *return_symbol;
    ASTNode *globals;
    char *error;
    size_t capacity;
    size_t next_auxiliary;
    int failed;
} TacBuilder;

typedef enum { TB_IMAGE, TB_DYNAMIC, TB_LOCAL } TbInitialization;
void *tb_allocate(size_t bytes);
void tb_fail(TacBuilder *builder, const ASTNode *node, const char *message);
ASTNode *tb_child(ASTNode *node, size_t index);
const Symbol *tb_symbol(ASTNode *node);
TacInstruction *tb_emit(TacBuilder *builder, ASTNode *node, TacOpcode op,
                         TacOperand dest, TacOperand src1, TacOperand src2);
TacOperand tb_value(TacBuilder *builder, ASTNode *node, TacOpcode op,
                     TacOperand src1, TacOperand src2);
TacBlock *tb_reserve_block(TacBuilder *builder);
void tb_jump(TacBuilder *builder, ASTNode *node, TacBlock *target);
TacOperand tb_expression(TacBuilder *builder, ASTNode *node);
TacOperand tb_call(TacBuilder *builder, ASTNode *node, int result_needed);
TacOperand tb_array_base(TacBuilder *builder, ASTNode *node);
TacOperand tb_array_offset(TacBuilder *builder, ASTNode *node);
void tb_statement(TacBuilder *builder, ASTNode *node);
void tb_statements(TacBuilder *builder, ASTNode *block);
void tb_flow(TacBuilder *builder, ASTNode *node);
void tb_return(TacBuilder *builder, ASTNode *node);
TacOperand tb_short_circuit(TacBuilder *builder, ASTNode *node);
void tb_initialize(TacBuilder *builder, ASTNode *node, TbInitialization phase);
#endif
