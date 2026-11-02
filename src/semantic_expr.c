/*
 * semantic_expr.c — Fase 3: tipos de expresiones, usos y asignaciones.
 *
 * Calcula el tipo de cada expresión una sola vez (ET_ERROR evita repetir
 * errores sobre ella), resuelve usos de nombres con corchetes como llamada,
 * índice o rebobinado, pliega constantes enteras con control de desborde de
 * 32 bits, y verifica condiciones, asignaciones e inicializadores de lista.
 *
 * Usa: semantic_internal.h, semantic_scope.h, semantic_calls.h, diagnostics.c y
 * symbol_table.c.
 * Lo usa: semantic.c y semantic_crypto.c.
 * Referencias: libro §2.7, §6.5.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic_internal.h"
#include "semantic_scope.h"
#include "semantic_calls.h"
#include "diagnostics.h"

// Rango de un int de 32 bits con signo.
#define INT32_LOWEST  (-2147483648LL)
#define INT32_HIGHEST 2147483647LL


/* ==========================================================================
 * TIPOS DE EXPRESIONES
 * ======================================================================= */


/*
 * make_type — crea una descripción de tipo sin valor constante.
 *
 * Entrada:
 *   kind  clase de expresión.
 * Salida:
 *   ExprType inicializado con base VOID y tamaños cero.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static ExprType make_type(ExprKind kind) {
    ExprType type;

    type.kind = kind;
    type.base = TYPE_VOID;
    type.dimensions = 0;
    type.is_const = 0;
    type.value = 0;

    for (int i = 0; i < MAX_DIMS; i++)
        type.sizes[i] = 0;

    return type;
}

/*
 * scalar_type — convierte un tipo declarado en tipo de expresión.
 *
 * Entrada:
 *   type  tipo de símbolo.
 * Salida:
 *   ET_INT, ET_BOOL o ET_VOID.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static ExprType scalar_type(SymbolType type) {
    switch (type) {
        case TYPE_INT:  return make_type(ET_INT);
        case TYPE_BOOL: return make_type(ET_BOOL);
        default:        return make_type(ET_VOID);
    }
}

/* semantic_symbol_value_type — véase el contrato en semantic_internal.h. */
ExprType semantic_symbol_value_type(Symbol *symbol) {
    if (symbol->dimensions == 0)
        return scalar_type(symbol->type);

    ExprType type = make_type(ET_ARRAY);

    type.base = symbol->type;
    type.dimensions = symbol->dimensions;

    for (int i = 0; i < symbol->dimensions; i++) {

        if (symbol->dim_sizes[i] <= 0)
            return make_type(ET_ERROR);

        type.sizes[i] = symbol->dim_sizes[i];
    }

    return type;
}

/*
 * same_type — compara tipos y forma de arreglos.
 *
 * Entrada:
 *   a  primer tipo.
 *   b  segundo tipo.
 * Salida:
 *   1 si coinciden clase y forma; 0 en otro caso.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static int same_type(ExprType a, ExprType b) {
    if (a.kind != b.kind)
        return 0;

    if (a.kind != ET_ARRAY)
        return 1;

    if (a.base != b.base || a.dimensions != b.dimensions)
        return 0;

    for (int i = 0; i < a.dimensions; i++) {

        if (a.sizes[i] != b.sizes[i])
            return 0;
    }

    return 1;
}

/* semantic_type_text — véase el contrato en semantic_internal.h. */
const char *semantic_type_text(ExprType type, char *buffer) {
    const char *base;

    switch (type.kind) {
        case ET_INT:   base = "int";  break;
        case ET_BOOL:  base = "bool"; break;
        case ET_VOID:  base = "void"; break;
        case ET_ARRAY: base = type.base == TYPE_BOOL ? "bool" : "int"; break;
        default:       base = "?";    break;
    }

    int length = snprintf(buffer, TYPE_TEXT_SIZE, "%s", base);

    for (int i = 0; type.kind == ET_ARRAY && i < type.dimensions; i++) {

        if (length < TYPE_TEXT_SIZE)
            length += snprintf(buffer + length,
                               TYPE_TEXT_SIZE - (size_t) length,
                               "[%d]",
                               type.sizes[i]);
    }

    return buffer;
}

