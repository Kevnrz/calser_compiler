/* Estado del parser real tras yyparse. No usar un AST recuperado para semántica
 * ni generación. Contador reiniciado por parse; límites: 32 errores sintácticos.
 * parser_finish libera solo el observador, no el AST entregado a main. */
#ifndef PARSER_SERVICE_H
#define PARSER_SERVICE_H
#include <stdio.h>
#include "ast.h"
int parser_syntax_errors(void);
int parser_recovery_limited(void);
void parser_finish(void);
/*
 * parser_accept — aceptación completa del análisis (Aho §4.8–4.9 y §5.3.1).
 * Entrada:
 *   input  flujo abierto; tree  salida no NULL para el árbol poseído.
 * Salida:
 *   1 solo si yyparse devuelve 0 y no hay errores sintácticos ni léxicos.
 * Efectos:
 *   analiza el flujo, entrega el árbol (parcial o NULL) y llama parser_finish.
 * Restricciones:
 *   el llamador libera siempre *tree; no abre/cierra input ni inicia diagnósticos.
 *   Las consultas del IDE conservan su modo lexer_parse_text independiente.
 */
int parser_accept(FILE *input, ASTNode **tree);
#endif
