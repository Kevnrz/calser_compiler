# Representación intermedia de tres direcciones

C2.1a define la forma construible a mano; C2.1 agrega su construcción desde el
AST validado y la bandera `-i` / `--tac`. El generador actual sigue produciendo
los binarios, sin consumir TAC. No se introducen optimizaciones. Referencias: Aho
§6.2 (cuádruplas), §8.4.1 (bloques) y §10.2.3 (registros virtuales).
La lista de instrucciones del organizador existente sigue siendo independiente.

## Niveles y etapas

| Etapa de `TacProgram.stage` | Operaciones | Registros y valores |
| --- | --- | --- |
| `TAC_STAGE_HIGH` | Operaciones de alto nivel, nunca `TAC_ISA` | TEMP, VARIABLE, INTEGER, LABEL, FUNCTION y NONE según posición; ningún PHYS |
| `TAC_STAGE_ISA_VIRTUAL` | Solo `TAC_ISA` | TEMP y únicamente PHYS rz/r0, rl/r1 y rx/r2; inmediatos y objetivos estructurados |
| `TAC_STAGE_ISA_PHYSICAL` | Solo `TAC_ISA` | PHYS r0–r15; ningún TEMP |

TEMP y PHYS son tipos distintos aunque su número coincida. Los temporales
virtuales son pseudorregistros ilimitados en el sentido de Aho §10.2.3: no se
limitan a doce. Se numeran desde cero por función, con `size_t`; agotamiento de
identificadores o memoria termina con diagnóstico. La asignación con spilling
los llevará a los registros físicos t0–t11. El listado imprime TEMP como `tN` en alto nivel y como `vN` en las etapas ISA.
PHYS se imprime con los alias `rz`, `rl`, `rx`, `rg`, `t0`–`t11` (r0–r15).
Así, `v0` es un temporal virtual ISA y `t0` es el registro físico r4;
los nombres `tN` de alto nivel solo aparecen en esa etapa. Las identidades
internas siguen siendo tipos distintos y no se comparan por texto.

El resto de `TacOperand` distingue NONE, VARIABLE (Symbol prestado), INTEGER
(entero de 64 bits), LABEL (referencia prestada al bloque) y FUNCTION (Symbol de
función prestado). Los símbolos se comparan por puntero, no por nombre. Las
etiquetas deben pertenecer a la función; el número impreso B0/B1 es local.
Las constantes de alto nivel no se truncan: C2.3 deberá aplicar la semántica de
32 bits y materializar las que no caben en un inmediato de 18 bits con signo.

## Operaciones de alto nivel

Las columnas D/A/B son `dest`, `src1`, `src2`, respectivamente. Un campo llamado
`dest` no necesariamente es definición: las máscaras de tabla indican su papel.
Resultado y valor admiten temporal o variable escalar; valor admite constante.

| Operación | D / A / B | Define | Usa valores | Memoria | Barrera / no eliminable | Otras propiedades |
| --- | --- | --- | --- | --- | --- | --- |
| COPY | resultado / valor / — | D | A | — | no / no | parámetro arreglo en A significa leer su dirección recibida |
| ADDRESS | resultado / arreglo declarado / — | D | — | — | no / no | materializa dirección estática |
| ADD SUB AND OR XOR EQ NE LT LE GT GE | resultado / valor / valor | D | A, B | — | no / no | operación ALU abstracta |
| NEG NOT | resultado / valor / — | D | A | — | no / no | unaria |
| MUL | resultado / valor / valor | D | A, B | — | no / no | may_create_blocks |
| DIV MOD POW | resultado / valor / valor | D | A, B | — | no / no | may_create_blocks y may_trap |
| LOAD | resultado / base / desplazamiento | D | A si TEMP, B | lectura | no / no | base VARIABLE es dirección estática |
| STORE | base / desplazamiento / valor | — | D si TEMP, A, B | escritura | no / sí | no redefine la base |
| AUTH | — / token / — | — | A | — | sí / sí | estado CRYPTO |
| LOGOUT | — / — / — | — | — | — | sí / sí | estado CRYPTO |
| KV_WRITE | — / valor / — | — | A | — | sí / sí | selectores k/w |
| KV_LOCK | — / — / — | — | — | — | sí / sí | estado CRYPTO |
| SANC | — / valor / — | — | A | — | sí / sí | estado CRYPTO |
| CIFRAR DESCIFRAR | origen / destino / bloques | — | D, A, B | lectura y escritura | sí / sí | selector k; may_create_blocks |
| PARAM | — / valor / posición | — | A | — | no / sí | posición entera desde cero |
| CALL | resultado opcional / FUNCTION / aridad | D si existe | globales implícitas | lectura y escritura conservadoras | sí / sí | define globales; destruye físicos 0xfff0; no termina bloque |
| RETURN | — / valor opcional / — | — | A y globales implícitas | — | sí / sí | termina bloque |
| GOTO | LABEL / — / — | — | — | — | sí / sí | termina bloque |
| IF_EQ IF_NE IF_LT IF_LE IF_GT IF_GE | LABEL / valor / valor | — | A, B | — | sí / sí | termina bloque, requiere caída |

