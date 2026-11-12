# Representación intermedia de tres direcciones

C2.1a define la forma construible a mano; C2.1 agrega su construcción desde el
AST validado y la bandera `-i` / `--tac`. El generador actual sigue produciendo
los binarios, sin consumir TAC. No se introducen optimizaciones. Referencias: Aho
§6.2 (cuádruplas), §8.4.1 (bloques) y §10.2.3 (registros virtuales).
La lista de instrucciones del organizador existente sigue siendo independiente.

## Niveles y etapas

| Etapa de `TacProgram.stage` | Operaciones | Registros y valores |
| --- | --- | --- |
| `TAC_STAGE_HIGH` | Alto nivel; sin `TAC_ISA` ni `TAC_PSEUDO` | TEMP, VARIABLE, INTEGER, LABEL, FUNCTION y NONE según posición; ningún PHYS |
| `TAC_STAGE_ISA_VIRTUAL` | `TAC_ISA` y `TAC_PSEUDO` | TEMP y únicamente PHYS rz/r0, rl/r1 y rx/r2; inmediatos y objetivos según forma |
| `TAC_STAGE_ISA_PHYSICAL` | `TAC_ISA` y `TAC_PSEUDO` hasta expandir; solo `TAC_ISA` antes de emitir | PHYS r0–r15; ningún TEMP |

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
etiquetas deben pertenecer a la función. `block->label` es un ID interno estable,
independiente del orden de la lista; `next_label` reserva IDs sin reutilizarlos.
El número de presentación B0, B1, B2… es la posición del bloque en la lista
de cada función, tanto en listados como en diagnósticos. Imprimir no modifica
IDs, punteros ni aristas; los sucesores conservan el orden [destino tomado, caída].
Si un diagnóstico encuentra un bloque sin función o fuera de su lista, usa su
ID interno porque no hay una posición válida.
Las constantes de alto nivel no se truncan: C2.3 deberá aplicar la semántica de
32 bits y materializar las que no caben en un inmediato de 18 bits con signo.
La selección de pasos de materialización vive en `isa_constant_plan`
(`include/isa_constant.h`, `src/isa_constant.c`), compartida entre `gen_mov` y la
bajada a ISA virtual. Devuelve uno o cuatro pasos con roles DEST/SCRATCH,
sin elegir registros, bundles ni huecos. El generador conserva su empaquetado P1;
la bajada vincula esos roles a TEMP y propaga línea/columna.

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
filas. `tac_op_info` devuelve una fila inmutable de alto nivel; para TAC_ISA y
TAC_PSEUDO se usa `tac_instruction_info`. Las reales toman la semántica de
`isa_table`; las pseudo toman su contrato de expansión de `tac_pseudo.c`.
`may_trap` exige que DCE conserve DIV/MOD/POW salvo prueba de que no fallan.
`may_create_blocks` avisa de una expansión que puede producir flujo interno,
incluidos lazos. La expansión final sucede después de asignar registros:
no obliga a crear esos bloques en el TAC virtual.
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

### Contrato de TAC_PSEUDO para C3.3 y C3.4 (C2.3 A)

`TAC_PSEUDO` representa una operación de destino pendiente de expansión, nunca
una instrucción hardware nueva. `TacPseudoData.kind` distingue MUL, DIV, MOD,
POW, AUTH, LOGOUT, KV_WRITE, KV_LOCK, SANC, CIFRAR, DESCIFRAR y EXIT.
NONE solo indica metadatos ausentes en instrucciones de otra clase y es inválido
como discriminante de TAC_PSEUDO. No se agregan filas a `isa_table`.

Se comparten `dest`, `src1`, `src2` y `selectors` de `TacInstruction`.
Los argumentos son registros materializados, nunca VARIABLE ni INTEGER.
Se admiten TEMP en virtual y PHYS según la etapa; la aritmética solo admite
TEMP en virtual y t0–t11 en física, por el contrato de `cg_arith_emit`.
Los selectores presentes son literales 0–3; los ausentes valen -1.
Los metadatos ISA y PSEUDO no pueden mezclarse en una misma instrucción.

