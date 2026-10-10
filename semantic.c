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

// Scope management

static Scope *create_scope(ScopeKind kind) {
    Scope *scope = scope_create(current_scope, kind);

    if (scope == NULL)
        return NULL;

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

static void enter_scope(ScopeKind kind) {
    create_scope(kind);
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

// Memory

// M-2: variables de ccom en 0x0000–0x0FFF; desde 0x1000, archivos de load_file
#define DATA_BASE  0x0000
#define DATA_LIMIT 0x1000          // primera dirección que NO se puede usar

static int next_address = DATA_BASE;   // libro §6.3.4: el "offset"

// Libro §6.3.4: el nombre recibe el offset actual y el offset avanza su ancho.
static int reserve(int width, const char *what, int *out_address) {
    if (next_address + width > DATA_LIMIT) {

        fprintf(stderr,
                "Semantic error: '%s' does not fit in data memory (limit 0x%04X)\n",
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
static void fill_dimensions(Symbol *symbol, ASTNode *dimensions) {

    // A-2: en esa posición puede llegar otro nodo (no DIMENSIONS).
    if (dimensions == NULL ||
        dimensions->kind != AST_DIMENSIONS)
        return;

    // Los hijos de DIMENSIONS son nodos NUMBER con el tamaño en number.
    ASTNode *dimension = dimensions->child;

    while (dimension != NULL &&
           symbol->dimensions < MAX_DIMS) {

        // Guardar el tamaño antes de incrementar dimensions.
        symbol->dim_sizes[symbol->dimensions] = (int) dimension->number;
        symbol->dimensions++;

        dimension = dimension->next;
    }
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

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);

    fill_dimensions(symbol, dimensions);

    // Libro §6.3.4: ancho = ancho del tipo × producto de las dimensiones
    int d1 = symbol->dimensions > 0 ? symbol->dim_sizes[0] : 1;
    int d2 = symbol->dimensions > 1 ? symbol->dim_sizes[1] : 1;

    symbol->width = type_width(symbol->type) * d1 * d2;   // d = 1 si no aplica

    if (!symbol_insert(current_scope, symbol)) {

        fprintf(stderr,
                "Semantic error: '%s' already declared in this scope\n",
                identifier->text);

        // symbol_insert() didn't take ownership because insertion failed.
        free(symbol->name);
        free(symbol);

        return 0;
    }

    // El símbolo ya pertenece al ámbito: si no cabe, se libera con él.
    if (!reserve(symbol->width, symbol->name, &symbol->address))
        return 0;

    return 1;
}


// Parameters

static int analyze_parameter(ASTNode *node, Symbol *function) {
    if (node == NULL)
        return 1;

    /*
     * PARAMETER
     *
     * child 0 = modifier
     * child 1 = type
     * child 2 = identifier
     * child 3 = dimensions
     */

    ASTNode *modifier = node->child;

    ASTNode *type;
    if (modifier->kind == AST_MODIFIER) {
        type =
            modifier ? modifier->next : NULL;
    } else {
        type = modifier ? modifier : NULL;
    }    

    ASTNode *identifier =
        type ? type->next : NULL;

    ASTNode *dimensions =
        identifier ? identifier->next : NULL;

    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {

        fprintf(stderr,
                "Semantic error: invalid parameter\n");

        return 0;
    }

    Symbol *symbol =
        symbol_create(identifier->text,
                      SYMBOL_PARAMETER,
                      get_type(type));

    symbol->modifier = get_modifier(modifier);

    fill_dimensions(symbol, dimensions);

    // escalar: el valor; arreglo: su dirección (paso por referencia)
    symbol->width = 4;

    if (!symbol_insert(current_scope, symbol)) {

        fprintf(stderr,
                "Semantic error: parameter '%s' already declared\n",
                identifier->text);

        free(symbol->name);
        free(symbol);

        return 0;
    }

    // Lista ordenada de parámetros de la función (también cuenta el parámetro).
    symbol_add_param(function, symbol);

    if (!reserve(symbol->width, symbol->name, &symbol->address))
        return 0;

    return 1;
}


// Statements

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
    enter_scope(kind);

    int result = analyze_statements(block);

    exit_scope();

    return result;
}

// Condicional: un ámbito por cada rama. Las condiciones no se analizan todavía.
static int analyze_conditional(ASTNode *node) {
    ASTNode *child = node->child;

    while (child != NULL) {

        if (child->kind == AST_BLOCK) {

            // Rama inicial  C [cond] ;
            if (!analyze_block(child, SCOPE_C))
                return 0;
        }
        else if (child->kind == AST_ALTERNATIVES) {

            // Ramas alternativas  A [cond] ;
            ASTNode *alternative = child->child;

            while (alternative != NULL) {

                if (alternative->kind == AST_ALTERNATIVE &&
                    !analyze_block(find_child(alternative, AST_BLOCK),
                                   SCOPE_A))
                    return 0;

                alternative = alternative->next;
            }
        }
        else if (child->kind == AST_RESIDUAL) {

            // Rama residual  A ;
            if (!analyze_block(find_child(child, AST_BLOCK),
                               SCOPE_RESIDUAL))
                return 0;
        }

        child = child->next;
    }

    return 1;
}

// Ciclo acotado: el ámbito del S contiene su variable de control y su cuerpo.
static int analyze_loop_s(ASTNode *node) {
    /*
     * LOOP_S
     *
     * IDENTIFIER = variable de control (siempre el primer IDENTIFIER)
     * inicio, fin, paso
     * BLOCK
     */
    ASTNode *identifier = find_child(node, AST_IDENTIFIER);

    if (identifier == NULL) {
        fprintf(stderr,
                "Semantic error: invalid S loop\n");
        return 0;
    }

    enter_scope(SCOPE_S);

    Symbol *control =
        symbol_create(identifier->text,
                      SYMBOL_VARIABLE,
                      TYPE_INT);

    // el ciclo la controla; el cuerpo no puede modificarla
    control->modifier = MODIFIER_VI;
    control->width = type_width(control->type);   // M-1

    if (!symbol_insert(current_scope, control)) {

        fprintf(stderr,
                "Semantic error: '%s' already declared in this scope\n",
                identifier->text);

        free(control->name);
        free(control);

        exit_scope();

        return 0;
    }

    // El símbolo ya pertenece al ámbito: si no cabe, se libera con él.
    int result =
        reserve(control->width, control->name, &control->address);

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
            return analyze_block(find_child(statement, AST_BLOCK),
                                 SCOPE_L);

        case AST_LOOP_S:
            return analyze_loop_s(statement);

        // ASSIGNMENT, CALL_STATEMENT, EMIT, REWIND: no declaran nombres.
        default:
            return 1;
    }
}


// Function

static int analyze_function(ASTNode *node) {
    /*
     * FUNCTION
     *
     * child 0 = identifier
     * child 1 = params
     * child 2 = return type
     * child 3 = body
     */
    ASTNode *identifier = node->child;
    ASTNode *params =
        identifier ? identifier->next : NULL;
    ASTNode *return_type =
        params ? params->next : NULL;
    ASTNode *body =
        return_type ? return_type->next : NULL;
    if (identifier == NULL ||
        identifier->kind != AST_IDENTIFIER) {
        fprintf(stderr,
                "Semantic error: invalid function\n");
        return 0;
    }
    // Funcion pertenece a scope global
    ASTNode *actual_return_type = return_type;
    SymbolType type = TYPE_VOID;

    if (actual_return_type != NULL) {
        if (actual_return_type->kind == AST_RETURN_TYPE) {
            ASTNode *type_node = actual_return_type->child;
            if (type_node != NULL)
                type = get_type(type_node);
        }
    }
    Symbol *function =
        symbol_create(identifier->text, SYMBOL_FUNCTION, type);

    if (!symbol_insert(global_scope, function)) {
        fprintf(stderr,
                "Semantic error: function '%s' already declared\n",
                identifier->text);

        free(function->name);
        free(function);

        return 0;
    }

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

        if (!reserve(4, function->name, &rl_address))
            return 0;
    }

    // Crea function scope.
    enter_scope(SCOPE_FUNCTION);

    // La variable de retorno va antes que los parámetros (queda en +4).
    if (return_type != NULL &&
        return_type->kind == AST_RETURN_TYPE) {

        ASTNode *ret_type = return_type->child;                 // TYPE
        ASTNode *ret_id   = ret_type ? ret_type->next : NULL;   // IDENTIFIER

        if (ret_id != NULL &&
            ret_id->kind == AST_IDENTIFIER) {

            Symbol *ret = symbol_create(ret_id->text,
                                        SYMBOL_RETURN,
                                        get_type(ret_type));

            ret->modifier = MODIFIER_VM;   // el cuerpo la asigna
            ret->width = type_width(ret->type);   // M-1

            if (!symbol_insert(current_scope, ret)) {
                fprintf(stderr,
                        "Semantic error: return variable '%s' already declared\n",
                        ret_id->text);
                free(ret->name);
                free(ret);
                return 0;
            }

            if (!reserve(ret->width, ret->name, &ret->address))
                return 0;
        }
    }

    // Parameters are inserted into the function scope.
    if (params != NULL && params->kind == AST_PARAMS) {

        ASTNode *parameter = params->child;

        while (parameter != NULL) {

            if (!analyze_parameter(parameter, function))
                return 0;

            parameter = parameter->next;
        }
    }

    // S-01: el cuerpo se recorre completo, en el mismo ámbito de la función.
    if (!analyze_statements(body))
        return 0;

    // El área termina donde quedó el offset.
    function->area_size = next_address - function->area_base;

    // Return to global scope.
    exit_scope();

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

    /*
     * PROGRAM
     *
     * child 0 = globals
     * child 1 = functions
     */

    ASTNode *globals = root->child;

    ASTNode *functions =
        globals ? globals->next : NULL;

    // Global declarations.
    if (globals != NULL &&
        globals->kind == AST_GLOBALS) {

        ASTNode *decl = globals->child;

        while (decl != NULL) {

            if (!analyze_declaration(decl))
                return 0;

            decl = decl->next;
        }
    }

    // Functions.
    if (functions != NULL &&
        functions->kind == AST_FUNCTIONS) {

        ASTNode *function = functions->child;

        while (function != NULL) {

            if (!analyze_function(function))
                return 0;

            function = function->next;
        }
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
}