La tabla en `tac.c` tiene inicializadores designados, sin macros que fabriquen
filas. `tac_op_info` devuelve una fila inmutable de alto nivel; para TAC_ISA se
usa `tac_instruction_info`, que resuelve la fila concreta de `isa_table`.
`may_trap` exige que DCE conserve DIV/MOD/POW salvo prueba de que no fallan.
`may_create_blocks` avisa a C2.3 de una expansión que puede producir un lazo.
No se garantiza expansión recta para DIV/MOD/POW con segundo operando constante.
CIFRAR/DESCIFRAR también pueden crear lazos al procesar bloques de 64 bits.

Las siete sentencias CRYPTO conservan el orden mediante barreras: AUTH, bóveda,
ancla y EXC son estado oculto compartido. Ninguna define valor; solo
CIFRAR/DESCIFRAR acceden a memoria general. Sus direcciones son expresiones
enteras, no nombres de arreglos. `bloques` cuenta bloques de 64 bits, no bytes.
El impreso respeta Calser: `cargar_llave [k, w, valor]` y
`cifrar [origen, destino, bloques, k]`. Selectores presentes son literales 0–3;
k/w/round ausentes deben ser -1. La validación de forma no repite las
comprobaciones semánticas de direcciones del compilador.

## Argumentos, globales y memoria

PARAM imprime `param x, i`. El siguiente CALL de aridad n consume exactamente
las posiciones 0..n-1, una vez cada una, dentro del mismo bloque. No exige orden
físico ascendente. No admite parámetros pendientes al terminar el bloque ni
atravesar otro CALL. Puede haber instrucciones de cálculo entre PARAM: sus
lecturas siguen limitando el reordenamiento. Las llamadas anidadas deben
resolverse primero a temporales antes de formar los argumentos de la exterior.
La forma valida el grupo; no comprueba la firma semántica del callee.

TacProgram presta `global_scope`. `tac_is_global(program, operand)` solo acepta
VARIABLE de clase SYMBOL_VARIABLE cuyo puntero aparece en
`global_scope->symbols`; esa lista contiene también funciones, que se excluyen.
El ámbito es obligatorio y debe ser SCOPE_GLOBAL, incluso si está vacío.
El productor real debe pasar el ámbito global del semántico. Un ámbito vacío
en una prueba manual declara que no hay globales; el productor es responsable
de suministrar la lista completa. `tac_program_new` con NULL o un ámbito de otra
clase termina con diagnóstico, como los fallos de los demás constructores;
no devuelve NULL y ese caso no se prueba en el mismo proceso. `tac_validate`
rechaza un programa alterado para dejar `global_scope` en NULL, y la validación
compartida con `tac_rebuild_edges` lo rechaza antes de reemplazar aristas.
No se añade Scope a cada operando ni se modifica Symbol. CALL y jas usan/definen implícitamente todas las
globales; RETURN y jefR de retorno las usan. Por ejemplo:

```text
g = 5
call f, 0
g = 6
```

DCE debe conservar `g = 5`: f puede leerlo. Tampoco puede suponer que g conserva
el valor 5 después de la llamada. Los análisis enumeran las variables del ámbito
global y suman estos efectos a las consultas explícitas.

