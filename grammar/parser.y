/*
 * parser.y — Fase 2: análisis sintáctico y construcción del AST con Bison.
 *
 * Construye nodos con sus posiciones y observa tokens para diagnosticar
 * bloques abiertos. LAC obtiene tokens esperados sin ejecutar las acciones.
 *
 * Usa: lexer.l, ast.c y diagnostics.c.
 * Lo usa: main.c mediante yyparse.
 * Referencias: libro §4.2 (notación), §4.8–4.9 (LALR y Bison),
 * §5.3.1 (construcción del AST).
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "diagnostics.h"

extern int yylex(void);
extern int yylineno;

/* 1 si un error léxico cortó la entrada (lexer.l). */
extern int lexical_fatal;

void yyerror(const char *msg);

ASTNode *root = NULL;

/* Observación de tokens para diagnósticos; no cambia reglas ni el AST.
 * El token anterior se registra solo cuando Bison solicita el siguiente. */
static int syntax_lex(void);
static void syntax_reset(void);
#define yylex syntax_lex
%}


/* ==========================================================================
 * VALORES SEMÁNTICOS
 * ======================================================================= */


%union
{
    long num;
    char *str;
    ASTNode *node;
}


/* ==========================================================================
 * TOKENS
 * ======================================================================= */


/* Palabras reservadas */

%token TOKEN_F "F"
%token TOKEN_VM "Vm"
%token TOKEN_VI "Vi"

%token TOKEN_INT "int"
%token TOKEN_BOOL "bool"
%token TOKEN_VOID "void"

%token TOKEN_C "C"
%token TOKEN_A "A"
%token TOKEN_L "L"
%token TOKEN_S "S"
%token TOKEN_E "E"
%token TOKEN_R "R"

%token TOKEN_TRUE "verdadero"
%token TOKEN_FALSE "falso"

%token TOKEN_AUTENTICAR "autenticar"
%token TOKEN_CERRAR_SESION "cerrar_sesion"
%token TOKEN_CARGAR_LLAVE "cargar_llave"
%token TOKEN_SELLAR "sellar"
%token TOKEN_CAMBIAR_ANCLA "cambiar_ancla"
%token TOKEN_CIFRAR "cifrar"
%token TOKEN_DESCIFRAR "descifrar"


/* Operadores */

%token TOKEN_OR "|"
%token TOKEN_AND "&"

%token TOKEN_EQUAL_EQUAL "=="
%token TOKEN_NOT_EQUAL "!="

%token TOKEN_LESS "<"
%token TOKEN_GREATER ">"
%token TOKEN_LESS_EQUAL "<="
%token TOKEN_GREATER_EQUAL ">="

%token TOKEN_PLUS "+"
%token TOKEN_MINUS "-"

%token TOKEN_MULTIPLY "*"
%token TOKEN_DIVIDE "/"
%token TOKEN_MODULO "%"

%token TOKEN_NOT "!"
%token TOKEN_POWER "^"

%token TOKEN_ASSIGN "="


/* Delimitadores */

%token TOKEN_LBRACKET "["
%token TOKEN_RBRACKET "]"

%token TOKEN_LPAREN "("
%token TOKEN_RPAREN ")"

%token TOKEN_COLON ":"
%token TOKEN_SEMICOLON ";"
%token TOKEN_COMMA ","
%token TOKEN_DOT "."



/* Valores */

%token <str> TOKEN_IDENTIFIER "identificador"
%token <num> TOKEN_NUMBER "numero"


/* ==========================================================================
 * TIPOS DE NO TERMINALES
 * ======================================================================= */


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
%type <node> crypto

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


/* P-03: cada token y cada símbolo de una producción lleva su línea (@n). */
%locations

/* CE1108 §8.2; calser_diseno §8.3/§8.5 (mensajes y primer error).
 * Libro, cap. 4: diagnóstico en el parser, sin producciones de recuperación.
 * LAC obtiene esperados válidos tras reducciones, sin ejecutar sus acciones. */
