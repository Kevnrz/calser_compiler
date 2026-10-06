%{
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

extern int yylex(void);
extern int yylineno;

void yyerror(const char *msg);

ASTNode *root = NULL;
%}


/* ============================================================
 * Semantic values
 * ============================================================ */

%union
{
    long num;
    char *str;
    ASTNode *node;
}


/* ============================================================
 * Tokens
 * ============================================================ */

/* Keywords */

%token TOKEN_F
%token TOKEN_VM
%token TOKEN_VI

%token TOKEN_INT
%token TOKEN_BOOL
%token TOKEN_VOID

%token TOKEN_C
%token TOKEN_A
%token TOKEN_L
%token TOKEN_S
%token TOKEN_E
%token TOKEN_R

%token TOKEN_TRUE
%token TOKEN_FALSE


/* Operators */

%token TOKEN_OR
%token TOKEN_AND

%token TOKEN_EQUAL_EQUAL
%token TOKEN_NOT_EQUAL

%token TOKEN_LESS
%token TOKEN_GREATER
%token TOKEN_LESS_EQUAL
%token TOKEN_GREATER_EQUAL

%token TOKEN_PLUS
%token TOKEN_MINUS

%token TOKEN_MULTIPLY
%token TOKEN_DIVIDE
%token TOKEN_MODULO

%token TOKEN_NOT
%token TOKEN_POWER

%token TOKEN_ASSIGN


/* Delimiters */

%token TOKEN_LBRACKET
%token TOKEN_RBRACKET

%token TOKEN_LPAREN
%token TOKEN_RPAREN

%token TOKEN_COLON
%token TOKEN_SEMICOLON
%token TOKEN_COMMA
%token TOKEN_DOT

%token TOKEN_ERROR


/* Values */

%token <str> TOKEN_IDENTIFIER
%token <num> TOKEN_NUMBER


/* ============================================================
 * Nonterminal types
 * ============================================================ */

%type <node> programa
%type <node> globales
%type <node> funciones
%type <node> funcion

%type <node> retorno
%type <node> params
%type <node> lista_param
%type <node> param

%type <node> modif
%type <node> modif_opt

%type <node> tipo
%type <node> dims_opt
%type <node> dims

%type <node> cuerpo
%type <node> sentencia
%type <node> decl
%type <node> inicial
%type <node> lista_init
%type <node> elems
%type <node> elem
%type <node> sent_id

%type <node> condicional
%type <node> alts
%type <node> residual

%type <node> ciclo_l
%type <node> ciclo_s
%type <node> paso

%type <node> emit
%type <node> rewind

%type <node> args
%type <node> lista_arg

%type <node> expr
%type <node> disy
%type <node> conj
%type <node> igualdad
%type <node> relacion
%type <node> aditivo
%type <node> multiplicativo
%type <node> unario
%type <node> potencia
%type <node> primario
%type <node> acceso


%start programa


%%


/* ============================================================
 * PROGRAM
 *
 * programa -> globales funciones
 * ============================================================ */

programa
    : globales funciones
      {
          root = ast_program($1, $2);
          $$ = root;
      }
    ;


/* ============================================================
 * GLOBALS
 *
 * globales -> globales decl
 *           | epsilon
 * ============================================================ */

globales
    : /* empty */
      {
          $$ = ast_list(AST_GLOBALS, NULL);
      }

    | globales decl
      {
          ast_add_child($1, $2);
          $$ = $1;
      }
    ;


/* ============================================================
 * FUNCTIONS
 *
 * funciones -> funciones funcion
 *            | funcion
 * ============================================================ */

funciones
    : funcion
      {
          $$ = ast_list(AST_FUNCTIONS, $1);
      }

    | funciones funcion
      {
          ast_add_child($1, $2);
          $$ = $1;
      }
    ;


/* ============================================================
 * FUNCTION
 *
 * funcion -> F id [ params ] : retorno ; cuerpo .
 * ============================================================ */

funcion
    : TOKEN_F
      TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      params
      TOKEN_RBRACKET
      TOKEN_COLON
      retorno
      TOKEN_SEMICOLON
      cuerpo
      TOKEN_DOT
      {
          $$ = ast_function(
                    $2,
                    $4,
                    $7,
                    $9
                );

          free($2);
      }
    ;


