# Gramática de Calser

Fuente: `grammar/parser.y` y `grammar/lexer.l` integrados en C6, provenientes de CALSIDE sobre develop. Los motores del IDE permanecen dentro de ccom en C. Las 101 producciones del lenguaje válido se conservan respecto de develop; recuperación y consultas se describen por separado.

Se ejecutó `bison -v --defines=/tmp/c6-implementacion/parser.h -o /tmp/c6-implementacion/parser.c grammar/parser.y` con GNU Bison 3.8.2: código 0, sin diagnósticos ni conflictos. Informe: `/tmp/c6-implementacion/parser.output`.

## Notación y producciones

Se sigue la notación solicitada de Aho §4.2.1–4.2.2: cabeza → cuerpo, alternativas de una misma cabeza separadas por | y cuerpo vacío ε. Los no terminales conservan exactamente sus nombres de Bison y aparecen en cursiva; las palabras terminales y las categorías **id** y **num**, en negrita. Los operadores y delimitadores conservan sus lexemas. Solo el terminal `|` se presenta en código para distinguirlo del separador de alternativas; los escapes de Markdown de otros operadores no forman parte de la gramática. **id** corresponde a `TOKEN_IDENTIFIER` (alias Bison `"identificador"`) y **num** a `TOKEN_NUMBER` (alias `"numero"`).

Las reglas 1–101 de esta exposición identifican el lenguaje válido y conservan la numeración didáctica usada por `gramatica_ll1.md`; no son los números actuales de Bison. La tabla de correspondencia muestra ambos números y las líneas actuales. Se omiten aquí las tres recuperaciones y la acción intermedia, que se detallan más abajo. Se omiten comentarios y acciones semánticas `{ ... }`. No se elimina recursión, no se factoriza, no se renombra ningún no terminal y se conserva el orden de las alternativas. Las referencias de cada grupo abarcan desde su cabeza hasta el `;` de cierre en `parser.y`; la tabla de correspondencia precisa las líneas de cada cuerpo, excluyendo las acciones. El símbolo inicial es *programa* (`%start programa`, línea 224).

*programa* → *globales* *funciones*

Referencia: `grammar/parser.y:236–242`; reglas del lenguaje válido 1.

*globales* → ε | *globales* *decl*

Referencia: `grammar/parser.y:251–269`; reglas del lenguaje válido 2, 3.

*funciones* → *funcion* | *funciones* *funcion*

Referencia: `grammar/parser.y:278–289`; reglas del lenguaje válido 4, 5.

*funcion* → **F** **id** [ *params* ] : *retorno* ; *cuerpo* .

Referencia: `grammar/parser.y:298–330`; reglas del lenguaje válido 6.

*retorno* → *tipo* **id** | **void**

Referencia: `grammar/parser.y:339–358`; reglas del lenguaje válido 7, 8.

*params* → ε | *lista_param*

Referencia: `grammar/parser.y:367–377`; reglas del lenguaje válido 9, 10.

*lista_param* → *param* | *lista_param* , *param*

Referencia: `grammar/parser.y:381–392`; reglas del lenguaje válido 11, 12.

*param* → *modif_opt* *tipo* **id** *dims_opt*

Referencia: `grammar/parser.y:396–410`; reglas del lenguaje válido 13.

*modif* → **Vm** | **Vi**

Referencia: `grammar/parser.y:414–428`; reglas del lenguaje válido 14, 15.

*modif_opt* → *modif* | ε

Referencia: `grammar/parser.y:432–442`; reglas del lenguaje válido 16, 17.

*tipo* → **int** | **bool**

Referencia: `grammar/parser.y:446–460`; reglas del lenguaje válido 18, 19.

*dims_opt* → *dims* | ε

Referencia: `grammar/parser.y:464–474`; reglas del lenguaje válido 20, 21.

*dims* → [ **num** ] | [ **num** ] [ **num** ]

Referencia: `grammar/parser.y:478–507`; reglas del lenguaje válido 22, 23.

*cuerpo* → ε | *cuerpo* *sentencia*

Referencia: `grammar/parser.y:516–537`; reglas del lenguaje válido 24, 25.

*sentencia* → *decl* | *sent_id* | *condicional* | *ciclo_l* | *ciclo_s* | *emit* | *rewind* | *crypto*

Referencia: `grammar/parser.y:546–586`; reglas del lenguaje válido 26, 27, 28, 29, 30, 31, 32, 33.