`tac_uses`/`tac_definitions` devuelven hasta tres valores explícitos, sin
repeticiones, en orden D/A/B. No incluyen constantes, etiquetas, funciones,
memoria implícita, globales implícitas ni registros implícitos de la ABI.
Es obligatorio consultar también TacOpInfo; no basta con esos tres valores.
Una escritura a rz no define ningún valor.

En LOAD/STORE, una base VARIABLE debe ser un arreglo declarado (SYMBOL_VARIABLE
con dimensiones). Representa su dirección estática: no aparece en tac_uses ni
requiere asignarle un registro para su contenido. Una base TEMP sí es un valor.
El desplazamiento se expresa en bytes; el productor calculará escalado y
linealización. TAC_ADDRESS permite `t0 = address a`, también útil al pasar a.

Un parámetro arreglo guarda una dirección recibida: primero se lee a TEMP,
`t0 = p`, y luego `t1 = t0[i]`. No puede usarse directamente como base simbólica.
R también admite COPY con el parámetro arreglo como destino para redefinir su dirección.
Es coherente con `cg_array_base`/`cg_array_pass`: para un arreglo declarado se
materializa su dirección; para un parámetro se carga el puntero de su casilla;
al pasar un arreglo se escribe la dirección, no sus elementos.

## Nivel ISA y ABI de ccom P1

El listado ISA es diagnóstico: sus cabeceras, sucesores y etiquetas B0/B1/B2
no forman un ensamblador para entregar entero a `cg_op`. El adaptador de C2.3
debe asignar etiquetas únicas entre funciones (hoy B2 frente a `.L_*` del
generador) y emitir instrucción por instrucción después de asignar registros.
Los operandos físicos usan sintaxis aceptada por `cg_op`, por ejemplo
`jas rl, .L_func_aux` y `lw t0, 4(rx)`; se conservan comas con espacio,
desplazamientos decimales explícitos (incluido cero) y selectores ISA.
El generador actual no es uniforme en sus nombres: `codegen_arith.c` y
`codegen_array.c` usan también `r%d`. La equivalencia es de sintaxis, no byte
a byte. `vN` no es un registro aceptado por el parser físico; los símbolos
de funciones tienen el prefijo `.L_func_` y las etiquetas de bloques, B.

TAC_ISA referencia una fila prestada de isa_table. No duplica su tabla de
lecturas, escrituras y efectos. Sus campos normalizados son:

| Forma ISA | D / A / B | Datos adicionales |
| --- | --- | --- |
| R3 y FEIS | destino / fuente / fuente | feis: k/round |
| IMM | destino / fuente / inmediato | — |
| MOV | destino / — / inmediato | — |
| BRANCH | — / fuente / fuente | target LABEL |
| LOAD | destino / base / — | displacement en bytes |
| STORE | — / base / dato | displacement en bytes |
| JEF | — / — / — | target LABEL |
| JAS | PHYS rl / — / — | target FUNCTION |
| JEFR | PHYS rz / PHYS rl / — | solo retorno |
| RS1 y KV_WRITE | — / fuente / — | kv_write: k/w |
| NONE | — / — / — | nopi, logout, kv_lock |

Inmediatos y desplazamientos ISA deben caber en 18 bits con signo. Los saltos
siguen siendo simbólicos; sus desplazamientos en bundles se resolverán al final.
ifmov y feis usan y definen el destino: `reads | preserves` de IsaSemantics.
El destino previo debe sobrevivir al renombramiento (si se cambia su identidad,
hay que preservar su valor) y al spilling (hay que cargarlo). C0 conserva sus
lecturas físicas separadas; esta regla no altera su calendario.

La ABI P1 permite que el llamado destruya t0–t11 físicos, máscara 0xfff0.
CALL y jas exponen `call_clobbers`; no significa destruir los TEMP virtuales.
El llamador debe conservar los virtuales vivos que terminen en esos registros.
C2.3 implementará la secuencia de llamada; esta tarea solo expresa el contrato.
Parámetros y retorno pasan por las casillas del área estática de la función.

rz/r0 es cero; rl/r1, enlace; rx/r2, pila; rg/r3 no se usa por la ABI P1.
`docs/isa.md` §8 describe rx como temporal libre y rg como puntero global.
ccom P1 reserva rx como pila (`generate_support.c:26,243-259`) y no usa rg:
es una convención del compilador, no una restricción del hardware.
CALL de alto nivel resume conservadoramente lecturas/escrituras de rx y rl;
jas define rl explícitamente, necesita rx y destruye t0–t11. En C2.3 los push,
pop y accesos de parámetros tendrán dependencias explícitas sobre rx. RETURN
abstracto deja esa implementación para C2.3; el jefR ISA usa rl explícitamente.