/* semantic_expect_type — véase el contrato en semantic_internal.h. */
void semantic_expect_type(ASTNode *node, ExprType type, ExprKind wanted,
                        const char *message) {
    if (type.kind != ET_ERROR &&
        type.kind != wanted)
        semantic_error(node, "%s", message);
}

/* semantic_check_assignable — véase el contrato en semantic_internal.h. */
void semantic_check_assignable(ASTNode *target, ExprType destination,
                             ExprType value) {
    if (destination.kind == ET_ERROR ||
        value.kind == ET_ERROR ||
        same_type(destination, value))
        return;

    char value_text[TYPE_TEXT_SIZE];
    char destination_text[TYPE_TEXT_SIZE];

    semantic_type_text(value, value_text);
    semantic_type_text(destination, destination_text);

    // Arreglo completo: copia el contenido, así que la forma debe coincidir.
    if (destination.kind == ET_ARRAY)
        semantic_error(target,
                       "no se puede asignar %s a %s",
                       value_text,
                       destination_text);
    else
        semantic_error(target,
                       "no se puede asignar '%s' a una variable '%s'",
                       value_text,
                       destination_text);
}

// Usos

/* ==========================================================================
 * USOS, ARGUMENTOS E ÍNDICES
 * ======================================================================= */


/*
 * check_identifier — verifica un nombre usado sin corchetes.
 *
 * Entrada:
 *   identifier  nodo AST_IDENTIFIER.
 * Salida:
 *   tipo escalar/arreglo o ET_ERROR.
 *   Efectos: resuelve y anota símbolo; reporta funciones usadas sin invocación.
 * Restricciones:
 *   ninguna.
 */
static ExprType check_identifier(ASTNode *identifier) {
    Symbol *symbol = scopes_resolve(identifier);

    if (symbol == NULL)
        return make_type(ET_ERROR);

    if (symbol->kind == SYMBOL_FUNCTION) {

        semantic_error(identifier,
                       "'%s' es una funcion; debe invocarse con corchetes",
                       identifier->text);

        return make_type(ET_ERROR);
    }

    return semantic_symbol_value_type(symbol);
}

/*
 * check_invocation — comprueba punto de entrada y cantidad de argumentos.
 *
 * Entrada:
 *   node  nodo CALL o REWIND.
 *   function  función resuelta.
 *   n_args  cantidad de argumentos.
 *   is_rewind  1 para R, 0 para llamada.
 * Salida:
 *   1 si cumple; 0 si reporta error.
 *   Efectos: reporta invocación de main o cantidad incorrecta.
 * Restricciones:
 *   function no NULL; aún no verifica tipos.
 */
static int check_invocation(ASTNode *node, Symbol *function,
                            int n_args, int is_rewind) {
    int valid = 1;

    // main solo la invoca el arranque del programa (no aplica a REWIND).
    if (!is_rewind &&
        strcmp(function->name, "main") == 0) {

        semantic_error(node,
                       "'main' es el punto de entrada y no puede invocarse");

        valid = 0;
    }

    if (n_args != function->parameter_count) {

        semantic_error(node,
                       "'%s' espera %d argumentos, se recibieron %d",
                       function->name,
                       function->parameter_count,
                       n_args);

        valid = 0;
    }

    return valid;
}

/*
 * check_argument — verifica tipo y mutabilidad de un argumento.
 *
 * Entrada:
 *   argument  expresión argumento.
 *   type  tipo ya calculado.
 *   function  función con param_list.
 *   position  posición desde 0.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta tipo/forma incompatibles o arreglo Vi pasado a parámetro mutable.
 * Restricciones:
 *   cantidad ya coincide; position menor que parameter_count.
 */
static void check_argument(ASTNode *argument, ExprType type,
                           Symbol *function, int position) {
    Symbol *parameter = function->param_list[position];

    if (type.kind == ET_ERROR)
        return;

    // Un arreglo se pasa por referencia: uno inmutable no puede llegar a un
    // parámetro que permite modificarlo. Los escalares se pasan por valor.
    Symbol *symbol =
        argument->kind == AST_IDENTIFIER ? argument->symbol : NULL;

    if (symbol != NULL &&
        type.kind == ET_ARRAY &&
        symbol->modifier == MODIFIER_VI &&
        parameter->dimensions > 0 &&
        parameter->modifier != MODIFIER_VI) {

        semantic_error(argument,
                       "'%s' es inmutable; el parametro '%s' permite modificacion",
                       symbol->name,
                       parameter->name);
    }

    // Escalar: el mismo tipo. Arreglo: misma base, dimensiones y tamaños.
    ExprType expected = semantic_symbol_value_type(parameter);

    if (expected.kind == ET_ERROR ||
        same_type(expected, type))
        return;

    char expected_text[TYPE_TEXT_SIZE];
    char received_text[TYPE_TEXT_SIZE];

    semantic_error(argument,
                   "argumento %d de '%s': se esperaba %s, se recibio %s",
                   position + 1,
                   function->name,
                   semantic_type_text(expected, expected_text),
                   semantic_type_text(type, received_text));
}