*decl* → *modif* **id** : *tipo* *dims_opt* ; | *modif* **id** : *tipo* *dims_opt* = *inicial* ;

Referencia: `grammar/parser.y:595–637`; reglas del lenguaje válido 34, 35.

*inicial* → *expr* | *lista_init*

Referencia: `grammar/parser.y:646–656`; reglas del lenguaje válido 36, 37.

*lista_init* → [ *elems* ]

Referencia: `grammar/parser.y:660–665`; reglas del lenguaje válido 38.

*elems* → *elem* | *elems* , *elem*

Referencia: `grammar/parser.y:669–682`; reglas del lenguaje válido 39, 40.

*elem* → *expr* | *lista_init*

Referencia: `grammar/parser.y:686–696`; reglas del lenguaje válido 41, 42.

*sent_id* → **id** = *expr* ; | **id** [ *args* ] = *expr* ; | **id** [ *args* ] [ *expr* ] = *expr* ; | **id** [ *args* ] ;

Referencia: `grammar/parser.y:705–781`; reglas del lenguaje válido 43, 44, 45, 46.

*condicional* → **C** [ *expr* ] ; *cuerpo* *alts* *residual* .

Referencia: `grammar/parser.y:790–810`; reglas del lenguaje válido 47.

*alts* → ε | *alts* **A** [ *expr* ] ; *cuerpo*

Referencia: `grammar/parser.y:814–841`; reglas del lenguaje válido 48, 49.

*residual* → ε | **A** ; *cuerpo*

Referencia: `grammar/parser.y:845–859`; reglas del lenguaje válido 50, 51.

*ciclo_l* → **L** [ *expr* ] ; *cuerpo* .

Referencia: `grammar/parser.y:868–884`; reglas del lenguaje válido 52.

*ciclo_s* → **S** [ **id** , *expr* , *expr* , *paso* ] ; *cuerpo* .

Referencia: `grammar/parser.y:893–923`; reglas del lenguaje válido 53.

*paso* → **num** | - **num**

Referencia: `grammar/parser.y:927–941`; reglas del lenguaje válido 54, 55.

*emit* → **E** ;

Referencia: `grammar/parser.y:950–957`; reglas del lenguaje válido 56.

*rewind* → **R** **id** [ *args* ] ;

Referencia: `grammar/parser.y:966–986`; reglas del lenguaje válido 57.

*crypto* → **autenticar** [ *expr* ] ; | **cerrar_sesion** [ ] ; | **cargar_llave** [ **num** , **num** , *expr* ] ; | **sellar** [ ] ; | **cambiar_ancla** [ *expr* ] ; | **cifrar** [ *expr* , *expr* , *expr* , **num** ] ; | **descifrar** [ *expr* , *expr* , *expr* , **num** ] ;

Referencia: `grammar/parser.y:1002–1124`; reglas del lenguaje válido 58, 59, 60, 61, 62, 63, 64.

*args* → ε | *lista_arg*

Referencia: `grammar/parser.y:1133–1143`; reglas del lenguaje válido 65, 66.

*lista_arg* → *expr* | *lista_arg* , *expr*

Referencia: `grammar/parser.y:1147–1158`; reglas del lenguaje válido 67, 68.

*expr* → *disy*

Referencia: `grammar/parser.y:1167–1172`; reglas del lenguaje válido 69.

*disy* → *conj* | *disy* `|` *conj*

Referencia: `grammar/parser.y:1176–1193`; reglas del lenguaje válido 70, 71.

*conj* → *igualdad* | *conj* & *igualdad*

Referencia: `grammar/parser.y:1197–1214`; reglas del lenguaje válido 72, 73.

*igualdad* → *relacion* | *igualdad* == *relacion* | *igualdad* != *relacion*

Referencia: `grammar/parser.y:1218–1247`; reglas del lenguaje válido 74, 75, 76.

*relacion* → *aditivo* | *relacion* < *aditivo* | *relacion* > *aditivo* | *relacion* <= *aditivo* | *relacion* >= *aditivo*

Referencia: `grammar/parser.y:1256–1309`; reglas del lenguaje válido 77, 78, 79, 80, 81.

*aditivo* → *multiplicativo* | *aditivo* + *multiplicativo* | *aditivo* - *multiplicativo*

Referencia: `grammar/parser.y:1313–1342`; reglas del lenguaje válido 82, 83, 84.

