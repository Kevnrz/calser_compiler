# Gramática LL(1) de Calser

Tarea C1.1a (compi), Proyecto Grupal II, Grupo 6. Base para el parser predictivo del IDE (D4, D5).

**Origen:** [gramatica_calser.md](gramatica_calser.md), extracción fiel de `grammar/parser.y` en `develop` (commit `7e9e811`, SHA-256 de `parser.y` `9da3995e…48d7d1`). La gramática original tiene 101 producciones, 43 no terminales, 47 terminales y 0 conflictos LALR.

**Resultado:** gramática LL(1) equivalente, con **118 producciones, 60 no terminales, 47 terminales**. La tabla predictiva M tiene **336 entradas y ninguna celda con dos producciones**.

## 1. Notación

Formato de Aho et al. §4.2.1–4.2.2: cabeza → cuerpo, alternativas separadas por |, cuerpo vacío ε. No terminales en *cursiva*, con los nombres de Bison; los nuevos llevan apóstrofo (*disy'*), como E' en el libro. En el código el apóstrofo se escribe `_p` (`disy_p`). Terminales: palabras y nombres de token en **negrita**, operadores y puntuación tal cual. El terminal `|` va en código para no confundirlo con el separador. No se usan E, T, F del libro, porque **E** y **F** son tokens de Calser.

Símbolo inicial: *programa*. Fin de entrada: $.

## 2. Transformaciones aplicadas

| Original (reglas Bison) | Problema | Técnica (Aho) | Resultado |
|---|---|---|---|
| *globales*, *funciones*, *lista_param*, *cuerpo*, *elems*, *lista_arg* (3, 5, 12, 25, 40, 68) | Recursión izquierda directa | §4.3.3: A → Aα \| β pasa a A → βA', A' → αA' \| ε | *globales* y *cuerpo* se reescriben sin no terminal nuevo (β = ε); el resto agrega *X'* |
| *disy*, *conj*, *igualdad*, *relacion*, *aditivo*, *multiplicativo* (70–88) | Recursión izquierda directa | §4.3.3, como E → T E' (Ej. 4.28) | Un no terminal *X'* por nivel |
| *alts* y *residual* (48–51) | Al quitar la recursión, M[*alts*, **A**] tendría dos producciones: **A** ∈ PRIMERO(*alts*) y **A** ∈ SIGUIENTE(*alts*), por *residual* | Unir y factorizar (§4.3.4) | *resto_c*, *tras_a*: se decide con el token después de **A** (`[` o `;`) |
| *dims* (22, 23) | Prefijo común [ **num** ] | §4.3.4 | *dims'* |
| *decl* (34, 35) | Prefijo común *modif* **id** : *tipo* *dims_opt* | §4.3.4 | *decl'* |
| *sent_id* (43–46) | Prefijos **id** y **id** [ *args* ] | §4.3.4, dos veces | *sent_id'*, *tras_args* |
| *potencia* (92, 93) | Prefijo común *primario* | §4.3.4 | *potencia'* |
| *acceso* (99–101) | Prefijos **id** y **id** [ *args* ] | §4.3.4, dos veces | *acceso'*, *indice* |

No hay recursión izquierda indirecta (anexo de `gramatica_calser.md`). Las reglas restantes ya eran LL(1) y no cambian: *funcion*, *retorno*, *params*, *param*, *modif*, *modif_opt*, *tipo*, *dims_opt*, *sentencia*, *inicial*, *lista_init*, *elem*, *ciclo_l*, *ciclo_s*, *paso*, *emit*, *rewind*, *crypto*, *args*, *expr*, *unario* y *primario*.

**Precedencia y asociatividad.** Se conservan los niveles. *unario* y *potencia* quedan igual que en Bison: `-2^2` es `-(2^2)`, `2^3^4` es `2^(3^4)` y `2^-3` es válido. Quitar la recursión izquierda no cambia el lenguaje, pero sí la forma del árbol. Si el IDE llega a construir árboles, la asociatividad izquierda se recupera con un atributo heredado (Aho §5.4.4). Para reconocer, sugerir y corregir no hace falta.

## 3. Gramática LL(1)

### Programa y funciones

*programa* → *globales* *funciones*

*globales* → *decl* *globales* | ε

*funciones* → *funcion* *funciones'*

*funciones'* → *funcion* *funciones'* | ε

*funcion* → **F** **id** [ *params* ] : *retorno* ; *cuerpo* .

*retorno* → *tipo* **id** | **void**

*params* → *lista_param* | ε

*lista_param* → *param* *lista_param'*

*lista_param'* → , *param* *lista_param'* | ε

*param* → *modif_opt* *tipo* **id** *dims_opt*

*modif* → **Vm** | **Vi**

*modif_opt* → *modif* | ε

*tipo* → **int** | **bool**

*dims_opt* → *dims* | ε

*dims* → [ **num** ] *dims'*

*dims'* → [ **num** ] | ε

### Cuerpo y sentencias

*cuerpo* → *sentencia* *cuerpo* | ε

*sentencia* → *decl* | *sent_id* | *condicional* | *ciclo_l* | *ciclo_s* | *emit* | *rewind* | *crypto*

*decl* → *modif* **id** : *tipo* *dims_opt* *decl'*

*decl'* → ; | = *inicial* ;

*inicial* → *expr* | *lista_init*

*lista_init* → [ *elems* ]

*elems* → *elem* *elems'*

*elems'* → , *elem* *elems'* | ε

*elem* → *expr* | *lista_init*

*sent_id* → **id** *sent_id'*

*sent_id'* → = *expr* ; | [ *args* ] *tras_args*

*tras_args* → = *expr* ; | [ *expr* ] = *expr* ; | ;

### Control: C/A, L, S

*condicional* → **C** [ *expr* ] ; *cuerpo* *resto_c* .

*resto_c* → **A** *tras_a* | ε

*tras_a* → [ *expr* ] ; *cuerpo* *resto_c* | ; *cuerpo*

*ciclo_l* → **L** [ *expr* ] ; *cuerpo* .

*ciclo_s* → **S** [ **id** , *expr* , *expr* , *paso* ] ; *cuerpo* .

*paso* → **num** | - **num**

### E, R y criptografía

*emit* → **E** ;

*rewind* → **R** **id** [ *args* ] ;

*crypto* → **autenticar** [ *expr* ] ; | **cerrar_sesion** [ ] ; | **cargar_llave** [ **num** , **num** , *expr* ] ; | **sellar** [ ] ; | **cambiar_ancla** [ *expr* ] ; | **cifrar** [ *expr* , *expr* , *expr* , **num** ] ; | **descifrar** [ *expr* , *expr* , *expr* , **num** ] ;

### Argumentos

*args* → *lista_arg* | ε

*lista_arg* → *expr* *lista_arg'*

*lista_arg'* → , *expr* *lista_arg'* | ε

### Expresiones

*expr* → *disy*

*disy* → *conj* *disy'*

*disy'* → `|` *conj* *disy'* | ε

*conj* → *igualdad* *conj'*

*conj'* → & *igualdad* *conj'* | ε

*igualdad* → *relacion* *igualdad'*

*igualdad'* → == *relacion* *igualdad'* | != *relacion* *igualdad'* | ε

*relacion* → *aditivo* *relacion'*

*relacion'* → < *aditivo* *relacion'* | > *aditivo* *relacion'* | <= *aditivo* *relacion'* | >= *aditivo* *relacion'* | ε

*aditivo* → *multiplicativo* *aditivo'*

*aditivo'* → + *multiplicativo* *aditivo'* | - *multiplicativo* *aditivo'* | ε

*multiplicativo* → *unario* *multiplicativo'*

*multiplicativo'* → \* *unario* *multiplicativo'* | / *unario* *multiplicativo'* | % *unario* *multiplicativo'* | ε

*unario* → - *unario* | ! *unario* | *potencia*

*potencia* → *primario* *potencia'*

*potencia'* → ^ *unario* | ε

*primario* → ( *expr* ) | *acceso* | **num** | **verdadero** | **falso**

*acceso* → **id** *acceso'*

*acceso'* → [ *args* ] *indice* | ε

*indice* → [ *expr* ] | ε

## 4. PRIMERO y SIGUIENTE

Calculados con los algoritmos de Aho §4.4.2 y verificados con un programa. Para que la tabla se pueda leer se usan estos conjuntos con nombre:

- 𝒮 = PRIMERO(*sentencia*) = { **C**, **E**, **L**, **R**, **S**, **Vi**, **Vm**, **autenticar**, **cambiar_ancla**, **cargar_llave**, **cerrar_sesion**, **cifrar**, **descifrar**, **id**, **sellar** }
- 𝒫 = PRIMERO(*expr*) = { `!`, `(`, `-`, **falso**, **id**, **num**, **verdadero** }
- X₀ = { `)`, `,`, `;`, `]` }
- X₁ = X₀ ∪ { `\|` }
- X₂ = X₁ ∪ { `&` }
- X₃ = X₂ ∪ { `!=`, `==` }
- X₄ = X₃ ∪ { `<`, `<=`, `>`, `>=` }
- X₅ = X₄ ∪ { `+`, `-` }
- X₆ = X₅ ∪ { `%`, `*`, `/` }
- X₇ = X₆ ∪ { `^` }

X₀ = SIGUIENTE(*expr*): *expr* aparece antes de `)`, `,`, `;` o `]`. Cada nivel de expresión suma los operadores del nivel de arriba.

| No terminal | PRIMERO | SIGUIENTE |
|---|---|---|
| *programa* | { **F**, **Vi**, **Vm** } | { $ } |
| *globales* | { **Vi**, **Vm**, ε } | { **F** } |
| *funciones* | { **F** } | { $ } |
| *funciones'* | { **F**, ε } | { $ } |
| *funcion* | { **F** } | { $, **F** } |
| *retorno* | { **bool**, **int**, **void** } | { `;` } |
| *params* | { **Vi**, **Vm**, **bool**, **int**, ε } | { `]` } |
| *lista_param* | { **Vi**, **Vm**, **bool**, **int** } | { `]` } |
| *lista_param'* | { `,`, ε } | { `]` } |
| *param* | { **Vi**, **Vm**, **bool**, **int** } | { `,`, `]` } |
| *modif* | { **Vi**, **Vm** } | { **bool**, **id**, **int** } |
| *modif_opt* | { **Vi**, **Vm**, ε } | { **bool**, **int** } |
| *tipo* | { **bool**, **int** } | { `;`, `=`, `[`, **id** } |
| *dims_opt* | { `[`, ε } | { `,`, `;`, `=`, `]` } |
| *dims* | { `[` } | { `,`, `;`, `=`, `]` } |
| *dims'* | { `[`, ε } | { `,`, `;`, `=`, `]` } |
| *cuerpo* | 𝒮 ∪ { ε } | { `.`, **A** } |
| *sentencia* | 𝒮 | 𝒮 ∪ { `.`, **A** } |
| *decl* | { **Vi**, **Vm** } | 𝒮 ∪ { `.`, **A**, **F** } |
| *decl'* | { `;`, `=` } | 𝒮 ∪ { `.`, **A**, **F** } |
| *inicial* | 𝒫 ∪ { `[` } | { `;` } |
| *lista_init* | { `[` } | { `,`, `;`, `]` } |
| *elems* | 𝒫 ∪ { `[` } | { `]` } |
| *elems'* | { `,`, ε } | { `]` } |
| *elem* | 𝒫 ∪ { `[` } | { `,`, `]` } |
| *sent_id* | { **id** } | 𝒮 ∪ { `.`, **A** } |
| *sent_id'* | { `=`, `[` } | 𝒮 ∪ { `.`, **A** } |
| *tras_args* | { `;`, `=`, `[` } | 𝒮 ∪ { `.`, **A** } |
| *condicional* | { **C** } | 𝒮 ∪ { `.`, **A** } |
| *resto_c* | { **A**, ε } | { `.` } |
| *tras_a* | { `;`, `[` } | { `.` } |
| *ciclo_l* | { **L** } | 𝒮 ∪ { `.`, **A** } |
| *ciclo_s* | { **S** } | 𝒮 ∪ { `.`, **A** } |
| *paso* | { `-`, **num** } | { `]` } |
| *emit* | { **E** } | 𝒮 ∪ { `.`, **A** } |
| *rewind* | { **R** } | 𝒮 ∪ { `.`, **A** } |
| *crypto* | { **autenticar**, **cambiar_ancla**, **cargar_llave**, **cerrar_sesion**, **cifrar**, **descifrar**, **sellar** } | 𝒮 ∪ { `.`, **A** } |
| *args* | 𝒫 ∪ { ε } | { `]` } |
| *lista_arg* | 𝒫 | { `]` } |
| *lista_arg'* | { `,`, ε } | { `]` } |
| *expr* | 𝒫 | X₀ |
| *disy* | 𝒫 | X₀ |
| *disy'* | { `\|`, ε } | X₀ |
| *conj* | 𝒫 | X₁ |
| *conj'* | { `&`, ε } | X₁ |
| *igualdad* | 𝒫 | X₂ |
| *igualdad'* | { `!=`, `==`, ε } | X₂ |
| *relacion* | 𝒫 | X₃ |
| *relacion'* | { `<`, `<=`, `>`, `>=`, ε } | X₃ |
| *aditivo* | 𝒫 | X₄ |
| *aditivo'* | { `+`, `-`, ε } | X₄ |
| *multiplicativo* | 𝒫 | X₅ |
| *multiplicativo'* | { `%`, `*`, `/`, ε } | X₅ |
| *unario* | 𝒫 | X₆ |
| *potencia* | { `(`, **falso**, **id**, **num**, **verdadero** } | X₆ |
| *potencia'* | { `^`, ε } | X₆ |
| *primario* | { `(`, **falso**, **id**, **num**, **verdadero** } | X₇ |
| *acceso* | { **id** } | X₇ |
| *acceso'* | { `[`, ε } | X₇ |
| *indice* | { `[`, ε } | X₇ |

## 5. Verificación LL(1) (Aho §4.4.3)

Una gramática es LL(1) si, para cada A → α | β: PRIMERO(α) ∩ PRIMERO(β) = ∅, y si β ⇒* ε, PRIMERO(α) ∩ SIGUIENTE(A) = ∅. Los casos que importan son los no terminales con alternativa ε:

| No terminal | PRIMERO sin ε | SIGUIENTE | Intersección |
|---|---|---|---|
| *globales* | **Vm**, **Vi** | **F** | ∅ |
| *funciones'* | **F** | $ | ∅ |
| *params* | **Vm**, **Vi**, **int**, **bool** | `]` | ∅ |
| *modif_opt* | **Vm**, **Vi** | **int**, **bool** | ∅ |
| *dims_opt*, *dims'* | `[` | `,` `;` `=` `]` | ∅ |
| *cuerpo* | 𝒮 | `.`, **A** | ∅ (𝒮 no contiene **A**) |
| *resto_c* | **A** | `.` | ∅ |
| *lista_param'*, *elems'*, *lista_arg'* | `,` | `]` | ∅ |
| *args* | 𝒫 | `]` | ∅ |
| *disy'* … *multiplicativo'* | operadores de su nivel | operadores de los niveles de arriba ∪ X₀ | ∅ (cada operador está en un solo nivel) |
| *aditivo'* | `+`, `-` | X₄ | ∅. Por esto el `-` binario y el unario no chocan |
| *potencia'* | `^` | X₆ | ∅ |
| *acceso'*, *indice* | `[` | X₇ | ∅ (X₇ tiene `]`, no `[`) |

Las alternativas sin ε también son disjuntas: *decl'* (`;` / `=`), *sent_id'* (`=` / `[`), *tras_args* (`=` / `[` / `;`), *tras_a* (`[` / `;`), *inicial* y *elem* (𝒫 / `[`; 𝒫 no contiene `[`) y *sentencia* (un token inicial distinto por alternativa).

**Comprobación con programa:**
1. `ide/nucleo/tabla.py` calcula PRIMERO, SIGUIENTE y M completos: 336 entradas, 0 celdas dobles. Lo verifica `ide/pruebas/test_tabla.py` (`make check-ide`), junto con los ejemplos de Aho (Ej. 4.30, Fig. 4.17 y Ej. 4.33).
2. Equivalencia de lenguajes por prueba aleatoria (hecha una vez, fuera del repo): 792 programas generados desde una gramática fueron aceptados por la otra (el parser LL(1) contra un reconocedor Earley de la gramática original), sin fallas. Además, 226 programas mutados (un token borrado, duplicado o intercambiado) recibieron el mismo veredicto con ambas gramáticas. Esto es evidencia, no una prueba formal; la equivalencia por construcción viene de que §4.3.3 y §4.3.4 conservan el lenguaje.

## 6. Filas de la tabla M de las partes difíciles

C/A (las celdas vacías son error):

| M | **A** | `[` | `;` | `.` | 𝒮 |
|---|---|---|---|---|---|
| *resto_c* | *resto_c* → **A** *tras_a* | | | *resto_c* → ε | |
| *tras_a* | | *tras_a* → [ *expr* ] ; *cuerpo* *resto_c* | *tras_a* → ; *cuerpo* | | |
| *cuerpo* | *cuerpo* → ε | | | *cuerpo* → ε | *cuerpo* → *sentencia* *cuerpo* |

Un C anidado no genera ambigüedad (no hay "dangling else"): el C interno se cierra con su propio `.` antes de que aparezca la **A** del externo.

Declaraciones y llamadas:

| M | `;` | `=` | `[` | X (resto de SIGUIENTE) |
|---|---|---|---|---|
| *decl'* | *decl'* → ; | *decl'* → = *inicial* ; | | |
| *sent_id'* | | *sent_id'* → = *expr* ; | *sent_id'* → [ *args* ] *tras_args* | |
| *tras_args* | *tras_args* → ; | *tras_args* → = *expr* ; | *tras_args* → [ *expr* ] = *expr* ; | |
| *dims'* | *dims'* → ε | *dims'* → ε | *dims'* → [ **num** ] | ε en `,` y `]` |
| *acceso'* | ε | | *acceso'* → [ *args* ] *indice* | ε en X₇ |

La tabla completa la genera `ide/nucleo/tabla.py`.

## 7. Ejemplos de ejecución (Aho §4.4.4, Fig. 4.21)

La pila se escribe con el tope a la izquierda.

### 7.1 C/A: `C [ x ] ; E ; A [ y ] ; A ; .`

| Pila | Entrada | Acción |
|---|---|---|
| **C** [ *expr* ] ; *cuerpo* *resto_c* . | C [ x ] ; E ; A … | emparejar C, [; *expr* ⇒* **id**; emparejar ], ; |
| *cuerpo* *resto_c* . | E ; A [ y ] ; A ; . | M[*cuerpo*, **E**] → *sentencia* *cuerpo* |
| *sentencia* *cuerpo* *resto_c* . | E ; A … | *sentencia* → *emit* → **E** ;; emparejar |
| *cuerpo* *resto_c* . | A [ y ] ; A ; . | M[*cuerpo*, **A**] → ε |
| *resto_c* . | A [ y ] ; A ; . | M[*resto_c*, **A**] → **A** *tras_a*; emparejar A |
| *tras_a* . | [ y ] ; A ; . | M[*tras_a*, [] → [ *expr* ] ; *cuerpo* *resto_c*; emparejar [ y ] ; |
| *cuerpo* *resto_c* . | A ; . | M[*cuerpo*, **A**] → ε |
| *resto_c* . | A ; . | **A** *tras_a*; emparejar A |
| *tras_a* . | ; . | M[*tras_a*, ;] → ; *cuerpo*; emparejar ; |
| *cuerpo* . | . | M[*cuerpo*, .] → ε |
| . | . | emparejar; aceptar |

### 7.2 Expresión: `x - 2 ^ 3 ;`

"cola" = *aditivo'* *relacion'* *igualdad'* *conj'* *disy'*.

| Pila | Entrada | Acción |
|---|---|---|
| *expr* ; | x - 2 ^ 3 ; | producciones únicas hasta *unario* *multiplicativo'* cola ; |
| *unario* *multiplicativo'* cola ; | x - … | M[*unario*, **id**] → *potencia* → *primario* *potencia'* |
| *primario* *potencia'* *multiplicativo'* cola ; | x - … | *primario* → *acceso* → **id** *acceso'*; emparejar x |
| *acceso'* *potencia'* *multiplicativo'* cola ; | - 2 ^ 3 ; | `-` ∈ X₇, X₆, X₅: ε, ε, ε |
| *aditivo'* … ; | - 2 ^ 3 ; | M[*aditivo'*, -] → - *multiplicativo* *aditivo'*; emparejar `-` (resta) |
| *multiplicativo* *aditivo'* … ; | 2 ^ 3 ; | → *unario* *multiplicativo'* → *primario* *potencia'*; emparejar 2 |
| *potencia'* *multiplicativo'* *aditivo'* … ; | ^ 3 ; | M[*potencia'*, ^] → ^ *unario*; emparejar ^ |
| *unario* *multiplicativo'* *aditivo'* … ; | 3 ; | → *primario* *potencia'*; emparejar 3 |
| *potencia'* … *disy'* ; | ; | todo ε (`;` ∈ X₀) |
| ; | ; | emparejar; aceptar |

