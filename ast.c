#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

static char *ast_strdup(const char *str) {
    if (str == NULL)
        return NULL;

    char *copy = malloc(strlen(str) + 1);

    if (copy == NULL) {
        fprintf(stderr, "AST error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    strcpy(copy, str);
    return copy;
}

// Node creation

ASTNode *ast_new(ASTKind kind) {
    ASTNode *node = malloc(sizeof(ASTNode));

    if (node == NULL) {
        fprintf(stderr, "AST error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    node->kind = kind;
    node->text = NULL;
    node->number = 0;
    node->line = 0;
    node->child = NULL;
    node->next = NULL;

    return node;
}

// Add node as the last child of parent
void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent == NULL || child == NULL)
        return;

    if (parent->child == NULL) {
        parent->child = child;
        return;
    }

    ASTNode *current = parent->child;

    while (current->next != NULL)
        current = current->next;

    current->next = child;
}

// P-03: asigna la línea al nodo y a todos sus descendientes que aún no tengan
// una. Los hijos que ya tienen línea se crearon en reducciones anteriores: se
// dejan como están y no se baja por ellos.
void ast_set_line(ASTNode *node, int line) {
    if (node == NULL)
        return;

    if (node->line == 0)
        node->line = line;

    ASTNode *child = node->child;

    while (child != NULL) {

        if (child->line == 0)
            ast_set_line(child, line);

        child = child->next;
    }
}

// Lists

ASTNode *ast_list(ASTKind kind, ASTNode *items) {
    ASTNode *node = ast_new(kind);
    node->child = items;
    return node;
}

// Program structure

ASTNode *ast_program(ASTNode *globals, ASTNode *functions) {
    ASTNode *node = ast_new(AST_PROGRAM);

    ast_add_child(node, globals);
    ast_add_child(node, functions);

    return node;
}

ASTNode *ast_function(char *name,
                      ASTNode *params,
                      ASTNode *return_type,
                      ASTNode *body) {
    ASTNode *node = ast_new(AST_FUNCTION);

    ast_add_child(node, ast_identifier(name));
    ast_add_child(node, params);
    ast_add_child(node, return_type);
    ast_add_child(node, body);

    return node;
}

// Types / parameters

ASTNode *ast_type(const char *name) {
    ASTNode *node = ast_new(AST_TYPE);
    node->text = ast_strdup(name);
    return node;
}

ASTNode *ast_modifier(const char *name) {
    ASTNode *node = ast_new(AST_MODIFIER);
    node->text = ast_strdup(name);
    return node;
}

ASTNode *ast_return_type(ASTNode *type, char *name) {
    ASTNode *node = ast_new(AST_RETURN_TYPE);

    ast_add_child(node, type);
    ast_add_child(node, ast_identifier(name));

    return node;
}

ASTNode *ast_return_void(void) {
    ASTNode *node = ast_new(AST_RETURN_TYPE);
    node->text = ast_strdup("void");

    return node;
}

ASTNode *ast_parameter(ASTNode *modifier,
                       ASTNode *type,
                       char *name,
                       ASTNode *dimensions) {
    ASTNode *node = ast_new(AST_PARAMETER);

    ast_add_child(node, modifier);
    ast_add_child(node, type);
    ast_add_child(node, ast_identifier(name));
    ast_add_child(node, dimensions);

    return node;
}

ASTNode *ast_dimensions(void) {
    return ast_list(AST_DIMENSIONS, NULL);
}

void ast_add_number(ASTNode *node, long number) {
    if (node == NULL)
        return;

    ASTNode *item = ast_new(AST_NUMBER);
    item->number = number;
    ast_add_child(node, item);
}

// Statements / declarations

ASTNode *ast_block(void) {
    return ast_list(AST_BLOCK, NULL);
}


ASTNode *ast_declaration(ASTNode *modifier,
                         char *name,
                         ASTNode *type,
                         ASTNode *dimensions,
                         ASTNode *initializer) {
    ASTNode *node = ast_new(AST_DECLARATION);

    ast_add_child(node, modifier);
    ast_add_child(node, ast_identifier(name));
    ast_add_child(node, type);
    ast_add_child(node, dimensions);
    ast_add_child(node, initializer);

    return node;
}


ASTNode *ast_initializer_list(ASTNode *elements) {
    return ast_list(AST_INITIALIZER_LIST, elements);
}


ASTNode *ast_assignment(char *target,
                        ASTNode *args,
                        ASTNode *index,
                        ASTNode *value) {
    ASTNode *node = ast_new(AST_ASSIGNMENT);
    ASTNode *target_node = ast_identifier(target);

    if (args != NULL && index != NULL) {
        target_node = ast_new(AST_INDEX);
        ast_add_child(target_node, ast_identifier(target));
        ast_add_child(target_node, args);
        ast_add_child(target_node, index);
    } else if (args != NULL) {
        target_node = ast_new(AST_CALL);
        ast_add_child(target_node, ast_identifier(target));
        ast_add_child(target_node, args);
    }

    ast_add_child(node, target_node);
    ast_add_child(node, value);

    return node;
}

ASTNode *ast_call_statement(char *call,
                           ASTNode *args) {
    ASTNode *node = ast_new(AST_CALL_STATEMENT);

    ast_add_child(node, ast_call(call, args));

    return node;
}

// Control flow

ASTNode *ast_conditional(ASTNode *condition,
                         ASTNode *body,
                         ASTNode *alternatives,
                         ASTNode *residual) {
    ASTNode *node = ast_new(AST_CONDITIONAL);

    ast_add_child(node, condition);
    ast_add_child(node, body);
    ast_add_child(node, alternatives);
    ast_add_child(node, residual);

    return node;
}

ASTNode *ast_alternative(ASTNode *condition,
                         ASTNode *body) {
    ASTNode *node = ast_new(AST_ALTERNATIVE);

    ast_add_child(node, condition);
    ast_add_child(node, body);

    return node;
}

ASTNode *ast_residual(ASTNode *body) {
    ASTNode *node = ast_new(AST_RESIDUAL);

    ast_add_child(node, body);

    return node;
}

ASTNode *ast_loop_l(ASTNode *condition,
                    ASTNode *body) {
    ASTNode *node = ast_new(AST_LOOP_L);

    ast_add_child(node, condition);
    ast_add_child(node, body);

    return node;
}

ASTNode *ast_loop_s(char *id,
                    ASTNode *start,
                    ASTNode *end,
                    ASTNode *step,
                    ASTNode *body) {
    ASTNode *node = ast_new(AST_LOOP_S);

    ast_add_child(node, ast_identifier(id));
    ast_add_child(node, start);
    ast_add_child(node, end);
    ast_add_child(node, step);
    ast_add_child(node, body);

    return node;
}

ASTNode *ast_emit(void) {
    return ast_new(AST_EMIT);
}

ASTNode *ast_rewind(char *id,
                    ASTNode *args) {
    ASTNode *node = ast_new(AST_REWIND);

    ast_add_child(node, ast_identifier(id));
    ast_add_child(node, args);

    return node;
}

// Arguments

ASTNode *ast_arguments(ASTNode *arguments) {
    return ast_list(AST_ARGUMENTS, arguments);
}

// Expressions

ASTNode *ast_identifier(const char *name) {
    ASTNode *node = ast_new(AST_IDENTIFIER);

    node->text = ast_strdup(name);

    return node;
}

ASTNode *ast_number(long value) {
    ASTNode *node = ast_new(AST_NUMBER);

    node->number = value;

    return node;
}

ASTNode *ast_boolean(int value) {
    ASTNode *node = ast_new(AST_BOOLEAN);

    node->number = value;

    return node;
}

ASTNode *ast_binary(ASTKind kind,
                    ASTNode *left,
                    ASTNode *right) {
    ASTNode *node = ast_new(kind);

    ast_add_child(node, left);
    ast_add_child(node, right);

    return node;
}

ASTNode *ast_unary(ASTKind kind,
                   ASTNode *operand) {
    ASTNode *node = ast_new(kind);

    ast_add_child(node, operand);

    return node;
}

ASTNode *ast_call(char *id,
                  ASTNode *args) {
    ASTNode *node = ast_new(AST_CALL);

    ast_add_child(node, ast_identifier(id));
    ast_add_child(node, args);

    return node;
}

ASTNode *ast_index(char *id,
                   ASTNode *args,
                   ASTNode *index) {
    ASTNode *node = ast_new(AST_INDEX);

    ast_add_child(node, ast_identifier(id));
    ast_add_child(node, args);
    ast_add_child(node, index);

    return node;
}

// Printing 

static const char *ast_kind_name(ASTKind kind) {
    switch (kind) {

        case AST_PROGRAM:          return "PROGRAM";
        case AST_GLOBALS:          return "GLOBALS";
        case AST_FUNCTIONS:        return "FUNCTIONS";
        case AST_FUNCTION:         return "FUNCTION";

        case AST_PARAMS:           return "PARAMS";
        case AST_PARAMETER:        return "PARAMETER";
        case AST_RETURN_TYPE:      return "RETURN_TYPE";
        case AST_TYPE:             return "TYPE";
        case AST_MODIFIER:         return "MODIFIER";
        case AST_DIMENSIONS:       return "DIMENSIONS";

        case AST_BLOCK:            return "BLOCK";
        case AST_DECLARATION:      return "DECLARATION";
        case AST_INITIALIZER_LIST: return "INITIALIZER_LIST";

        case AST_ASSIGNMENT:       return "ASSIGNMENT";
        case AST_CALL_STATEMENT:   return "CALL_STATEMENT";

        case AST_CONDITIONAL:      return "CONDITIONAL";
        case AST_ALTERNATIVES:     return "ALTERNATIVES";
        case AST_ALTERNATIVE:      return "ALTERNATIVE";
        case AST_RESIDUAL:         return "RESIDUAL";

        case AST_LOOP_L:           return "LOOP_L";
        case AST_LOOP_S:           return "LOOP_S";

        case AST_EMIT:             return "EMIT";
        case AST_REWIND:           return "REWIND";

        case AST_ARGUMENTS:        return "ARGUMENTS";

        case AST_IDENTIFIER:       return "IDENTIFIER";
        case AST_NUMBER:           return "NUMBER";
        case AST_BOOLEAN:          return "BOOLEAN";

        case AST_CALL:             return "CALL";
        case AST_INDEX:            return "INDEX";

        case AST_OR:               return "OR";
        case AST_AND:              return "AND";
        case AST_EQUAL:            return "EQUAL";
        case AST_NOT_EQUAL:        return "NOT_EQUAL";
        case AST_LESS:             return "LESS";
        case AST_GREATER:          return "GREATER";
        case AST_LESS_EQUAL:       return "LESS_EQUAL";
        case AST_GREATER_EQUAL:    return "GREATER_EQUAL";
        case AST_ADD:              return "ADD";
        case AST_SUB:              return "SUB";
        case AST_MUL:              return "MUL";
        case AST_DIV:              return "DIV";
        case AST_MOD:              return "MOD";
        case AST_NEG:              return "NEG";
        case AST_NOT:              return "NOT";
        case AST_POWER:            return "POWER";

        default:                   return "UNKNOWN";
    }
}

static void ast_print_internal(ASTNode *node, int depth) {
    while (node != NULL) {

        for (int i = 0; i < depth; i++)
            printf("  ");

        printf("%s", ast_kind_name(node->kind));

        if (node->text != NULL)
            printf(" \"%s\"", node->text);

        if (node->kind == AST_NUMBER)
            printf(" %ld", node->number);

        if (node->kind == AST_BOOLEAN)
            printf(" %s", node->number ? "true" : "false");

        printf("\n");

        if (node->child != NULL)
            ast_print_internal(node->child, depth + 1);

        node = node->next;
    }
}

void ast_print(ASTNode *root, int depth) {
    ast_print_internal(root, depth);
}


void ast_free(ASTNode *node) {
    if (node == NULL)
        return;

    /*
     * First free all children.
     */
    ast_free(node->child);

    /*
     * Then free siblings.
     */
    ast_free(node->next);

    /*
     * Free dynamically allocated text.
     */
    free(node->text);

    /*
     * Finally free the node itself.
     */
    free(node);
}