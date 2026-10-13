#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic.h"
#include "symbol_table.h"
#include "ast.h"
#include "diagnostics.h"

static Scope *global_scope = NULL;
static Scope *current_scope = NULL;

//List of all scopes.
static Scope *scope_list_head = NULL;
static Scope *scope_list_tail = NULL;

// Función que se está analizando; NULL fuera de las funciones (globales).
static Symbol *current_function = NULL;

// Scope management

static Scope *create_scope(ScopeKind kind, int line) {
    Scope *scope = scope_create(current_scope, kind);

    if (scope == NULL)
        return NULL;

    // P-03: línea donde abre el bloque y función a la que pertenece.
    scope->line = line;
    scope->owner = current_function;

    // Add scope to the list of all scopes.
    if (scope_list_head == NULL) {
        scope_list_head = scope;
        scope_list_tail = scope;
    }
    else {
        scope_list_tail->next = scope;
        scope_list_tail = scope;
    }

    current_scope = scope;

    return scope;
}

static void enter_global_scope(void) {
    current_scope = NULL;

    global_scope = scope_create(NULL, SCOPE_GLOBAL);

    if (global_scope == NULL) {
        fprintf(stderr, "Error: could not create global scope\n");
        exit(EXIT_FAILURE);
    }

    scope_list_head = global_scope;
    scope_list_tail = global_scope;

    current_scope = global_scope;
}

static void enter_scope(ScopeKind kind, int line) {
    create_scope(kind, line);
}

static void exit_scope(void) {
    if (current_scope != NULL)
        current_scope = current_scope->parent;
}

// Type conversion

static SymbolType get_type(ASTNode *node) {
    if (node == NULL)
        return TYPE_VOID;

    if (node->kind != AST_TYPE)
        return TYPE_VOID;

    if (node->text == NULL)
        return TYPE_VOID;

    if (strcmp(node->text, "int") == 0)
        return TYPE_INT;

    if (strcmp(node->text, "bool") == 0)
        return TYPE_BOOL;

    if (strcmp(node->text, "void") == 0)
        return TYPE_VOID;

    return TYPE_VOID;
}


// Modifier conversion

static SymbolModifier get_modifier(ASTNode *node) {
    if (node == NULL)
        return MODIFIER_NONE;

    if (node->kind != AST_MODIFIER)
        return MODIFIER_NONE;

    if (node->text == NULL)
        return MODIFIER_NONE;

    if (strcmp(node->text, "Vm") == 0)
        return MODIFIER_VM;

    if (strcmp(node->text, "Vi") == 0)
        return MODIFIER_VI;

    return MODIFIER_NONE;
}

// AST helpers

// Primer hijo de node con el kind dado. Los hijos se identifican por kind,
// nunca por posición: los opcionales NULL no se agregan y corren las posiciones.
static ASTNode *find_child(ASTNode *node, ASTKind kind) {
    if (node == NULL)
        return NULL;

    ASTNode *child = node->child;

    while (child != NULL) {

        if (child->kind == kind)
            return child;

        child = child->next;
    }

    return NULL;
}

// Condición de un CONDITIONAL, ALTERNATIVE o LOOP_L: el hijo que no es
// BLOCK, ALTERNATIVES ni RESIDUAL.
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

// Errors

// Símbolo de un operador (binario o unario); NULL si el nodo no es operador.
static const char *operator_symbol(ASTKind kind) {
    switch (kind) {
        case AST_OR:            return "|";
        case AST_AND:           return "&";
        case AST_EQUAL:         return "==";
        case AST_NOT_EQUAL:     return "!=";
        case AST_LESS:          return "<";
        case AST_GREATER:       return ">";
        case AST_LESS_EQUAL:    return "<=";
        case AST_GREATER_EQUAL: return ">=";
        case AST_ADD:           return "+";
        case AST_SUB:           return "-";
        case AST_MUL:           return "*";
        case AST_DIV:           return "/";
        case AST_MOD:           return "%";
        case AST_POWER:         return "^";
        case AST_NEG:           return "-";
        case AST_NOT:           return "!";
        default:                return NULL;
    }
}

// Cantidad de dígitos de un número (sin signo).
static int count_digits(long number) {
    int digits = 1;

    while (number >= 10 || number <= -10) {
        number /= 10;
        digits++;
    }

    return digits;
}

/*
 * Único punto de salida de los errores semánticos (formato de §8.2).
 * Los errores se acumulan: después de reportar, el análisis sigue.
 *
 * Posición y largo del señalador según node:
 *
 * NULL                        = sin posición
 * IDENTIFIER                  = el nodo, strlen(text)
 * NUMBER                      = el nodo, cantidad de dígitos
 * NEG de un NUMBER            = el nodo, desde el - hasta el último dígito
 * operador (binario o unario) = el nodo, strlen de su símbolo
 * BOOLEAN                     = el nodo, largo de verdadero o falso
 * otro con un hijo IDENTIFIER = ese hijo (el nombre declarado), strlen(text)
 * otro sin hijo IDENTIFIER    = el nodo, 1
 */
static void semantic_error(ASTNode *node, const char *fmt, ...) {
    int line = 0;
    int column = 0;
    int length = 1;

    if (node != NULL) {

        ASTNode *name =
            node->kind == AST_IDENTIFIER
                ? node
                : find_child(node, AST_IDENTIFIER);

        // Número literal (con o sin signo): el señalador cubre el número.
        ASTNode *number =
            node->kind == AST_NEG ? node->child : node;

        if (number != NULL && number->kind == AST_NUMBER) {
            line = node->line;
            column = node->column;
            length = count_digits(number->number);

            // Con signo: desde el - hasta el último dígito.
            if (number != node)
                length += number->line == node->line
                              ? number->column - node->column
                              : 1;
        }
        // Operador: el nodo está en la posición del operador (parser.y).
        // Va antes de buscar el hijo IDENTIFIER, que sería un operando.
        else if (operator_symbol(node->kind) != NULL) {
            line = node->line;
            column = node->column;
            length = (int) strlen(operator_symbol(node->kind));
        }
        else if (node->kind == AST_BOOLEAN) {
            line = node->line;
            column = node->column;
            length = node->number ? 9 : 5;   // verdadero / falso
        }
        else if (name != NULL && name->text != NULL) {
            line = name->line;
            column = name->column;
            length = (int) strlen(name->text);
        }
        else {
            line = node->line;
            column = node->column;
        }
    }

    va_list args;

    va_start(args, fmt);
    diag_verror("error semantico", line, column, length, fmt, args);
    va_end(args);
}

// Libera un símbolo que todavía no pertenece a ningún ámbito.
static void discard_symbol(Symbol *symbol) {
    free(symbol->name);
    free(symbol);
}

// Memory

// M-2: variables de ccom en 0x0000–0x0FFF; desde 0x1000, archivos de load_file
#define DATA_BASE  0x0000
#define DATA_LIMIT 0x1000          // primera dirección que NO se puede usar

static int next_address = DATA_BASE;   // libro §6.3.4: el "offset"

