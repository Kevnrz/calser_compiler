#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantic.h"
#include "symbol_table.h"
#include "ast.h"

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

// Errors

// Único punto de salida de los errores semánticos.
// P-03: line > 0 pone la línea al inicio; 0 = el error no tiene línea.
static void semantic_error(int line, const char *fmt, ...) {
    va_list args;

    if (line > 0)
        fprintf(stderr, "Error semantico (linea %d): ", line);
    else
        fprintf(stderr, "Error semantico: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
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

// Libro §6.3.4: el nombre recibe el offset actual y el offset avanza su ancho.
// line es la del símbolo que se reserva (para el rl, la de la función).
static int reserve(int width, const char *what, int line, int *out_address) {
    if (next_address + width > DATA_LIMIT) {

        semantic_error(line,
                       "'%s' no cabe en la memoria de datos (limite 0x%04X)",
                       what,
                       (unsigned int) (DATA_LIMIT - 1));

        return 0;
    }

    *out_address = next_address;
    next_address += width;

    return 1;
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
// Devuelve 0 si algún tamaño no es válido.
static int fill_dimensions(Symbol *symbol, ASTNode *dimensions) {

    // A-2: en esa posición puede llegar otro nodo (no DIMENSIONS).
    if (dimensions == NULL ||
        dimensions->kind != AST_DIMENSIONS)
        return 1;

    // Los hijos de DIMENSIONS son nodos NUMBER con el tamaño en number.
    ASTNode *dimension = dimensions->child;

    while (dimension != NULL &&
           symbol->dimensions < MAX_DIMS) {

        // S-06: una dimensión de tamaño 0 no reserva nada.
        if (dimension->number <= 0) {

            semantic_error(symbol->line,
                           "la dimension de '%s' debe ser mayor que 0",
                           symbol->name);

            return 0;
        }

        // Guardar el tamaño antes de incrementar dimensions.
        symbol->dim_sizes[symbol->dimensions] = (int) dimension->number;
        symbol->dimensions++;

        dimension = dimension->next;
    }

    return 1;
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
static int check_name(ASTNode *identifier) {
    if (identifier == NULL || identifier->text == NULL)
        return 1;

    if (symbol_lookup(current_scope, identifier->text) != NULL)
        return 1;

    // Fuera de alcance: el nombre existe en un bloque de esta función que ya
    // se cerró. No hace falta excluir los ámbitos abiertos: symbol_lookup ya
    // buscó en el actual y en sus padres.
    Scope *closed = find_closed_scope(identifier->text);

    if (closed != NULL) {

        semantic_error(identifier->line,
                       "'%s' esta fuera de alcance; "
                       "fue declarado en el bloque de la linea %d",
                       identifier->text,
                       closed->line);

        return 0;
    }

    semantic_error(identifier->line,
                   "'%s' no ha sido declarado",
                   identifier->text);

    return 0;
}

static int check_expression(ASTNode *node);

// Verifica cada hijo de node como expresión.
static int check_children(ASTNode *node) {
    if (node == NULL)
        return 1;

    ASTNode *child = node->child;

    while (child != NULL) {

        if (!check_expression(child))
            return 0;

        child = child->next;
    }

    return 1;
}

// CALL, INDEX y REWIND: el nombre y cada argumento. Todavía no se distingue
// arreglo de función ni se cuentan índices o argumentos.
static int check_call(ASTNode *node) {
    if (!check_name(find_child(node, AST_IDENTIFIER)))
        return 0;

    ASTNode *arguments = find_child(node, AST_ARGUMENTS);

    if (!check_children(arguments))
        return 0;

    // INDEX: el segundo índice es el hijo que sigue a ARGUMENTS (cualquier
    // expresión, incluso un IDENTIFIER).
    if (node->kind == AST_INDEX && arguments != NULL)
        return check_expression(arguments->next);

    return 1;
}

// Devuelve 1 si todo nombre usado en la expresión existe, 0 si no.
static int check_expression(ASTNode *node) {
    if (node == NULL)
        return 1;

    switch (node->kind) {

        case AST_IDENTIFIER:
            return check_name(node);

        case AST_NUMBER:
        case AST_BOOLEAN:
            return 1;

        case AST_CALL:
        case AST_INDEX:
            return check_call(node);

        // INITIALIZER_LIST y cualquier operador binario o unario.
        default:
            return check_children(node);
    }
}

// Un local (o la variable de un S) no puede llamarse como un parámetro de
// su función, en ningún bloque. Las globales no se revisan.
static int check_not_parameter(const char *name, int line) {
    if (current_function == NULL)
        return 1;

    for (int i = 0; i < current_function->parameter_count; i++) {

        if (strcmp(current_function->param_list[i]->name, name) == 0) {

            semantic_error(line,
                           "'%s' ya es un parametro de esta funcion",
                           name);

            return 0;
        }
    }

    return 1;
}

// Inserta symbol en el ámbito actual. Si el nombre ya existe en ese ámbito,
// reporta el error en la línea de la nueva declaración, señalando la línea
// del símbolo existente, y descarta symbol.
static int insert_symbol(Symbol *symbol) {
    Symbol *existing =
        symbol_lookup_current(current_scope, symbol->name);

    if (existing != NULL ||
        !symbol_insert(current_scope, symbol)) {

        semantic_error(symbol->line,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       symbol->name,
                       existing ? existing->line : 0);

        // symbol_insert() didn't take ownership because insertion failed.
        discard_symbol(symbol);

        return 0;
    }

    return 1;
}

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
    if (!check_expression(initializer))
        return 0;

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);
    symbol->line = node->line;

    if (!fill_dimensions(symbol, dimensions) ||
        !check_not_parameter(symbol->name, symbol->line)) {

        discard_symbol(symbol);

        return 0;
    }

    // Libro §6.3.4: ancho = ancho del tipo × producto de las dimensiones
    int d1 = symbol->dimensions > 0 ? symbol->dim_sizes[0] : 1;
    int d2 = symbol->dimensions > 1 ? symbol->dim_sizes[1] : 1;

    symbol->width = type_width(symbol->type) * d1 * d2;   // d = 1 si no aplica

    if (!insert_symbol(symbol))
        return 0;

    // El símbolo ya pertenece al ámbito: si no cabe, se libera con él.
    if (!reserve(symbol->width, symbol->name, symbol->line, &symbol->address))
        return 0;

    return 1;
}


// Parameters

// Crea el símbolo de un parámetro, sin insertarlo en ningún ámbito ni darle
// dirección. Devuelve NULL si hay error.
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

    if (!fill_dimensions(symbol, dimensions)) {

        discard_symbol(symbol);

        return NULL;
    }

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
    if (!check_expression(find_condition(node)))
        return 0;

    if (!analyze_block(find_child(node, AST_BLOCK), SCOPE_C))
        return 0;

    // Ramas alternativas  A [cond] ;
    ASTNode *alternatives = find_child(node, AST_ALTERNATIVES);
    ASTNode *alternative = alternatives ? alternatives->child : NULL;

    while (alternative != NULL) {

        if (alternative->kind == AST_ALTERNATIVE) {

            if (!check_expression(find_condition(alternative)))
                return 0;

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
    if (!check_expression(find_condition(node)))
        return 0;

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
    if (!check_expression(start) ||
        !check_expression(end))
        return 0;

    if (!check_not_parameter(identifier->text, node->line))
        return 0;

    enter_scope(SCOPE_S, node->line);

    Symbol *control =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      TYPE_INT);

    // el ciclo la controla; el cuerpo no puede modificarla
    control->modifier = MODIFIER_VI;
    control->width = type_width(control->type);   // M-1
    control->line = node->line;

    // El símbolo pasa a pertenecer al ámbito: si no cabe, se libera con él.
    int result = insert_symbol(control);

    if (result)
        result = reserve(control->width,
                         control->name,
                         control->line,
                         &control->address);

    // El cuerpo comparte el ámbito del S con su variable (no abre otro).
    if (result)
        result = analyze_statements(find_child(node, AST_BLOCK));

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
            return check_children(statement);

        // El nombre y cada argumento.
        case AST_REWIND:
            return check_call(statement);

        // EMIT: nada que verificar.
        default:
            return 1;
    }
}


// Function

// Parámetro de la función con ese nombre, o NULL.
static Symbol *find_parameter(Symbol *function, const char *name) {
    for (int i = 0; i < function->parameter_count; i++) {

        if (strcmp(function->param_list[i]->name, name) == 0)
            return function->param_list[i];
    }

    return NULL;
}

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

    // RETURN_TYPE sin hijos es void.
    ASTNode *ret_type = find_child(return_type, AST_TYPE);
    ASTNode *ret_id = find_child(return_type, AST_IDENTIFIER);

    // Funcion pertenece a scope global
    Symbol *function =
        symbol_create(identifier->text,
                      SYMBOL_FUNCTION,
                      ret_type ? get_type(ret_type) : TYPE_VOID);

    function->line = node->line;

    Symbol *existing =
        symbol_lookup_current(global_scope, function->name);

    if (existing != NULL ||
        !symbol_insert(global_scope, function)) {

        // Línea del segundo FUNCTION, señalando la del primero.
        semantic_error(function->line,
                       "la funcion '%s' ya fue declarada (linea %d)",
                       function->name,
                       existing ? existing->line : 0);

        discard_symbol(function);

        return 0;
    }

    /*
     * Los parámetros solo quedan en param_list, que no es dueña: pasan a
     * pertenecer al ámbito de la función cuando la pasada 2 los inserta.
     * Si el análisis se detiene antes de que la pasada 2 llegue a esta
     * función, sus parámetros quedan sin liberar. Es una fuga aceptada solo
     * en ese camino de error.
     */
    ASTNode *parameter = params ? params->child : NULL;

    while (parameter != NULL) {

        Symbol *symbol = create_parameter(parameter);

        if (symbol == NULL)
            return 0;

        // Todos los parámetros comparten el ámbito de la función.
        Symbol *first = find_parameter(function, symbol->name);

        if (first != NULL) {

            semantic_error(symbol->line,
                           "'%s' ya fue declarado en este ambito (linea %d)",
                           symbol->name,
                           first->line);

            discard_symbol(symbol);

            return 0;
        }

        // Lista ordenada de parámetros de la función (también lo cuenta).
        symbol_add_param(function, symbol);

        parameter = parameter->next;
    }

    // La variable de retorno comparte ese ámbito. En el texto el parámetro
    // aparece primero: el error se marca en la variable de retorno y señala
    // al parámetro.
    Symbol *clash =
        ret_id ? find_parameter(function, ret_id->text) : NULL;

    if (clash != NULL) {

        semantic_error(return_type->line,
                       "'%s' ya fue declarado en este ambito (linea %d)",
                       ret_id->text,
                       clash->line);

        return 0;
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
static int check_main(void) {
    Symbol *main_function = symbol_lookup_current(global_scope, "main");

    if (main_function == NULL ||
        main_function->kind != SYMBOL_FUNCTION) {

        semantic_error(0, "falta la funcion 'main'");

        return 0;
    }

    if (main_function->parameter_count > 0 ||
        main_function->type != TYPE_VOID) {

        semantic_error(main_function->line,
                       "'main' debe declararse como F main [] : void");

        return 0;
    }

    return 1;
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

    // La pasada 1 ya creó el símbolo de la función.
    Symbol *function =
        identifier
            ? symbol_lookup_current(global_scope, identifier->text)
            : NULL;

    if (function == NULL ||
        function->kind != SYMBOL_FUNCTION) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }

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

        if (!reserve(4, function->name, function->line, &rl_address))
            return 0;
    }

    // Crea function scope.
    enter_scope(SCOPE_FUNCTION, function->line);

    // La variable de retorno va antes que los parámetros (queda en +4).
    ASTNode *ret_type = find_child(return_type, AST_TYPE);
    ASTNode *ret_id = find_child(return_type, AST_IDENTIFIER);

    if (ret_id != NULL) {

        Symbol *ret = symbol_create(ret_id->text,
                                    SYMBOL_RETURN,
                                    get_type(ret_type));

        ret->modifier = MODIFIER_VM;   // el cuerpo la asigna
        ret->width = type_width(ret->type);   // M-1
        ret->line = return_type->line;

        if (!insert_symbol(ret))
            return 0;

        if (!reserve(ret->width, ret->name, ret->line, &ret->address))
            return 0;
    }

    // Los parámetros ya existen en param_list: desde aquí pertenecen al
    // ámbito de la función. La inserción no puede fallar (los duplicados se
    // detectaron en la pasada 1).
    for (int i = 0; i < function->parameter_count; i++) {

        Symbol *parameter = function->param_list[i];

        if (!symbol_insert(current_scope, parameter))
            return 0;

        if (!reserve(parameter->width,
                     parameter->name,
                     parameter->line,
                     &parameter->address))
            return 0;
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

    enter_global_scope();

    next_address = DATA_BASE;
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

    if (!check_main())
        return 0;

    // Pasada 2: ámbitos, direcciones y cuerpos.
    ASTNode *function = functions ? functions->child : NULL;

    while (function != NULL) {

        if (!analyze_function(function))
            return 0;

        function = function->next;
    }

    return 1;
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