| PSEUDO | D / A / B | Usa | Define | Auxiliares | Barrera / estado | Memoria |
| --- | --- | --- | --- | --- | --- | --- |
| MUL | resultado / fuente / fuente | A, B | D | 8 | no / no | — |
| DIV, MOD, POW | resultado / fuente / fuente | A, B | D | 8 | no / no; may_trap | — |
| AUTH, SANC | — / valor / — | A | — | 0 | sí / sí | — |
| LOGOUT, KV_LOCK | — / — / — | — | — | 0 | sí / sí | — |
| KV_WRITE | — / valor / —; k/w | A | — | 0 | sí / sí | — |
| CIFRAR, DESCIFRAR | origen / destino / bloques; k | D, A, B | — | 0 | sí / sí | lectura y escritura |
| EXIT | — / — / — | globales implícitas | — | 0 | sí / no | — |

Las pasadas consultan **siempre** `tac_instruction_info`, `tac_uses` y
`tac_definitions`, igual que para TAC_ISA: no deben identificar MUL o CRYPTO
por su opcode para reconstruir sus efectos. Las consultas de valores explícitos
no incluyen memoria, globales, auxiliares ni registros destruidos; se agregan los
resúmenes de `TacOpInfo`. Las siete CRYPTO son no eliminables y conservan orden
por su estado oculto. DIV/MOD/POW pueden fallar; DCE necesita demostrar que no
fallan antes de eliminarlas. MUL sigue siendo pura y ninguna aritmética es barrera.

`TacOpInfo.scratch_count` vale 8 en las cuatro aritméticas, en ambas etapas ISA.
En virtual, `pseudo.scratch_clobbers` es cero: no se han elegido físicos.
En física contiene exactamente ocho bits de t0–t11, distintos de fuentes y
destino; el destino tampoco puede coincidir con una fuente. Las dos fuentes
sí pueden coincidir entre sí. Es el contrato físico de `cg_arith_emit`; el futuro
asignador/emisor debe resolver coincidencias y conservar vivos antes de dejar
válida esta forma. La máscara se consulta como `TacOpInfo.call_clobbers`.
El contador y la máscara describen los mismos auxiliares: no se suman como
dos reservas diferentes. Esa máscara no incluye la definición explícita D.

Para CRYPTO, `call_clobbers` resume conservadoramente t0–t11 (`0xfff0`),
incluido el trabajo de preparar argumentos para las plantillas; `scratch_count`
es cero porque no solicita auxiliares elegibles como la aritmética. En las CRYPTO
y EXIT, `pseudo.scratch_clobbers` debe ser cero. EXIT no destruye temporales,
solo es válido en main, termina sin sucesores locales y se expandirá a la salida
del programa, nunca a `jefR rz, rl`. Conserva las lecturas globales de RETURN.

`may_create_blocks` permanece activo para aritméticas y cifrar/descifrar:
la expansión P1 posterior puede emitir lazos internos. Los guardados y la
recuperación son responsabilidad de asignación/emisión posteriores; A no los
implementa ni cambia la semántica de memoria general de la operación.

`tac_validate` admite PSEUDO en ISA_VIRTUAL e ISA_PHYSICAL para permitir las
pasadas anteriores a expansión. **Antes de emitir**, el productor debe llamar
`tac_validate_for_emission`: exige programa válido, etapa ISA_PHYSICAL y ninguna
TAC_PSEUDO pendiente. No expande ni corrige el programa. El emisor de C2.3 aún
no existe en A; el generador actual no consume este programa.

El listado diagnóstico usa `pseudo.mul v0, v1, v2`,
`pseudo.kv_write v1, 2, 3`, `pseudo.cifrar v0, v1, v2, 2` o `pseudo.exit`.
KV_WRITE imprime valor, k, w; no es la sintaxis de sentencia Calser.
Este texto no es ensamblador para `cg_op` y no altera el impreso HIGH.