// El desborde de la memoria de datos se reporta una sola vez por programa.
static int memory_reported = 0;

// Libro §6.3.4: el nombre recibe el offset actual y el offset avanza su ancho.
// node es el nodo que declara lo que se reserva (para el rl, la función).
// Si no cabe se reporta (solo la primera vez) y se sigue asignando.
static void reserve(int width, const char *what, ASTNode *node,
                    int *out_address) {
    if (next_address + width > DATA_LIMIT &&
        !memory_reported) {

        semantic_error(node,
                       "'%s' no cabe en la memoria de datos (limite 0x%04X)",
                       what,
                       (unsigned int) (DATA_LIMIT - 1));

        memory_reported = 1;
    }

    *out_address = next_address;
    next_address += width;
}

// M-1: int y bool ocupan 4 bytes (alineados para lw/sw)
static int type_width(SymbolType type) {
    switch (type) {
        case TYPE_INT:
        case TYPE_BOOL: return 4;
        default:        return 0;
    }
}

// Llena dimensions y dim_sizes a partir del nodo DIMENSIONS.
// El ancho no se calcula aquí: es distinto en declaraciones y en parámetros.
// node es la declaración o el parámetro, para ubicar el error.
static void fill_dimensions(Symbol *symbol, ASTNode *dimensions,
                            ASTNode *node) {

    // A-2: en esa posición puede llegar otro nodo (no DIMENSIONS).
    if (dimensions == NULL ||
        dimensions->kind != AST_DIMENSIONS)
        return;

    // Los hijos de DIMENSIONS son nodos NUMBER con el tamaño en number.
    ASTNode *dimension = dimensions->child;

    int reported = 0;

    while (dimension != NULL &&
           symbol->dimensions < MAX_DIMS) {

        // S-06: una dimensión de tamaño 0 no reserva nada. Se reporta una vez
        // por declaración y se sigue: el tamaño 0 queda guardado.
        if (dimension->number <= 0 && !reported) {

            semantic_error(node,
                           "el tamano de un arreglo debe ser mayor que cero");

            reported = 1;
        }

        // Guardar el tamaño antes de incrementar dimensions.
        symbol->dim_sizes[symbol->dimensions] = (int) dimension->number;
        symbol->dimensions++;

        dimension = dimension->next;
    }
}

// Name checking

// Primer ámbito de la función actual (en la lista de todos los ámbitos) que
// contiene el nombre; NULL si no hay ninguno o si no se está en una función.
static Scope *find_closed_scope(const char *name) {
    if (current_function == NULL)
        return NULL;

    Scope *scope = scope_list_head;

    while (scope != NULL) {

        if (scope->owner == current_function &&
            symbol_lookup_current(scope, name) != NULL)
            return scope;

        scope = scope->next;
    }

    return NULL;
}

// Libro §1.6.3 y §2.7: ámbito estático, regla del bloque anidado más cercano.
// Un nombre usado antes de su declaración todavía no está en la tabla.
// Se reporta cada uso de un nombre que no existe.
// Devuelve el símbolo del nombre, o NULL si no existe (ya reportado).
static Symbol *resolve_name(ASTNode *identifier) {
    if (identifier == NULL || identifier->text == NULL)
        return NULL;

    Symbol *symbol = symbol_lookup(current_scope, identifier->text);

    if (symbol != NULL) {

        // Enlace uso -> símbolo: lo usará la generación de código.
        identifier->symbol = symbol;

        return symbol;
    }

    // Fuera de alcance: el nombre existe en un bloque de esta función que ya
    // se cerró. No hace falta excluir los ámbitos abiertos: symbol_lookup ya
    // buscó en el actual y en sus padres.
    Scope *closed = find_closed_scope(identifier->text);

    if (closed != NULL) {

        semantic_error(identifier,
                       "'%s' esta fuera de alcance; "
                       "fue declarado en el bloque de la linea %d",
                       identifier->text,
                       closed->line);

        return NULL;
    }

    semantic_error(identifier,
                   "'%s' no ha sido declarado",
                   identifier->text);

    return NULL;
}

// Types

// Tipo de una expresión.
typedef enum {
    ET_ERROR,   // ya hubo un error en esta expresión: no reportar nada más sobre ella
    ET_INT,
    ET_BOOL,
    ET_VOID,    // resultado de llamar a una función void
    ET_ARRAY    // arreglo completo (nombre suelto de un arreglo)
} ExprKind;

typedef struct {
    ExprKind kind;
    SymbolType base;       // ET_ARRAY: tipo de los elementos
    int dimensions;        // ET_ARRAY: 1 o 2
    int sizes[MAX_DIMS];   // ET_ARRAY: tamaño de cada dimensión

    int is_const;          // 1 si la expresión es una constante entera conocida
    long value;            // su valor (solo si is_const)
} ExprType;

// Rango de un int de 32 bits con signo.
#define INT32_LOWEST  (-2147483648LL)
#define INT32_HIGHEST 2147483647LL

#define TYPE_TEXT_SIZE 64   // alcanza para "bool[N][M]"

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

// Tipo escalar que corresponde a un tipo declarado.
static ExprType scalar_type(SymbolType type) {
    switch (type) {
        case TYPE_INT:  return make_type(ET_INT);
        case TYPE_BOOL: return make_type(ET_BOOL);
        default:        return make_type(ET_VOID);
    }
}

