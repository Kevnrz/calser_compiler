#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM,
    AST_GLOBALS,
    AST_FUNCTIONS,
    AST_FUNCTION,

    AST_PARAMS,
    AST_PARAMETER,

    AST_RETURN_TYPE,
    AST_TYPE,
    AST_MODIFIER,
    AST_DIMENSIONS,

    AST_BLOCK,
    AST_DECLARATION,
    AST_INITIALIZER_LIST,

    AST_ASSIGNMENT,
    AST_CALL_STATEMENT,

    AST_CONDITIONAL,
    AST_ALTERNATIVES,
    AST_ALTERNATIVE,
    AST_RESIDUAL,

    AST_LOOP_L,
    AST_LOOP_S,

    AST_EMIT,
    AST_REWIND,

    AST_ARGUMENTS,

    AST_IDENTIFIER,
    AST_NUMBER,
    AST_BOOLEAN,

    AST_CALL,
    AST_INDEX,

    AST_OR,
    AST_AND,

    AST_EQUAL,
    AST_NOT_EQUAL,

    AST_LESS,
    AST_GREATER,
    AST_LESS_EQUAL,
    AST_GREATER_EQUAL,

    AST_ADD,
    AST_SUB,

    AST_MUL,
    AST_DIV,
    AST_MOD,

    AST_NEG,
    AST_NOT,

    AST_POWER

} ASTKind;

typedef struct ASTNode {
    ASTKind kind;

    char *text;
    long number;

    struct ASTNode *child;
    struct ASTNode *next;

} ASTNode;

// Generic 
ASTNode *ast_new(ASTKind kind);
void ast_add_child(ASTNode *parent, ASTNode *child);
ASTNode *ast_list(ASTKind kind, ASTNode *items);

// Program
ASTNode *ast_program(ASTNode *globals,
                     ASTNode *functions);

ASTNode *ast_function(char *name,
                      ASTNode *params,
                      ASTNode *return_type,
                      ASTNode *body);


// Types
ASTNode *ast_type(const char *name);
ASTNode *ast_modifier(const char *name);
ASTNode *ast_return_type(ASTNode *type,
                         char *name);
ASTNode *ast_return_void(void);

ASTNode *ast_dimensions(void);
void ast_add_number(ASTNode *node,
                    long number);


// Parameters
ASTNode *ast_parameter(ASTNode *modifier,
                       ASTNode *type,
                       char *name,
                       ASTNode *dims);


// Statements
ASTNode *ast_block(void);

ASTNode *ast_declaration(ASTNode *modifier,
                         char *name,
                         ASTNode *type,
                         ASTNode *dims,
                         ASTNode *initializer);

ASTNode *ast_assignment(char *target,
                        ASTNode *args,
                        ASTNode *index,
                        ASTNode *value);

ASTNode *ast_call_statement(char *call,
                           ASTNode *args);

ASTNode *ast_conditional(ASTNode *condition,
                         ASTNode *body,
                         ASTNode *alts,
                         ASTNode *residual);

ASTNode *ast_alternative(ASTNode *condition,
                         ASTNode *body);

ASTNode *ast_residual(ASTNode *body);

ASTNode *ast_loop_l(ASTNode *condition,
                    ASTNode *body);

ASTNode *ast_loop_s(char *variable,
                    ASTNode *start,
                    ASTNode *end,
                    ASTNode *step,
                    ASTNode *body);

ASTNode *ast_emit(void);

ASTNode *ast_rewind(char *name,
                    ASTNode *args);

// Expressions
ASTNode *ast_number(long value);
ASTNode *ast_boolean(int value);
ASTNode *ast_identifier(const char *name);

ASTNode *ast_binary(ASTKind kind,
                    ASTNode *left,
                    ASTNode *right);

ASTNode *ast_unary(ASTKind kind,
                   ASTNode *child);

ASTNode *ast_call(char *name,
                  ASTNode *args);

ASTNode *ast_index(char *name,
                   ASTNode *args,
                   ASTNode *index);


// Printing
void ast_print(ASTNode *node,
               int indent);

void ast_free(ASTNode *node);

#endif