/* ============================================================
 * RETURN
 *
 * retorno -> tipo id
 *          | void
 * ============================================================ */

retorno
    : tipo TOKEN_IDENTIFIER
      {
          $$ = ast_return_type(
                    $1,
                    $2
                );

          free($2);
      }

    | TOKEN_VOID
      {
          $$ = ast_return_void();
      }
    ;


/* ============================================================
 * PARAMETERS
 * ============================================================ */

params
    : /* empty */
      {
          $$ = ast_list(AST_PARAMS, NULL);
      }

    | lista_param
      {
          $$ = $1;
      }
    ;


lista_param
    : param
      {
          $$ = ast_list(AST_PARAMS, $1);
      }

    | lista_param TOKEN_COMMA param
      {
          ast_add_child($1, $3);
          $$ = $1;
      }
    ;


param
    : modif_opt tipo TOKEN_IDENTIFIER dims_opt
      {
          $$ = ast_parameter(
                    $1,
                    $2,
                    $3,
                    $4
                );

          free($3);
      }
    ;


modif
    : TOKEN_VM
      {
          $$ = ast_modifier("Vm");
      }

    | TOKEN_VI
      {
          $$ = ast_modifier("Vi");
      }
    ;


modif_opt
    : modif
      {
          $$ = $1;
      }

    | /* empty */
      {
          $$ = NULL;
      }
    ;


tipo
    : TOKEN_INT
      {
          $$ = ast_type("int");
      }

    | TOKEN_BOOL
      {
          $$ = ast_type("bool");
      }
    ;


dims_opt
    : dims
      {
          $$ = $1;
      }

    | /* empty */
      {
          $$ = NULL;
      }
    ;


dims
    : TOKEN_LBRACKET
      TOKEN_NUMBER
      TOKEN_RBRACKET
      {
          $$ = ast_dimensions();

          ast_add_number(
              $$,
              $2
          );
      }

    | TOKEN_LBRACKET
      TOKEN_NUMBER
      TOKEN_RBRACKET
      TOKEN_LBRACKET
      TOKEN_NUMBER
      TOKEN_RBRACKET
      {
          $$ = ast_dimensions();

          ast_add_number($$, $2);
          ast_add_number($$, $5);
      }
    ;


/* ============================================================
 * BODY
 *
 * cuerpo -> cuerpo sentencia
 *         | epsilon
 * ============================================================ */

cuerpo
    : /* empty */
      {
          $$ = ast_block();
      }

    | cuerpo sentencia
      {
          ast_add_child($1, $2);
          $$ = $1;
      }
    ;


/* ============================================================
 * STATEMENT
 * ============================================================ */

sentencia
    : decl
      {
          $$ = $1;
      }

    | sent_id
      {
          $$ = $1;
      }

    | condicional
      {
          $$ = $1;
      }

    | ciclo_l
      {
          $$ = $1;
      }

    | ciclo_s
      {
          $$ = $1;
      }

    | emit
      {
          $$ = $1;
      }

    | rewind
      {
          $$ = $1;
      }
    ;


/* ============================================================
 * DECLARATION
 *
 * decl -> modif id : tipo dims_opt ;
 *      | modif id : tipo dims_opt = inicial ;
 * ============================================================ */

decl
    : modif
      TOKEN_IDENTIFIER
      TOKEN_COLON
      tipo
      dims_opt
      TOKEN_SEMICOLON
      {
          $$ = ast_declaration(
                    $1,
                    $2,
                    $4,
                    $5,
                    NULL
                );

          free($2);
      }

    | modif
      TOKEN_IDENTIFIER
      TOKEN_COLON
      tipo
      dims_opt
      TOKEN_ASSIGN
      inicial
      TOKEN_SEMICOLON
      {
          $$ = ast_declaration(
                    $1,
                    $2,
                    $4,
                    $5,
                    $7
                );

          free($2);
      }
    ;


/* ============================================================
 * INITIALIZER
 * ============================================================ */

inicial
    : expr
      {
          $$ = $1;
      }

    | lista_init
      {
          $$ = $1;
      }
    ;


lista_init
    : TOKEN_LBRACKET elems TOKEN_RBRACKET
      {
          $$ = $2;
      }
    ;


