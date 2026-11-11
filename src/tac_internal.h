/* tac_internal.h — ayudas privadas; Aho §6.2 y §8.4.1. No es API pública. */
#ifndef TAC_INTERNAL_H
#define TAC_INTERNAL_H
#include "tac.h"
#define TAC_KIND(kind) (1u << (kind))
void *tac_allocate(size_t bytes);
size_t tac_next_identifier(size_t *counter);
int tac_error(char *error, size_t capacity, const char *format, ...);
int tac_has_edge(TacBlock *const *edges, size_t count, const TacBlock *block);
size_t tac_block_presentation_number(const TacBlock *block);
int tac_has_block(const TacFunction *function, const TacBlock *block);
int tac_has_instruction(const TacBlock *block, const TacInstruction *instruction);
int tac_isa_info(const TacInstruction *instruction, TacOpInfo *out);
int tac_isa_validate(const TacInstruction *instruction, char *error, size_t cap);
int tac_pseudo_info(const TacInstruction *instruction, TacOpInfo *out);
int tac_pseudo_validate(const TacFunction *function, const TacInstruction *instruction,
                        char *error, size_t cap);
int tac_validate_function_form(const TacFunction *function, char *error, size_t cap);
int tac_expected_successors(const TacBlock *block, TacBlock *out[2],
                            size_t *count, char *error, size_t cap);
#endif
