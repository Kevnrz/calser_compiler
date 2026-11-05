# Gramática de Calser

Fuente: archivos locales `grammar/parser.y` y `grammar/lexer.l` del repositorio indicado `Kevnrz/calser_compiler`. Corresponde a `develop` en el commit `7e9e811`: las huellas SHA-256 del final coinciden con ese commit, y `grammar/` no cambió hasta `626893f`.

Se ejecutó `bison -v -o /tmp/calser.c grammar/parser.y` con GNU Bison 3.8.2, con código de salida 0 y sin diagnósticos. Referencia de comprobación: `/tmp/calser.output`. No se modificaron archivos del repositorio.

## Notación y producciones

Se sigue la notación solicitada de Aho §4.2.1–4.2.2: cabeza → cuerpo, alternativas de una misma cabeza separadas por | y cuerpo vacío ε. Los no terminales conservan exactamente sus nombres de Bison y aparecen en cursiva; las palabras terminales y las categorías **id** y **num**, en negrita. Los operadores y delimitadores conservan sus lexemas. Solo el terminal `|` se presenta en código para distinguirlo del separador de alternativas; los escapes de Markdown de otros operadores no forman parte de la gramática. **id** corresponde a `TOKEN_IDENTIFIER` (alias Bison `"identificador"`) y **num** a `TOKEN_NUMBER` (alias `"numero"`).

Se omiten comentarios y acciones semánticas `{ ... }`. No se elimina recursión, no se factoriza, no se renombra ningún no terminal y se conserva el orden de las alternativas. Las referencias de cada grupo abarcan desde su cabeza hasta el `;` de cierre en `parser.y`; la tabla de correspondencia precisa las líneas de cada cuerpo, excluyendo las acciones. El símbolo inicial es *programa* (`%start programa`, línea 202).

*programa* → *globales* *funciones*

Referencia: `grammar/parser.y:214–220`; reglas Bison 1.

*globales* → ε | *globales* *decl*

Referencia: `grammar/parser.y:229–240`; reglas Bison 2, 3.

*funciones* → *funcion* | *funciones* *funcion*

Referencia: `grammar/parser.y:249–260`; reglas Bison 4, 5.

*funcion* → **F** **id** [ *params* ] : *retorno* ; *cuerpo* .

Referencia: `grammar/parser.y:269–292`; reglas Bison 6.

*retorno* → *tipo* **id** | **void**

Referencia: `grammar/parser.y:301–320`; reglas Bison 7, 8.

*params* → ε | *lista_param*

Referencia: `grammar/parser.y:329–339`; reglas Bison 9, 10.

*lista_param* → *param* | *lista_param* , *param*

Referencia: `grammar/parser.y:343–354`; reglas Bison 11, 12.

*param* → *modif_opt* *tipo* **id** *dims_opt*

Referencia: `grammar/parser.y:358–372`; reglas Bison 13.

*modif* → **Vm** | **Vi**

Referencia: `grammar/parser.y:376–390`; reglas Bison 14, 15.

*modif_opt* → *modif* | ε

Referencia: `grammar/parser.y:394–404`; reglas Bison 16, 17.

*tipo* → **int** | **bool**

Referencia: `grammar/parser.y:408–422`; reglas Bison 18, 19.

*dims_opt* → *dims* | ε

Referencia: `grammar/parser.y:426–436`; reglas Bison 20, 21.

*dims* → [ **num** ] | [ **num** ] [ **num** ]

Referencia: `grammar/parser.y:440–469`; reglas Bison 22, 23.

*cuerpo* → ε | *cuerpo* *sentencia*

Referencia: `grammar/parser.y:478–489`; reglas Bison 24, 25.

*sentencia* → *decl* | *sent_id* | *condicional* | *ciclo_l* | *ciclo_s* | *emit* | *rewind* | *crypto*

Referencia: `grammar/parser.y:498–538`; reglas Bison 26, 27, 28, 29, 30, 31, 32, 33.

*decl* → *modif* **id** : *tipo* *dims_opt* ; | *modif* **id** : *tipo* *dims_opt* = *inicial* ;