Cada instrucción conserva `source_line` y `source_column`; cero indica posición
desconocida. `tb_emit` copia ambas desde el mismo nodo AST, o cero si no hay nodo.
Las futuras traducciones deben propagarlas para diagnósticos archivo:L:C.
Los constructores manuales comienzan con ambos campos en cero y el listado
no imprime la posición. Referencias: Aho §8.1–8.2, §8.6, §8.8 y §9.2.5.

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

**unrolling → DCE → nivel ISA (MUL/DIV/MOD/POW y CRYPTO como una instrucción)
→ reordenamiento + renombramiento → asignación de registros (con spilling)
→ expansión con el código del P1 → calendarización en bundles**.

| Pasada | Qué recibe y qué conserva |
| --- | --- |
| Unrolling y DCE | HIGH: operaciones Calser, variables y TEMP tN |
| Selección de C2.3 | HIGH → ISA_VIRTUAL: instrucciones reales y PSEUDO con TEMP vN |
| Reordenamiento y renombramiento (C3.3) | ISA_VIRTUAL; PSEUDO sigue siendo una instrucción, con sus efectos consultables |
| Asignación y spilling (C3.4) | ISA_VIRTUAL → ISA_PHYSICAL; resuelve auxiliares y conserva valores según las máscaras |
| Expansión P1 | ISA_PHYSICAL con PSEUDO → instrucciones reales; agrega guardado/recuperación y flujo interno |
| Calendarización | Solo ISA real con físicos; organiza bundles y conserva las plantillas fijas del P1 |

El spilling puede agregar cargas/almacenamientos antes de la expansión final y
la calendarización. A solo define forma, consultas, validación y posición fuente;
no implementa selección, asignación, expansión ni las banderas de optimización.
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
2. B1 captura n y calcula la condición; la caída B2 devuelve acc.
3. B3 captura n-1 y acc*n antes de escribir n y acc, y vuelve a B1.
4. B4 conserva el RETURN implícito del final, aunque aquí no tiene predecesores.
5. main inicializa f, pasa 3/1 y conserva el resultado inmediato del CALL.

El esperado completo está en `pruebas/tac_build/esperados/factorial_r.tac`.
Su función factorial se imprime así:

```text
initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = n
  t1 = t0 <= 1
  if t1 == 0 goto B3
B2: successors []
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
B4: successors []
  return r

```

`make check-tac-build` verifica memoria y esperados de 22 programas, incluido R
con arreglos globales, locales, matrices y parámetros reenviados. Estas pruebas
de C1 no ejecutan TAC; las pruebas de valores de C2 usan el intérprete descrito abajo.
El `.tac` sigue siendo un listado diagnóstico, no un formato deserializable ni
un archivo para entregar entero a `cg_op`.

## Bajada a ISA virtual (C2.3 B)

`include/tac_lower.h` expone `tac_lower(high, error, capacity)`. Recibe el
programa HIGH de `TacBuildResult` y devuelve otro `TacProgram` propio, en etapa
ISA_VIRTUAL y validado. Ante forma o anotaciones insuficientes devuelve NULL;
los diagnósticos de selección incluyen `linea L:C`. No modifica HIGH, AST,
símbolos, imagen inicial ni estado del generador. Requiere símbolos semánticos
con direcciones, `param_list` y `decl_node` anotados, no solo nombres.

Ambos programas comparten símbolos y ámbito global prestados. La imagen inicial
y los símbolos auxiliares de S siguen perteneciendo a `TacBuildResult`.
Liberar primero ISA con `tac_program_free`, después el resultado HIGH con
`tac_build_free` y al final el semántico. La salida tiene bloques propios:
sus objetivos se mapean por puntero, conservando orden y caída de la función.
Nunca se interpreta Bn como identidad ni se ordenan los sucesores.

Cada TEMP de HIGH conserva su ID local en la función nueva. Los virtuales
adicionales se reservan desde `next_temp`; no hay límite de doce ni registros
físicos asignables en esta etapa. Cada instrucción seleccionada hereda línea
y columna de su instrucción HIGH, incluso los pasos de constantes y direcciones.
Una posición desconocida sigue siendo cero.

