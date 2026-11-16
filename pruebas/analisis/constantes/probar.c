/*
 * probar.c — política constante compartida con la imagen de datos del generador.
 * Usa: ast_constant.h, símbolos prestados y constructores AST; sin emisor.
 * Lo usa make check-constantes. Referencias: Aho §6.4 y §6.3.4.
 */
#include "ast_constant.h"
#include "symbol_table.h"
#include <assert.h>
#include <stdio.h>

/*
 * expect — comprueba un valor y libera la expresión.
 * Entrada: node propio; value esperado de 32 bits.
 * Salida: ninguna.
 * Efectos: evalúa, comprueba y libera node.
 * Restricciones: símbolos referenciados deben seguir vivos durante la consulta.
 */
static void expect(ASTNode *node, uint32_t value) {
    uint32_t actual = 0;
    assert(ast_constant_value(node, &actual));
    assert(actual == value);
    ast_free(node);
}

/*
 * reject — comprueba rechazo sin modificar la salida y libera la expresión.
 * Entrada: node propio o NULL.
 * Salida: ninguna.
 * Efectos: evalúa, comprueba y libera node.
 * Restricciones: no debe producir una constante según la política actual.
 */
static void reject(ASTNode *node) {
    uint32_t actual = UINT32_C(0x12345678);
    assert(!ast_constant_value(node, &actual));
    assert(actual == UINT32_C(0x12345678));
    ast_free(node);
}

/*
 * literals_and_operators — prueba truncamiento, signos y operadores admitidos.
 * Entrada: ninguna. Salida: ninguna.
 * Efectos: crea, evalúa y libera árboles de expresiones.
 * Restricciones: la aritmética esperada es de palabras de 32 bits, no de long.
 */
static void literals_and_operators(void) {
    static const struct {
        ASTKind kind;
        long left;
        long right;
        uint32_t value;
    } cases[] = {
        {AST_ADD, -1, 1, 0},
        {AST_ADD, 2147483647L, 1, UINT32_C(0x80000000)},
        {AST_SUB, 0, 1, UINT32_MAX},
        {AST_MUL, 65536, 65536, 0},
        {AST_DIV, -7, 3, UINT32_C(0xfffffffe)},
        {AST_DIV, 7, -3, UINT32_C(0xfffffffe)},
        {AST_DIV, -7, -3, 2},
        {AST_DIV, -2147483647L - 1, -1, UINT32_C(0x80000000)},
        {AST_MOD, -7, 3, UINT32_MAX},
        {AST_MOD, 7, -3, 1},
        {AST_MOD, -7, -3, UINT32_MAX},
        {AST_MOD, -2147483647L - 1, -1, 0},
        {AST_POWER, 2, 32, 0},
        {AST_POWER, -2, 3, UINT32_C(0xfffffff8)},
        {AST_POWER, 0, 0, 1},
        {AST_AND, 3, 2, 1},
        {AST_AND, 0, 2, 0},
        {AST_OR, 0, 2, 1},
        {AST_OR, 0, 0, 0},
        {AST_EQUAL, -1, -1, 1},
        {AST_NOT_EQUAL, 1, -1, 1},
        {AST_LESS, -1, 0, 1},
        {AST_GREATER, 0, -1, 1},
        {AST_LESS_EQUAL, -1, -1, 1},
        {AST_GREATER_EQUAL, -1, 0, 0},
    };
    expect(ast_number(-1), UINT32_MAX);
    expect(ast_number(42), 42);
    expect(ast_boolean(0), 0);
    expect(ast_boolean(1), 1);
    expect(ast_unary(AST_NEG, ast_number(1)), UINT32_MAX);
    expect(ast_unary(AST_NEG, ast_number(-2147483647L - 1)), UINT32_C(0x80000000));
    expect(ast_unary(AST_NOT, ast_number(0)), 1);
    expect(ast_unary(AST_NOT, ast_number(-1)), 0);
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        expect(ast_binary(cases[i].kind, ast_number(cases[i].left),
                          ast_number(cases[i].right)), cases[i].value);
    }
}

/*
 * initializers — comprueba búsqueda con y sin dimensiones e inicializador.
 * Entrada: ninguna. Salida: ninguna.
 * Efectos: construye y libera declaraciones manuales.
 * Restricciones: prueba forma del AST, no validación semántica de las listas.
 */