elems
    : elem
      {
          $$ = ast_list(AST_INITIALIZER_LIST, $1);
      }

    | elems TOKEN_COMMA elem
      {
          ast_add_child($1, $3);
          $$ = $1;
      }
    ;


elem
    : expr
      {
          $$ = $1;
      }

    | lista_init
      {
          $$ = $1;
      }
    ;


/* ============================================================
 * ID STATEMENTS
 * ============================================================ */

sent_id
    : TOKEN_IDENTIFIER
      TOKEN_ASSIGN
      expr
      TOKEN_SEMICOLON
      {
          $$ = ast_assignment(
                    $1,
                    NULL,
                    NULL,
                    $3
                );

          free($1);
      }

    | TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      TOKEN_ASSIGN
      expr
      TOKEN_SEMICOLON
      {
          $$ = ast_assignment(
                    $1,
                    $3,
                    NULL,
                    $6
                );

          free($1);
      }

    | TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_ASSIGN
      expr
      TOKEN_SEMICOLON
      {
          $$ = ast_assignment(
                    $1,
                    $3,
                    $6,
                    $9
                );

          free($1);
      }

    | TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_call_statement(
                    $1,
                    $3
                );

          free($1);
      }
    ;


/* ============================================================
 * CONDITIONAL
 *
 * C [ expr ] ; cuerpo alts residual .
 * ============================================================ */

condicional
    : TOKEN_C
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      cuerpo
      alts
      residual
      TOKEN_DOT
      {
          $$ = ast_conditional(
                    $3,
                    $6,
                    $7,
                    $8
                );
      }
    ;


alts
    : /* empty */
      {
          $$ = ast_list(AST_ALTERNATIVES, NULL);
      }

    | alts
      TOKEN_A
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      cuerpo
      {
          ASTNode *alternative =
              ast_alternative(
                  $4,
                  $7
              );

          ast_add_child($1, alternative);

          $$ = $1;
      }
    ;


residual
    : /* empty */
      {
          $$ = NULL;
      }

    | TOKEN_A
      TOKEN_SEMICOLON
      cuerpo
      {
          $$ = ast_residual($3);
      }
    ;


/* ============================================================
 * L LOOP
 *
 * L [ expr ] ; cuerpo .
 * ============================================================ */

ciclo_l
    : TOKEN_L
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      cuerpo
      TOKEN_DOT
      {
          $$ = ast_loop_l(
                    $3,
                    $6
                );
      }
    ;


/* ============================================================
 * S LOOP
 *
 * S [ id , expr , expr , paso ] ; cuerpo .
 * ============================================================ */

ciclo_s
    : TOKEN_S
      TOKEN_LBRACKET
      TOKEN_IDENTIFIER
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      paso
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      cuerpo
      TOKEN_DOT
      {
          $$ = ast_loop_s(
                    $3,
                    $5,
                    $7,
                    $9,
                    $12
                );

          free($3);
      }
    ;


paso
    : TOKEN_NUMBER
      {
          $$ = ast_number($1);
      }

    | TOKEN_MINUS TOKEN_NUMBER
      {
          $$ = ast_number(-$2);
      }
    ;


/* ============================================================
 * EMIT
 * ============================================================ */

emit
    : TOKEN_E TOKEN_SEMICOLON
      {
          $$ = ast_emit();
      }
    ;


/* ============================================================
 * REWIND
 * ============================================================ */

rewind
    : TOKEN_R
      TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_rewind(
                    $2,
                    $4
                );

          free($2);
      }
    ;


/* ============================================================
 * ARGUMENTS
 * ============================================================ */

args
    : /* empty */
      {
          $$ = ast_list(AST_ARGUMENTS, NULL);
      }

    | lista_arg
      {
          $$ = $1;
      }
    ;


lista_arg
    : expr
      {
          $$ = ast_list(AST_ARGUMENTS, $1);
      }

    | lista_arg TOKEN_COMMA expr
      {
          ast_add_child($1, $3);
          $$ = $1;
      }
    ;


/* ============================================================
 * EXPRESSIONS
 *
 * These preserve your original 9 precedence levels.
 * ============================================================ */

expr
    : disy
      {
          $$ = $1;
      }
    ;