/*
 * check_index_range — verifica un índice conocido en compilación.
 *
 * Entrada:
 *   index  expresión índice.
 *   type  tipo ya calculado.
 *   array  arreglo resuelto.
 *   position  dimensión desde 0.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta índice constante fuera de rango.
 * Restricciones:
 *   position menor que dimensions; no verifica índices dinámicos.
 */
static void check_index_range(ASTNode *index, ExprType type, Symbol *array,
                              int position) {
    if (!type.is_const)
        return;

    long value = type.value;

    int size = array->dim_sizes[position];

    // Dimensión 0: ya se reportó en la declaración (S-06).
    if (size <= 0)
        return;

    if (value < 0 || value >= size) {

        semantic_error(index,
                       "indice %ld fuera del rango [0..%d]",
                       value,
                       size - 1);
    }
}

// Qué resultó ser un CALL, INDEX o REWIND al resolver su nombre.
typedef enum {
    RESOLVED_NONE,     // nombre no encontrado o mal usado (ya reportado)
    RESOLVED_CALL,     // llamada a una función
    RESOLVED_ACCESS    // acceso a un elemento de un arreglo
} Resolution;

/*
 * check_operand — verifica una vez un argumento o índice.
 *
 * Entrada:
 *   operand  expresión o NULL.
 *   symbol  símbolo resuelto o NULL.
 *   resolution  resolución de llamada/acceso.
 *   valid  1 si coincide la cantidad.
 *   position  posición desde 0.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: evalúa tipo, anota nombres y reporta errores.
 * Restricciones:
 *   symbol válido si resolution exige comprobarlo; posición válida cuando
 *   valid=1.
 */
static void check_operand(ASTNode *operand, Symbol *symbol,
                          Resolution resolution, int valid, int position) {
    ExprType type = semantic_check_expression(operand);

    if (resolution == RESOLVED_ACCESS) {

        semantic_expect_type(operand, type, ET_INT,
                    "el indice debe ser de tipo int");

        if (valid)
            check_index_range(operand, type, symbol, position);
    }
    else if (resolution == RESOLVED_CALL && valid) {
        check_argument(operand, type, symbol, position);
    }
}

/*
 * classify_use — decide qué es un uso con corchetes según su contexto.
 *
 * Entrada:
 *   node  nodo CALL, INDEX o REWIND.
 *   symbol  símbolo ya resuelto, no NULL.
 *   context  dónde aparece el uso.
 * Salida:
 *   RESOLVED_CALL, RESOLVED_ACCESS o RESOLVED_NONE si el símbolo no sirve
 *   para ese contexto (el llamador reporta el error).
 *   Efectos: ninguno.
 * Restricciones:
 *   en una expresión, un CALL es llamada o acceso; un INDEX solo puede ser
 *   acceso. Solo se puede asignar a un elemento de un arreglo. Una
 *   sentencia de llamada y un R solo admiten funciones.
 */
static Resolution classify_use(ASTNode *node, Symbol *symbol,
                               UseContext context) {
    int is_function = symbol->kind == SYMBOL_FUNCTION;
    int is_array = !is_function && symbol->dimensions > 0;

    if (context == USE_EXPRESSION &&
        is_function && node->kind == AST_CALL)
        return RESOLVED_CALL;

    if ((context == USE_STATEMENT || context == USE_REWIND) &&
        is_function)
        return RESOLVED_CALL;

    if ((context == USE_EXPRESSION || context == USE_TARGET) &&
        is_array)
        return RESOLVED_ACCESS;

    return RESOLVED_NONE;
}

