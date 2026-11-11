/* Estado del parser real tras yyparse. No usar un AST recuperado para semántica
 * ni generación. Contador reiniciado por parse; límites: 32 errores sintácticos.
 * parser_finish libera solo el observador, no el AST entregado a main. */
#ifndef PARSER_SERVICE_H
#define PARSER_SERVICE_H
int parser_syntax_errors(void);
int parser_recovery_limited(void);
void parser_finish(void);
#endif