*multiplicativo* → *unario* | *multiplicativo* \* *unario* | *multiplicativo* / *unario* | *multiplicativo* % *unario*

Referencia: `grammar/parser.y:1350–1391`; reglas del lenguaje válido 85, 86, 87, 88.

*unario* → - *unario* | ! *unario* | *potencia*

Referencia: `grammar/parser.y:1395–1420`; reglas del lenguaje válido 89, 90, 91.

*potencia* → *primario* | *primario* ^ *unario*

Referencia: `grammar/parser.y:1424–1441`; reglas del lenguaje válido 92, 93.

*primario* → ( *expr* ) | *acceso* | **num** | **verdadero** | **falso**

Referencia: `grammar/parser.y:1445–1476`; reglas del lenguaje válido 94, 95, 96, 97, 98.

*acceso* → **id** | **id** [ *args* ] | **id** [ *args* ] [ *expr* ]

Referencia: `grammar/parser.y:1480–1522`; reglas del lenguaje válido 99, 100, 101.

## Tabla de terminales

Son **47 terminales del lenguaje**, todos usados en las producciones y devueltos por el lexer. Existe además un terminal privado `TOKEN_CURSOR` (`parser.y:65`), sin producción y emitido únicamente por la envoltura del parser al consultar un prefijo; no es un lexema Calser. Los patrones se transcriben de `lexer.l`; las comillas de Flex delimitan lexemas literales. Se incluyen las líneas de declaración en Bison para facilitar la trazabilidad.

| Token de Bison | En el documento | Patrón o lexema de Flex | Línea de lexer.l | Línea de parser.y |
| --- | --- | --- | ---: | ---: |
| `TOKEN_F` | **F** | `"F"` | 166 | 66 |
| `TOKEN_VM` | **Vm** | `"Vm"` | 167 | 67 |
| `TOKEN_VI` | **Vi** | `"Vi"` | 168 | 68 |
| `TOKEN_INT` | **int** | `"int"` | 170 | 70 |
| `TOKEN_BOOL` | **bool** | `"bool"` | 171 | 71 |
| `TOKEN_VOID` | **void** | `"void"` | 172 | 72 |
| `TOKEN_C` | **C** | `"C"` | 185 | 74 |
| `TOKEN_A` | **A** | `"A"` | 186 | 75 |
| `TOKEN_L` | **L** | `"L"` | 187 | 76 |
| `TOKEN_S` | **S** | `"S"` | 188 | 77 |
| `TOKEN_E` | **E** | `"E"` | 189 | 78 |
| `TOKEN_R` | **R** | `"R"` | 190 | 79 |
| `TOKEN_TRUE` | **verdadero** | `"verdadero"` | 192 | 81 |
| `TOKEN_FALSE` | **falso** | `"falso"` | 193 | 82 |
| `TOKEN_AUTENTICAR` | **autenticar** | `"autenticar"` | 177 | 84 |
| `TOKEN_CERRAR_SESION` | **cerrar_sesion** | `"cerrar_sesion"` | 178 | 85 |
| `TOKEN_CARGAR_LLAVE` | **cargar_llave** | `"cargar_llave"` | 179 | 86 |
| `TOKEN_SELLAR` | **sellar** | `"sellar"` | 180 | 87 |
| `TOKEN_CAMBIAR_ANCLA` | **cambiar_ancla** | `"cambiar_ancla"` | 181 | 88 |
| `TOKEN_CIFRAR` | **cifrar** | `"cifrar"` | 182 | 89 |
| `TOKEN_DESCIFRAR` | **descifrar** | `"descifrar"` | 183 | 90 |
| `TOKEN_OR` | `\|` | `"\|"` | 204 | 95 |
| `TOKEN_AND` | & | `"&"` | 205 | 96 |
| `TOKEN_EQUAL_EQUAL` | == | `"=="` | 199 | 98 |
| `TOKEN_NOT_EQUAL` | != | `"!="` | 200 | 99 |
| `TOKEN_LESS` | < | `"<"` | 207 | 101 |
| `TOKEN_GREATER` | > | `">"` | 208 | 102 |
| `TOKEN_LESS_EQUAL` | <= | `"<="` | 201 | 103 |
| `TOKEN_GREATER_EQUAL` | >= | `">="` | 202 | 104 |
| `TOKEN_PLUS` | + | `"+"` | 210 | 106 |
| `TOKEN_MINUS` | - | `"-"` | 211 | 107 |
| `TOKEN_MULTIPLY` | \* | `"*"` | 213 | 109 |
| `TOKEN_DIVIDE` | / | `"/"` | 214 | 110 |
| `TOKEN_MODULO` | % | `"%"` | 215 | 111 |
| `TOKEN_NOT` | ! | `"!"` | 217 | 113 |
| `TOKEN_POWER` | ^ | `"^"` | 218 | 114 |
| `TOKEN_ASSIGN` | = | `"="` | 220 | 116 |
| `TOKEN_LBRACKET` | [ | `"["` | 222 | 121 |
| `TOKEN_RBRACKET` | ] | `"]"` | 223 | 122 |
| `TOKEN_LPAREN` | ( | `"("` | 225 | 124 |
| `TOKEN_RPAREN` | ) | `")"` | 226 | 125 |
| `TOKEN_COLON` | : | `":"` | 228 | 127 |
| `TOKEN_SEMICOLON` | ; | `";"` | 229 | 128 |
| `TOKEN_COMMA` | , | `","` | 230 | 129 |
| `TOKEN_DOT` | . | `"."` | 231 | 130 |
| `TOKEN_IDENTIFIER` | **id** | `[a-z_][a-zA-Z0-9_]*` | 268 | 136 |
| `TOKEN_NUMBER` | **num** | `[0-9]+` | 237 | 137 |