/*
 * check_call_use — verifica una llamada a función y la registra en el grafo.
 *
 * Entrada:
 *   node  nodo de la llamada.
 *   symbol  símbolo de la función.
 *   n_args  cantidad de argumentos escritos.
 *   context  dónde aparece el uso.
 *   poisoned  salida: 1 si la llamada no puede usarse como valor.
 * Salida:
 *   1 si la cantidad de argumentos coincide; 0 si no.
 *   Efectos: reporta errores y agrega la arista al grafo de llamadas.
 * Restricciones:
 *   S-05: R no es una llamada, y llamar a main ya es un error.
 */
static int check_call_use(ASTNode *node, Symbol *symbol, int n_args,
                          UseContext context, int *poisoned) {
    int valid = check_invocation(node,
                                 symbol,
                                 n_args,
                                 context == USE_REWIND);

    if (context != USE_REWIND &&
        strcmp(symbol->name, "main") != 0) {

        if (symbol == scopes_function()) {

            // Recursión directa: no se agrega al grafo.
            semantic_error(node,
                           "'%s' se invoca a si misma; "
                           "la recursion debe expresarse con 'R'",
                           symbol->name);

            *poisoned = 1;
        }
        else if (scopes_function() != NULL) {
            calls_add_edge(scopes_function(), symbol, node);
        }
    }

    return valid;
}

/*
 * check_access_use — verifica que un acceso use todas las dimensiones del arreglo.
 *
 * Entrada:
 *   node  nodo del acceso.
 *   symbol  símbolo del arreglo.
 *   n_args  cantidad de índices escritos.
 * Salida:
 *   1 si la cantidad coincide con las dimensiones; 0 si no.
 *   Efectos: reporta el error de dimensiones.
 * Restricciones:
 *   todos los índices llegan en ARGUMENTS.
 */
static int check_access_use(ASTNode *node, Symbol *symbol, int n_args) {
    int valid = n_args == symbol->dimensions;

    if (!valid)
        semantic_error(node,
                       "'%s' tiene %d dimensiones, se usaron %d indices",
                       symbol->name,
                       symbol->dimensions,
                       n_args);

    return valid;
}

/* semantic_check_call — véase el contrato en semantic_internal.h. */
ExprType semantic_check_call(ASTNode *node, UseContext context) {
    ASTNode *arguments = ast_find_child(node, AST_ARGUMENTS);

    int n_args = ast_count_children(arguments);

    // Si el nombre no existe ya se reportó y no se dice nada más de este uso.
    Symbol *symbol = scopes_resolve(ast_find_child(node, AST_IDENTIFIER));

    Resolution resolution = RESOLVED_NONE;
    int valid = 0;

    // Llamada que se resolvió pero no puede usarse como valor (recursión
    // directa): sus argumentos se verifican, pero el resultado es ET_ERROR.
    int poisoned = 0;

    // R solo reinicia la función actual. Si apunta a otra, se verifican los
    // nombres de sus argumentos, pero no su cantidad ni sus tipos.
    if (context == USE_REWIND &&
        symbol != NULL &&
        symbol->kind == SYMBOL_FUNCTION &&
        symbol != scopes_function()) {

        semantic_error(node,
                       "'R' solo puede reiniciar la funcion actual");
    }
    else if (symbol != NULL) {

        resolution = classify_use(node, symbol, context);

        if (resolution == RESOLVED_CALL) {
            valid = check_call_use(node, symbol, n_args, context, &poisoned);
        }
        else if (resolution == RESOLVED_ACCESS) {
            valid = check_access_use(node, symbol, n_args);
        }
        else if (context == USE_STATEMENT || context == USE_REWIND) {
            semantic_error(node,
                           "'%s' no es una funcion",
                           symbol->name);
        }
        else {
            semantic_error(node,
                           "'%s' no es un arreglo",
                           symbol->name);
        }
    }

    // Argumentos o índices, en orden. Si valid, position nunca pasa de la
    // cantidad de parámetros ni de MAX_DIMS.
    int position = 0;

    ASTNode *operand = arguments ? arguments->child : NULL;

    while (operand != NULL) {
        check_operand(operand, symbol, resolution, valid, position);
        position++;
        operand = operand->next;
    }

    // Recorrido adicional de un hermano de ARGUMENTS si estuviera presente.
    // ast_index normaliza los índices dentro de ARGUMENTS: en los nodos
    // construidos por el parser, arguments->next es NULL.
    if (node->kind == AST_INDEX && arguments != NULL)
        check_operand(arguments->next, symbol, resolution, valid, position);

    if (!valid || poisoned)
        return make_type(ET_ERROR);

    // Llamada: el tipo de retorno. Acceso: el tipo de los elementos (un
    // índice fuera de rango no lo cambia).
    return scalar_type(symbol->type);
}

