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

/*
 * Único punto de salida de los errores semánticos (formato de §8.2).
 * Los errores se acumulan: después de reportar, el análisis sigue.
 *
 * Posición y largo del señalador según node:
 *
 * NULL                        = sin posición
 * IDENTIFIER                  = el nodo, strlen(text)
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

        if (name != NULL && name->text != NULL) {
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
static void check_name(ASTNode *identifier) {
    if (identifier == NULL || identifier->text == NULL)
        return;

    if (symbol_lookup(current_scope, identifier->text) != NULL)
        return;

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

        return;
    }

    semantic_error(identifier,
                   "'%s' no ha sido declarado",
                   identifier->text);
}

static void check_expression(ASTNode *node);

// Verifica cada hijo de node como expresión.
static void check_children(ASTNode *node) {
    if (node == NULL)
        return;

    ASTNode *child = node->child;

    while (child != NULL) {
        check_expression(child);
        child = child->next;
    }
}

// CALL, INDEX y REWIND: el nombre y cada argumento. Todavía no se distingue
// arreglo de función ni se cuentan índices o argumentos.
static void check_call(ASTNode *node) {
    check_name(find_child(node, AST_IDENTIFIER));

    ASTNode *arguments = find_child(node, AST_ARGUMENTS);

    check_children(arguments);

    // INDEX: el segundo índice es el hijo que sigue a ARGUMENTS (cualquier
    // expresión, incluso un IDENTIFIER).
    if (node->kind == AST_INDEX && arguments != NULL)
        check_expression(arguments->next);
}

// Verifica que todo nombre usado en la expresión exista.
static void check_expression(ASTNode *node) {
    if (node == NULL)
        return;

    switch (node->kind) {

        case AST_IDENTIFIER:
            check_name(node);
            break;

        case AST_NUMBER:
        case AST_BOOLEAN:
            break;

        case AST_CALL:
        case AST_INDEX:
            check_call(node);
            break;

        // INITIALIZER_LIST y cualquier operador binario o unario.
        default:
            check_children(node);
            break;
    }
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

    // El inicializador se verifica antes de insertar el símbolo:
    // Vm x : int = x ;  usa el x de afuera, o es error.
    check_expression(initializer);

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;
    symbol->decl_node = node;

    fill_dimensions(symbol, dimensions, node);

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

    while (statement != NULL) {

        if (!analyze_statement(statement))
            return 0;

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
    check_expression(find_condition(node));

    if (!analyze_block(find_child(node, AST_BLOCK), SCOPE_C))
        return 0;

    // Ramas alternativas  A [cond] ;
    ASTNode *alternatives = find_child(node, AST_ALTERNATIVES);
    ASTNode *alternative = alternatives ? alternatives->child : NULL;

    while (alternative != NULL) {

        if (alternative->kind == AST_ALTERNATIVE) {

            check_expression(find_condition(alternative));

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
    check_expression(find_condition(node));

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
    check_expression(start);
    check_expression(end);

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

        if (insert_symbol(control, node))
            reserve(control->width, control->name, node, &control->address);
    }

    // El cuerpo comparte el ámbito del S con su variable (no abre otro).
    int result = analyze_statements(find_child(node, AST_BLOCK));

    exit_scope();

    return result;
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

        // Destino (IDENTIFIER, CALL o INDEX) y valor; o el CALL de la sentencia.
        case AST_ASSIGNMENT:
        case AST_CALL_STATEMENT:
            check_children(statement);
            return 1;

        // El nombre y cada argumento.
        case AST_REWIND:
            check_call(statement);
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

    // El área termina donde quedó el offset.
    function->area_size = next_address - function->area_base;

    // Return to global scope.
    exit_scope();

    current_function = NULL;

    return 1;
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
}