Las acciones de los patrones numéricos y de identificador retornan `TOKEN_NUMBER` y `TOKEN_IDENTIFIER` en las líneas 265 y 294, respectivamente. Los límites y recuperaciones léxicas de sus acciones no cambian los patrones transcritos. Espacios y comentarios se omiten y no son terminales de esta gramática.

Los símbolos internos de Bison se contabilizan por separado: además de los 48 terminales declarados (47 del lenguaje y el cursor privado), el C generado incluye `$end`, `error` y `$undefined`: `YYNTOKENS = 51`. `error` participa únicamente en recuperación, y `$end` en la producción aumentada. Ninguno de esos tres es un lexema del lenguaje.


## Precedencia y asociatividad

No hay declaraciones `%left`, `%right` ni usos de `%prec`; tampoco `%nonassoc` ni `%precedence`. La estructura de las producciones determina la precedencia y la agrupación. *expr* → *disy* es la regla 69 (`parser.y:1168`).

La tabla presenta los niveles en el orden solicitado: *disy* → *conj* → *igualdad* → *relacion* → *aditivo* → *multiplicativo* → *potencia* → *unario* → *primario*. **Ese orden no es una cadena literal de las reglas en los dos niveles potencia/unario**: la cadena de alternativas base real es *multiplicativo* → *unario* → *potencia* → *primario* (85, 91, 92), y *potencia* también vuelve a *unario* en su operando derecho (93). Se conserva esa estructura sin modificarla.

| Nivel | Operadores / forma | Reglas Bison y líneas de parser.y | Asociatividad y fundamento |
| --- | --- | --- | --- |
| *disy* | `\|` | 70: 1177; 71: 1182 | Izquierda: *disy* → *disy* `\|` *conj* (71). |
| *conj* | & | 72: 1198; 73: 1203 | Izquierda: *conj* → *conj* & *igualdad* (73). |
| *igualdad* | ==, != | 74: 1219; 75: 1224; 76: 1236 | Izquierda por recursión izquierda en 75–76. |
| *relacion* | <, >, <=, >= | 77: 1257; 78: 1262; 79: 1274; 80: 1286; 81: 1298 | Izquierda por recursión izquierda en 78–81; la sintaxis permite encadenarlas. |
| *aditivo* | +, - | 82: 1314; 83: 1319; 84: 1331 | Izquierda por recursión izquierda en 83–84. |
| *multiplicativo* | \*, /, % | 85: 1351; 86: 1356; 87: 1368; 88: 1380 | Izquierda por recursión izquierda en 86–88; sus operandos son *unario*. |
| *potencia* | ^ | 92: 1425; 93: 1430 | Derecha por recursión derecha indirecta: *potencia* → *primario* ^ *unario* → *primario* ^ *potencia* (93, 91). |
| *unario* | -, ! (prefijos) | 89: 1396; 90: 1406; 91: 1416 | Agrupación a derecha por recursión derecha en 89–90; 91 delega en *potencia*. No es asociatividad de operadores binarios. |
| *primario* | ( *expr* ), *acceso*, **num**, **verdadero**, **falso** | 94: 1446; 95: 1451; 96: 1456; 97: 1463; 98: 1470 | Sin asociatividad binaria propia; los paréntesis fijan la agrupación. |

