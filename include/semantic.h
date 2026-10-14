#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

int semantic_analyze(ASTNode *root);

void semantic_print_symbols(void);

// Primera dirección libre después de la última área asignada (múltiplo de
// 4). Desde ahí hasta 0x0FFF está la pila de llamadas de la Fase 4.
int semantic_data_end(void);

void semantic_free(void);

#endif