La obtención de fuentes usa `rz` para INTEGER exactamente cero cuando el
consumidor admite registros fijos, sin reservar un virtual. No propaga ceros
conocidos de TEMP ni VARIABLE. Las PSEUDO aritméticas exigen fuentes TEMP por
el contrato de `cg_arith_emit` (registros físicos 4–15); esta política se elige
por sus propiedades de auxiliares. CRYPTO admite `rz` en sus fuentes de valor,
pero conserva los selectores como literales. Las bases simbólicas de dirección
cero usan el mismo ayudante. COPY hacia TEMP y ADDRESS conservan la definición
del destino, incluso con `mov vN, 0`; ADD con inmediato derecho cero conserva
`saddi`. El plan compartido de constantes no cambia.

La tabla siguiente usa `g@32`, `a@64`, parámetro 0 en dirección 8 y retorno en 4
como ejemplos; vN es ilustrativo. Las fuentes VARIABLE se cargan antes de la ALU
y los resultados VARIABLE se escriben después. No hay plegado, DCE ni reducción
de fuerza: incluso multiplicar un índice por cuatro conserva una PSEUDO MUL.

| HIGH | Selección | Ejemplo ISA virtual |
| --- | --- | --- |
| COPY de literal | materialización directa | `t0 = 7` → `mov v0, 7` |
| COPY de TEMP | ADD con rz | `t1 = t0` → `add v1, v0, rz` |
| COPY desde VARIABLE | LW de su casilla | `t0 = g` → `lw v0, 32(rz)` |
| COPY hacia VARIABLE | SW del valor materializado | `g = t0` → `sw v0, 32(rz)` |
| ADDRESS | materializa la dirección, no el contenido | `t0 = address a` → `mov v0, 64` |
| ADD | ADD o SADDI si la fuente derecha cabe en 18 bits | `t2 = t0 + 1` → `saddi v2, v0, 1` |
| SUB | SUB | `sub v2, v0, v1` |
| AND | AND | `and v2, v0, v1` |
| OR | OR | `or v2, v0, v1` |
| XOR | XOR | `xor v2, v0, v1` |
| EQ | SEQ | `seq v2, v0, v1` |
| NE | SNE | `sne v2, v0, v1` |
| LT | SLT | `slt v2, v0, v1` |
| LE | SLE | `sle v2, v0, v1` |
| GT | SLT con fuentes intercambiadas | `slt v2, v1, v0` |
| GE | SLE con fuentes intercambiadas | `sle v2, v1, v0` |
| NEG | SUB desde cero | `sub v1, rz, v0` |
| NOT | SEQ contra cero | `seq v1, v0, rz` |
| LOAD, base simbólica | dirección absoluta si cabe | `t0 = a[4]` → `lw v0, 68(rz)` |
| LOAD, offset constante | LW con desplazamiento | `t1 = t0[4]` → `lw v1, 4(v0)` |
| LOAD, offset calculado | ADD de dirección y LW | `add v3, v0, v1; lw v2, 0(v3)` |
| STORE | dirección seleccionada y SW | `t0[4] = t1` → `sw v1, 4(v0)` |
| IF_EQ | BEQ | `beq v0, v1, B2` |
| IF_NE | BNE | `bne v0, v1, B2` |
| IF_LT | BLT | `blt v0, v1, B2` |
| IF_LE | BGE con fuentes intercambiadas | `bge v1, v0, B2` |
| IF_GT | BLT con fuentes intercambiadas | `blt v1, v0, B2` |
| IF_GE | BGE | `bge v0, v1, B2` |
| GOTO | JEF a LABEL propio | `jef B2` |
| PARAM | SW a `callee.param_list[posición]` | `param t0, 0` → `sw v0, 8(rz)` |
| CALL | JAS y captura inmediata del resultado | `jas rl, .L_func_f; lw v0, 4(rz)` |
| RETURN de función | casilla de retorno y JEFR | `sw v0, 4(rz); jefR rz, rl` |
| RETURN de main | PSEUDO EXIT | `pseudo.exit` |
| MUL/DIV/MOD/POW | una PSEUDO con fuentes materializadas | `pseudo.mul v2, v0, v1` |
| Cada CRYPTO | una PSEUDO con valores y selectores | `pseudo.cifrar v0, v1, v2, 2` |

