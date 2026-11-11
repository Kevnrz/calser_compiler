/*
 * lexer.l — Fase 1: reconocimiento de tokens de Calser con Flex.
 *
 * Reconoce palabras reservadas, operadores, literales e identificadores.
 * Mantiene posiciones y estados de comentarios y reporta errores léxicos.
 *
 * Usa: parser.tab.h, ast.h y diagnostics.c.
 * Lo usa: parser.y mediante yylex; main.c consulta lexical_errors.
 * Referencias: libro §3.5.3; enunciado §8.2, §8.4.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.tab.h"
#include "diagnostics.h"
#include "diagnostics_json.h"
#include "lexer_service.h"
#include "parser_service.h"

// Columna del siguiente carácter. Una tabulación cuenta como UNA columna:
// el señalador de los errores queda alineado porque copia las tabulaciones
// de la línea original.
static int column = 1;
/* Posiciones absolutas UTF-8: Flex consume bytes, también dentro de comentarios.
 * El observador es inerte en la CLI clásica y no altera lo que recibe Bison. */
static size_t lex_offset = 0, lex_start = 0, comment_start = 0;
static LexerObserver lex_observer;
void lexer_set_observer(LexerObserver observer) { lex_observer = observer; }
static void lexer_emit(const char *kind, size_t start, size_t end) {
    diag_json_token(kind, start, end);
    if (lex_observer) lex_observer(kind, start, end);
}
#define LEX_RETURN(token, kind) do { \
    lexer_emit(kind, lex_start, lex_offset); \
    return token; \
} while (0)

// P-03 / §8.2: cada token guarda línea y columnas (Bison %locations).
#define YY_USER_ACTION                                  \
    yylloc.first_line   = yylloc.last_line = yylineno;  \
    yylloc.first_column = column;                       \
    yylloc.last_column  = column + yyleng - 1;          \
    column += yyleng; \
    lex_start = lex_offset; \
    lex_offset += (size_t) yyleng;

// §8.4: largo máximo de un identificador y mayor literal entero que admite
// el analizador léxico. 2147483648 no es error aquí: el signo no forma parte
// del literal y -2147483648 es válido (lo resuelve el semántico).
#define MAX_IDENTIFIER 32
#define MAX_LITERAL    2147483648LL

// Cantidad de errores léxicos. Si hay alguno, el análisis semántico no corre.
int lexical_errors = 0;

// 1 si un error léxico cortó la entrada (comentario sin cerrar): el error
// sintáctico que provoque ese fin abrupto sería una cascada.
int lexical_fatal = 0;

// Posición del #[ que abrió el comentario de bloque actual.
static int comment_line = 0;
static int comment_column = 0;

void yyerror(const char *msg);

/* ==========================================================================
 * DIAGNÓSTICOS LÉXICOS
 * ======================================================================= */


/*
 * lexical_error — reporta un error léxico con posición.
 *
 * Entrada:
 *   line  línea desde 1.
 *   col  columna desde 1.
 *   length  longitud del lexema.
 *   fmt  formato printf.
 *   ...  argumentos del formato.
 * Salida:
 *   ninguna (solo efectos).
 *   Efectos: invoca diag_verror e incrementa lexical_errors.
 * Restricciones:
 *   diag_init previo; reportar no detiene por sí solo el escaneo.
 */
static void lexical_error(int line, int col, int length,
                          const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    diag_verror("error lexico", line, col, length, fmt, args);
    va_end(args);

    lexical_errors++;
}
%}

/* ==========================================================================
 * DEFINICIONES
 * ======================================================================= */

%option noyywrap
%option noinput
%option nounput
%option yylineno

%x COMMENT


%%



 /* =========================================================================
  * ESPACIOS Y COMENTARIOS
  * ====================================================================== */

[ \t\r]+              ;

\n                    { column = 1; }

"#["                  {
                         /* L-04: se guarda dónde abre, por si nunca cierra */
                         comment_start = lex_start;
                         comment_line = yylineno;
                         comment_column = yylloc.first_column;

                         BEGIN(COMMENT);
                       }

<COMMENT>"]#"         { lexer_emit("comentario", comment_start, lex_offset); BEGIN(INITIAL); }

<COMMENT>\n           { column = 1; }

<COMMENT>.            ;

<COMMENT><<EOF>>      {
                         /* L-04: error fatal, en la posición del #[ */
                         lexical_error(comment_line,
                                       comment_column,
                                       2,
                                       "comentario abierto en la linea %d "
                                       "nunca se cierra",
                                       comment_line);

                         lexer_emit("comentario", comment_start, lex_offset);
                         lexical_fatal = 1;

                         return 0;
                       }


 /* L-03: un # seguido de [ nunca es comentario de línea. Ante #[ esta regla
    solo alcanza el # (largo 1) y gana "#[" (largo 2) por prefijo más largo
    (libro §3.5.3). Un # solo al final de la línea sigue siendo comentario. */