Referencia: `grammar/parser.y:547–589`; reglas Bison 34, 35.

*inicial* → *expr* | *lista_init*

Referencia: `grammar/parser.y:598–608`; reglas Bison 36, 37.

*lista_init* → [ *elems* ]

Referencia: `grammar/parser.y:612–617`; reglas Bison 38.

*elems* → *elem* | *elems* , *elem*

Referencia: `grammar/parser.y:621–634`; reglas Bison 39, 40.

*elem* → *expr* | *lista_init*

Referencia: `grammar/parser.y:638–648`; reglas Bison 41, 42.

*sent_id* → **id** = *expr* ; | **id** [ *args* ] = *expr* ; | **id** [ *args* ] [ *expr* ] = *expr* ; | **id** [ *args* ] ;

Referencia: `grammar/parser.y:657–733`; reglas Bison 43, 44, 45, 46.

*condicional* → **C** [ *expr* ] ; *cuerpo* *alts* *residual* .

Referencia: `grammar/parser.y:742–762`; reglas Bison 47.

*alts* → ε | *alts* **A** [ *expr* ] ; *cuerpo*

Referencia: `grammar/parser.y:766–793`; reglas Bison 48, 49.

*residual* → ε | **A** ; *cuerpo*

Referencia: `grammar/parser.y:797–811`; reglas Bison 50, 51.

*ciclo_l* → **L** [ *expr* ] ; *cuerpo* .

Referencia: `grammar/parser.y:820–836`; reglas Bison 52.

*ciclo_s* → **S** [ **id** , *expr* , *expr* , *paso* ] ; *cuerpo* .

Referencia: `grammar/parser.y:845–875`; reglas Bison 53.

*paso* → **num** | - **num**

Referencia: `grammar/parser.y:879–893`; reglas Bison 54, 55.

*emit* → **E** ;

Referencia: `grammar/parser.y:902–909`; reglas Bison 56.

*rewind* → **R** **id** [ *args* ] ;

Referencia: `grammar/parser.y:918–938`; reglas Bison 57.

*crypto* → **autenticar** [ *expr* ] ; | **cerrar_sesion** [ ] ; | **cargar_llave** [ **num** , **num** , *expr* ] ; | **sellar** [ ] ; | **cambiar_ancla** [ *expr* ] ; | **cifrar** [ *expr* , *expr* , *expr* , **num** ] ; | **descifrar** [ *expr* , *expr* , *expr* , **num** ] ;

Referencia: `grammar/parser.y:954–1076`; reglas Bison 58, 59, 60, 61, 62, 63, 64.

*args* → ε | *lista_arg*

Referencia: `grammar/parser.y:1085–1095`; reglas Bison 65, 66.

*lista_arg* → *expr* | *lista_arg* , *expr*

Referencia: `grammar/parser.y:1099–1110`; reglas Bison 67, 68.

*expr* → *disy*

Referencia: `grammar/parser.y:1119–1124`; reglas Bison 69.

*disy* → *conj* | *disy* `|` *conj*

Referencia: `grammar/parser.y:1128–1145`; reglas Bison 70, 71.

*conj* → *igualdad* | *conj* & *igualdad*

Referencia: `grammar/parser.y:1149–1166`; reglas Bison 72, 73.

*igualdad* → *relacion* | *igualdad* == *relacion* | *igualdad* != *relacion*

Referencia: `grammar/parser.y:1170–1199`; reglas Bison 74, 75, 76.

*relacion* → *aditivo* | *relacion* < *aditivo* | *relacion* > *aditivo* | *relacion* <= *aditivo* | *relacion* >= *aditivo*

Referencia: `grammar/parser.y:1208–1261`; reglas Bison 77, 78, 79, 80, 81.

*aditivo* → *multiplicativo* | *aditivo* + *multiplicativo* | *aditivo* - *multiplicativo*

Referencia: `grammar/parser.y:1265–1294`; reglas Bison 82, 83, 84.