### 7.3 Declaración: `Vm v : int [ 2 ] = [ 1 , 2 ] ;`

| Pila | Entrada | Acción |
|---|---|---|
| *decl* | Vm v : int [ 2 ] = [ 1 , 2 ] ; | → *modif* **id** : *tipo* *dims_opt* *decl'* |
| *modif* **id** : *tipo* *dims_opt* *decl'* | Vm v : int … | *modif* → **Vm**; emparejar Vm, v, : |
| *tipo* *dims_opt* *decl'* | int [ 2 ] = … | *tipo* → **int**; emparejar |
| *dims_opt* *decl'* | [ 2 ] = … | M[*dims_opt*, [] → *dims* → [ **num** ] *dims'*; emparejar [ 2 ] |
| *dims'* *decl'* | = [ 1 , 2 ] ; | `=` ∈ SIGUIENTE(*dims'*): ε |
| *decl'* | = [ 1 , 2 ] ; | M[*decl'*, =] → = *inicial* ;; emparejar = |
| *inicial* ; | [ 1 , 2 ] ; | M[*inicial*, [] → *lista_init* → [ *elems* ]; emparejar [ |
| *elems* ] ; | 1 , 2 ] ; | → *elem* *elems'*; *elem* → *expr* ⇒* 1 |
| *elems'* ] ; | , 2 ] ; | M[*elems'*, ,] → , *elem* *elems'*; emparejar , ; *expr* ⇒* 2 |
| *elems'* ] ; | ] ; | ε |
| ] ; | ] ; | emparejar; aceptar |