El RETURN de main sigue siendo RETURN abstracto: C2.3 debe bajarlo a la salida
del programa, nunca a jefR. Se excluyen saltos indirectos, listas de destinos,
marcas de pausa del pipeline y banderas de argumentos: Calser no los necesita
en esta forma. `auth` congela el pipeline en hardware; no se insertan esperas
artificiales. El ensamblador general sigue admitiendo más formas de jas/jefR;
la representación solo admite jas rl a FUNCTION y jefR rz, rl.

## Bloques, edición y validación

El programa posee funciones, cada una ligada a su Symbol. Cada función posee
bloques y su primer bloque es entry. Los bloques poseen instrucciones y vectores
explícitos de sucesores/predecesores. Se recorre con first/next en cada nivel,
en orden de lista. Esas listas deben ser acíclicas y contener memoria viva;
el grafo sí puede tener ciclos y autoaristas.

GOTO/jef terminan bloque y tienen un sucesor; IF/branch termina con sucesores
[destino tomado, siguiente bloque], sin duplicar si coinciden. RETURN/jefR
terminan y no tienen sucesores locales. Sin terminador se cae al siguiente.
El último bloque de cada función debe terminar en RETURN/GOTO (o jefR/jef).
Un IF último es inválido porque no hay caída. El programa vacío es válido,
pero una función sin bloques no lo es. No se exige alcanzabilidad, SSA ni
análisis de tipos o definiciones anteriores.

CALL y jas de llamada NO terminan bloque: son barreras interiores. Es una
desviación deliberada de Aho §8.4.1. El grafo no mezcla bloques del callee con
los del llamador. El organizador final puede cortar regiones dentro del bloque.

| API de edición | Contrato |
| --- | --- |
| tac_insert_after | previo NULL: inicio; actualiza first/last; previo ajeno: NULL |
| tac_remove | quita y libera nodo propio; mantiene extremos; ajeno/NULL: 0 |
| tac_block_insert_after | previo NULL: inicio y nueva entry; etiqueta única |
| tac_connect | ambos sentidos, sin duplicados; conserva orden de llamadas |
| tac_disconnect | quita ambos sentidos, conserva orden restante |
| tac_rebuild_edges | verifica toda la función, luego reemplaza aristas; ante error conserva las anteriores |

`tac_cfg.c` define una sola función privada de sucesores esperados, usada por
validación y reconstrucción. Una edición puede dejar temporalmente inválido
el programa; guardar next antes de quitar el nodo visitado, reconstruir al
terminar y llamar tac_validate. Esta devuelve 1 o 0 y diagnóstico legible.
La liberación del programa no libera Scope, Symbol ni filas ISA prestadas.

## Orden de pasadas y responsabilidades

**unrolling → DCE → expansión a operaciones del ISA con temporales virtuales
→ reordenamiento + renombramiento → asignación con spilling
→ calendarización en bundles con registros físicos**.

Unrolling y DCE trabajan en alto nivel. C2.3 pasa a ISA virtual y crea los
bloques que requiera la expansión. Reordenamiento/renombramiento y spilling
trabajan sobre ISA; la asignación deja ISA física y la calendarización forma
bundles. El spilling puede agregar cargas/almacenamientos antes de calendarizar.
`-O0` seguirá usando el generador actual; `-O1/-O2` usarán este camino cuando
se implementen. C2.1 construye e imprime alto nivel, pero todavía no implementa
expansión, asignación, optimizaciones ni adaptador al organizador.

## Guía y sexto ejemplo, paso a paso

Para Gabriel, Saddy y Kevin: crear Scope global obligatorio (puede estar vacío), programa con
`tac_program_new`, Symbol de función prestado y `tac_function_new`. Crear
bloques con `tac_block_new`, temporales con `tac_temp_new`, operandos con los
constructores y cuádruplas con `tac_append`. Para ISA, fijar stage y usar
`tac_append_isa`. Los selectores nuevos son -1; asignar los aplicables antes
de reconstruir/validar. Imprimir antes/después de la pasada y liberar el
programa antes de destruir los símbolos. No comparar identidades por texto.