Consecuencias de las reglas 89–93: `-2^2` se agrupa como `-(2^2)`; `2^3^4`, como `2^(3^4)`; `2^-3` se admite como `2^(-3)`. Por eso no corresponde afirmar que el orden solicitado potencia → unario significa que todos los operadores unarios tengan mayor precedencia que ^. *acceso* (99–101) aporta las formas con identificador a *primario*, sin introducir un operador binario adicional.

## Recuperación y consultas de prefijos

Las tres producciones de recuperación agregadas son:

| Regla Bison | Producción | Línea de parser.y |
| ---: | --- | ---: |
| 4 | globales → globales error ; | 262 |
| 9 | funcion → F error . | 323 |
| 29 | cuerpo → cuerpo error | 527 |

`error` es el símbolo especial de Bison, no una palabra aceptada por Flex.
`syntax_recover` (`parser.y:1714`) sincroniza conservadoramente con `;`, `.`,
ramas `A` y límites de función; limita los errores a 32 y los descartes por
sincronización a 4096 tokens. La CLI comprueba `parser_syntax_errors()` además
del retorno de `yyparse()`: un árbol recuperado no se exporta ni pasa a semántica
o generación. Las cascadas adicionales en e71, e72 y e74 son un problema conocido
aceptado para C6; no se corrigen en esta integración.

La acción intermedia después del nombre de función (`parser.y:301`) genera
`$@1 → ε` (regla Bison 7). Informa a sugerencias si la función es `main`;
la regla 8 de función conserva los hijos del AST con índices `$2/$5/$8/$10`.

`TOKEN_CURSOR` permite obtener los esperados LAC aun cuando el prefijo sea un
programa completo. Ninguna producción lo acepta. `--suggest-json` termina al
consultar ese contexto; `--syntax-json` y reparaciones usan EOF real y el primer
error, con recuperación desactivada. `%define parse.error custom` (205) y
`%define parse.lac full` (206) siguen activos. El LL(1) Python conserva el lenguaje
válido como API académica; no reproduce la recuperación ni sustituye los motores C.

## Verificación contra Bison

Las 101 producciones del lenguaje válido conservan cabezas, terminales y
construcción del AST. El fuente agrega tres alternativas de recuperación y
una acción intermedia; la tabla siguiente distingue los conteos internos.

| Conteo | Valor actual |
| --- | ---: |
| Producciones del lenguaje válido, sin acción intermedia | 101 |
| Alternativas escritas, incluyendo recuperación | 104 |
| Reglas con acción intermedia, sin `$accept` | 105 |
| Reglas incluyendo `$accept` (`YYNRULES`) | 106 |
| No terminales del lenguaje | 43 |
| No terminales con `$@1` y `$accept` (`YYNNTS`) | 45 |
| Terminales del lenguaje | 47 |
| Terminales declarados, incluyendo cursor | 48 |
| Terminales con internos (`YYNTOKENS`) | 51 |
| Estados (`YYNSTATES`) | 248 |
| Conflictos shift/reduce | 0 |
| Conflictos reduce/reduce | 0 |

### Correspondencia completa de reglas

Cada fila corresponde a una alternativa del grupo con la misma cabeza en la primera sección. Las reglas Bison 4, 7, 9 y 29 son las recuperaciones y la acción intermedia descritas arriba. Las líneas señalan el separador `:` o `|` y el último símbolo del cuerpo; para ε señalan la línea de la alternativa vacía.