*multiplicativo* → *unario* | *multiplicativo* \* *unario* | *multiplicativo* / *unario* | *multiplicativo* % *unario*

Referencia: `grammar/parser.y:1302–1343`; reglas Bison 85, 86, 87, 88.

*unario* → - *unario* | ! *unario* | *potencia*

Referencia: `grammar/parser.y:1347–1372`; reglas Bison 89, 90, 91.

*potencia* → *primario* | *primario* ^ *unario*

Referencia: `grammar/parser.y:1376–1393`; reglas Bison 92, 93.

*primario* → ( *expr* ) | *acceso* | **num** | **verdadero** | **falso**

Referencia: `grammar/parser.y:1397–1428`; reglas Bison 94, 95, 96, 97, 98.

*acceso* → **id** | **id** [ *args* ] | **id** [ *args* ] [ *expr* ]

Referencia: `grammar/parser.y:1432–1474`; reglas Bison 99, 100, 101.

## Tabla de terminales

Son **47 terminales declarados con `%token`**, todos usados en las producciones y devueltos por el lexer. Los patrones se transcriben de `lexer.l`; las comillas de Flex delimitan lexemas literales. Se incluyen las líneas de declaración en Bison para facilitar la trazabilidad.

| Token de Bison | En el documento | Patrón o lexema de Flex | Línea de lexer.l | Línea de parser.y |
| --- | --- | --- | ---: | ---: |
| `TOKEN_F` | **F** | `"F"` | 146 | 59 |
| `TOKEN_VM` | **Vm** | `"Vm"` | 147 | 60 |
| `TOKEN_VI` | **Vi** | `"Vi"` | 148 | 61 |
| `TOKEN_INT` | **int** | `"int"` | 150 | 63 |
| `TOKEN_BOOL` | **bool** | `"bool"` | 151 | 64 |
| `TOKEN_VOID` | **void** | `"void"` | 152 | 65 |
| `TOKEN_C` | **C** | `"C"` | 165 | 67 |
| `TOKEN_A` | **A** | `"A"` | 166 | 68 |
| `TOKEN_L` | **L** | `"L"` | 167 | 69 |
| `TOKEN_S` | **S** | `"S"` | 168 | 70 |
| `TOKEN_E` | **E** | `"E"` | 169 | 71 |
| `TOKEN_R` | **R** | `"R"` | 170 | 72 |
| `TOKEN_TRUE` | **verdadero** | `"verdadero"` | 172 | 74 |
| `TOKEN_FALSE` | **falso** | `"falso"` | 173 | 75 |
| `TOKEN_AUTENTICAR` | **autenticar** | `"autenticar"` | 157 | 77 |
| `TOKEN_CERRAR_SESION` | **cerrar_sesion** | `"cerrar_sesion"` | 158 | 78 |
| `TOKEN_CARGAR_LLAVE` | **cargar_llave** | `"cargar_llave"` | 159 | 79 |
| `TOKEN_SELLAR` | **sellar** | `"sellar"` | 160 | 80 |
| `TOKEN_CAMBIAR_ANCLA` | **cambiar_ancla** | `"cambiar_ancla"` | 161 | 81 |
| `TOKEN_CIFRAR` | **cifrar** | `"cifrar"` | 162 | 82 |
| `TOKEN_DESCIFRAR` | **descifrar** | `"descifrar"` | 163 | 83 |
| `TOKEN_OR` | `\|` | `"\|"` | 184 | 88 |
| `TOKEN_AND` | & | `"&"` | 185 | 89 |
| `TOKEN_EQUAL_EQUAL` | == | `"=="` | 179 | 91 |
| `TOKEN_NOT_EQUAL` | != | `"!="` | 180 | 92 |
| `TOKEN_LESS` | < | `"<"` | 187 | 94 |
| `TOKEN_GREATER` | > | `">"` | 188 | 95 |
| `TOKEN_LESS_EQUAL` | <= | `"<="` | 181 | 96 |
| `TOKEN_GREATER_EQUAL` | >= | `">="` | 182 | 97 |
| `TOKEN_PLUS` | + | `"+"` | 190 | 99 |
| `TOKEN_MINUS` | - | `"-"` | 191 | 100 |
| `TOKEN_MULTIPLY` | \* | `"*"` | 193 | 102 |
| `TOKEN_DIVIDE` | / | `"/"` | 194 | 103 |
| `TOKEN_MODULO` | % | `"%"` | 195 | 104 |
| `TOKEN_NOT` | ! | `"!"` | 197 | 106 |
| `TOKEN_POWER` | ^ | `"^"` | 198 | 107 |
| `TOKEN_ASSIGN` | = | `"="` | 200 | 109 |
| `TOKEN_LBRACKET` | [ | `"["` | 202 | 114 |
| `TOKEN_RBRACKET` | ] | `"]"` | 203 | 115 |
| `TOKEN_LPAREN` | ( | `"("` | 205 | 117 |
| `TOKEN_RPAREN` | ) | `")"` | 206 | 118 |
| `TOKEN_COLON` | : | `":"` | 208 | 120 |
| `TOKEN_SEMICOLON` | ; | `";"` | 209 | 121 |
| `TOKEN_COMMA` | , | `","` | 210 | 122 |
| `TOKEN_DOT` | . | `"."` | 211 | 123 |
| `TOKEN_IDENTIFIER` | **id** | `[a-z_][a-zA-Z0-9_]*` | 248 | 129 |
| `TOKEN_NUMBER` | **num** | `[0-9]+` | 217 | 130 |