// Familias de operadores (§2).
typedef enum {
    OP_ARITHMETIC,   // + - * / % ^ y - unario: int -> int
    OP_RELATIONAL,   // < > <= >=: int -> bool
    OP_LOGICAL,      // & | y ! unario: bool -> bool
    OP_EQUALITY      // == !=: dos escalares del mismo tipo -> bool
} OperatorFamily;

/* ==========================================================================
 * OPERADORES Y CONSTANTES
 * ======================================================================= */


/*
 * operator_family — clasifica un operador por la regla de tipos.
 *
 * Entrada:
 *   kind  clase del operador AST.
 * Salida:
 *   familia relacional, lógica, igualdad o aritmética.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static OperatorFamily operator_family(ASTKind kind) {
    switch (kind) {

        case AST_LESS:
        case AST_GREATER:
        case AST_LESS_EQUAL:
        case AST_GREATER_EQUAL:
            return OP_RELATIONAL;

        case AST_OR:
        case AST_AND:
        case AST_NOT:
            return OP_LOGICAL;

        case AST_EQUAL:
        case AST_NOT_EQUAL:
            return OP_EQUALITY;

        default:
            return OP_ARITHMETIC;
    }
}

/*
 * number_type — verifica el rango de un literal entero.
 *
 * Entrada:
 *   number  nodo AST_NUMBER no NULL.
 *   negated  1 si es operando directo de menos unario.
 * Salida:
 *   ET_INT constante; ET_ERROR ante desborde.
 *   Efectos: reporta literales mayores que el máximo permitido.
 * Restricciones:
 *   el léxico entrega magnitudes no negativas; admite 2147483648 solo con
 *   negated=1.
 */
static ExprType number_type(ASTNode *number, int negated) {
    long long highest = negated ? INT32_HIGHEST + 1 : INT32_HIGHEST;

    if (number->number > highest) {

        semantic_error(number,
                       "la expresion constante desborda el rango de int");

        return make_type(ET_ERROR);
    }

    ExprType type = make_type(ET_INT);

    type.is_const = 1;
    type.value = number->number;

    return type;
}

/*
 * const_power — calcula potencia por cuadrados sucesivos.
 *
 * Entrada:
 *   base  base en rango int32.
 *   exponent  exponente no negativo.
 *   overflow  puntero a bandera inicializada a 0.
 * Salida:
 *   potencia; 0 al detectar desborde.
 *   Efectos: pone *overflow=1 si no cabe en int32.
 * Restricciones:
 *   x^0 y 0^0 valen 1.
 */
static long long const_power(long long base, long long exponent,
                             int *overflow) {
    long long result = 1;

    while (exponent > 0) {

        if (exponent & 1) {

            result *= base;

            if (result < INT32_LOWEST || result > INT32_HIGHEST) {
                *overflow = 1;
                return 0;
            }
        }

        exponent >>= 1;

        if (exponent > 0) {

            base *= base;

            // base se volverá a usar y result no es 0: ya no cabe.
            if (base > INT32_HIGHEST) {
                *overflow = 1;
                return 0;
            }
        }
    }

    return result;
}

/*
 * fold_constant — calcula operadores aritméticos constantes.
 *
 * Entrada:
 *   node  nodo de operador.
 *   left  tipo izquierdo.
 *   right  tipo derecho, igual al izquierdo en unario.
 * Salida:
 *   ET_INT con valor si se conoce; ET_ERROR ante error.
 *   Efectos: reporta división por cero, exponente negativo o desborde.
 * Restricciones:
 *   operandos ya int; división trunca hacia cero; no calcula constantes
 *   lógicas.
 */