| Regla del lenguaje válido | Regla Bison actual | Producción en este documento | Líneas de parser.y |
| ---: | ---: | --- | --- |
| — | 0 | `$accept → programa $end` (aumentada; fuera de los conteos propios) | Generada por Bison; `%start` en 224 |
| 1 | 1 | *programa* → *globales* *funciones* | 237 |
| 2 | 2 | *globales* → ε | 252 |
| 3 | 3 | *globales* → *globales* *decl* | 257 |
| 4 | 5 | *funciones* → *funcion* | 279 |
| 5 | 6 | *funciones* → *funciones* *funcion* | 284 |
| 6 | 8 | *funcion* → **F** **id** [ *params* ] : *retorno* ; *cuerpo* . | 299–309 |
| 7 | 10 | *retorno* → *tipo* **id** | 340 |
| 8 | 11 | *retorno* → **void** | 352 |
| 9 | 12 | *params* → ε | 368 |
| 10 | 13 | *params* → *lista_param* | 373 |
| 11 | 14 | *lista_param* → *param* | 382 |
| 12 | 15 | *lista_param* → *lista_param* , *param* | 387 |
| 13 | 16 | *param* → *modif_opt* *tipo* **id** *dims_opt* | 397 |
| 14 | 17 | *modif* → **Vm** | 415 |
| 15 | 18 | *modif* → **Vi** | 422 |
| 16 | 19 | *modif_opt* → *modif* | 433 |
| 17 | 20 | *modif_opt* → ε | 438 |
| 18 | 21 | *tipo* → **int** | 447 |
| 19 | 22 | *tipo* → **bool** | 454 |
| 20 | 23 | *dims_opt* → *dims* | 465 |
| 21 | 24 | *dims_opt* → ε | 470 |
| 22 | 25 | *dims* → [ **num** ] | 479–481 |
| 23 | 26 | *dims* → [ **num** ] [ **num** ] | 493–498 |
| 24 | 27 | *cuerpo* → ε | 517 |
| 25 | 28 | *cuerpo* → *cuerpo* *sentencia* | 522 |
| 26 | 30 | *sentencia* → *decl* | 547 |
| 27 | 31 | *sentencia* → *sent_id* | 552 |
| 28 | 32 | *sentencia* → *condicional* | 557 |
| 29 | 33 | *sentencia* → *ciclo_l* | 562 |
| 30 | 34 | *sentencia* → *ciclo_s* | 567 |
| 31 | 35 | *sentencia* → *emit* | 572 |
| 32 | 36 | *sentencia* → *rewind* | 577 |
| 33 | 37 | *sentencia* → *crypto* | 582 |
| 34 | 38 | *decl* → *modif* **id** : *tipo* *dims_opt* ; | 596–601 |
| 35 | 39 | *decl* → *modif* **id** : *tipo* *dims_opt* = *inicial* ; | 616–623 |
| 36 | 40 | *inicial* → *expr* | 647 |
| 37 | 41 | *inicial* → *lista_init* | 652 |
| 38 | 42 | *lista_init* → [ *elems* ] | 661 |
| 39 | 43 | *elems* → *elem* | 670 |
| 40 | 44 | *elems* → *elems* , *elem* | 677 |
| 41 | 45 | *elem* → *expr* | 687 |
| 42 | 46 | *elem* → *lista_init* | 692 |
| 43 | 47 | *sent_id* → **id** = *expr* ; | 706–709 |
| 44 | 48 | *sent_id* → **id** [ *args* ] = *expr* ; | 723–729 |
| 45 | 49 | *sent_id* → **id** [ *args* ] [ *expr* ] = *expr* ; | 743–752 |
| 46 | 50 | *sent_id* → **id** [ *args* ] ; | 766–770 |
| 47 | 51 | *condicional* → **C** [ *expr* ] ; *cuerpo* *alts* *residual* . | 791–799 |
| 48 | 52 | *alts* → ε | 815 |
| 49 | 53 | *alts* → *alts* **A** [ *expr* ] ; *cuerpo* | 820–826 |
| 50 | 54 | *residual* → ε | 846 |
| 51 | 55 | *residual* → **A** ; *cuerpo* | 851–853 |
| 52 | 56 | *ciclo_l* → **L** [ *expr* ] ; *cuerpo* . | 869–875 |
| 53 | 57 | *ciclo_s* → **S** [ **id** , *expr* , *expr* , *paso* ] ; *cuerpo* . | 894–906 |
| 54 | 58 | *paso* → **num** | 928 |
| 55 | 59 | *paso* → - **num** | 935 |
| 56 | 60 | *emit* → **E** ; | 951 |
| 57 | 61 | *rewind* → **R** **id** [ *args* ] ; | 967–972 |
| 58 | 62 | *crypto* → **autenticar** [ *expr* ] ; | 1003–1007 |
| 59 | 63 | *crypto* → **cerrar_sesion** [ ] ; | 1014–1017 |
| 60 | 64 | *crypto* → **cargar_llave** [ **num** , **num** , *expr* ] ; | 1024–1032 |
| 61 | 65 | *crypto* → **sellar** [ ] ; | 1050–1053 |
| 62 | 66 | *crypto* → **cambiar_ancla** [ *expr* ] ; | 1060–1064 |
| 63 | 67 | *crypto* → **cifrar** [ *expr* , *expr* , *expr* , **num** ] ; | 1071–1081 |
| 64 | 68 | *crypto* → **descifrar** [ *expr* , *expr* , *expr* , **num** ] ; | 1098–1108 |
| 65 | 69 | *args* → ε | 1134 |
| 66 | 70 | *args* → *lista_arg* | 1139 |
| 67 | 71 | *lista_arg* → *expr* | 1148 |
| 68 | 72 | *lista_arg* → *lista_arg* , *expr* | 1153 |
| 69 | 73 | *expr* → *disy* | 1168 |
| 70 | 74 | *disy* → *conj* | 1177 |
| 71 | 75 | *disy* → *disy* `\|` *conj* | 1182 |
| 72 | 76 | *conj* → *igualdad* | 1198 |
| 73 | 77 | *conj* → *conj* & *igualdad* | 1203 |
| 74 | 78 | *igualdad* → *relacion* | 1219 |
| 75 | 79 | *igualdad* → *igualdad* == *relacion* | 1224 |
| 76 | 80 | *igualdad* → *igualdad* != *relacion* | 1236 |
| 77 | 81 | *relacion* → *aditivo* | 1257 |
| 78 | 82 | *relacion* → *relacion* < *aditivo* | 1262 |
| 79 | 83 | *relacion* → *relacion* > *aditivo* | 1274 |
| 80 | 84 | *relacion* → *relacion* <= *aditivo* | 1286 |
| 81 | 85 | *relacion* → *relacion* >= *aditivo* | 1298 |
| 82 | 86 | *aditivo* → *multiplicativo* | 1314 |
| 83 | 87 | *aditivo* → *aditivo* + *multiplicativo* | 1319 |
| 84 | 88 | *aditivo* → *aditivo* - *multiplicativo* | 1331 |
| 85 | 89 | *multiplicativo* → *unario* | 1351 |
| 86 | 90 | *multiplicativo* → *multiplicativo* \* *unario* | 1356 |
| 87 | 91 | *multiplicativo* → *multiplicativo* / *unario* | 1368 |
| 88 | 92 | *multiplicativo* → *multiplicativo* % *unario* | 1380 |
| 89 | 93 | *unario* → - *unario* | 1396 |
| 90 | 94 | *unario* → ! *unario* | 1406 |
| 91 | 95 | *unario* → *potencia* | 1416 |
| 92 | 96 | *potencia* → *primario* | 1425 |
| 93 | 97 | *potencia* → *primario* ^ *unario* | 1430 |
| 94 | 98 | *primario* → ( *expr* ) | 1446 |
| 95 | 99 | *primario* → *acceso* | 1451 |
| 96 | 100 | *primario* → **num** | 1456 |
| 97 | 101 | *primario* → **verdadero** | 1463 |
| 98 | 102 | *primario* → **falso** | 1470 |
| 99 | 103 | *acceso* → **id** | 1481 |
| 100 | 104 | *acceso* → **id** [ *args* ] | 1489–1492 |
| 101 | 105 | *acceso* → **id** [ *args* ] [ *expr* ] | 1504–1510 |