`pruebas/tac/ejemplo.cls`, compilado por check-tac, contiene:

```text
F main [] : void ;
    Vm res : int ;
    res = 3 ;
    C [ res < 10 ] ;
        res = res * 2 ;
    .
    E ;
.
```

1. Crear función main con su Symbol y bloques B0, B1 (salida), B2 (cuerpo).
2. B0: COPY res,3 y IF_LT hacia B2. Su caída es B1.
3. B1: RETURN sin valor. B2: MUL res,res,2 y GOTO B1.
4. Reconstruir aristas, validar e imprimir:

```text
function main (entry B0)
B0: successors [B2, B1]
  res = 3
  if res < 10 goto B2
B1: successors []
  return
B2: successors [B1]
  res = res * 2
  goto B1
```

`make check-tac` compara seis casos de alto nivel contra esperado.txt y los
casos ISA virtual/física contra esperado_isa.txt. Los binarios C incluyen
aserciones de consultas, mutación y rechazos de formas inválidas. Make usa
solo esos binarios, diff y ccom; no Python. La suite forma parte de make check.

## Construcción desde el AST validado (C2.1, enunciado §4.1)

`include/tac_build.h` recibe un AST que ya pasó el semántico y el ámbito real
`scopes_global()`. Usa los Symbol anotados, nunca vuelve a resolver nombres.
`TacBuildResult` posee el TacProgram, la imagen inicial y los Symbol auxiliares;
se libera antes de `semantic_free`. No modifica AST, ámbitos, direcciones,
etiquetas ni estado del generador. La construcción solo ocurre con `-i` o `--tac`.

La imagen inicial se imprime como `initial_data bytes=N`, seguida de palabras
`@desplazamiento = valor` en hexadecimal. Conserva el mismo tamaño y palabras
que la imagen de globales del generador. `ast_constant` decide qué es constante,
con la política compartida. Las celdas globales no constantes empiezan en cero
y se asignan al entrar al cuerpo de main. Las declaraciones locales se evalúan
cada vez que se ejecutan; los elementos sin inicializador se ponen en cero.
R main vuelve al cuerpo y repite las inicializaciones globales dinámicas,
pero no vuelve a aplicar la imagen inicial.

Los bloques se crean durante la traducción (Aho §8.4.1), con caída según el
orden de lista. Cada función termina con RETURN implícito si queda un camino
abierto; `tac_rebuild_edges` y `tac_validate` comprueban el resultado. Puede haber
bloques sin predecesores: esta etapa no elimina código muerto.

Cada lectura escalar se captura en un temporal antes de evaluar el operando
siguiente (Aho §6.4). En asignaciones, el valor derecho se evalúa antes que los
índices del destino. Los índices se capturan de izquierda a derecha; una matriz
usa `(fila * columnas + columna) * 4` bytes (Aho §6.4.3). Las llamadas capturan
sus argumentos antes de formar el grupo PARAM; los CALL anidados se resuelven
primero. Arreglos declarados pasan por ADDRESS; parámetros arreglo leen la
dirección recibida mediante COPY.

`&` y `|` crean saltos y una unión con el temporal del resultado: el lado
derecho solo se ejecuta cuando corresponde (Aho §6.6). No se traducen a
TAC_AND/TAC_OR de evaluación plena. C/A/residual prueba las ramas en orden;
L reevalúa la condición. S captura inicio y fin antes de escribir el control,
compara con extremo exclusivo según el signo del paso y conserva el fin en un
Symbol auxiliar `$limite_SN`, en la dirección `limit_address` que ya reservó el
semántico. Ese Symbol pertenece al resultado, no a los ámbitos prestados.

Una llamada entra por el bloque que inicializa el retorno a cero. R captura
todos sus argumentos antes de escribir parámetros mediante COPY, incluidos los
parámetros arreglo, y salta al cuerpo sin reiniciar el retorno ni crear CALL.
La copia completa de arreglos se traduce como lazo ascendente de LOAD/STORE;
el unrolling la verá como otro lazo del CFG. Las siete CRYPTO conservan una
operación por sentencia y los selectores literales, sin expansión al ISA.

