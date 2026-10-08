#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

int semantic_analyze(ASTNode *root);

void semantic_print_symbols(void);

void semantic_free(void);

#endif