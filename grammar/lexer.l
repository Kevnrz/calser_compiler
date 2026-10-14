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

// Columna del siguiente carácter. Una tabulación cuenta como UNA columna:
// el señalador de los errores queda alineado porque copia las tabulaciones
// de la línea original.
static int column = 1;

// P-03 / §8.2: cada token guarda línea y columnas (Bison %locations).
#define YY_USER_ACTION                                  \
    yylloc.first_line   = yylloc.last_line = yylineno;  \
    yylloc.first_column = column;                       \
    yylloc.last_column  = column + yyleng - 1;          \
    column += yyleng;

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
                         comment_line = yylineno;
                         comment_column = yylloc.first_column;

                         BEGIN(COMMENT);
                       }

<COMMENT>"]#"         { BEGIN(INITIAL); }

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

                         lexical_fatal = 1;

                         return 0;
                       }


 /* L-03: un # seguido de [ nunca es comentario de línea. Ante #[ esta regla
    solo alcanza el # (largo 1) y gana "#[" (largo 2) por prefijo más largo
    (libro §3.5.3). Un # solo al final de la línea sigue siendo comentario. */
"#"([^\[\n][^\n]*)?   ;

 /* =========================================================================
  * PALABRAS RESERVADAS
  * ====================================================================== */

"F"                   return TOKEN_F;
"Vm"                  return TOKEN_VM;
"Vi"                  return TOKEN_VI;

"int"                 return TOKEN_INT;
"bool"                return TOKEN_BOOL;
"void"                return TOKEN_VOID;

 /* CRYPTO: palabras reservadas. Van antes de ID: con igual largo gana el
    patrón que está primero; cifrar_total sigue siendo ID por prefijo más
    largo (libro §3.5.3). */
"autenticar"          return TOKEN_AUTENTICAR;
"cerrar_sesion"       return TOKEN_CERRAR_SESION;
"cargar_llave"        return TOKEN_CARGAR_LLAVE;
"sellar"              return TOKEN_SELLAR;
"cambiar_ancla"       return TOKEN_CAMBIAR_ANCLA;
"cifrar"              return TOKEN_CIFRAR;
"descifrar"           return TOKEN_DESCIFRAR;

"C"                   return TOKEN_C;
"A"                   return TOKEN_A;
"L"                   return TOKEN_L;
"S"                   return TOKEN_S;
"E"                   return TOKEN_E;
"R"                   return TOKEN_R;

"verdadero"           return TOKEN_TRUE;
"falso"               return TOKEN_FALSE;

 /* =========================================================================
  * OPERADORES Y DELIMITADORES
  * ====================================================================== */

"=="                  return TOKEN_EQUAL_EQUAL;
"!="                  return TOKEN_NOT_EQUAL;
"<="                  return TOKEN_LESS_EQUAL;
">="                  return TOKEN_GREATER_EQUAL;

"|"                   return TOKEN_OR;
"&"                   return TOKEN_AND;

"<"                   return TOKEN_LESS;
">"                   return TOKEN_GREATER;

"+"                   return TOKEN_PLUS;
"-"                   return TOKEN_MINUS;

"*"                   return TOKEN_MULTIPLY;
"/"                   return TOKEN_DIVIDE;
"%"                   return TOKEN_MODULO;

"!"                   return TOKEN_NOT;
"^"                   return TOKEN_POWER;

"="                   return TOKEN_ASSIGN;

"["                   return TOKEN_LBRACKET;
"]"                   return TOKEN_RBRACKET;

"("                   return TOKEN_LPAREN;
")"                   return TOKEN_RPAREN;

":"                   return TOKEN_COLON;
";"                   return TOKEN_SEMICOLON;
","                   return TOKEN_COMMA;
"."                   return TOKEN_DOT;

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

                         return TOKEN_NUMBER;
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

                         return TOKEN_IDENTIFIER;
                       }

 /* =========================================================================
  * ERRORES LÉXICOS
  * ====================================================================== */

.                     {
                         /* Un byte que no es ASCII imprimible (0x20-0x7E)
                            se muestra como \xHH. Cada byte es un error. */
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