static void initializers(void) {
    assert(!ast_declaration_initializer(NULL));
    for (int array = 0; array < 2; array++) {
        for (int initialized = 0; initialized < 2; initialized++) {
            ASTNode *dims = array ? ast_dimensions() : NULL;
            if (dims) {
                ast_add_number(dims, 3);
            }
            ASTNode *init = initialized ? ast_number(9) : NULL;
            ASTNode *decl = ast_declaration(ast_modifier("Vi"), "v",
                                            ast_type("int"), dims, init);
            assert(ast_declaration_initializer(decl) == init);
            ast_free(decl);
        }
    }
}

/*
 * identifiers — prueba Vi encadenados y la frontera de profundidad 64/65.
 * Entrada: ninguna. Salida: ninguna.
 * Efectos: crea declaraciones y referencias con símbolos prestados.
 * Restricciones: libera los árboles antes de que caduquen los símbolos locales.
 */
static void identifiers(void) {
    Symbol symbols[65] = {0};
    ASTNode *declarations[65];
    for (int i = 0; i < 65; i++) {
        ASTNode *init = i ? ast_identifier("v") : ast_number(7);
        if (i) {
            init->symbol = &symbols[i - 1];
        }
        declarations[i] = ast_declaration(ast_modifier("Vi"), "v",
                                           ast_type("int"), NULL, init);
        symbols[i].kind = SYMBOL_VARIABLE;
        symbols[i].modifier = MODIFIER_VI;
        symbols[i].decl_node = declarations[i];
    }
    ASTNode *id = ast_identifier("v");
    id->symbol = &symbols[1];
    expect(id, 7);
    id = ast_identifier("v");
    id->symbol = &symbols[63];
    expect(id, 7);
    id = ast_identifier("v");
    id->symbol = &symbols[64];
    reject(id);
    symbols[0].modifier = MODIFIER_VM;
    id = ast_identifier("v");
    id->symbol = &symbols[0];
    reject(id);
    symbols[0].modifier = MODIFIER_VI;
    symbols[0].decl_node = NULL;
    id = ast_identifier("v");
    id->symbol = &symbols[0];
    reject(id);
    reject(ast_identifier("sin_anotacion"));
    for (int i = 0; i < 65; i++) {
        ast_free(declarations[i]);
    }
}

/*
 * rejection_and_depth — comprueba rechazos y ausencia de corto circuito nuevo.
 * Entrada: ninguna. Salida: ninguna.
 * Efectos: crea, evalúa y libera expresiones.
 * Restricciones: no ejecuta llamadas ni introduce diagnósticos del compilador.
 */
static void rejection_and_depth(void) {
    reject(NULL);
    reject(ast_call("f", ast_arguments(ast_number(1))));
    reject(ast_binary(AST_DIV, ast_number(1), ast_number(0)));
    reject(ast_binary(AST_MOD, ast_number(1), ast_number(0)));
    reject(ast_binary(AST_POWER, ast_number(2), ast_number(-1)));
    reject(ast_binary(AST_INDEX, ast_number(1), ast_number(2)));
    reject(ast_binary(AST_AND, ast_boolean(0), ast_call("f", NULL)));
    reject(ast_binary(AST_OR, ast_boolean(1), ast_call("f", NULL)));
    reject(ast_binary(AST_AND, ast_boolean(0),
                      ast_binary(AST_DIV, ast_number(1), ast_number(0))));
    for (int depth = 64; depth <= 65; depth++) {
        ASTNode *node = ast_number(7);
        for (int i = 0; i < depth; i++) {
            node = ast_unary(AST_NEG, node);
        }
        if (depth == 64) {
            expect(node, 7);
        } else {
            reject(node);
        }
    }
}

/*
 * main — ejecuta las pruebas directas del módulo compartido.
 * Entrada: ninguna. Salida: 0 si pasan las aserciones.
 * Efectos: imprime un resumen estable.
 * Restricciones: ejecutar sin NDEBUG para conservar las comprobaciones.
 */
int main(void) {
    literals_and_operators();
    initializers();
    identifiers();
    rejection_and_depth();
    puts("PASS constantes: inicializadores, Vi, profundidad, 32 bits, signos y rechazos sin corto circuito");
    return 0;
}