Las acciones de los patrones numéricos y de identificador retornan `TOKEN_NUMBER` y `TOKEN_IDENTIFIER` en las líneas 245 y 274, respectivamente. Los límites y recuperaciones léxicas de sus acciones no cambian los patrones transcritos. Espacios y comentarios se omiten y no son terminales de esta gramática.

Los símbolos internos de Bison se contabilizan por separado: `/tmp/calser.output` enumera además `$end` y `error` (49 entradas terminales en total); `error` no tiene reglas asociadas. El C generado declara `YYNTOKENS = 50`, porque también incluye el símbolo interno de token no definido, `$undefined` (`YYUNDEF`). Ninguno de estos tres es un `%token TOKEN_...` de Calser; `$end` aparece solo en la producción aumentada.

## Precedencia y asociatividad

No hay declaraciones `%left`, `%right` ni usos de `%prec`; tampoco `%nonassoc` ni `%precedence`. La estructura de las producciones determina la precedencia y la agrupación. *expr* → *disy* es la regla 69 (`parser.y:1120`).

La tabla presenta los niveles en el orden solicitado: *disy* → *conj* → *igualdad* → *relacion* → *aditivo* → *multiplicativo* → *potencia* → *unario* → *primario*. **Ese orden no es una cadena literal de las reglas en los dos niveles potencia/unario**: la cadena de alternativas base real es *multiplicativo* → *unario* → *potencia* → *primario* (85, 91, 92), y *potencia* también vuelve a *unario* en su operando derecho (93). Se conserva esa estructura sin modificarla.

| Nivel | Operadores / forma | Reglas Bison y líneas de parser.y | Asociatividad y fundamento |
| --- | --- | --- | --- |
| *disy* | `\|` | 70: 1129; 71: 1134 | Izquierda: *disy* → *disy* `\|` *conj* (71). |
| *conj* | & | 72: 1150; 73: 1155 | Izquierda: *conj* → *conj* & *igualdad* (73). |
| *igualdad* | ==, != | 74: 1171; 75: 1176; 76: 1188 | Izquierda por recursión izquierda en 75–76. |
| *relacion* | <, >, <=, >= | 77: 1209; 78: 1214; 79: 1226; 80: 1238; 81: 1250 | Izquierda por recursión izquierda en 78–81; la sintaxis permite encadenarlas. |
| *aditivo* | +, - | 82: 1266; 83: 1271; 84: 1283 | Izquierda por recursión izquierda en 83–84. |
| *multiplicativo* | \*, /, % | 85: 1303; 86: 1308; 87: 1320; 88: 1332 | Izquierda por recursión izquierda en 86–88; sus operandos son *unario*. |
| *potencia* | ^ | 92: 1377; 93: 1382 | Derecha por recursión derecha indirecta: *potencia* → *primario* ^ *unario* → *primario* ^ *potencia* (93, 91). |
| *unario* | -, ! (prefijos) | 89: 1348; 90: 1358; 91: 1368 | Agrupación a derecha por recursión derecha en 89–90; 91 delega en *potencia*. No es asociatividad de operadores binarios. |
| *primario* | ( *expr* ), *acceso*, **num**, **verdadero**, **falso** | 94: 1398; 95: 1403; 96: 1408; 97: 1415; 98: 1422 | Sin asociatividad binaria propia; los paréntesis fijan la agrupación. |