disy
    : conj
      {
          $$ = $1;
      }

    | disy TOKEN_OR conj
      {
          $$ = ast_binary(
                    AST_OR,
                    $1,
                    $3
                );
      }
    ;


conj
    : igualdad
      {
          $$ = $1;
      }

    | conj TOKEN_AND igualdad
      {
          $$ = ast_binary(
                    AST_AND,
                    $1,
                    $3
                );
      }
    ;


igualdad
    : relacion
      {
          $$ = $1;
      }

    | igualdad TOKEN_EQUAL_EQUAL relacion
      {
          $$ = ast_binary(
                    AST_EQUAL,
                    $1,
                    $3
                );
      }

    | igualdad TOKEN_NOT_EQUAL relacion
      {
          $$ = ast_binary(
                    AST_NOT_EQUAL,
                    $1,
                    $3
                );
      }
    ;


relacion
    : aditivo
      {
          $$ = $1;
      }

    | relacion TOKEN_LESS aditivo
      {
          $$ = ast_binary(
                    AST_LESS,
                    $1,
                    $3
                );
      }

    | relacion TOKEN_GREATER aditivo
      {
          $$ = ast_binary(
                    AST_GREATER,
                    $1,
                    $3
                );
      }

    | relacion TOKEN_LESS_EQUAL aditivo
      {
          $$ = ast_binary(
                    AST_LESS_EQUAL,
                    $1,
                    $3
                );
      }

    | relacion TOKEN_GREATER_EQUAL aditivo
      {
          $$ = ast_binary(
                    AST_GREATER_EQUAL,
                    $1,
                    $3
                );
      }
    ;


aditivo
    : multiplicativo
      {
          $$ = $1;
      }

    | aditivo TOKEN_PLUS multiplicativo
      {
          $$ = ast_binary(
                    AST_ADD,
                    $1,
                    $3
                );
      }

    | aditivo TOKEN_MINUS multiplicativo
      {
          $$ = ast_binary(
                    AST_SUB,
                    $1,
                    $3
                );
      }
    ;


multiplicativo
    : unario
      {
          $$ = $1;
      }

    | multiplicativo TOKEN_MULTIPLY unario
      {
          $$ = ast_binary(
                    AST_MUL,
                    $1,
                    $3
                );
      }

    | multiplicativo TOKEN_DIVIDE unario
      {
          $$ = ast_binary(
                    AST_DIV,
                    $1,
                    $3
                );
      }

    | multiplicativo TOKEN_MODULO unario
      {
          $$ = ast_binary(
                    AST_MOD,
                    $1,
                    $3
                );
      }
    ;


unario
    : TOKEN_MINUS unario
      {
          $$ = ast_unary(
                    AST_NEG,
                    $2
                );
      }

    | TOKEN_NOT unario
      {
          $$ = ast_unary(
                    AST_NOT,
                    $2
                );
      }

    | potencia
      {
          $$ = $1;
      }
    ;


potencia
    : primario
      {
          $$ = $1;
      }

    | primario TOKEN_POWER unario
      {
          $$ = ast_binary(
                    AST_POWER,
                    $1,
                    $3
                );
      }
    ;


primario
    : TOKEN_LPAREN expr TOKEN_RPAREN
      {
          $$ = $2;
      }

    | acceso
      {
          $$ = $1;
      }

    | TOKEN_NUMBER
      {
          $$ = ast_number($1);
      }

    | TOKEN_TRUE
      {
          $$ = ast_boolean(1);
      }

    | TOKEN_FALSE
      {
          $$ = ast_boolean(0);
      }
    ;


acceso
    : TOKEN_IDENTIFIER
      {
          $$ = ast_identifier($1);
          free($1);
      }

    | TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      {
          $$ = ast_call(
                    $1,
                    $3
                );

          free($1);
      }

    | TOKEN_IDENTIFIER
      TOKEN_LBRACKET
      args
      TOKEN_RBRACKET
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      {
          $$ = ast_index(
                    $1,
                    $3,
                    $6
                );

          free($1);
      }
    ;


%%


/* ============================================================
 * Error reporting
 * ============================================================ */

void yyerror(const char *msg)
{
    fprintf(stderr,
            "Syntax error at line %d: %s\n",
            yylineno,
            msg);
}