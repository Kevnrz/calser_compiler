/* Adaptación del Flex real para servicios M2–M5, sin reglas de lenguaje nuevas.
 * Un proceso atiende una consulta; el scanner/observador son globales, no son
 * reentrantes. Los callbacks reciben kind prestado y bytes UTF-8 [start,end).
 * No retener yytext/yylval; los servicios copian índices o lexemas propios. */
#ifndef LEXER_SERVICE_H
#define LEXER_SERVICE_H
#include <stddef.h>
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
/* Analiza exactamente length bytes, completos o prefijo. El llamador activa
 * suggestions_active o syntax_service_active para los destructores temporales.
 * El marcador de cursor solo se emite en modo sugerencias. No hace semántica.
 * Propiedad: libera pila/AST de consulta y buffers, no el texto del llamador. */
int lexer_parse_text(const char *text, size_t length);
#endif
