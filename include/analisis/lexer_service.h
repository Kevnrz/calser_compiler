/* Adaptación del Flex real para servicios M2–M5, sin reglas de lenguaje nuevas.
 * Un proceso atiende una consulta; el scanner/observador son globales, no son
 * reentrantes. Los callbacks reciben kind prestado y bytes UTF-8 [start,end).
 * No retener yytext/yylval; los servicios copian índices o lexemas propios. */
#ifndef LEXER_SERVICE_H
#define LEXER_SERVICE_H
#include <stddef.h>
#include <stdio.h>
/* Reinicia el scanner por archivo; importaciones mínimas, diseño §12. */
void lexer_begin(FILE *input);
typedef void (*LexerObserver)(const char *kind, size_t start, size_t end);
/* Instalar antes de escanear y desconectar con NULL al terminar. */
void lexer_set_observer(LexerObserver observer);
/* Escanea yyin hasta EOF; libera las cadenas creadas para Bison. */
int lexer_scan_only(void);
/* Variante en memoria: copia de Flex, longitud acotada por el servicio y menor
 * que INT_MAX. Destruye buffers antes/después; retorna 1 si hubo error léxico. */
int lexer_scan_text(const char *text, size_t length);
size_t lexer_current_start(void);
size_t lexer_current_end(void);
/* Visita de tokens sin llamar a yyparse. EOF también se entrega al consumidor.
 * Los identificadores se liberan después de cada visita, incluso al detenerse.
 * El consumidor devuelve cero para detener el escaneo. Los errores conservan
 * la línea y columna informadas por Flex. No se conservan los callbacks.
 * Retorna cero sin errores léxicos, uno con errores y menos uno por argumentos
 * inválidos. La longitud debe caber en un int y la visita no es reentrante.
 */
typedef int (*LexerTokenConsumer)(int token, size_t start, size_t end,
                                  int line, int column, void *data);
typedef void (*LexerErrorConsumer)(int line, int column, size_t width, void *data);
int lexer_visit_text(const char *text, size_t length, LexerTokenConsumer token,
                     LexerErrorConsumer error, void *data);
/* Metadatos del comentario durante una visita. No analizan el texto de nuevo.
 * comment_open también vale uno durante el evento de comentario de línea.
 * Ambos valores se consultan únicamente desde los callbacks de la visita.
 */
int lexer_comment_open(void);
int lexer_in_block_comment(void);
#endif