%define parse.error custom
%define parse.lac full
%initial-action { syntax_reset(); }

%start programa


%%


/* ==========================================================================
 * PROGRAMA
 * ======================================================================= */


/* programa → globales funciones */
programa
    : globales funciones
      {
          root = ast_program($1, $2);
          $$ = root;
      }
    ;


/* ==========================================================================
 * GLOBALES
 * ======================================================================= */


/* globales → ε | globales decl */
globales
    : /* vacío */
      {
          $$ = ast_list(AST_GLOBALS, NULL);
      }

    | globales decl
      {
          ast_add_child($1, $2);
          $$ = $1;
      }
    ;


/* ==========================================================================
 * FUNCIONES
 * ======================================================================= */


/* funciones → funcion | funciones funcion */
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


/* ==========================================================================
 * FUNCIÓN
 * ======================================================================= */


/* funcion → F id [ params ] : retorno ; cuerpo . */
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

          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* ==========================================================================
 * RETORNO
 * ======================================================================= */


/* retorno → tipo id | void */
retorno
    : tipo TOKEN_IDENTIFIER
      {
          $$ = ast_return_type(
                    $1,
                    $2
                );

          free($2);

          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | TOKEN_VOID
      {
          $$ = ast_return_void();

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * PARÁMETROS
 * ======================================================================= */


/* params → ε | lista_param */
params
    : /* vacío */
      {
          $$ = ast_list(AST_PARAMS, NULL);
      }

    | lista_param
      {
          $$ = $1;
      }
    ;


/* lista_param → param | lista_param , param */
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


/* param → modif_opt tipo id dims_opt */
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

          ast_set_location($$, @3.first_line, @3.first_column);
      }
    ;


/* modif → Vm | Vi */
modif
    : TOKEN_VM
      {
          $$ = ast_modifier("Vm");

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_VI
      {
          $$ = ast_modifier("Vi");

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* modif_opt → modif | ε */
modif_opt
    : modif
      {
          $$ = $1;
      }

    | /* vacío */
      {
          $$ = NULL;
      }
    ;


/* tipo → int | bool */
tipo
    : TOKEN_INT
      {
          $$ = ast_type("int");

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_BOOL
      {
          $$ = ast_type("bool");

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* dims_opt → dims | ε */
dims_opt
    : dims
      {
          $$ = $1;
      }

    | /* vacío */
      {
          $$ = NULL;
      }
    ;


/* dims → [ numero ] | [ numero ] [ numero ] */
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

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * CUERPO
 * ======================================================================= */


/* cuerpo → ε | cuerpo sentencia */
cuerpo
    : /* vacío */
      {
          $$ = ast_block();
      }

    | cuerpo sentencia
      {
          ast_add_child($1, $2);
          $$ = $1;
      }
    ;


/* ==========================================================================
 * SENTENCIAS
 * ======================================================================= */


/* sentencia → decl | sent_id | condicional | ciclo_l | ciclo_s | emit | rewind | crypto */
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

    | crypto
      {
          $$ = $1;
      }
    ;


/* ==========================================================================
 * DECLARACIONES
 * ======================================================================= */


/* decl → modif id : tipo dims_opt ; | modif id : tipo dims_opt = inicial ; */
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

          ast_set_location($$, @2.first_line, @2.first_column);
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

          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* ==========================================================================
 * INICIALIZADORES
 * ======================================================================= */


/* inicial → expr | lista_init */
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


/* lista_init → [ elems ] */
lista_init
    : TOKEN_LBRACKET elems TOKEN_RBRACKET
      {
          $$ = $2;
      }
    ;


/* elems → elem | elems , elem */
elems
    : elem
      {
          $$ = ast_list(AST_INITIALIZER_LIST, $1);

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | elems TOKEN_COMMA elem
      {
          ast_add_child($1, $3);
          $$ = $1;
      }
    ;


/* elem → expr | lista_init */
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


/* ==========================================================================
 * SENTENCIAS CON IDENTIFICADOR
 * ======================================================================= */


/* sent_id → id = expr ; | id [ args ] = expr ; | id [ args ] [ expr ] = expr ; | id [ args ] ; */
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

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * CONDICIONAL
 * ======================================================================= */


/* condicional → C [ expr ] ; cuerpo alts residual . */
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* alts → ε | alts A [ expr ] ; cuerpo */
alts
    : /* vacío */
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

          /* P-03: línea de la A */
          ast_set_location(alternative, @2.first_line, @2.first_column);

          ast_add_child($1, alternative);

          $$ = $1;
      }
    ;


/* residual → ε | A ; cuerpo */
residual
    : /* vacío */
      {
          $$ = NULL;
      }

    | TOKEN_A
      TOKEN_SEMICOLON
      cuerpo
      {
          $$ = ast_residual($3);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * CICLO L
 * ======================================================================= */


/* ciclo_l → L [ expr ] ; cuerpo . */
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * CICLO S
 * ======================================================================= */


/* ciclo_s → S [ id , expr , expr , paso ] ; cuerpo . */
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

          /* La variable de control recibe su posición real antes del relleno. */
          ast_set_location($$->child, @3.first_line, @3.first_column);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* paso → numero | - numero */
paso
    : TOKEN_NUMBER
      {
          $$ = ast_number($1);

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_MINUS TOKEN_NUMBER
      {
          $$ = ast_number(-$2);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * EMISIÓN
 * ======================================================================= */


/* emit → E ; */
emit
    : TOKEN_E TOKEN_SEMICOLON
      {
          $$ = ast_emit();

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * REINICIO
 * ======================================================================= */


/* rewind → R id [ args ] ; */
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

          /* El nombre recibe su posición real antes del relleno. */
          ast_set_location($$->child, @2.first_line, @2.first_column);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * CIFRADO
 * ======================================================================= */


/* crypto → autenticar [ expr ] ;
 *        | cerrar_sesion [ ] ;
 *        | cargar_llave [ numero , numero , expr ] ;
 *        | sellar [ ] ;
 *        | cambiar_ancla [ expr ] ;
 *        | cifrar [ expr , expr , expr , numero ] ;
 *        | descifrar [ expr , expr , expr , numero ] ;
 */
crypto
    : TOKEN_AUTENTICAR
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_crypto("autenticar", ast_list(AST_ARGUMENTS, $3));

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_CERRAR_SESION
      TOKEN_LBRACKET
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_crypto("cerrar_sesion", ast_list(AST_ARGUMENTS, NULL));

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_CARGAR_LLAVE
      TOKEN_LBRACKET
      TOKEN_NUMBER
      TOKEN_COMMA
      TOKEN_NUMBER
      TOKEN_COMMA
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          ASTNode *k = ast_number($3);
          ASTNode *w = ast_number($5);

          ast_set_location(k, @3.first_line, @3.first_column);
          ast_set_location(w, @5.first_line, @5.first_column);

          ASTNode *arguments = ast_list(AST_ARGUMENTS, k);

          ast_add_child(arguments, w);
          ast_add_child(arguments, $7);

          $$ = ast_crypto("cargar_llave", arguments);

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_SELLAR
      TOKEN_LBRACKET
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_crypto("sellar", ast_list(AST_ARGUMENTS, NULL));

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_CAMBIAR_ANCLA
      TOKEN_LBRACKET
      expr
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          $$ = ast_crypto("cambiar_ancla", ast_list(AST_ARGUMENTS, $3));

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_CIFRAR
      TOKEN_LBRACKET
      expr
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      TOKEN_NUMBER
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          ASTNode *k = ast_number($9);

          ast_set_location(k, @9.first_line, @9.first_column);

          ASTNode *arguments = ast_list(AST_ARGUMENTS, $3);

          ast_add_child(arguments, $5);
          ast_add_child(arguments, $7);
          ast_add_child(arguments, k);

          $$ = ast_crypto("cifrar", arguments);

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_DESCIFRAR
      TOKEN_LBRACKET
      expr
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      expr
      TOKEN_COMMA
      TOKEN_NUMBER
      TOKEN_RBRACKET
      TOKEN_SEMICOLON
      {
          ASTNode *k = ast_number($9);

          ast_set_location(k, @9.first_line, @9.first_column);

          ASTNode *arguments = ast_list(AST_ARGUMENTS, $3);

          ast_add_child(arguments, $5);
          ast_add_child(arguments, $7);
          ast_add_child(arguments, k);

          $$ = ast_crypto("descifrar", arguments);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* ==========================================================================
 * ARGUMENTOS
 * ======================================================================= */


/* args → ε | lista_arg */
args
    : /* vacío */
      {
          $$ = ast_list(AST_ARGUMENTS, NULL);
      }

    | lista_arg
      {
          $$ = $1;
      }
    ;


/* lista_arg → expr | lista_arg , expr */
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


/* ==========================================================================
 * EXPRESIONES
 * ======================================================================= */


/* expr → disy */
expr
    : disy
      {
          $$ = $1;
      }
    ;


/* disy → conj | disy '|' conj */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* conj → igualdad | conj & igualdad */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* igualdad → relacion | igualdad == relacion | igualdad != relacion */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | igualdad TOKEN_NOT_EQUAL relacion
      {
          $$ = ast_binary(
                    AST_NOT_EQUAL,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* relacion → aditivo
 *          | relacion < aditivo
 *          | relacion > aditivo
 *          | relacion <= aditivo
 *          | relacion >= aditivo
 */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | relacion TOKEN_GREATER aditivo
      {
          $$ = ast_binary(
                    AST_GREATER,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | relacion TOKEN_LESS_EQUAL aditivo
      {
          $$ = ast_binary(
                    AST_LESS_EQUAL,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | relacion TOKEN_GREATER_EQUAL aditivo
      {
          $$ = ast_binary(
                    AST_GREATER_EQUAL,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* aditivo → multiplicativo | aditivo + multiplicativo | aditivo - multiplicativo */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | aditivo TOKEN_MINUS multiplicativo
      {
          $$ = ast_binary(
                    AST_SUB,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* multiplicativo → unario
 *                | multiplicativo * unario
 *                | multiplicativo / unario
 *                | multiplicativo % unario
 */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | multiplicativo TOKEN_DIVIDE unario
      {
          $$ = ast_binary(
                    AST_DIV,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }

    | multiplicativo TOKEN_MODULO unario
      {
          $$ = ast_binary(
                    AST_MOD,
                    $1,
                    $3
                );

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* unario → - unario | ! unario | potencia */
unario
    : TOKEN_MINUS unario
      {
          $$ = ast_unary(
                    AST_NEG,
                    $2
                );

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_NOT unario
      {
          $$ = ast_unary(
                    AST_NOT,
                    $2
                );

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | potencia
      {
          $$ = $1;
      }
    ;


/* potencia → primario | primario ^ unario */
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

          /* El nodo queda en la posición del operador (señalador de errores). */
          ast_set_location($$, @2.first_line, @2.first_column);
      }
    ;


/* primario → ( expr ) | acceso | numero | verdadero | falso */
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_TRUE
      {
          $$ = ast_boolean(1);

          ast_set_location($$, @1.first_line, @1.first_column);
      }

    | TOKEN_FALSE
      {
          $$ = ast_boolean(0);

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


/* acceso → id | id [ args ] | id [ args ] [ expr ] */
acceso
    : TOKEN_IDENTIFIER
      {
          $$ = ast_identifier($1);
          free($1);

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
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

          ast_set_location($$, @1.first_line, @1.first_column);
      }
    ;


%%


/* Diagnósticos de calser_diseno §8.5, sin recuperación (§8.3).
 * C, todas sus A y su residual comparten un único cierre '.'. */
#undef yylex

typedef struct {
    int token;                 /* F/C/L/S: dueño del cierre */
    int shown;                 /* A identifica la rama actual de un C */
    YYLTYPE location;
    int header;                /* antes del ';' que abre el cuerpo */
    int delimiters;            /* profundidad al iniciar su cabecera */
    int commas;                /* comas de nivel superior en S */
} SyntaxBlock;

static SyntaxBlock *syntax_blocks;
static int syntax_count, syntax_capacity, syntax_delimiters;
static int syntax_previous;
static YYLTYPE syntax_previous_location;
static int syntax_pending;
static YYLTYPE syntax_pending_location;

/* ==========================================================================
 * REPORTE DE ERRORES Y BLOQUES ABIERTOS
 * ======================================================================= */


/*
 * syntax_reset — reinicia la observación de tokens para diagnósticos.
 *
 * Entrada: ninguna
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: libera syntax_blocks y reinicia posiciones y contadores.
 * Restricciones:
 *   se llama desde %initial-action.
 */
static void syntax_reset(void) {
    free(syntax_blocks);
    syntax_blocks = NULL;
    syntax_count = syntax_capacity = syntax_delimiters = 0;
    syntax_previous = syntax_pending = 0;
    syntax_previous_location = syntax_pending_location = (YYLTYPE) {1, 1, 1, 1};
}

/*
 * block_word — nombra el bloque observado.
 *
 * Entrada:
 *   token  token F, C, A, L o S.
 * Salida:
 *   cadena literal; ? para otros tokens.
 *   Efectos: ninguno.
 * Restricciones:
 *   ninguna.
 */
static const char *block_word(int token) {
    switch (token) {
        case TOKEN_F: return "F";
        case TOKEN_C: return "C";
        case TOKEN_A: return "A";
        case TOKEN_L: return "L";
        case TOKEN_S: return "S";
        default: return "?";
    }
}

/*
 * syntax_accept — registra el token anterior y el estado de bloques.
 *
 * Entrada:
 *   token  token leído.
 *   location  posición del token.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: actualiza pila de bloques, delimitadores, comas y token anterior.
 * Restricciones:
 *   A comparte cierre del C; el observador se usa para mensajes, sin
 *   cambiar reglas.
 */
static void syntax_accept(int token, YYLTYPE location) {
    syntax_previous = token;
    syntax_previous_location = location;
    if (token == TOKEN_F || token == TOKEN_C || token == TOKEN_L || token == TOKEN_S) {
        if (syntax_count == syntax_capacity) {
            int capacity = syntax_capacity ? syntax_capacity * 2 : 16;
            SyntaxBlock *blocks = realloc(syntax_blocks, (size_t) capacity * sizeof(*blocks));
            if (!blocks) {
                yyerror("memoria insuficiente");
                exit(EXIT_FAILURE);
            }
            syntax_blocks = blocks;
            syntax_capacity = capacity;
        }
        syntax_blocks[syntax_count++] = (SyntaxBlock) {
            token, token, location, 1, syntax_delimiters, 0
        };
    } else if (token == TOKEN_A && syntax_count && syntax_blocks[syntax_count - 1].token == TOKEN_C) {
        /* No se apila: la rama usa el cierre del C, incluso varias A. */
        SyntaxBlock *block = &syntax_blocks[syntax_count - 1];
        block->shown = TOKEN_A;
        block->location = location;
        block->header = 1;
    } else if (token == TOKEN_DOT && syntax_count) {
        syntax_count--;
    }
    if (syntax_count) {
        SyntaxBlock *block = &syntax_blocks[syntax_count - 1];
        if (block->header && token == TOKEN_SEMICOLON && syntax_delimiters == block->delimiters)
            block->header = 0;
        if (block->token == TOKEN_S && block->header && token == TOKEN_COMMA &&
            syntax_delimiters == block->delimiters + 1)
            block->commas++;
    }
    if (token == TOKEN_LBRACKET || token == TOKEN_LPAREN) syntax_delimiters++;
    if (token == TOKEN_RBRACKET || token == TOKEN_RPAREN) syntax_delimiters--;
}

/*
 * syntax_lex — envuelve yylex para observar tokens aceptados.
 *
 * Entrada: ninguna
 * Salida:
 *   token del léxico; 0 al fin de entrada.
 *   Efectos: registra token pendiente, actualiza observador y libera pila al EOF sin bloques.
 * Restricciones:
 *   se invoca mediante macro yylex en Bison; llama al léxico real después
 *   de #undef.
 */
static int syntax_lex(void) {
    if (syntax_pending) syntax_accept(syntax_pending, syntax_pending_location);
    int token = yylex();
    syntax_pending = token;
    syntax_pending_location = yylloc;
    /* Al aceptar EOF sin bloques no queda memoria de observación pendiente. */
    if (!token && !syntax_count) {
        free(syntax_blocks);
        syntax_blocks = NULL;
        syntax_capacity = 0;
    }
    return token;
}

/*
 * syntax_report — emite diagnóstico sintáctico y libera el observador.
 *
 * Entrada:
 *   location  posición para señalador.
 *   message  mensaje no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: invoca diag_error, libera syntax_blocks y reinicia contadores.
 * Restricciones:
 *   ninguna.
 */
static void syntax_report(YYLTYPE location, const char *message) {
    diag_error("error sintactico", location.first_line, location.first_column,
               location.last_column - location.first_column + 1, "%s", message);
    free(syntax_blocks);
    syntax_blocks = NULL;
    syntax_count = syntax_capacity = 0;
}

/*
 * token_word — escribe el nombre legible de un símbolo Bison.
 *
 * Entrada:
 *   token  símbolo Bison.
 *   out  buffer escribible.
 *   size  capacidad del buffer.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: escribe out mediante snprintf.
 * Restricciones:
 *   buffer de size bytes; nombres se truncan si no caben.
 */
static void token_word(yysymbol_kind_t token, char *out, size_t size) {
    if (token == YYSYMBOL_YYEOF) {
        snprintf(out, size, "fin de archivo");
    } else if (token == YYSYMBOL_TOKEN_IDENTIFIER || token == YYSYMBOL_TOKEN_NUMBER) {
        snprintf(out, size, "%s", token == YYSYMBOL_TOKEN_IDENTIFIER ? "identificador" : "numero");
    } else {
        const char *name = yysymbol_name(token);
        size_t length = strlen(name);
        if (length >= 2 && name[0] == '"' && name[length - 1] == '"') {
            name++;
            length -= 2;
        }
        snprintf(out, size, "'%.*s'", (int) length, name);
    }
}

/*
 * yyreport_syntax_error — selecciona un diagnóstico a partir de tokens esperados.
 *
 * Entrada:
 *   context  contexto Bison no NULL.
 * Salida:
 *   0 si reporta o suprime; valor negativo si falla consulta de esperados.
 *   Efectos: reporta primer error y libera observador; suprime cascada de lexical_fatal.
 * Restricciones:
 *   Bison con LAC; no hay producciones de recuperación.
 */
static int yyreport_syntax_error(const yypcontext_t *context) {
    if (lexical_fatal) {
        free(syntax_blocks);
        syntax_blocks = NULL;
        syntax_count = syntax_capacity = 0;
        return 0;             /* conservar supresión del error léxico fatal */
    }
    yysymbol_kind_t unexpected = yypcontext_token(context);
    yysymbol_kind_t expected[YYNTOKENS];
    int count = yypcontext_expected_tokens(context, expected, YYNTOKENS);
    if (count < 0) return count;
    int semicolon = 0, paren = 0, bracket = 0;
    for (int i = 0; i < count; i++) {
        semicolon |= expected[i] == YYSYMBOL_TOKEN_SEMICOLON;
        paren |= expected[i] == YYSYMBOL_TOKEN_RPAREN;
        bracket |= expected[i] == YYSYMBOL_TOKEN_RBRACKET;
    }
    YYLTYPE location = *yypcontext_location(context);
    char message[512];
    if (unexpected == YYSYMBOL_YYEOF) {
        if (syntax_count) {
            SyntaxBlock *block = &syntax_blocks[syntax_count - 1];
            location = block->location;
            snprintf(message, sizeof(message), "el bloque '%s' abierto en la linea %d nunca se cierra",
                     block_word(block->shown), location.first_line);
        } else {
            location = (YYLTYPE) {diag_last_line(), 1, diag_last_line(), 1};
            snprintf(message, sizeof(message), "fin de archivo inesperado");
        }
    } else if (unexpected == YYSYMBOL_TOKEN_F && syntax_count) {
        snprintf(message, sizeof(message), "no se permiten funciones anidadas; probablemente falta el '.'");
    } else if (unexpected == YYSYMBOL_TOKEN_A &&
               (!syntax_count || syntax_blocks[syntax_count - 1].token != TOKEN_C)) {
        snprintf(message, sizeof(message), "'A' sin una estructura 'C' que lo preceda");
    } else if (unexpected == YYSYMBOL_TOKEN_DOT && !syntax_count) {
        snprintf(message, sizeof(message), "cierre de bloque sin bloque correspondiente");
    } else if (syntax_count && syntax_blocks[syntax_count - 1].token == TOKEN_S &&
               syntax_blocks[syntax_count - 1].header &&
               syntax_delimiters == syntax_blocks[syntax_count - 1].delimiters + 1 &&
               ((unexpected == YYSYMBOL_TOKEN_RBRACKET && syntax_blocks[syntax_count - 1].commas != 3) ||
                (unexpected == YYSYMBOL_TOKEN_COMMA && syntax_blocks[syntax_count - 1].commas >= 3))) {
        snprintf(message, sizeof(message), "'S' requiere exactamente 4 partes: variable, inicio, fin, paso");
    } else if (semicolon) {
        location = syntax_previous_location;
        location.first_line = location.last_line;
        location.first_column = location.last_column + 1;
        location.last_column = location.first_column;
        snprintf(message, sizeof(message), "se esperaba ';' al final de la sentencia");
    } else if (paren || bracket) {
        snprintf(message, sizeof(message), "se esperaba '%c'", paren ? ')' : ']');
    } else {
        char word[64];
        token_word(unexpected, word, sizeof(word));
        snprintf(message, sizeof(message), "token inesperado %s", word);
        if (count > 0 && count <= 4) {
            size_t used = strlen(message);
            snprintf(message + used, sizeof(message) - used, "; se esperaba ");
            for (int i = 0; i < count; i++) {
                token_word(expected[i], word, sizeof(word));
                used = strlen(message);
                snprintf(message + used, sizeof(message) - used, "%s%s", i ? " o " : "", word);
            }
        }
    }
    syntax_report(location, message);
    return 0;
}

/*
 * yyerror — reporta errores internos que Bison o Flex delegan.
 *
 * Entrada:
 *   message  mensaje no NULL.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: reporta con posición actual y libera observador salvo lexical_fatal.
 * Restricciones:
 *   fallos sintácticos normales pasan por yyreport_syntax_error.
 */
void yyerror(const char *message) {
    if (!lexical_fatal) {
        YYLTYPE location = yylloc;
        if (location.first_line < 1 || location.first_line > diag_last_line())
            location = (YYLTYPE) {diag_last_line(), 1, diag_last_line(), 1};
        syntax_report(location, message);
    }
}