static ExprType fold_constant(ASTNode *node, ExprType left, ExprType right) {
    ExprType result = make_type(ET_INT);

    int is_unary = node->kind == AST_NEG;

    // Aunque el dividendo o la base no sean constantes.
    if ((node->kind == AST_DIV || node->kind == AST_MOD) &&
        right.is_const && right.value == 0) {

        semantic_error(node, "division entre cero");

        return make_type(ET_ERROR);
    }

    if (node->kind == AST_POWER &&
        right.is_const && right.value < 0) {

        semantic_error(node,
                       "exponente negativo: "
                       "resultado no representable como int");

        return make_type(ET_ERROR);
    }

    if (!left.is_const ||
        (!is_unary && !right.is_const))
        return result;

    long long a = left.value;
    long long b = right.value;
    long long value = 0;

    int overflow = 0;

    switch (node->kind) {
        case AST_NEG:   value = -a;    break;
        case AST_ADD:   value = a + b; break;
        case AST_SUB:   value = a - b; break;
        case AST_MUL:   value = a * b; break;
        case AST_DIV:   value = a / b; break;
        case AST_MOD:   value = a % b; break;
        case AST_POWER: value = const_power(a, b, &overflow); break;
        default:        return result;
    }

    // Solo se reporta el operador más interno que se sale de rango: su padre
    // recibe ET_ERROR.
    if (overflow ||
        value < INT32_LOWEST || value > INT32_HIGHEST) {

        semantic_error(node,
                       "la expresion constante desborda el rango de int");

        return make_type(ET_ERROR);
    }

    result.is_const = 1;
    result.value = (long) value;

    return result;
}

/*
 * check_operator — verifica tipos de un operador y pliega aritmética constante.
 *
 * Entrada:
 *   node  nodo de operador no NULL.
 * Salida:
 *   tipo de familia; ET_ERROR si el operador reporta error.
 *   Efectos: verifica operandos una vez y reporta incompatibilidad.
 * Restricciones:
 *   si un operando es ET_ERROR devuelve el tipo de familia sin otro
 *   diagnóstico.
 */
static ExprType check_operator(ASTNode *node) {
    const char *symbol = semantic_operator_symbol(node->kind);

    ASTNode *first = node->child;
    ASTNode *second = first ? first->next : NULL;   // NULL en los unarios

    // -2147483648: el literal es operando directo del - unario.
    ExprType left =
        (node->kind == AST_NEG && first != NULL && first->kind == AST_NUMBER)
            ? number_type(first, 1)
            : semantic_check_expression(first);

    ExprType right = second ? semantic_check_expression(second) : left;

    OperatorFamily family = operator_family(node->kind);

    ExprType result =
        make_type(family == OP_ARITHMETIC ? ET_INT : ET_BOOL);

    if (left.kind == ET_ERROR ||
        right.kind == ET_ERROR)
        return result;

    char text[TYPE_TEXT_SIZE];

    int errors_before = diag_count();

    switch (family) {

        case OP_ARITHMETIC:
            if (left.kind != ET_INT || right.kind != ET_INT)
                semantic_error(node,
                               "el operador '%s' requiere operandos de tipo int",
                               symbol);
            break;

        // Se nombra el tipo del primer operando que no es int.
        case OP_RELATIONAL:
            if (left.kind != ET_INT || right.kind != ET_INT)
                semantic_error(node,
                               "el operador '%s' no acepta operandos de tipo %s",
                               symbol,
                               semantic_type_text(left.kind != ET_INT ? left : right,
                                         text));
            break;

        case OP_LOGICAL:
            if (left.kind != ET_BOOL || right.kind != ET_BOOL)
                semantic_error(node,
                               "el operador '%s' requiere operandos de tipo bool",
                               symbol);
            break;

        // Solo se comparan escalares, y del mismo tipo.
        case OP_EQUALITY:
            if (left.kind == ET_ARRAY || right.kind == ET_ARRAY)
                semantic_error(node,
                               "el operador '%s' no acepta operandos de tipo %s",
                               symbol,
                               semantic_type_text(left.kind == ET_ARRAY ? left : right,
                                         text));
            else if (left.kind != right.kind)
                semantic_error(node,
                               "el operador '%s' requiere operandos del mismo tipo",
                               symbol);
            break;
    }

    // El operador reportó un error: no se dice nada más sobre esta expresión.
    if (diag_count() != errors_before)
        return make_type(ET_ERROR);

    // Tipos correctos: valor constante, si se conoce en compilación.
    if (family == OP_ARITHMETIC)
        return fold_constant(node, left, right);

    return result;
}

