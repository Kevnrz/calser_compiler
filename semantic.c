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

static Scope *create_scope(void) {
    Scope *scope = scope_create(current_scope);

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

    global_scope = scope_create(NULL);

    if (global_scope == NULL) {
        fprintf(stderr, "Error: could not create global scope\n");
        exit(EXIT_FAILURE);
    }

    scope_list_head = global_scope;
    scope_list_tail = global_scope;

    current_scope = global_scope;
}

static void enter_scope(void) {
    create_scope();
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

    return 1;
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

    // Crea function scope.
    enter_scope();

    // Parameters are inserted into the function scope.
    if (params != NULL && params->kind == AST_PARAMS) {

        ASTNode *parameter = params->child;

        while (parameter != NULL) {

            if (!analyze_parameter(parameter, function))
                return 0;

            parameter = parameter->next;
        }
    }

    // Analyze function body.
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

            if (!symbol_insert(current_scope, ret)) {   // choque con un parámetro
                fprintf(stderr,
                        "Semantic error: return variable '%s' already declared\n",
                        ret_id->text);
                free(ret->name);
                free(ret);
                return 0;
            }
        }
    }

    if (body != NULL &&
        body->kind == AST_BLOCK) {

        ASTNode *statement = body->child;

        while (statement != NULL) {

            if (statement->kind == AST_DECLARATION) {

                if (!analyze_declaration(statement))
                    return 0;
            }

            statement = statement->next;
        }
    }

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