### Factorial desde el AST hasta los bloques

`pruebas/programas/factorial_r.cls` contiene el factorial con dos acumuladores.
Su AST tiene FUNCTION factorial, PARAMS n/acc, RETURN_TYPE r y un BLOCK con
CONDITIONAL: condición LESS_EQUAL(n,1), rama ASSIGNMENT(r,acc) y EMIT,
y RESIDUAL con REWIND factorial(n-1,acc*n).

1. B0 inicializa r a cero y salta a B1, entrada del cuerpo.
2. B1 captura n y calcula la condición; la caída B4 devuelve acc.
3. B3 captura n-1 y acc*n antes de escribir n y acc, y vuelve a B1.
4. B2 conserva el RETURN implícito del final, aunque aquí no tiene predecesores.
5. main inicializa f, pasa 3/1 y conserva el resultado inmediato del CALL.

El esperado completo está en `pruebas/tac_build/esperados/factorial_r.tac`.
Su función factorial se imprime así:

```text
initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B4]
  t0 = n
  t1 = t0 <= 1
  if t1 == 0 goto B3
B4: successors []
  t2 = acc
  r = t2
  return r
B3: successors [B1]
  t3 = n
  t4 = t3 - 1
  t5 = acc
  t6 = n
  t7 = t5 * t6
  n = t4
  acc = t7
  goto B1
B2: successors []
  return r

```

`make check-tac-build` verifica memoria y esperados de 22 programas, incluido R
con arreglos globales, locales, matrices y parámetros reenviados. Estas pruebas
de C1 no ejecutan TAC; las pruebas de valores de C2 usan el intérprete descrito abajo.
El `.tac` sigue siendo un listado diagnóstico, no un formato deserializable ni
un archivo para entregar entero a `cg_op`.

## Intérprete de pruebas de alto nivel (C2)

`make check-tac-values` ejecuta `pruebas/tac_build/probar_valores.py` y el auxiliar
`build/interpretar_tac`. Importa la tabla CASES de `pruebas/programas/probar.py`,
sin duplicar sus valores, y compara las variables finales de los 22 programas
sin CRYPTO. P09 y P11 se listan como omitidos, sin sumarlos a aprobados.

`pruebas/tac_build/interpretar.h` recibe un TacBuildResult en memoria, nunca el
texto `.tac`. Carga su imagen en una memoria estática compartida de 64 KiB y
accede a variables mediante Symbol.address. No crea locales nuevas al llamar;
solo temporales y capturas PARAM pertenecen a cada invocación. La pila explícita
permite CALL/RETURN, mientras R conserva temporales y vuelve por el GOTO del TAC.
LOAD/STORE/ADDRESS usan bytes; COPY de parámetro arreglo lee/escribe su dirección.

La aritmética se trunca a 32 bits. Comparaciones y DIV/MOD usan signo; DIV trunca
hacia cero y MOD conserva el signo del dividendo. División/módulo por cero,
exponente negativo, límite de pasos y forma inválida tienen estados distintos
de la finalización normal. El presupuesto de pasos es global para las llamadas.
CRYPTO se rechaza explícitamente antes de ejecutar el programa, incluso si no
se alcanzaría; no se simula AUTH, bóveda, ancla, cifrado ni pipeline. Los errores
no publican valores como si fueran resultados normales.

Para comprobar una optimización de alto nivel, ejecutar `tac_interpret(build,
limite)` antes de la pasada y conservar el resultado. Aplicar la pasada,
reconstruir aristas y validar; ejecutar de nuevo y comparar estado terminal y
las variables/memoria observables seleccionadas por el caso de prueba. Cada
invocación del intérprete parte de una memoria nueva con la misma imagen; no
reutiliza el estado de la ejecución anterior ni modifica el programa. No hace
falta que coincidan el número de pasos o los temporales internos: el presupuesto
debe ser suficiente para ambas versiones. Liberar ambos resultados con
`tac_run_free`, y liberar TacBuildResult antes del semántico.

Esto permite detectar diferencias de traducción o de optimización sin generar
ISA desde TAC. El intérprete solo se enlaza con el auxiliar de pruebas, no con
ccom. Los controles adicionales verifican orden de evaluación, signos y desborde,
errores aritméticos, rechazo de CRYPTO y detención por presupuesto.