/* semantic_check_expression — véase el contrato en semantic_internal.h. */
ExprType semantic_check_expression(ASTNode *node) {
    if (node == NULL)
        return make_type(ET_ERROR);

    switch (node->kind) {

        case AST_IDENTIFIER:
            return check_identifier(node);

        case AST_NUMBER:
            return number_type(node, 0);

        case AST_BOOLEAN:
            return make_type(ET_BOOL);

        case AST_CALL:
        case AST_INDEX: {
            ExprType type = semantic_check_call(node, USE_EXPRESSION);

            if (type.kind == ET_VOID) {

                ASTNode *name = ast_find_child(node, AST_IDENTIFIER);

                semantic_error(node,
                               "la funcion '%s' no devuelve valor",
                               name ? name->text : "");

                return make_type(ET_ERROR);
            }

            return type;
        }

        default:
            break;
    }

    if (semantic_operator_symbol(node->kind) != NULL)
        return check_operator(node);

    // Cualquier otro nodo no es una expresión con tipo: solo se verifican
    // los nombres que contiene.
    ASTNode *child = node->child;

    while (child != NULL) {
        semantic_check_expression(child);
        child = child->next;
    }

    return make_type(ET_ERROR);
}

/* ==========================================================================
 * CONDICIONES Y ASIGNACIONES
 * ======================================================================= */


/*
 * find_condition — busca la condición de C, A o L.
 *
 * Entrada:
 *   node  nodo de control o NULL.
 * Salida:
 *   primer hijo distinto de BLOCK, ALTERNATIVES y RESIDUAL, o NULL.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static ASTNode *find_condition(ASTNode *node) {
    if (node == NULL)
        return NULL;

    ASTNode *child = node->child;

    while (child != NULL) {

        if (child->kind != AST_BLOCK &&
            child->kind != AST_ALTERNATIVES &&
            child->kind != AST_RESIDUAL)
            return child;

        child = child->next;
    }

    return NULL;
}

/* semantic_check_condition — véase el contrato en semantic_internal.h. */
void semantic_check_condition(ASTNode *node) {
    ASTNode *condition = find_condition(node);

    semantic_expect_type(condition, semantic_check_expression(condition), ET_BOOL,
                "la condicion debe ser de tipo bool");
}

/*
 * check_target — verifica el destino y su mutabilidad.
 *
 * Entrada:
 *   target  nodo destino o NULL.
 * Salida:
 *   tipo destino o ET_ERROR.
 *   Efectos: resuelve nombres y reporta escritura a Vi.
 * Restricciones:
 *   la mutabilidad incorrecta se reporta sin cambiar necesariamente el
 *   tipo.
 */
static ExprType check_target(ASTNode *target) {
    if (target == NULL)
        return make_type(ET_ERROR);

    if (target->kind == AST_CALL ||
        target->kind == AST_INDEX) {

        ExprType type = semantic_check_call(target, USE_TARGET);

        ASTNode *name = ast_find_child(target, AST_IDENTIFIER);
        Symbol *symbol = name ? name->symbol : NULL;

        if (symbol != NULL &&
            symbol->kind != SYMBOL_FUNCTION &&
            symbol->dimensions > 0 &&
            symbol->modifier == MODIFIER_VI) {

            semantic_error(target,
                           "'%s' es inmutable; sus elementos no pueden modificarse",
                           symbol->name);
        }

        return type;
    }

    ExprType type = semantic_check_expression(target);

    Symbol *symbol =
        target->kind == AST_IDENTIFIER ? target->symbol : NULL;

    if (symbol != NULL &&
        symbol->kind != SYMBOL_FUNCTION &&
        symbol->modifier == MODIFIER_VI) {

        semantic_error(target,
                       "'%s' es inmutable y no puede modificarse",
                       symbol->name);
    }

    return type;
}

/* semantic_check_assignment — véase el contrato en semantic_internal.h. */
void semantic_check_assignment(ASTNode *statement) {
    ASTNode *target = statement->child;               // primer hijo
    ASTNode *value = target ? target->next : NULL;    // segundo hijo

    ExprType destination = check_target(target);

    semantic_check_assignable(target, destination, semantic_check_expression(value));
}

/* ==========================================================================
 * LISTAS DE INICIALIZACIÓN
 * ======================================================================= */


/*
 * check_list_names — verifica expresiones de una lista sin exigir forma.
 *
 * Entrada:
 *   node  lista, valor o NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: recorre valores y reporta sus errores de expresión.
 * Restricciones:
 *   ninguna.
 */
static void check_list_names(ASTNode *node) {
    if (node == NULL)
        return;

    if (node->kind != AST_INITIALIZER_LIST) {
        semantic_check_expression(node);
        return;
    }

    ASTNode *child = node->child;

    while (child != NULL) {
        check_list_names(child);
        child = child->next;
    }
}