// Tipo de un símbolo usado por su nombre suelto: escalar o arreglo completo.
// Un arreglo con una dimensión 0 ya se reportó (S-06): cuenta como ET_ERROR
// para no provocar más errores.
static ExprType symbol_value_type(Symbol *symbol) {
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

// Mismo tipo: igual clase y, en arreglos, igual base, dimensiones y tamaños.
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

// Texto del tipo para los mensajes: int, bool, void, int[3], bool[2][4].
static const char *type_text(ExprType type, char *buffer) {
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

// Exige un tipo en un lugar (condición, límite de S, índice). ET_ERROR no
// reporta nada: el error ya se dijo.
static void expect_type(ASTNode *node, ExprType type, ExprKind wanted,
                        const char *message) {
    if (type.kind != ET_ERROR &&
        type.kind != wanted)
        semantic_error(node, "%s", message);
}

// Asignación de un valor a un destino (ASSIGNMENT, o DECLARATION con un
// inicializador que no es lista). El error se reporta sobre el destino.
static void check_assignable(ASTNode *target, ExprType destination,
                             ExprType value) {
    if (destination.kind == ET_ERROR ||
        value.kind == ET_ERROR ||
        same_type(destination, value))
        return;

    char value_text[TYPE_TEXT_SIZE];
    char destination_text[TYPE_TEXT_SIZE];

    type_text(value, value_text);
    type_text(destination, destination_text);

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

// Call graph

// S-05: sin recursión no hace falta pila. Cada llamada de la pasada 2 se
// guarda como (función que llama, función llamada, nodo CALL), en el orden
// en que aparece. R no es una llamada.
typedef struct {
    Symbol *caller;
    Symbol *callee;
    ASTNode *call;
} CallEdge;

static CallEdge *call_edges = NULL;
static int call_count = 0;
static int call_capacity = 0;

// Funciones analizadas en la pasada 2, en el orden del archivo.
static Symbol **function_list = NULL;
static int function_count = 0;
static int function_capacity = 0;

static void add_call(Symbol *caller, Symbol *callee, ASTNode *call) {
    if (call_count == call_capacity) {

        int capacity = call_capacity == 0 ? 16 : call_capacity * 2;

        CallEdge *edges =
            realloc(call_edges, (size_t) capacity * sizeof(CallEdge));

        if (edges == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        call_edges = edges;
        call_capacity = capacity;
    }

    call_edges[call_count].caller = caller;
    call_edges[call_count].callee = callee;
    call_edges[call_count].call = call;
    call_count++;
}

static void add_function(Symbol *function) {
    if (function_count == function_capacity) {

        int capacity = function_capacity == 0 ? 16 : function_capacity * 2;

        Symbol **list =
            realloc(function_list, (size_t) capacity * sizeof(Symbol *));

        if (list == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(EXIT_FAILURE);
        }

        function_list = list;
        function_capacity = capacity;
    }

    function_list[function_count] = function;
    function_count++;
}

// Libera el grafo (los símbolos y los nodos no son suyos).
static void free_call_graph(void) {
    free(call_edges);
    free(function_list);

    call_edges = NULL;
    call_count = 0;
    call_capacity = 0;

    function_list = NULL;
    function_count = 0;
    function_capacity = 0;
}

// Uses

// A-1: la gramática produce el mismo nodo para v [ i ] y para f [ x ]. Cada
// uso se resuelve contra la tabla de símbolos, según dónde aparece.
typedef enum {
    USE_EXPRESSION,   // CALL o INDEX dentro de una expresión
    USE_TARGET,       // CALL o INDEX como destino de una asignación
    USE_STATEMENT,    // CALL de un CALL_STATEMENT
    USE_REWIND        // REWIND
} UseContext;

// Devuelve el tipo de la expresión y verifica nombres, usos y tipos.
// Cada expresión se evalúa UNA sola vez, para no repetir sus errores.
static ExprType check_expression(ASTNode *node);

// Cantidad de hijos de node.
static int count_children(ASTNode *node) {
    int count = 0;

    ASTNode *child = node ? node->child : NULL;

    while (child != NULL) {
        count++;
        child = child->next;
    }

    return count;
}

// Nombre suelto (en una expresión o como destino): una función solo puede
// usarse invocándola. Un arreglo suelto es el arreglo completo.
static ExprType check_identifier(ASTNode *identifier) {
    Symbol *symbol = resolve_name(identifier);

    if (symbol == NULL)
        return make_type(ET_ERROR);

    if (symbol->kind == SYMBOL_FUNCTION) {

        semantic_error(identifier,
                       "'%s' es una funcion; debe invocarse con corchetes",
                       identifier->text);

        return make_type(ET_ERROR);
    }

    return symbol_value_type(symbol);
}

// Llamada a una función: punto de entrada y cantidad de argumentos.
// node es el CALL o el REWIND. Devuelve 0 si hubo error.
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

// Argumento número position (desde 0) de una llamada, contra el parámetro en
// la misma posición. Solo se llama si la cantidad de argumentos coincide.
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
    ExprType expected = symbol_value_type(parameter);

    if (expected.kind == ET_ERROR ||
        same_type(expected, type))
        return;

    char expected_text[TYPE_TEXT_SIZE];
    char received_text[TYPE_TEXT_SIZE];

    semantic_error(argument,
                   "argumento %d de '%s': se esperaba %s, se recibio %s",
                   position + 1,
                   function->name,
                   type_text(expected, expected_text),
                   type_text(type, received_text));
}

// Índice número position (desde 0) de un acceso: si es una constante
// conocida (un literal o una expresión constante como 2 + 3), debe estar
// dentro de esa dimensión. Los índices variables no se verifican (§5.4.5).
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

// Evalúa un argumento o índice (una sola vez) y lo verifica según lo que
// resultó ser el nodo que lo contiene. valid indica que la cantidad de
// argumentos o de índices es correcta.
static void check_operand(ASTNode *operand, Symbol *symbol,
                          Resolution resolution, int valid, int position) {
    ExprType type = check_expression(operand);

    if (resolution == RESOLVED_ACCESS) {

        expect_type(operand, type, ET_INT,
                    "el indice debe ser de tipo int");

        if (valid)
            check_index_range(operand, type, symbol, position);
    }
    else if (resolution == RESOLVED_CALL && valid) {
        check_argument(operand, type, symbol, position);
    }
}

// CALL, INDEX y REWIND: resuelve el nombre, verifica que se use bien según
// el contexto y devuelve el tipo del resultado (ET_VOID si es una llamada a
// una función void; ET_ERROR si hubo un error de resolución o de cantidad de
// índices o argumentos). Los argumentos e índices se verifican aunque haya
// error.
static ExprType check_call(ASTNode *node, UseContext context) {
    ASTNode *arguments = find_child(node, AST_ARGUMENTS);

    int n_args = count_children(arguments);

    // Si el nombre no existe ya se reportó y no se dice nada más de este uso.
    Symbol *symbol = resolve_name(find_child(node, AST_IDENTIFIER));

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
        symbol != current_function) {

        semantic_error(node,
                       "'R' solo puede reiniciar la funcion actual");
    }
    else if (symbol != NULL) {

        int is_function = symbol->kind == SYMBOL_FUNCTION;
        int is_array = !is_function && symbol->dimensions > 0;

        // En una expresión, un CALL es llamada o acceso; un INDEX solo puede
        // ser acceso. Solo se puede asignar a un elemento de un arreglo. Una
        // sentencia de llamada y un R solo admiten funciones.
        if (context == USE_EXPRESSION &&
            is_function && node->kind == AST_CALL)
            resolution = RESOLVED_CALL;
        else if ((context == USE_STATEMENT || context == USE_REWIND) &&
                 is_function)
            resolution = RESOLVED_CALL;
        else if ((context == USE_EXPRESSION || context == USE_TARGET) &&
                 is_array)
            resolution = RESOLVED_ACCESS;

        if (resolution == RESOLVED_CALL) {

            valid = check_invocation(node,
                                     symbol,
                                     n_args,
                                     context == USE_REWIND);

            // S-05: R no es una llamada, y llamar a main ya es un error.
            if (context != USE_REWIND &&
                strcmp(symbol->name, "main") != 0) {

                if (symbol == current_function) {

                    // Recursión directa: no se agrega al grafo.
                    semantic_error(node,
                                   "'%s' se invoca a si misma; "
                                   "la recursion debe expresarse con 'R'",
                                   symbol->name);

                    poisoned = 1;
                }
                else if (current_function != NULL) {
                    add_call(current_function, symbol, node);
                }
            }
        }
        else if (resolution == RESOLVED_ACCESS) {

            // Todos los indices en ARGUMETNS.
            int used = n_args;

            valid = used == symbol->dimensions;

            if (!valid)
                semantic_error(node,
                               "'%s' tiene %d dimensiones, se usaron %d indices",
                               symbol->name,
                               symbol->dimensions,
                               used);
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

    // INDEX: el segundo índice es el hijo que sigue a ARGUMENTS (cualquier
    // expresión, incluso un IDENTIFIER).
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

// Literal entero. Todo NUMBER mayor que 2147483647 desborda, salvo
// 2147483648 como operando directo de un - unario (negated), para que
// -2147483648 sea válido.
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

// base ^ exponent con exponent >= 0 (x ^ 0 = 1, también 0 ^ 0), por
// cuadrados sucesivos. *overflow = 1 si el resultado no cabe en 32 bits.
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

// Valor constante de un operador aritmético cuyos operandos ya son int.
// Aritmética de un int de 32 bits con signo, calculada en 64 bits. La
// división trunca hacia cero y % cumple a == (a/b)*b + a%b (C99).
// Reporta sobre el operador y devuelve ET_ERROR si hay división entre cero,
// exponente negativo o desborde.
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

// Operador binario o unario. Se reporta un error por operador como máximo.
// Si algún operando es ET_ERROR no se reporta nada y el resultado es el de
// su familia. Un operador que reporta error devuelve ET_ERROR, para no
// provocar otro error en el operador de afuera.
static ExprType check_operator(ASTNode *node) {
    const char *symbol = operator_symbol(node->kind);

    ASTNode *first = node->child;
    ASTNode *second = first ? first->next : NULL;   // NULL en los unarios

    // -2147483648: el literal es operando directo del - unario.
    ExprType left =
        (node->kind == AST_NEG && first != NULL && first->kind == AST_NUMBER)
            ? number_type(first, 1)
            : check_expression(first);

    ExprType right = second ? check_expression(second) : left;

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
                               type_text(left.kind != ET_INT ? left : right,
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
                               type_text(left.kind == ET_ARRAY ? left : right,
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

// Devuelve el tipo de la expresión. Nunca devuelve ET_VOID: una función void
// usada como valor se reporta aquí y cuenta como ET_ERROR hacia arriba.
static ExprType check_expression(ASTNode *node) {
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
            ExprType type = check_call(node, USE_EXPRESSION);

            if (type.kind == ET_VOID) {

                ASTNode *name = find_child(node, AST_IDENTIFIER);

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

    if (operator_symbol(node->kind) != NULL)
        return check_operator(node);

    // Cualquier otro nodo no es una expresión con tipo: solo se verifican
    // los nombres que contiene.
    ASTNode *child = node->child;

    while (child != NULL) {
        check_expression(child);
        child = child->next;
    }

    return make_type(ET_ERROR);
}

// Condición de un C, A [ ] o L: debe ser bool.
static void check_condition(ASTNode *node) {
    ASTNode *condition = find_condition(node);

    expect_type(condition, check_expression(condition), ET_BOOL,
                "la condicion debe ser de tipo bool");
}

// Destino de una asignación: un nombre suelto, o un elemento de un arreglo.
// Verifica la mutabilidad (§5.2) y devuelve el tipo del destino.
// Es inmutable todo símbolo con MODIFIER_VI: variables Vi, parámetros Vi y la
// variable de un S. Los parámetros sin modificador son modificables.
static ExprType check_target(ASTNode *target) {
    if (target == NULL)
        return make_type(ET_ERROR);

    if (target->kind == AST_CALL ||
        target->kind == AST_INDEX) {

        ExprType type = check_call(target, USE_TARGET);

        ASTNode *name = find_child(target, AST_IDENTIFIER);
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

    ExprType type = check_expression(target);

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

// Asignación: mutabilidad del destino (§5.2) y después tipos (§5.1).
static void check_assignment(ASTNode *statement) {
    ASTNode *target = statement->child;               // primer hijo
    ASTNode *value = target ? target->next : NULL;    // segundo hijo

    ExprType destination = check_target(target);

    check_assignable(target, destination, check_expression(value));
}

// Verifica los nombres de todos los valores de una lista, sin exigir tipos.
static void check_list_names(ASTNode *node) {
    if (node == NULL)
        return;

    if (node->kind != AST_INITIALIZER_LIST) {
        check_expression(node);
        return;
    }

    ASTNode *child = node->child;

    while (child != NULL) {
        check_list_names(child);
        child = child->next;
    }
}

// Tipo incorrecto de los valores de una lista: se reporta una sola vez por
// declaración, con el tipo del primer valor incorrecto (ET_ERROR = ninguno).
static void report_list_type(ExprType wrong, Symbol *symbol, ASTNode *node) {
    if (wrong.kind == ET_ERROR)
        return;

    char wrong_text[TYPE_TEXT_SIZE];
    char base_text[TYPE_TEXT_SIZE];

    semantic_error(node,
                   "no se puede asignar '%s' a una variable '%s'",
                   type_text(wrong, wrong_text),
                   type_text(scalar_type(symbol->type), base_text));
}

// Valores de una lista de una dimensión, o de una fila: ninguno es una lista
// (se reporta una vez por lista o por fila) y todos son del tipo base del
// arreglo. El primer tipo incorrecto se guarda en wrong, que comparten todas
// las filas: lo reporta quien llama, con report_list_type.
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

            ExprType type = check_expression(value);

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

// Forma y tipos de una lista de inicialización contra la declaración.
// Todos los errores se reportan sobre la DECLARATION (node). Las filas se
// numeran desde 0, como los índices.
static void check_initializer_list(Symbol *symbol, ASTNode *list,
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

    int count = count_children(list);

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

            check_expression(row);
        }
        else {

            int values = count_children(row);

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

// Parámetro de la función con ese nombre, o NULL.
static Symbol *find_parameter(Symbol *function, const char *name) {
    for (int i = 0; i < function->parameter_count; i++) {

        if (strcmp(function->param_list[i]->name, name) == 0)
            return function->param_list[i];
    }

    return NULL;
}

// Un local (o la variable de un S) no puede llamarse como un parámetro de
// su función, en ningún bloque. Las globales no se revisan.
// node es la declaración local, para ubicar el error. Devuelve 0 si choca.
static int check_not_parameter(const char *name, ASTNode *node) {
    if (current_function == NULL ||
        find_parameter(current_function, name) == NULL)
        return 1;

    semantic_error(node,
                   "'%s' ya es un parametro de esta funcion",
                   name);

    return 0;
}

// Inserta symbol en el ámbito actual. Si el nombre ya existe en ese ámbito,
// reporta el error en la nueva declaración (node), señalando la línea del
// símbolo existente, y descarta symbol: los usos posteriores se refieren al
// primero.
static int insert_symbol(Symbol *symbol, ASTNode *node) {
    Symbol *existing =
        symbol_lookup_current(current_scope, symbol->name);

    if (existing != NULL ||
        !symbol_insert(current_scope, symbol)) {

        semantic_error(node,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       symbol->name,
                       existing ? existing->line : 0);

        // symbol_insert() didn't take ownership because insertion failed.
        discard_symbol(symbol);

        return 0;
    }

    return 1;
}

/*
 * Las funciones analyze_* devuelven 0 solo ante un error interno (un nodo
 * inválido), que detiene el análisis. Los errores semánticos se reportan con
 * semantic_error y el análisis sigue.
 */

// Variable declaration

static int analyze_declaration(ASTNode *node) {
    if (node == NULL)
        return 1;

    /*
     * DECLARATION
     *
     * child 0 = modifier
     * child 1 = identifier
     * child 2 = type
     * child 3 = dimensions
     * child 4 = initializer
     */

    ASTNode *modifier = node->child;

    ASTNode *identifier =
        modifier ? modifier->next : NULL;

    ASTNode *type =
        identifier ? identifier->next : NULL;

    ASTNode *dimensions =
        type ? type->next : NULL;

    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {

        fprintf(stderr,
                "Semantic error: invalid declaration\n");

        return 0;
    }

    // A-2: el inicializador es el hijo que sigue a TYPE o a DIMENSIONS y que
    // no es DIMENSIONS.
    ASTNode *initializer = dimensions;

    if (initializer != NULL &&
        initializer->kind == AST_DIMENSIONS)
        initializer = initializer->next;

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;
    symbol->decl_node = node;

    identifier->symbol = symbol;

    fill_dimensions(symbol, dimensions, node);

    // El inicializador se verifica antes de insertar el símbolo:
    // Vm x : int = x ;  usa el x de afuera, o es error.
    // Una lista se verifica contra la forma del arreglo; cualquier otro
    // valor, como una asignación a la variable declarada (sin mutabilidad:
    // es su valor inicial).
    if (initializer != NULL &&
        initializer->kind == AST_INITIALIZER_LIST)
        check_initializer_list(symbol, initializer, node);
    else if (initializer != NULL)
        check_assignable(node,
                         symbol_value_type(symbol),
                         check_expression(initializer));

    // Un local que oculta un parámetro no se inserta.
    if (!check_not_parameter(symbol->name, node)) {

        discard_symbol(symbol);

        return 1;
    }

    // Libro §6.3.4: ancho = ancho del tipo × producto de las dimensiones
    int d1 = symbol->dimensions > 0 ? symbol->dim_sizes[0] : 1;
    int d2 = symbol->dimensions > 1 ? symbol->dim_sizes[1] : 1;

    symbol->width = type_width(symbol->type) * d1 * d2;   // d = 1 si no aplica

    // Una redeclaración no se inserta.
    if (!insert_symbol(symbol, node))
        return 1;

    reserve(symbol->width, symbol->name, node, &symbol->address);

    return 1;
}


// Parameters

// Crea el símbolo de un parámetro, sin insertarlo en ningún ámbito ni darle
// dirección. Devuelve NULL solo si el nodo es inválido.
static Symbol *create_parameter(ASTNode *node) {
    /*
     * PARAMETER
     *
     * MODIFIER (opcional), TYPE, IDENTIFIER, DIMENSIONS (opcional)
     */
    ASTNode *modifier = find_child(node, AST_MODIFIER);
    ASTNode *type = find_child(node, AST_TYPE);
    ASTNode *identifier = find_child(node, AST_IDENTIFIER);
    ASTNode *dimensions = find_child(node, AST_DIMENSIONS);

    if (identifier == NULL) {

        fprintf(stderr,
                "Semantic error: invalid parameter\n");

        return NULL;
    }

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_PARAMETER,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;
    symbol->decl_node = node;

    fill_dimensions(symbol, dimensions, node);

    // escalar: el valor; arreglo: su dirección (paso por referencia)
    symbol->width = 4;

    return symbol;
}


// Statements

static int analyze_statement(ASTNode *statement);

// Recorre las sentencias de un BLOCK en el ámbito actual (no abre ámbito).
static int analyze_statements(ASTNode *block) {
    if (block == NULL || block->kind != AST_BLOCK)
        return 1;

    ASTNode *statement = block->child;
    ASTNode *previous = NULL;

    int unreachable_reported = 0;

    while (statement != NULL) {

        // E y R transfieren el control: lo que sigue en el mismo bloque no
        // se ejecuta. Se reporta una vez por bloque (la primera sentencia) y
        // se sigue analizando.
        if (!unreachable_reported &&
            previous != NULL &&
            (previous->kind == AST_EMIT || previous->kind == AST_REWIND)) {

            semantic_error(statement,
                           "codigo inalcanzable: "
                           "la sentencia anterior transfiere el control");

            unreachable_reported = 1;
        }

        if (!analyze_statement(statement))
            return 0;

        previous = statement;
        statement = statement->next;
    }

    return 1;
}

// Abre un ámbito del tipo dado, recorre el BLOCK y lo cierra.
// Libro §1.6.3 y §2.7: cada bloque tiene su propio ámbito anidado.
static int analyze_block(ASTNode *block, ScopeKind kind) {

    // P-03: la línea del BLOCK es la de la palabra clave que lo abre.
    enter_scope(kind, block ? block->line : 0);

    int result = analyze_statements(block);

    exit_scope();

    return result;
}

// Condicional: un ámbito por cada rama. Cada condición se verifica antes de
// abrir el ámbito de su rama.
static int analyze_conditional(ASTNode *node) {

    // Rama inicial  C [cond] ;
    check_condition(node);

    if (!analyze_block(find_child(node, AST_BLOCK), SCOPE_C))
        return 0;

    // Ramas alternativas  A [cond] ;
    ASTNode *alternatives = find_child(node, AST_ALTERNATIVES);
    ASTNode *alternative = alternatives ? alternatives->child : NULL;

    while (alternative != NULL) {

        if (alternative->kind == AST_ALTERNATIVE) {

            check_condition(alternative);

            if (!analyze_block(find_child(alternative, AST_BLOCK),
                               SCOPE_A))
                return 0;
        }

        alternative = alternative->next;
    }

    // Rama residual  A ;
    ASTNode *residual = find_child(node, AST_RESIDUAL);

    if (residual != NULL &&
        !analyze_block(find_child(residual, AST_BLOCK),
                       SCOPE_RESIDUAL))
        return 0;

    return 1;
}

// Ciclo por predicado: la condición se verifica antes de abrir el ámbito.
static int analyze_loop_l(ASTNode *node) {
    check_condition(node);

    return analyze_block(find_child(node, AST_BLOCK), SCOPE_L);
}

// Ciclo acotado: el ámbito del S contiene su variable de control y su cuerpo.
static int analyze_loop_s(ASTNode *node) {
    /*
     * LOOP_S (posición fija: los 5 hijos siempre existen)
     *
     * child 0 = identifier (variable de control)
     * child 1 = inicio
     * child 2 = fin
     * child 3 = paso (NUMBER)
     * child 4 = body
     */
    ASTNode *identifier = node->child;
    ASTNode *start =
        identifier ? identifier->next : NULL;
    ASTNode *end =
        start ? start->next : NULL;

    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {
        fprintf(stderr,
                "Semantic error: invalid S loop\n");
        return 0;
    }

    // Inicio y fin se verifican antes de abrir el ámbito del S:
    // S [ i , 0 , i , 1 ]  usa el i de afuera.
    ExprType start_type = check_expression(start);
    ExprType end_type = check_expression(end);

    expect_type(start, start_type, ET_INT,
                "el limite de 'S' debe ser de tipo int");
    expect_type(end, end_type, ET_INT,
                "el limite de 'S' debe ser de tipo int");

    // El paso es un NUMBER con su signo ya aplicado.
    ASTNode *step = end ? end->next : NULL;

    if (step != NULL && step->kind == AST_NUMBER) {

        if (step->number == 0) {

            semantic_error(step, "el paso de 'S' no puede ser cero");
        }
        else if (start_type.is_const && end_type.is_const) {

            // Con inicio y fin constantes se conoce el sentido. El fin es
            // exclusivo: el ciclo solo se ejecuta si el paso se acerca a él.
            int runs =
                (step->number > 0 && start_type.value < end_type.value) ||
                (step->number < 0 && start_type.value > end_type.value);

            if (!runs)
                semantic_error(node,
                               "el ciclo 'S' nunca se ejecuta: "
                               "inicio %ld, fin %ld, paso %ld",
                               start_type.value,
                               end_type.value,
                               step->number);
        }
    }

    // Si la variable de control oculta un parámetro no se crea: dentro del
    // cuerpo, el nombre se refiere al parámetro.
    int declare = check_not_parameter(identifier->text, node);

    enter_scope(SCOPE_S, node->line);

    if (declare) {

        Symbol *control =
            symbol_create(identifier->text,
                          SYMBOL_VARIABLE,
                          TYPE_INT);

        // el ciclo la controla; el cuerpo no puede modificarla
        control->modifier = MODIFIER_VI;
        control->width = type_width(control->type);   // M-1
        control->line = node->line;
        control->decl_node = node;

        if (insert_symbol(control, node)) {
            reserve(control->width,
                    control->name,
                    node,
                    &control->address);

            identifier->symbol = control;
        }
    }

    // El cuerpo comparte el ámbito del S con su variable (no abre otro).
    int result = analyze_statements(find_child(node, AST_BLOCK));

    exit_scope();

    return result;
}

// CRYPTO

// Zona de datos de la memoria y tamaño de un bloque de cifrado, en bytes.
#define DATA_ZONE_FIRST    4096
#define DATA_ZONE_LAST     65535
#define CRYPTO_BLOCK_BYTES 8

// Argumento expr número position (desde 1) de una operación: debe ser int.
// Mismo mensaje que en las llamadas. ET_ERROR no reporta nada más.
static ExprType check_crypto_argument(ASTNode *argument, int position,
                                      const char *operation) {
    ExprType type = check_expression(argument);

    if (type.kind != ET_ERROR &&
        type.kind != ET_INT) {

        char received_text[TYPE_TEXT_SIZE];

        semantic_error(argument,
                       "argumento %d de '%s': se esperaba int, se recibio %s",
                       position,
                       operation,
                       type_text(type, received_text));
    }

    return type;
}

// k o w: un NUMBER escrito tal cual (lo garantiza la gramática), entre 0 y 3.
static void check_crypto_selector(ASTNode *number, const char *name,
                                  const char *operation) {
    if (number == NULL || number->kind != AST_NUMBER)
        return;

    if (number->number < 0 || number->number > 3)
        semantic_error(number,
                       "el argumento '%s' de '%s' debe estar entre 0 y 3",
                       name,
                       operation);
}

// Dirección de origen o destino: solo se verifica si es constante. Un solo
// error por dirección: primero la zona de datos; si pasa, la alineación.
// Devuelve 1 si es una constante que pasó las dos verificaciones.
static int check_crypto_address(ASTNode *argument, ExprType type,
                                int position, const char *operation) {
    if (type.kind != ET_INT || !type.is_const)
        return 0;

    if (type.value < DATA_ZONE_FIRST || type.value > DATA_ZONE_LAST) {

        semantic_error(argument,
                       "argumento %d de '%s': la direccion %ld esta fuera "
                       "de la zona de datos [%d..%d]",
                       position,
                       operation,
                       type.value,
                       DATA_ZONE_FIRST,
                       DATA_ZONE_LAST);

        return 0;
    }

    if (type.value % 4 != 0) {

        semantic_error(argument,
                       "argumento %d de '%s': la direccion %ld no es "
                       "multiplo de 4",
                       position,
                       operation,
                       type.value);

        return 0;
    }

    return 1;
}

// Los bloques (constantes y > 0) desde una dirección ya verificada no pueden
// pasar del final de la memoria. Se calcula en 64 bits.
static void check_crypto_extent(ASTNode *blocks, ExprType blocks_type,
                                ExprType address, const char *operation) {
    long long end =
        (long long) address.value +
        (long long) CRYPTO_BLOCK_BYTES * blocks_type.value;

    if (end > DATA_ZONE_LAST + 1)
        semantic_error(blocks,
                       "argumento 3 de '%s': %ld bloques desde la direccion "
                       "%ld se salen de la memoria",
                       operation,
                       blocks_type.value,
                       address.value);
}

// Operación de CRYPTO: no usa la tabla de símbolos ni el grafo de llamadas.
// La cantidad y la forma de los argumentos las garantiza la gramática; los
// argumentos se verifican en orden.
static void analyze_crypto(ASTNode *node) {
    const char *operation = node->text;

    ASTNode *arguments = find_child(node, AST_ARGUMENTS);
    ASTNode *argument = arguments ? arguments->child : NULL;

    // cerrar_sesion, sellar: sin argumentos.
    if (argument == NULL)
        return;

    // cargar_llave [ k , w , valor ]
    if (strcmp(operation, "cargar_llave") == 0) {

        ASTNode *w = argument->next;

        check_crypto_selector(argument, "k", operation);
        check_crypto_selector(w, "w", operation);

        if (w != NULL && w->next != NULL)
            check_crypto_argument(w->next, 3, operation);

        return;
    }

    // autenticar, cambiar_ancla: una sola expr.
    if (argument->next == NULL) {
        check_crypto_argument(argument, 1, operation);
        return;
    }

    // cifrar, descifrar [ origen , destino , bloques , k ]
    ASTNode *origin = argument;
    ASTNode *destination = origin->next;
    ASTNode *blocks = destination->next;

    if (blocks == NULL)
        return;

    ExprType origin_type = check_crypto_argument(origin, 1, operation);
    int origin_valid =
        check_crypto_address(origin, origin_type, 1, operation);

    ExprType destination_type =
        check_crypto_argument(destination, 2, operation);
    int destination_valid =
        check_crypto_address(destination, destination_type, 2, operation);

    ExprType blocks_type = check_crypto_argument(blocks, 3, operation);

    if (blocks_type.kind == ET_INT &&
        blocks_type.is_const &&
        blocks_type.value > 0) {

        if (origin_valid)
            check_crypto_extent(blocks, blocks_type, origin_type, operation);

        if (destination_valid)
            check_crypto_extent(blocks, blocks_type, destination_type,
                                operation);
    }

    check_crypto_selector(blocks->next, "k", operation);
}

// Decide qué hacer con una sentencia según su kind.
static int analyze_statement(ASTNode *statement) {
    if (statement == NULL)
        return 1;

    switch (statement->kind) {

        case AST_DECLARATION:
            return analyze_declaration(statement);

        case AST_CONDITIONAL:
            return analyze_conditional(statement);

        case AST_LOOP_L:
            return analyze_loop_l(statement);

        case AST_LOOP_S:
            return analyze_loop_s(statement);

        // Destino (primer hijo: IDENTIFIER, CALL o INDEX) y valor (segundo).
        case AST_ASSIGNMENT:
            check_assignment(statement);
            return 1;

        // El CALL de la sentencia debe ser una llamada.
        case AST_CALL_STATEMENT:
            check_call(find_child(statement, AST_CALL), USE_STATEMENT);
            return 1;

        // El nombre debe ser una función; se verifica cada argumento.
        case AST_REWIND:
            check_call(statement, USE_REWIND);
            return 1;

        // Operación de CRYPTO: se verifican sus argumentos.
        case AST_CRYPTO:
            analyze_crypto(statement);
            return 1;

        // EMIT: nada que verificar.
        default:
            return 1;
    }
}


// Function

// Pasada 1: registra la firma de una función (símbolo y parámetros), para que
// una función pueda llamar a otra escrita después.
static int register_signature(ASTNode *node) {
    /*
     * FUNCTION
     *
     * IDENTIFIER, PARAMS, RETURN_TYPE, BLOCK
     */
    ASTNode *identifier = find_child(node, AST_IDENTIFIER);
    ASTNode *params = find_child(node, AST_PARAMS);
    ASTNode *return_type = find_child(node, AST_RETURN_TYPE);

    if (identifier == NULL) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }

    // Función repetida: no se inserta ni se registran sus parámetros. Como
    // ningún símbolo apunta a este nodo, la pasada 2 no lo analiza.
    Symbol *existing =
        symbol_lookup_current(global_scope, identifier->text);

    if (existing != NULL) {

        // Se marca el segundo FUNCTION, señalando la línea del primero.
        semantic_error(node,
                       "la funcion '%s' ya fue declarada (linea %d)",
                       identifier->text,
                       existing->line);

        return 1;
    }

    // RETURN_TYPE sin hijos es void.
    ASTNode *ret_type = find_child(return_type, AST_TYPE);
    ASTNode *ret_id = find_child(return_type, AST_IDENTIFIER);

    // Funcion pertenece a scope global
    Symbol *function =
        symbol_create(identifier->text,
                      SYMBOL_FUNCTION,
                      ret_type ? get_type(ret_type) : TYPE_VOID);

    function->line = node->line;
    function->decl_node = node;

    symbol_insert(global_scope, function);

    /*
     * Los parámetros solo quedan en param_list, que no es dueña: pasan a
     * pertenecer al ámbito de la función cuando la pasada 2 los inserta.
     * La pasada 2 llega a todas las funciones registradas, salvo que un
     * error interno (nodo inválido) detenga el análisis: solo en ese camino
     * quedarían sin liberar.
     */
    ASTNode *parameter = params ? params->child : NULL;

    while (parameter != NULL) {

        Symbol *symbol = create_parameter(parameter);

        if (symbol == NULL)
            return 0;

        // Todos los parámetros comparten el ámbito de la función. Un
        // parámetro duplicado no se agrega a param_list.
        Symbol *first = find_parameter(function, symbol->name);

        if (first != NULL) {

            semantic_error(parameter,
                           "'%s' ya fue declarado en este ambito (linea %d)",
                           symbol->name,
                           first->line);

            discard_symbol(symbol);
        }
        else {

            // Lista ordenada de parámetros de la función (también lo cuenta).
            symbol_add_param(function, symbol);
        }

        parameter = parameter->next;
    }

    // La variable de retorno comparte ese ámbito. En el texto el parámetro
    // aparece primero: el error se marca en la variable de retorno y señala
    // al parámetro. El parámetro se conserva y la pasada 2 no crea la
    // variable de retorno.
    Symbol *clash =
        ret_id ? find_parameter(function, ret_id->text) : NULL;

    if (clash != NULL) {

        semantic_error(return_type,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       ret_id->text,
                       clash->line);
    }

    return 1;
}

// Pasada 1 sobre todas las funciones.
static int register_signatures(ASTNode *functions) {
    if (functions == NULL || functions->kind != AST_FUNCTIONS)
        return 1;

    ASTNode *function = functions->child;

    while (function != NULL) {

        if (!register_signature(function))
            return 0;

        function = function->next;
    }

    return 1;
}

// El programa empieza en main, que no recibe ni devuelve nada.
static void check_main(void) {
    Symbol *main_function = symbol_lookup_current(global_scope, "main");

    if (main_function == NULL ||
        main_function->kind != SYMBOL_FUNCTION) {

        semantic_error(NULL,
                       "el archivo principal debe declarar 'main'");

        return;
    }

    if (main_function->parameter_count > 0 ||
        main_function->type != TYPE_VOID) {

        semantic_error(main_function->decl_node,
                       "'main' debe declararse como F main [] : void");
    }
}

// ¿El subárbol de node contiene algún nodo de ese kind?
static int contains_kind(ASTNode *node, ASTKind kind) {
    if (node == NULL)
        return 0;

    if (node->kind == kind)
        return 1;

    ASTNode *child = node->child;

    while (child != NULL) {

        if (contains_kind(child, kind))
            return 1;

        child = child->next;
    }

    return 0;
}

// Pasada 2: ámbito, direcciones y cuerpo de una función ya registrada.
static int analyze_function(ASTNode *node) {
    /*
     * FUNCTION
     *
     * IDENTIFIER, PARAMS, RETURN_TYPE, BLOCK
     */
    ASTNode *identifier = find_child(node, AST_IDENTIFIER);
    ASTNode *return_type = find_child(node, AST_RETURN_TYPE);
    ASTNode *body = find_child(node, AST_BLOCK);

    if (identifier == NULL) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }

    // La pasada 1 ya creó el símbolo de la función.
    Symbol *function =
        symbol_lookup_current(global_scope, identifier->text);

    // Función repetida (ya reportada en la pasada 1): el símbolo con ese
    // nombre no apunta a este nodo, así que no se analiza.
    if (function == NULL ||
        function->decl_node != node)
        return 1;

    current_function = function;

    // Orden del archivo, para buscar ciclos de llamadas.
    add_function(function);

    /*
     * Área estática de la función (libro §7.1.1):
     *
     * +0  rl guardado (todas excepto main)
     * +4  variable de retorno, si no es void
     * ... parámetros, en orden
     * ... locales de todos sus bloques, en el orden del recorrido
     */
    function->area_base = next_address;

    // M-5: main no guarda rl
    if (strcmp(function->name, "main") != 0) {

        int rl_address;   // no hay símbolo para rl

        reserve(4, function->name, node, &rl_address);
    }

    // Crea function scope.
    enter_scope(SCOPE_FUNCTION, function->line);

    // La variable de retorno va antes que los parámetros (queda en +4).
    // Si se llama como un parámetro (ya reportado en la pasada 1) no se
    // crea: el nombre se refiere al parámetro.
    ASTNode *ret_type = find_child(return_type, AST_TYPE);
    ASTNode *ret_id = find_child(return_type, AST_IDENTIFIER);

    if (ret_id != NULL &&
        find_parameter(function, ret_id->text) == NULL) {

        Symbol *ret = symbol_create(ret_id->text,
                                    SYMBOL_RETURN,
                                    get_type(ret_type));

        ret->modifier = MODIFIER_VM;   // el cuerpo la asigna
        ret->width = type_width(ret->type);   // M-1
        ret->line = return_type->line;
        ret->decl_node = return_type;

        ret_id->symbol = ret;

        if (insert_symbol(ret, return_type))
            reserve(ret->width, ret->name, return_type, &ret->address);
    }

    // Los parámetros ya existen en param_list: desde aquí pertenecen al
    // ámbito de la función. La inserción no puede fallar (los duplicados se
    // descartaron en la pasada 1).
    for (int i = 0; i < function->parameter_count; i++) {

        Symbol *parameter = function->param_list[i];

        if (!symbol_insert(current_scope, parameter))
            continue;

        reserve(parameter->width,
                parameter->name,
                parameter->decl_node,
                &parameter->address);
    }

    // S-01: el cuerpo se recorre completo, en el mismo ámbito de la función.
    if (!analyze_statements(body))
        return 0;

    // Toda función debe tener al menos un E en su cuerpo, contando los
    // bloques anidados (también main). Con R y sin E nunca terminaría.
    if (!contains_kind(body, AST_EMIT)) {

        if (contains_kind(body, AST_REWIND))
            semantic_error(node,
                           "la funcion '%s' usa 'R' pero no contiene ningun 'E'; "
                           "la recursion nunca terminaria",
                           function->name);
        else
            semantic_error(node,
                           "la funcion '%s' no contiene ninguna sentencia 'E'",
                           function->name);
    }

    // El área termina donde quedó el offset.
    function->area_size = next_address - function->area_base;

    // Return to global scope.
    exit_scope();

    current_function = NULL;

    return 1;
}


// Recursion

// Colores del recorrido en profundidad.
enum { WHITE, GRAY, BLACK };

// Posición de la función en function_list, o -1.
static int function_index(Symbol *function) {
    for (int i = 0; i < function_count; i++) {

        if (function_list[i] == function)
            return i;
    }

    return -1;
}

// Reporta el ciclo que cierra una llamada hacia una función gris: las
// funciones desde esa (stack[from]) hasta la actual (stack[depth]), más la
// primera de nuevo. Se reporta sobre el CALL de esa llamada.
static void report_cycle(int *stack, int from, int depth, ASTNode *call) {
    size_t size = 1;

    for (int i = from; i <= depth; i++)
        size += strlen(function_list[stack[i]]->name) + 4;   // " -> "

    size += strlen(function_list[stack[from]]->name);

    char *text = malloc(size);

    if (text == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    text[0] = '\0';

    for (int i = from; i <= depth; i++) {
        strcat(text, function_list[stack[i]]->name);
        strcat(text, " -> ");
    }

    strcat(text, function_list[stack[from]]->name);

    semantic_error(call, "ciclo de llamadas: %s", text);

    free(text);
}

// Recorre las llamadas de la función número index, que queda en la posición
// depth de la pila del recorrido.
static void visit_function(int index, int *colors, int *stack, int depth) {
    colors[index] = GRAY;
    stack[depth] = index;

    for (int e = 0; e < call_count; e++) {

        if (call_edges[e].caller != function_list[index])
            continue;

        // Solo la primera llamada de cada par (quien llama, a quien llama),
        // para que cada ciclo se reporte una vez.
        int repeated = 0;

        for (int p = 0; p < e && !repeated; p++)
            repeated = call_edges[p].caller == call_edges[e].caller &&
                       call_edges[p].callee == call_edges[e].callee;

        if (repeated)
            continue;

        int callee = function_index(call_edges[e].callee);

        if (callee < 0)
            continue;

        if (colors[callee] == GRAY) {

            // La función llamada está en la pila: hay un ciclo.
            int from = 0;

            while (stack[from] != callee)
                from++;

            report_cycle(stack, from, depth, call_edges[e].call);
        }
        else if (colors[callee] == WHITE) {
            visit_function(callee, colors, stack, depth + 1);
        }
    }

    // Una función negra no se vuelve a recorrer.
    colors[index] = BLACK;
}

// S-05: recursión indirecta. Recorrido en profundidad (blanco/gris/negro)
// del grafo de llamadas, empezando por las funciones en orden del archivo.
static void check_cycles(void) {
    if (function_count == 0)
        return;

    int *colors = calloc((size_t) function_count, sizeof(int));   // WHITE
    int *stack = calloc((size_t) function_count, sizeof(int));

    if (colors == NULL || stack == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < function_count; i++) {

        if (colors[i] == WHITE)
            visit_function(i, colors, stack, 0);
    }

    free(colors);
    free(stack);
}

// Program

int semantic_analyze(ASTNode *root) {
    if (root == NULL)
        return 0;

    if (root->kind != AST_PROGRAM) {
        fprintf(stderr,
                "Semantic error: root is not PROGRAM\n");
        return 0;
    }

    // Los errores semánticos se acumulan: el resultado sale del contador.
    int errors_before = diag_count();

    enter_global_scope();

    next_address = DATA_BASE;
    memory_reported = 0;
    current_function = NULL;

    free_call_graph();

    /*
     * PROGRAM
     *
     * GLOBALS, FUNCTIONS
     */
    ASTNode *globals = find_child(root, AST_GLOBALS);
    ASTNode *functions = find_child(root, AST_FUNCTIONS);

    // Global declarations.
    ASTNode *decl = globals ? globals->child : NULL;

    while (decl != NULL) {

        if (!analyze_declaration(decl))
            return 0;

        decl = decl->next;
    }

    // Pasada 1: todas las firmas antes que cualquier cuerpo.
    if (!register_signatures(functions))
        return 0;

    check_main();

    // Pasada 2: ámbitos, direcciones y cuerpos.
    ASTNode *function = functions ? functions->child : NULL;

    while (function != NULL) {

        if (!analyze_function(function))
            return 0;

        function = function->next;
    }

    // Recursión indirecta: después de analizar todos los cuerpos.
    check_cycles();

    return diag_count() == errors_before;
}

// Symbol table output

void semantic_print_symbols(void) {
    symbol_table_print(scope_list_head);
}

void semantic_free(void)
{
    Scope *scope = scope_list_head;

    while (scope != NULL) {

        Scope *next = scope->next;

        /*
         * Prevent scope_free from following
         * anything unintended.
         */
        scope->next = NULL;

        scope_free(scope);

        scope = next;
    }

    scope_list_head = NULL;
    scope_list_tail = NULL;
    global_scope = NULL;
    current_scope = NULL;
    current_function = NULL;

    free_call_graph();
}