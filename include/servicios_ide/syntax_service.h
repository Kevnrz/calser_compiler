/* Análisis sintáctico LL(1) reutilizable en memoria, con tokens reales de Flex.
 * Cada llamada reinicia buffers, diagnósticos y pila. No construye árboles.
 * Lo usan repairs.c y --syntax-json desde main.c. Estado estático de una sola
 * consulta; no reentrante. El resultado es por valor, sin punteros prestados.
 * Offsets absolutos en bytes UTF-8 [inicio,fin). No depende de archivos Qt. */
#ifndef SYNTAX_SERVICE_H
#define SYNTAX_SERVICE_H
#include <stdio.h>
#define SYNTAX_SOURCE_LIMIT 200000
#define SYNTAX_TOKEN_LIMIT 50000
typedef struct {
    int accepted, lexical, limited, failed, eof, next_statement;
    /* Bits 0..3: ; . ] ), exclusivamente si LL(1) los esperaba. */
    unsigned closers;
    /* Token inesperado real y token anterior, no el rango visual del mensaje. */
    size_t offset, end, previous_start, previous_end;
} SyntaxAnalysis;
/* Texto prestado: no se modifica ni se guarda; bloquea ante errores léxicos. */
SyntaxAnalysis syntax_analyze(const char *text, size_t length);
/* Retorna buffer sin prometer NUL, de *length bytes. El llamador hace free.
 * NULL distingue lectura/memoria fallida de límite mediante *limited. */
char *syntax_read_source(const char *source, size_t *length, int *limited);
int syntax_json_run(FILE *out, const char *source);
/* Abre el informe check_ll1 y valida sintaxis con el reconocedor nativo.
 * Retorna cero solo si se puede construir el AST para revisar semántica.
 * El llamador termina el informe con diag_json_finish y libera diagnósticos.
 */
int syntax_check_ll1(const char *source);
#endif