## Anexo: marcas sobre las producciones originales

### (a) Recursión por la izquierda

Se detectan **20 alternativas con recursión izquierda directa, en 13 no terminales**. Se listan sin transformarlas. No hay recursión izquierda indirecta a través de otros no terminales: se verificó el grafo de esquinas izquierdas, considerando todos los prefijos anulables. Los no terminales anulables son *alts*, *args*, *cuerpo*, *dims_opt*, *globales*, *modif_opt*, *params*, *residual*.

| Regla Bison | Producción con recursión izquierda directa | Líneas de parser.y |
| ---: | --- | --- |
| 3 | 3 | *globales* → *globales* *decl* | 257 |
| 5 | 6 | *funciones* → *funciones* *funcion* | 284 |
| 12 | 15 | *lista_param* → *lista_param* , *param* | 387 |
| 25 | 28 | *cuerpo* → *cuerpo* *sentencia* | 522 |
| 40 | 44 | *elems* → *elems* , *elem* | 677 |
| 49 | 53 | *alts* → *alts* **A** [ *expr* ] ; *cuerpo* | 820–826 |
| 68 | 72 | *lista_arg* → *lista_arg* , *expr* | 1153 |
| 71 | 75 | *disy* → *disy* `\|` *conj* | 1182 |
| 73 | 77 | *conj* → *conj* & *igualdad* | 1203 |
| 75 | 79 | *igualdad* → *igualdad* == *relacion* | 1224 |
| 76 | 80 | *igualdad* → *igualdad* != *relacion* | 1236 |
| 78 | 82 | *relacion* → *relacion* < *aditivo* | 1262 |
| 79 | 83 | *relacion* → *relacion* > *aditivo* | 1274 |
| 80 | 84 | *relacion* → *relacion* <= *aditivo* | 1286 |
| 81 | 85 | *relacion* → *relacion* >= *aditivo* | 1298 |
| 83 | 87 | *aditivo* → *aditivo* + *multiplicativo* | 1319 |
| 84 | 88 | *aditivo* → *aditivo* - *multiplicativo* | 1331 |
| 86 | 90 | *multiplicativo* → *multiplicativo* \* *unario* | 1356 |
| 87 | 91 | *multiplicativo* → *multiplicativo* / *unario* | 1368 |
| 88 | 92 | *multiplicativo* → *multiplicativo* % *unario* | 1380 |

