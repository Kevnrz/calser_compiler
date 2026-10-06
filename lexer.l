%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.tab.h"

int line_number = 1;

void yyerror(const char *msg);
%}

%option noyywrap
%option noinput
%option nounput
%option yylineno

%x COMMENT


%%



[ \t\r]+              ; /* Whitespace */

\n                    { line_number++; }

"#["                  { BEGIN(COMMENT); } /* Multiline comment: #[ ... ]# */

<COMMENT>"]#"         { BEGIN(INITIAL); }

<COMMENT>\n           { line_number++; }

<COMMENT>.            ;

<COMMENT><<EOF>>      {
                         yyerror("unterminated multiline comment");
                         return 0;
                       }


"#"[^\n]*             ; /* Single-line comment */

"F"                   return TOKEN_F; /* Keywords */
"Vm"                  return TOKEN_VM;
"Vi"                  return TOKEN_VI;

"int"                 return TOKEN_INT;
"bool"                return TOKEN_BOOL;
"void"                return TOKEN_VOID;

"C"                   return TOKEN_C;
"A"                   return TOKEN_A;
"L"                   return TOKEN_L;
"S"                   return TOKEN_S;
"E"                   return TOKEN_E;
"R"                   return TOKEN_R;

"verdadero"           return TOKEN_TRUE;
"falso"               return TOKEN_FALSE;

"=="                  return TOKEN_EQUAL_EQUAL; /* Multi-character operators */
"!="                  return TOKEN_NOT_EQUAL;
"<="                  return TOKEN_LESS_EQUAL;
">="                  return TOKEN_GREATER_EQUAL;

"|"                   return TOKEN_OR; /* Single-character operators */
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

"["                   return TOKEN_LBRACKET; /* Delimiters */
"]"                   return TOKEN_RBRACKET;

"("                   return TOKEN_LPAREN;
")"                   return TOKEN_RPAREN;

":"                   return TOKEN_COLON;
";"                   return TOKEN_SEMICOLON;
","                   return TOKEN_COMMA;
"."                   return TOKEN_DOT;

[0-9]+                {
                         yylval.num = strtol(yytext, NULL, 10);
                         return TOKEN_NUMBER;
                       } /* Number */

[a-z_][a-zA-Z0-9_]* {
                         yylval.str = strdup(yytext);

                         if (!yylval.str)
                         {
                             yyerror("out of memory");
                             exit(EXIT_FAILURE);
                         }

                         return TOKEN_IDENTIFIER;
                       } /* Identifier */

.                     {
                         fprintf(stderr,
                                 "Lexical error at line %d: "
                                 "unexpected character '%s'\n",
                                 yylineno,
                                 yytext);

                         return TOKEN_ERROR;
                       } /* Invalid character */


%%