Consecuencias de las reglas 89–93: `-2^2` se agrupa como `-(2^2)`; `2^3^4`, como `2^(3^4)`; `2^-3` se admite como `2^(-3)`. Por eso no corresponde afirmar que el orden solicitado potencia → unario significa que todos los operadores unarios tengan mayor precedencia que ^. *acceso* (99–101) aporta las formas con identificador a *primario*, sin introducir un operador binario adicional.

## Producciones con error

No hay ninguna producción que contenga el token especial `error`, ni una declaración `%token error`: no existen producciones de recuperación sintáctica. `parser.y:198` contiene exactamente `%define parse.error custom`; configura el reporte personalizado de errores, sin agregar producciones. `parser.y:199` contiene `%define parse.lac full`. La aparición de `error (256)` en el inventario interno de Bison, sin números de regla a su lado, no contradice su ausencia en los cuerpos de la gramática.

## Verificación contra calser.output

Se comparó cada regla extraída del cuerpo real de `parser.y`, ignorando comentarios y acciones, contra el apartado `Grammar` de `/tmp/calser.output`, sustituyendo únicamente los nombres `TOKEN_...` por sus alias Bison para la comparación. Coinciden las **101 producciones**, su orden, cabezas y cuerpos. También coinciden exactamente los conjuntos de 43 no terminales propios y 47 terminales declarados.

| Conteo | Esperado en la tarea | Verificado | Resultado |
| --- | --- | --- | --- |
| Producciones propias, excluyendo `$accept` | 102 | **101**, reglas 1–101 | Diferencia: una menos que lo solicitado. |
| Producciones incluyendo `$accept` | — | **102**, reglas 0–101; `YYNRULES = 102` | El valor 102 incluye la producción aumentada. |
| No terminales propios | 43 | **43** | Coincide; con `$accept`, 44 (`YYNNTS = 44`). |
| Terminales `%token` de Calser | — | **47** | Todos aparecen en lexer, parser e informe. |
| Conflictos shift/reduce | 0 | **0** | Sin avisos ni estados con conflictos en el informe. |
| Conflictos reduce/reduce | 0 | **0** | Sin avisos ni estados con conflictos en el informe. |

La producción aumentada 0 es `$accept → programa $end`; se documenta aquí como control y se excluye de la copia de las producciones propias. El autómata generado tiene 242 estados (`YYNSTATES = 242`). **No hay diferencias entre la extracción y Bison**; sí difiere la expectativa de 102 producciones propias de la tarea. También se señala arriba la salvedad del orden potencia/unario solicitado frente a las reglas reales.

### Correspondencia completa de reglas

Cada fila corresponde a una alternativa del grupo con la misma cabeza en la primera sección. Las líneas señalan el separador `:` o `|` y el último símbolo del cuerpo; para ε señalan la línea de la alternativa vacía.