## 8. Notas para el código

1. **Sugerencias.** Son los terminales con entrada en la fila M[X, ·] del tope X. Si X es anulable, se juntan las filas de la pila hacia abajo hasta el primer símbolo no anulable. Ejemplo: después de `x` (tope *acceso'*) salen `[`, `^`, `*`, `/`, `%`, `+`, `-`, comparaciones, `==`, `!=`, `&`, `|`, `;`, `,`, `]` y `)`. Si el tope es un terminal, la sugerencia es ese terminal.
2. **Autocorrección.** Recuperación a nivel de frase (Aho §4.4.5): se llenan las celdas vacías de M con rutinas de corrección. Su diseño queda pendiente (módulo `correccion`).
3. **Anulables (21).** *globales*, *funciones'*, *params*, *lista_param'*, *modif_opt*, *dims_opt*, *dims'*, *cuerpo*, *elems'*, *resto_c*, *args*, *lista_arg'*, los siete *X'* de expresiones, *acceso'* e *indice*.
4. **Pendiente D1.** Cuando C2.2 agregue la declaración de módulo P a `parser.y` (con su token de cadena), *programa* pasa a *programa* → *modulo_opt* *globales* *funciones*. Como P es un token nuevo, no genera conflictos, pero hay que recalcular la tabla.