La recursión derecha indirecta entre *potencia* y *unario* no es recursión izquierda: en la regla 93 aparece ^ antes de *unario*; las reglas 89–90 también consumen un terminal antes de recurrir.

### (b) Alternativas con prefijo común

Se comparan prefijos no vacíos de las secuencias originales de símbolos de alternativas con la misma cabeza, sin expandir no terminales. Se informa el prefijo máximo para cada conjunto de alternativas que lo comparte; los casos anidados de *sent_id* y *acceso* se muestran por separado. Un cuerpo completo puede ser prefijo de otro. ε no se cuenta como prefijo común. Esto marca posibilidades de factorización, sin aplicarla, y no implica conflictos LALR.

| Cabeza | Reglas Bison | Prefijo común | Líneas de parser.y por regla |
| --- | --- | --- | --- |
| *dims* | 22, 23 | [ **num** ] | 22: 479–481; 23: 493–498 |
| *decl* | 34, 35 | *modif* **id** : *tipo* *dims_opt* | 34: 596–601; 35: 616–623 |
| *sent_id* | 43, 44, 45, 46 | **id** | 43: 706–709; 44: 723–729; 45: 743–752; 46: 766–770 |
| *sent_id* | 44, 45, 46 | **id** [ *args* ] | 44: 723–729; 45: 743–752; 46: 766–770 |
| *igualdad* | 75, 76 | *igualdad* | 75: 1224; 76: 1236 |
| *relacion* | 78, 79, 80, 81 | *relacion* | 78: 1262; 79: 1274; 80: 1286; 81: 1298 |
| *aditivo* | 83, 84 | *aditivo* | 83: 1319; 84: 1331 |
| *multiplicativo* | 86, 87, 88 | *multiplicativo* | 86: 1356; 87: 1368; 88: 1380 |
| *potencia* | 92, 93 | *primario* | 92: 1425; 93: 1430 |
| *acceso* | 99, 100, 101 | **id** | 99: 1481; 100: 1489–1492; 101: 1504–1510 |
| *acceso* | 100, 101 | **id** [ *args* ] | 100: 1489–1492; 101: 1504–1510 |

Son **11 conjuntos de alternativas con prefijo común, en 9 no terminales**. Las demás cabezas no presentan prefijos simbólicos no vacíos compartidos entre sus alternativas originales.

## Identificación de los archivos verificados

Huellas SHA-256 del contenido actual y del informe de verificación:

| Archivo | SHA-256 |
| --- | --- |
| `grammar/parser.y` | `b178c639e722bbbeb6f5b0e15001c8d923efb11e22c51cb61affa7afc5b5f73d` |
| `grammar/lexer.l` | `c92e415b91ffda56765925bf8e090e2ea1e7646c46de994b0ef89039a4a077e9` |
| `/tmp/c6-implementacion/parser.output` | `58e6f4748519c201adb81e74cd17cec7eed82bdead39f2a9908107eae44d9264` |
