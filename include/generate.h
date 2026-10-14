#ifndef GENERATE_H
#define GENERATE_H
 
#include "ast.h"
#include "label_table.h"
 
/* Fase 4: genera el código de todo el programa en el emisor (codegen.h).
 * Debe llamarse DESPUÉS del análisis semántico y ANTES de semantic_free,
 * porque los nodos apuntan a los símbolos (G-02).
 * labels es la tabla de etiquetas que construyó la Fase 3: todas las
 * etiquetas de control del .asm salen de ella (label_table_get).
 * Devuelve 0 si ok, distinto de 0 si hubo un error interno. */
int generate_code(ASTNode *root, const LabelTable *labels);
 
#endif
 
