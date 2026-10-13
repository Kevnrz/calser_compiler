#ifndef GENERATE_H
#define GENERATE_H
 
#include "ast.h"
 
/* Fase 4: genera el código de todo el programa en el emisor (codegen.h).
 * Debe llamarse DESPUÉS del análisis semántico y ANTES de semantic_free,
 * porque los nodos apuntan a los símbolos (G-02).
 * Devuelve 0 si ok, distinto de 0 si hubo un error interno. */
int generate_code(ASTNode *root);
 
#endif
 