"#"([^\[\n][^\n]*)?   { lexer_emit("comentario", lex_start, lex_offset); }

 /* =========================================================================
  * PALABRAS RESERVADAS
  * ====================================================================== */

"F"                   LEX_RETURN(TOKEN_F, "reservada");
"Vm"                  LEX_RETURN(TOKEN_VM, "reservada");
"Vi"                  LEX_RETURN(TOKEN_VI, "reservada");

"int"                 LEX_RETURN(TOKEN_INT, "tipo");
"bool"                LEX_RETURN(TOKEN_BOOL, "tipo");
"void"                LEX_RETURN(TOKEN_VOID, "tipo");

 /* CRYPTO: palabras reservadas. Van antes de ID: con igual largo gana el
    patrón que está primero; cifrar_total sigue siendo ID por prefijo más
    largo (libro §3.5.3). */
"autenticar"          LEX_RETURN(TOKEN_AUTENTICAR, "reservada");
"cerrar_sesion"       LEX_RETURN(TOKEN_CERRAR_SESION, "reservada");
"cargar_llave"        LEX_RETURN(TOKEN_CARGAR_LLAVE, "reservada");
"sellar"              LEX_RETURN(TOKEN_SELLAR, "reservada");
"cambiar_ancla"       LEX_RETURN(TOKEN_CAMBIAR_ANCLA, "reservada");
"cifrar"              LEX_RETURN(TOKEN_CIFRAR, "reservada");
"descifrar"           LEX_RETURN(TOKEN_DESCIFRAR, "reservada");

"C"                   LEX_RETURN(TOKEN_C, "reservada");
"A"                   LEX_RETURN(TOKEN_A, "reservada");
"L"                   LEX_RETURN(TOKEN_L, "reservada");
"S"                   LEX_RETURN(TOKEN_S, "reservada");
"E"                   LEX_RETURN(TOKEN_E, "reservada");
"R"                   LEX_RETURN(TOKEN_R, "reservada");

"verdadero"           LEX_RETURN(TOKEN_TRUE, "booleano");
"falso"               LEX_RETURN(TOKEN_FALSE, "booleano");

 /* =========================================================================
  * OPERADORES Y DELIMITADORES
  * ====================================================================== */

"=="                  LEX_RETURN(TOKEN_EQUAL_EQUAL, "operador");
"!="                  LEX_RETURN(TOKEN_NOT_EQUAL, "operador");
"<="                  LEX_RETURN(TOKEN_LESS_EQUAL, "operador");
">="                  LEX_RETURN(TOKEN_GREATER_EQUAL, "operador");

"|"                   LEX_RETURN(TOKEN_OR, "operador");
"&"                   LEX_RETURN(TOKEN_AND, "operador");

"<"                   LEX_RETURN(TOKEN_LESS, "operador");
">"                   LEX_RETURN(TOKEN_GREATER, "operador");

"+"                   LEX_RETURN(TOKEN_PLUS, "operador");
"-"                   LEX_RETURN(TOKEN_MINUS, "operador");

"*"                   LEX_RETURN(TOKEN_MULTIPLY, "operador");
"/"                   LEX_RETURN(TOKEN_DIVIDE, "operador");
"%"                   LEX_RETURN(TOKEN_MODULO, "operador");

"!"                   LEX_RETURN(TOKEN_NOT, "operador");
"^"                   LEX_RETURN(TOKEN_POWER, "operador");

"="                   LEX_RETURN(TOKEN_ASSIGN, "operador");

"["                   LEX_RETURN(TOKEN_LBRACKET, "operador");
"]"                   LEX_RETURN(TOKEN_RBRACKET, "operador");

"("                   LEX_RETURN(TOKEN_LPAREN, "operador");
")"                   LEX_RETURN(TOKEN_RPAREN, "operador");

":"                   LEX_RETURN(TOKEN_COLON, "operador");
";"                   LEX_RETURN(TOKEN_SEMICOLON, "operador");
","                   LEX_RETURN(TOKEN_COMMA, "operador");
"."                   LEX_RETURN(TOKEN_DOT, "operador");

 /* =========================================================================
  * LITERALES E IDENTIFICADORES
  * ====================================================================== */

[0-9]+                {
                         /* L-06: se convierte sin desbordar. Sin ceros a la
                            izquierda, con más de 10 dígitos ya está fuera
                            de rango; si no, cabe en un long long. */
                         const char *digits = yytext;

                         while (digits[0] == '0' && digits[1] != '\0')
                             digits++;

                         long long value =
                             strlen(digits) > 10
                                 ? MAX_LITERAL + 1
                                 : strtoll(digits, NULL, 10);

                         if (value > MAX_LITERAL) {

                             lexical_error(yylineno,
                                           yylloc.first_column,
                                           yyleng,
                                           "constante entera fuera del rango "
                                           "de 32 bits");

                             /* Recuperación: se sustituye por 0. */
                             value = 0;
                         }

                         yylval.num = (long) value;

                         LEX_RETURN(TOKEN_NUMBER, "numero");
                       }

[a-z_][a-zA-Z0-9_]* {
                         /* L-02: el contexto CE1108 §5.7 pedía un buffer
                            fijo; el grupo decidió mantener la copia con
                            memoria dinámica (strdup) porque el
                            comportamiento es idéntico. */
                         if (yyleng > MAX_IDENTIFIER) {

                             lexical_error(yylineno,
                                           yylloc.first_column,
                                           yyleng,
                                           "identificador excede la longitud "
                                           "maxima");

                             /* Recuperación: se trunca a 32 caracteres. */
                             yylval.str = strndup(yytext, MAX_IDENTIFIER);
                         }
                         else {
                             yylval.str = strdup(yytext);
                         }

                         if (!yylval.str)
                         {
                             yyerror("out of memory");
                             exit(EXIT_FAILURE);
                         }

                         LEX_RETURN(TOKEN_IDENTIFIER, "identificador");
                       }

 /* =========================================================================
  * ERRORES LÉXICOS
  * ====================================================================== */

.                     {
                         /* Un byte que no es ASCII imprimible (0x20-0x7E)
                            se muestra como \xHH. Cada byte es un error. */
                         if (lex_observer) lex_observer("invalido", lex_start, lex_offset);
                         unsigned char byte = (unsigned char) yytext[0];
                         char shown[8];

                         if (byte >= 0x20 && byte <= 0x7E)
                             snprintf(shown, sizeof(shown), "%c", byte);
                         else
                             snprintf(shown, sizeof(shown), "\\x%02X", byte);

                         lexical_error(yylineno,
                                       yylloc.first_column,
                                       1,
                                       "caracter no reconocido: '%s'",
                                       shown);

                         /* L-01 / §8.3: se descarta el carácter y se sigue
                            desde el siguiente token (no se devuelve nada). */
                       }


%%
/* Servicio --tokens-json. Recorre hasta EOF sin llamar yyparse: una sintaxis
 * incompleta no puede cortar los colores del resto del documento. El lexer
 * reserva strings para Bison; aquí el consumidor los libera inmediatamente. */
int lexer_scan_only(void) {
    int token;
    column = yylineno = 1;
    lex_offset = lex_start = comment_start = 0;
    lexical_errors = lexical_fatal = 0;
    BEGIN(INITIAL);
    yyrestart(yyin);
    while ((token = yylex()) != 0) {
        if (token == TOKEN_IDENTIFIER) {
            free(yylval.str);
            yylval.str = NULL;
        }
    }
    return lexical_errors ? 1 : 0;
}

/* El parser recibe length bytes de un prefijo M3 o una copia completa M4.
 * Sus modos activan destructores de consulta; el AST nunca se entrega a main.
 * Flex copia la entrada: yy_delete_buffer libera esa copia, no el texto ajeno.
 * Destruir el escaneo anterior evita retener memoria al usar yy_scan_bytes. */
int lexer_parse_text(const char *text, size_t length) {
    extern int yyparse(void);
    extern ASTNode *root;
    root = NULL;
    yylex_destroy();
    column = yylineno = 1;
    lex_offset = lex_start = 0;
    lexical_errors = lexical_fatal = 0;
    BEGIN(INITIAL);
    YY_BUFFER_STATE buffer = yy_scan_bytes(text, (int) length);
    int result = yyparse();
    parser_finish();
    root = NULL;  /* El servicio liberó los símbolos mediante destructores. */
    yy_delete_buffer(buffer);
    yylex_destroy();
    return result;
}

/* Escaneo íntegro de una copia candidata: mismos tokens y recuperación, sin
 * archivo auxiliar ni AST. Nunca deja cadenas de identificadores sin liberar. */
size_t lexer_current_start(void) { return lex_start; }
size_t lexer_current_end(void) { return lex_offset; }
int lexer_scan_text(const char *text, size_t length) {
    yylex_destroy();
    column = yylineno = 1;
    lex_offset = lex_start = comment_start = 0;
    lexical_errors = lexical_fatal = 0;
    BEGIN(INITIAL);
    YY_BUFFER_STATE buffer = yy_scan_bytes(text, (int) length);
    int token;
    while ((token = yylex()) != 0) {
        if (token == TOKEN_IDENTIFIER) { free(yylval.str); yylval.str = NULL; }
    }
    int errors = lexical_errors;
    yy_delete_buffer(buffer);
    yylex_destroy();
    return errors ? 1 : 0;
}