| Regla Bison | Producción en este documento | Líneas de parser.y |
| ---: | --- | --- |
| 0 | `$accept → programa $end` (aumentada; fuera de los conteos propios) | Generada por Bison; `%start` en 202 |
| 1 | *programa* → *globales* *funciones* | 215 |
| 2 | *globales* → ε | 230 |
| 3 | *globales* → *globales* *decl* | 235 |
| 4 | *funciones* → *funcion* | 250 |
| 5 | *funciones* → *funciones* *funcion* | 255 |
| 6 | *funcion* → **F** **id** [ *params* ] : *retorno* ; *cuerpo* . | 270–279 |
| 7 | *retorno* → *tipo* **id** | 302 |
| 8 | *retorno* → **void** | 314 |
| 9 | *params* → ε | 330 |
| 10 | *params* → *lista_param* | 335 |
| 11 | *lista_param* → *param* | 344 |
| 12 | *lista_param* → *lista_param* , *param* | 349 |
| 13 | *param* → *modif_opt* *tipo* **id** *dims_opt* | 359 |
| 14 | *modif* → **Vm** | 377 |
| 15 | *modif* → **Vi** | 384 |
| 16 | *modif_opt* → *modif* | 395 |
| 17 | *modif_opt* → ε | 400 |
| 18 | *tipo* → **int** | 409 |
| 19 | *tipo* → **bool** | 416 |
| 20 | *dims_opt* → *dims* | 427 |
| 21 | *dims_opt* → ε | 432 |
| 22 | *dims* → [ **num** ] | 441–443 |
| 23 | *dims* → [ **num** ] [ **num** ] | 455–460 |
| 24 | *cuerpo* → ε | 479 |
| 25 | *cuerpo* → *cuerpo* *sentencia* | 484 |
| 26 | *sentencia* → *decl* | 499 |
| 27 | *sentencia* → *sent_id* | 504 |
| 28 | *sentencia* → *condicional* | 509 |
| 29 | *sentencia* → *ciclo_l* | 514 |
| 30 | *sentencia* → *ciclo_s* | 519 |
| 31 | *sentencia* → *emit* | 524 |
| 32 | *sentencia* → *rewind* | 529 |
| 33 | *sentencia* → *crypto* | 534 |
| 34 | *decl* → *modif* **id** : *tipo* *dims_opt* ; | 548–553 |
| 35 | *decl* → *modif* **id** : *tipo* *dims_opt* = *inicial* ; | 568–575 |
| 36 | *inicial* → *expr* | 599 |
| 37 | *inicial* → *lista_init* | 604 |
| 38 | *lista_init* → [ *elems* ] | 613 |
| 39 | *elems* → *elem* | 622 |
| 40 | *elems* → *elems* , *elem* | 629 |
| 41 | *elem* → *expr* | 639 |
| 42 | *elem* → *lista_init* | 644 |
| 43 | *sent_id* → **id** = *expr* ; | 658–661 |
| 44 | *sent_id* → **id** [ *args* ] = *expr* ; | 675–681 |
| 45 | *sent_id* → **id** [ *args* ] [ *expr* ] = *expr* ; | 695–704 |
| 46 | *sent_id* → **id** [ *args* ] ; | 718–722 |
| 47 | *condicional* → **C** [ *expr* ] ; *cuerpo* *alts* *residual* . | 743–751 |
| 48 | *alts* → ε | 767 |
| 49 | *alts* → *alts* **A** [ *expr* ] ; *cuerpo* | 772–778 |
| 50 | *residual* → ε | 798 |
| 51 | *residual* → **A** ; *cuerpo* | 803–805 |
| 52 | *ciclo_l* → **L** [ *expr* ] ; *cuerpo* . | 821–827 |
| 53 | *ciclo_s* → **S** [ **id** , *expr* , *expr* , *paso* ] ; *cuerpo* . | 846–858 |
| 54 | *paso* → **num** | 880 |
| 55 | *paso* → - **num** | 887 |
| 56 | *emit* → **E** ; | 903 |
| 57 | *rewind* → **R** **id** [ *args* ] ; | 919–924 |
| 58 | *crypto* → **autenticar** [ *expr* ] ; | 955–959 |
| 59 | *crypto* → **cerrar_sesion** [ ] ; | 966–969 |
| 60 | *crypto* → **cargar_llave** [ **num** , **num** , *expr* ] ; | 976–984 |
| 61 | *crypto* → **sellar** [ ] ; | 1002–1005 |
| 62 | *crypto* → **cambiar_ancla** [ *expr* ] ; | 1012–1016 |
| 63 | *crypto* → **cifrar** [ *expr* , *expr* , *expr* , **num** ] ; | 1023–1033 |
| 64 | *crypto* → **descifrar** [ *expr* , *expr* , *expr* , **num** ] ; | 1050–1060 |
| 65 | *args* → ε | 1086 |
| 66 | *args* → *lista_arg* | 1091 |
| 67 | *lista_arg* → *expr* | 1100 |
| 68 | *lista_arg* → *lista_arg* , *expr* | 1105 |
| 69 | *expr* → *disy* | 1120 |
| 70 | *disy* → *conj* | 1129 |
| 71 | *disy* → *disy* `\|` *conj* | 1134 |
| 72 | *conj* → *igualdad* | 1150 |
| 73 | *conj* → *conj* & *igualdad* | 1155 |
| 74 | *igualdad* → *relacion* | 1171 |
| 75 | *igualdad* → *igualdad* == *relacion* | 1176 |
| 76 | *igualdad* → *igualdad* != *relacion* | 1188 |
| 77 | *relacion* → *aditivo* | 1209 |
| 78 | *relacion* → *relacion* < *aditivo* | 1214 |
| 79 | *relacion* → *relacion* > *aditivo* | 1226 |
| 80 | *relacion* → *relacion* <= *aditivo* | 1238 |
| 81 | *relacion* → *relacion* >= *aditivo* | 1250 |
| 82 | *aditivo* → *multiplicativo* | 1266 |
| 83 | *aditivo* → *aditivo* + *multiplicativo* | 1271 |
| 84 | *aditivo* → *aditivo* - *multiplicativo* | 1283 |
| 85 | *multiplicativo* → *unario* | 1303 |
| 86 | *multiplicativo* → *multiplicativo* \* *unario* | 1308 |
| 87 | *multiplicativo* → *multiplicativo* / *unario* | 1320 |
| 88 | *multiplicativo* → *multiplicativo* % *unario* | 1332 |
| 89 | *unario* → - *unario* | 1348 |
| 90 | *unario* → ! *unario* | 1358 |
| 91 | *unario* → *potencia* | 1368 |
| 92 | *potencia* → *primario* | 1377 |
| 93 | *potencia* → *primario* ^ *unario* | 1382 |
| 94 | *primario* → ( *expr* ) | 1398 |
| 95 | *primario* → *acceso* | 1403 |
| 96 | *primario* → **num** | 1408 |
| 97 | *primario* → **verdadero** | 1415 |
| 98 | *primario* → **falso** | 1422 |
| 99 | *acceso* → **id** | 1433 |
| 100 | *acceso* → **id** [ *args* ] | 1441–1444 |
| 101 | *acceso* → **id** [ *args* ] [ *expr* ] | 1456–1462 |