Para una base simbólica se combina su dirección con un offset inmediato cuando
el acceso resultante cabe en 18 bits. En los demás casos se materializa la base;
si el offset tampoco cabe, se materializa y suma a la base antes del LW/SW con
desplazamiento cero. Un parámetro arreglo se lee y redefine como puntero en su
casilla. El constructor ya capturó los argumentos de R antes de esas escrituras;
la bajada conserva sus COPY y el salto al cuerpo, sin llamada nueva.

`tl_constant` consume `isa_constant_plan`, sin copiar su descomposición. DEST
es el TEMP del resultado y SCRATCH, otro TEMP. Sus pasos son TAC_ISA reales;
no agrega bundles ni huecos. Por ejemplo, 200000 se construye como
`mov v0, 6; mov v1, 15; sll v0, v0, v1; saddi v0, v0, 3392`.
Un NEG presente en HIGH se conserva aunque su operando sea constante.

Cada PARAM encuentra el CALL de su grupo y usa la posición explícita, sin
suponer orden físico 0,1,2. La aridad debe coincidir con la firma semántica.
CALL captura el retorno antes de cualquier otra llamada; si se descarta, no
lo carga. RETURN no recarga una casilla que ya es el resultado de la función;
si recibe otro valor, lo escribe antes de JEFR. Main siempre termina con EXIT.

`make check-tac-lower` prueba selección manual y todo el corpus válido, incluidos
CRYPTO y los cuatro R con arreglos. Compara doce listados y captura HIGH,
sus metadatos, aristas e imagen antes/después sin normalización.

Esta etapa se enlaza solo con los binarios de prueba, no con ccom. No cambia
`-i`, el intérprete ni la CLI. Quedan para los siguientes commits liveness,
asignación, conservación de vivos/rl, expansión P1 y emisión.
Por eso la salida virtual aún no es un programa físico ejecutable: el control
`tac_validate_for_emission` la rechaza. Referencias: Aho §7.2, §8.1–8.2 y §8.6.

## Plantillas CRYPTO compartidas con el generador P1

`cg_crypto_emit_template` (`codegen_crypto.h`) recibe una `CGCryptoTemplate`
con operación, tres registros físicos y selectores `k/w`, más un puntero al
contador de etiquetas único por programa. No recibe AST ni callback, no mueve
argumentos y no guarda/restaura vivos. Valida forma, registros fijos, selectores
y contador antes de emitir; un rechazo no modifica emisión ni contador.
`cg_crypto` conserva el recorrido del AST para -O0: validación previa, guardado,
evaluación, llamada a la misma plantilla y restauración. `emit_loop` y sus
bundles fijos se comparten sin duplicación.

El futuro emisor de -O1 deberá preservar los vivos afectados **antes** de
sobrescribir los registros fijos; colocar origen/destino/bloques en `{6, 7, 9}`
(t2/t3/t5), o token/palabra/ancla en `{10, -1, -1}` (t6); y resolver movimientos
como copias paralelas, incluidos ciclos y fuentes `rz`. Para operaciones sin
valores se usa `{-1, -1, -1}`. Los selectores presentes son 0–3 y los ausentes -1.
Debe garantizar la separación requerida antes de que la plantilla lea sus
fuentes, conservar un contador compartido por todo el programa y recuperar
los vivos después de emitir. El lazo modifica r4–r8 y lee r9. El resumen
conservador `call_clobbers = 0xfff0` sigue incluyendo la preparación; no cambia.

Las etiquetas `.L_crypto_*` mantienen su formato. Cifrar/descifrar consumen un
identificador, aun cuando los bloques en ejecución sean cero o negativos;
rechazan el contador `UINT_MAX`. Las demás operaciones no lo incrementan.
La preparación física, spilling y emisión desde TAC quedan para los siguientes
commits: esta separación no conecta -O1 con el ensamblador.

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