/*
 * report_list_type — reporta el primer tipo incorrecto de un inicializador.
 *
 * Entrada:
 *   wrong  primer tipo incorrecto o ET_ERROR.
 *   symbol  arreglo declarado.
 *   node  nodo de declaración.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: emite diagnóstico salvo wrong=ET_ERROR.
 * Restricciones:
 *   ninguna.
 */
static void report_list_type(ExprType wrong, Symbol *symbol, ASTNode *node) {
    if (wrong.kind == ET_ERROR)
        return;

    char wrong_text[TYPE_TEXT_SIZE];
    char base_text[TYPE_TEXT_SIZE];

    semantic_error(node,
                   "no se puede asignar '%s' a una variable '%s'",
                   semantic_type_text(wrong, wrong_text),
                   semantic_type_text(scalar_type(symbol->type), base_text));
}

/*
 * check_list_values — verifica los valores escalares de una lista o fila.
 *
 * Entrada:
 *   list  lista de inicialización.
 *   symbol  arreglo declarado.
 *   node  nodo para diagnóstico.
 *   wrong  puntero al primer tipo incorrecto, inicialmente ET_ERROR.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza *wrong, verifica expresiones y reporta listas anidadas inesperadas.
 * Restricciones:
 *   list, symbol y wrong no NULL; forma exterior la verifica el llamador.
 */
static void check_list_values(ASTNode *list, Symbol *symbol, ASTNode *node,
                              ExprType *wrong) {
    ExprType base = scalar_type(symbol->type);

    int nested = 0;

    ASTNode *value = list->child;

    while (value != NULL) {

        if (value->kind == AST_INITIALIZER_LIST) {
            nested = 1;
            check_list_names(value);
        }
        else {

            ExprType type = semantic_check_expression(value);

            if (wrong->kind == ET_ERROR &&
                type.kind != ET_ERROR &&
                !same_type(type, base))
                *wrong = type;
        }

        value = value->next;
    }

    if (nested)
        semantic_error(node,
                       "se recibio una lista donde se esperaba un valor");
}

/* semantic_check_initializer_list — véase el contrato en semantic_internal.h. */
void semantic_check_initializer_list(Symbol *symbol, ASTNode *list,
                                   ASTNode *node) {

    // Un escalar no admite lista: solo se verifican los nombres.
    if (symbol->dimensions == 0) {

        semantic_error(node,
                       "'%s' no es un arreglo",
                       symbol->name);

        check_list_names(list);

        return;
    }

    // Dimensión 0: ya se reportó en la declaración (S-06).
    for (int i = 0; i < symbol->dimensions; i++) {

        if (symbol->dim_sizes[i] <= 0) {
            check_list_names(list);
            return;
        }
    }

    int count = ast_count_children(list);

    if (count != symbol->dim_sizes[0])
        semantic_error(node,
                       "se recibieron %d valores, se esperaban %d",
                       count,
                       symbol->dim_sizes[0]);

    // Primer valor de tipo incorrecto en toda la lista.
    ExprType wrong = make_type(ET_ERROR);

    // Una dimensión: cada elemento es un valor.
    if (symbol->dimensions == 1) {
        check_list_values(list, symbol, node, &wrong);
        report_list_type(wrong, symbol, node);
        return;
    }

    // Dos dimensiones: si el número de filas no coincide, no se revisa cada
    // fila (solo sus nombres).
    if (count != symbol->dim_sizes[0]) {
        check_list_names(list);
        return;
    }

    // Cada elemento es una fila de dim_sizes[1] valores.
    int columns = symbol->dim_sizes[1];
    int row_number = 0;

    ASTNode *row = list->child;

    while (row != NULL) {

        if (row->kind != AST_INITIALIZER_LIST) {

            semantic_error(node,
                           "la fila %d debe ser una lista de %d valores",
                           row_number,
                           columns);

            semantic_check_expression(row);
        }
        else {

            int values = ast_count_children(row);

            if (values != columns)
                semantic_error(node,
                               "la fila %d tiene %d valores, se esperaban %d",
                               row_number,
                               values,
                               columns);

            check_list_values(row, symbol, node, &wrong);
        }

        row_number++;
        row = row->next;
    }

    report_list_type(wrong, symbol, node);
}