## Anexo: marcas sobre las producciones originales

### (a) Recursión por la izquierda

Se detectan **20 alternativas con recursión izquierda directa, en 13 no terminales**. Se listan sin transformarlas. No hay recursión izquierda indirecta a través de otros no terminales: se verificó el grafo de esquinas izquierdas, considerando todos los prefijos anulables. Los no terminales anulables son *alts*, *args*, *cuerpo*, *dims_opt*, *globales*, *modif_opt*, *params*, *residual*.

| Regla Bison | Producción con recursión izquierda directa | Líneas de parser.y |
| ---: | --- | --- |
| 3 | *globales* → *globales* *decl* | 235 |
| 5 | *funciones* → *funciones* *funcion* | 255 |
| 12 | *lista_param* → *lista_param* , *param* | 349 |
| 25 | *cuerpo* → *cuerpo* *sentencia* | 484 |
| 40 | *elems* → *elems* , *elem* | 629 |
| 49 | *alts* → *alts* **A** [ *expr* ] ; *cuerpo* | 772–778 |
| 68 | *lista_arg* → *lista_arg* , *expr* | 1105 |
| 71 | *disy* → *disy* `\|` *conj* | 1134 |
| 73 | *conj* → *conj* & *igualdad* | 1155 |
| 75 | *igualdad* → *igualdad* == *relacion* | 1176 |
| 76 | *igualdad* → *igualdad* != *relacion* | 1188 |
| 78 | *relacion* → *relacion* < *aditivo* | 1214 |
| 79 | *relacion* → *relacion* > *aditivo* | 1226 |
| 80 | *relacion* → *relacion* <= *aditivo* | 1238 |
| 81 | *relacion* → *relacion* >= *aditivo* | 1250 |
| 83 | *aditivo* → *aditivo* + *multiplicativo* | 1271 |
| 84 | *aditivo* → *aditivo* - *multiplicativo* | 1283 |
| 86 | *multiplicativo* → *multiplicativo* \* *unario* | 1308 |
| 87 | *multiplicativo* → *multiplicativo* / *unario* | 1320 |
| 88 | *multiplicativo* → *multiplicativo* % *unario* | 1332 |

