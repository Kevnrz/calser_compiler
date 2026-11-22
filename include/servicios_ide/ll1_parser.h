/* Reconocedor sintáctico LL(1) sobre los tokens reales de Flex.
 * No construye AST, no ejecuta semántica y no llama al parser Bison.
 * Los rangos se expresan en bytes UTF-8 y las líneas/columnas parten de uno.
 * El llamador conserva el texto. El resultado no contiene memoria prestada.
 * Flex y los diagnósticos son globales, por lo que no admite consultas simultáneas.
 */
#ifndef LL1_PARSER_H
#define LL1_PARSER_H
#include <stddef.h>
#include <stdint.h>

#define LL1_SOURCE_MAX 200000
#define LL1_TOKEN_MAX 50000
#define LL1_STEP_MAX 1000000
#define LL1_STACK_MAX 4096

typedef enum {
    LL1_ACCEPTED, LL1_SYNTAX_ERROR, LL1_LEXICAL_ERROR,
    LL1_LIMIT, LL1_CANCELLED, LL1_INTERNAL_ERROR,
    LL1_PREFIX_VALID, LL1_IN_COMMENT, LL1_INVALID_CONTEXT
} LL1Status;

typedef enum {
    LL1_NO_LIMIT, LL1_SOURCE_LIMIT, LL1_TOKEN_LIMIT, LL1_STEP_LIMIT, LL1_STACK_LIMIT
} LL1Limit;

typedef struct {
    size_t source, tokens, steps, stack;
} LL1Limits;

typedef struct {
    LL1Status status;
    LL1Limit limit;
    size_t offset, end, consumed, steps;
    int line, column, unexpected, lexical_errors;
    /* Bits de terminales esperados desde toda la continuación de la pila.
     * Incluye el fin de entrada, pero nunca epsilon ni TOKEN_CURSOR.
     * Usar ll1_terminal_name para interpretar los índices.
     */
    uint64_t expected;
} LL1Result;

typedef struct {
    size_t terminals, nonterminals, productions, cells, nullable, conflicts;
} LL1GrammarInfo;

typedef enum { LL1_NO_COMMENT, LL1_LINE_COMMENT, LL1_BLOCK_COMMENT } LL1Comment;

typedef struct {
    LL1Result analysis;
    size_t cursor, point, comment_start;
    LL1Comment comment;
    /* Verdadero solo si el prefijo puede terminar como programa completo. */
    int can_end;
} LL1Context;

typedef int (*LL1Cancel)(void *data);

/* Calcula PRIMERO, SIGUIENTE y tabla una vez por proceso. Retorna cero si
 * detecta un conflicto. Los datos permanecen internos y no se modifican luego.
 */
int ll1_grammar_info(LL1GrammarInfo *info);
const char *ll1_terminal_name(size_t index);
int ll1_expects(const LL1Result *result, const char *terminal);
const char *ll1_status_name(LL1Status status);
LL1Limits ll1_default_limits(void);

/* Analiza el documento completo y se detiene en el primer obstáculo.
 * NULL con longitud cero representa un archivo vacío. Los límites NULL usan
 * los valores predeterminados. Los límites personalizados deben ser positivos
 * y no exceder los máximos. La cancelación se consulta entre pasos y tokens.
 * Un comentario largo se termina de escanear antes de consultar cancelación.
 * El uso es exclusivo respecto de otros servicios Flex y de diagnósticos.
 */
LL1Result ll1_analyze(const char *text, size_t length, const LL1Limits *limits,
                      LL1Cancel cancel, void *data);
/* Consulta el prefijo sin convertir el cursor en EOF obligatorio.
 * cursor y point son offsets UTF-8 del texto original, con point <= cursor.
 * point == cursor consume todos los tokens hasta el cursor. Si point es menor,
 * el intervalo indica el fragmento parcial que se está completando. Debe empezar
 * en una frontera de token y contener solo una palabra, número u operador parcial.
 * El sufijo posterior al cursor no interviene. Un comentario en el cursor bloquea
 * propuestas sin fabricar un error por su cierre todavía pendiente.
 * LL1_PREFIX_VALID no certifica que exista un programa completo. can_end informa
 * ese caso por separado. No filtra sugerencias ni aplica cambios al documento.
 */
LL1Context ll1_context(const char *text, size_t length, size_t cursor, size_t point,
                       const LL1Limits *limits, LL1Cancel cancel, void *data);
#endif
