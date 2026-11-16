/*
 * ast_constant.h — constantes de la imagen inicial desde el AST anotado.
 * Usa: ast.h y enteros de la biblioteca C; no depende del generador ni del emisor.
 * Lo usan generate_init.c, las pruebas y el futuro constructor TAC.
 * Referencias: Aho §6.4 (expresiones), §6.3.4 (almacenamiento).
 */
#ifndef AST_CONSTANT_H
#define AST_CONSTANT_H

#include <stdint.h>
#include "ast.h"

/*
 * ast_declaration_initializer — localiza el inicializador de una declaración.
 *
 * Entrada:
 *   node: AST_DECLARATION o NULL.
 * Salida:
 *   inicializador prestado o NULL si no existe.
 * Efectos:
 *   ninguno.
 * Restricciones:
 *   conserva la búsqueda por TYPE y DIMENSIONS; no valida la declaración.
 */
ASTNode *ast_declaration_initializer(ASTNode *node);

/*
 * ast_constant_value — aplica la política constante de la imagen inicial.
 *
 * Entrada:
 *   node: expresión anotada o NULL; out: palabra escribible no NULL.
 * Salida:
 *   1 con el valor de 32 bits; 0 si no puede evaluarlo, sin cambiar *out.
 * Efectos:
 *   escribe *out al obtener un valor; no modifica AST ni símbolos.
 * Restricciones:
 *   sigue solo identificadores Vi con decl_node; raíz a profundidad 0,
 *   máximo 64. No evalúa llamadas. Evalúa ambos operandos lógicos antes
 *   de combinarlos; no agrega corto circuito. Comparación, división y módulo
 *   usan signo; las palabras se truncan a 32 bits (Aho §6.4, §6.3.4).
 */
int ast_constant_value(ASTNode *node, uint32_t *out);

#endif