La recursión derecha indirecta entre *potencia* y *unario* no es recursión izquierda: en la regla 93 aparece ^ antes de *unario*; las reglas 89–90 también consumen un terminal antes de recurrir.

### (b) Alternativas con prefijo común

Se comparan prefijos no vacíos de las secuencias originales de símbolos de alternativas con la misma cabeza, sin expandir no terminales. Se informa el prefijo máximo para cada conjunto de alternativas que lo comparte; los casos anidados de *sent_id* y *acceso* se muestran por separado. Un cuerpo completo puede ser prefijo de otro. ε no se cuenta como prefijo común. Esto marca posibilidades de factorización, sin aplicarla, y no implica conflictos LALR.

| Cabeza | Reglas Bison | Prefijo común | Líneas de parser.y por regla |
| --- | --- | --- | --- |
| *dims* | 22, 23 | [ **num** ] | 22: 441–443; 23: 455–460 |
| *decl* | 34, 35 | *modif* **id** : *tipo* *dims_opt* | 34: 548–553; 35: 568–575 |
| *sent_id* | 43, 44, 45, 46 | **id** | 43: 658–661; 44: 675–681; 45: 695–704; 46: 718–722 |
| *sent_id* | 44, 45, 46 | **id** [ *args* ] | 44: 675–681; 45: 695–704; 46: 718–722 |
| *igualdad* | 75, 76 | *igualdad* | 75: 1176; 76: 1188 |
| *relacion* | 78, 79, 80, 81 | *relacion* | 78: 1214; 79: 1226; 80: 1238; 81: 1250 |
| *aditivo* | 83, 84 | *aditivo* | 83: 1271; 84: 1283 |
| *multiplicativo* | 86, 87, 88 | *multiplicativo* | 86: 1308; 87: 1320; 88: 1332 |
| *potencia* | 92, 93 | *primario* | 92: 1377; 93: 1382 |
| *acceso* | 99, 100, 101 | **id** | 99: 1433; 100: 1441–1444; 101: 1456–1462 |
| *acceso* | 100, 101 | **id** [ *args* ] | 100: 1441–1444; 101: 1456–1462 |

Son **11 conjuntos de alternativas con prefijo común, en 9 no terminales**. Las demás cabezas no presentan prefijos simbólicos no vacíos compartidos entre sus alternativas originales.

## Identificación de los archivos verificados

Huellas SHA-256 del contenido leído y del informe usado para comprobar la extracción:

| Archivo | SHA-256 |
| --- | --- |
| `grammar/parser.y` | `9da3995eea8a7d454138596db03ec64fafb78455a117b87cbfcfdbea4648d7d1` |
| `grammar/lexer.l` | `ca12c87c9ebb22d9bf5d82b28940f20ae2cda414e86b042a7663e84d307b8bee` |
| `/tmp/calser.output` | `556534369764580fc35cda46a8c828a35a3376edac34a75ae3be870f2f429042` |
