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
(`include/isa/isa_constant.h`, `src/isa/isa_constant.c`), compartida entre `gen_mov` y la
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

## Camino optimizante: contrato integral

Regla: **sin cambiar el comportamiento de -O0 ni romper contratos existentes;
se permiten las extensiones acordadas**. -O0 mantiene el generador y el
organizador P1. E conecta las banderas y el gestor con ccom; las cinco pasadas siguen vacías
hasta C3. C1 ofrece análisis, marcas y edición.

**AST → construcción HIGH con marcas S/L → unrolling C3.1 → renombramiento local
C3.3a → DCE C3.2 → bajada C2.3 → ISA virtual: reordenamiento C3.3b → asignación
linear scan LRU C2.3 + spilling C3.4 → ISA física: emisión y expansión P1 →
cg_schedule → ensamblador → .bin/.hex/.mem**.

Hay dos calendarizaciones: list scheduling antes de asignar (ISA virtual) y
`cg_schedule` después de expandir (físicos). La segunda también puede reordenar,
además de empaquetar, conservando los bundles fijos de CRYPTO.

| Pasada | Recibe → entrega | Restricciones | Métricas propias |
| --- | --- | --- | --- |
| C3.1 unrolling | HIGH con marcas → HIGH replicado con residuo | Factor configurable, heurística total/parcial; ignora marcas sin retroceso; respeta ROM | Lazos, factor e instrucciones antes/después |
| C3.3a renombramiento | HIGH → HIGH, local al bloque | Solo Vm escalares propias; TEMP nuevo por redefinición, usos posteriores actualizados y escritura final si sigue viva; respeta escrituras condicionales | Definiciones renombradas |
| C3.2 DCE | HIGH + vivacidad → HIGH sin muertos/inalcanzables | Raíces observables; CALL usa/define globales, RETURN las usa; no elimina stores, CRYPTO ni llamadas; DIV/MOD/POW solo con prueba de que no fallan | Instrucciones y bloques eliminados |
| C2.3 bajada | HIGH → ISA virtual | Selección, sin plegado ni DCE; HIGH intacto | Instrucciones ISA antes de expandir, MUL cuenta una |
| C3.3b reordenamiento | ISA virtual → mismo bloque con otro orden | DAG RAW/WAR/WAW de valores/memoria, direcciones rz+constante; CALL/CRYPTO barreras; nada cruza salto; respeta may_trap | Instrucciones reordenadas |
| C2.3 asignación | ISA virtual → ISA física | Linear scan sobre intervalos, registro libre más antiguo (LRU), ABI, ocho auxiliares aritméticos; integra la política de spilling antes del error definitivo | Conteos comunes de instrucciones |
| C3.4 spilling | Intervalos + profundidad natural → ISA física con cargas/guardados | Víctima que termina más lejos evitando mayor profundidad; área propia de derrame | Valores derramados, cargas y guardados |
| C2.3 emisión | ISA física → flujo de cg_op/cg_bundle/cg_label/cg_gap | Expande PSEUDO con cg_arith_emit/cg_crypto_emit_template; preserva vivos, usa temp_base alrededor de aritmética cuando corresponde; ninguna PSEUDO llega al ensamblador | Instrucciones emitidas |
| cg_schedule | ISA real física → bundles | Respeta efectos, separaciones y bundles fijos P1 | Bundles, slots NOP y bytes de instrucciones (16 por bundle) |

`may_trap` se respeta **en ambas direcciones**: no adelantar una operación que
pueda fallar por delante de un efecto observable anterior, ni atrasarla detrás
de un efecto observable posterior; conservar también el orden de fallos
potencialmente distinguibles. Esto exige aristas de efectos, no convertir toda
aritmética en barrera. Una prueba de que no falla permite relajar esas aristas.
MUL/DIV/MOD/POW son operaciones largas indivisibles en la calendarización virtual:
sus instrucciones internas y lazos solo aparecen al emitir. No se intercalan
otras instrucciones dentro de una PSEUDO ni se confunde su costo estimado con
una latencia ISA exacta. CRYPTO sigue siendo barrera por su estado oculto.

### Estado observable y vivacidad

El estado observable es: globales, variables de main (escalares/arreglos),
memoria externa 0x1000–0xFFFF y estado CRYPTO. No son observables las variables de
otras funciones, la pila ni temp_base. Las comparaciones futuras con -O0 se
hacen sobre ese estado, no sobre la asignación de registros o memoria privada.

En HIGH el cliente pasa raíces VARIABLE escalares de main/globales, vivas a la
salida de main. Arreglos y memoria externa son efectos de memoria, no valores a
asignar a registros. En ISA las variables ya son memoria: las raíces VARIABLE
se ignoran como registros; stores, llamadas y CRYPTO conservan sus efectos no
eliminables. Los metadatos siguen describiendo memoria y estado oculto.

`tac_live_analyze` calcula USE/DEF y LIVE_IN/OUT hasta punto fijo (Aho §9.2.5),
y luego vivos antes/después de cada instrucción. Las claves son Symbol por
identidad, TEMP por ID local y PHYS por número; nunca se comparan direcciones de
casillas para identificar valores. Las vistas y conjuntos pertenecen al resultado.
Conservar función/instrucciones/símbolos; liberar análisis antes de editar.

El adaptador común `tac_locations` agrega globales HIGH y registros implícitos
ISA a las consultas de tac.h. Cada instrucción separa (a) usos/definiciones y
vivacidad lógica, (b) lecturas/escrituras, clobbers y auxiliares físicos y (c)
candidatos de preservación vivos a través de una llamada/expansión. Clobbers
nunca matan un virtual. En virtual los candidatos de aritmética son conservadores:
C2 decidirá qué físicos se destruyen realmente. rz no ocupa una ubicación viva
ni un registro asignable. `preserves` de ifmov/feis sí es uso lógico del destino;
no se agrega como lectura física artificial, conservando el contrato ISA A.

La consulta `tac_variable_is_rename_candidate` recibe la lista prestada de ámbitos
(`scopes_first`). Exige SYMBOL_VARIABLE, MODIFIER_VM, dimensions=0,
limit_address=-1, propietario Scope.owner igual a la función y no global. No
modifica Symbol ni depende de nombres. Parámetros, retorno, control Vi de S y
límite auxiliar no son candidatos.

### Dos fuentes de lazos y vida de las marcas

Cada TacFunction HIGH posee `loop_marks`, creada al traducir S/L. Una marca
presta cabecera, cuerpo y latch opcional, y conserva profundidad sintáctica.
En S presta control/límite y guarda operandos de inicio/fin **ya capturados** y
paso: no contiene expresiones para reevaluar. No cambia IDs, CFG ni el .tac de -i.
Las marcas no se copian a ISA.

`tac_loops_analyze` recalcula alcanzabilidad y dominadores desde entry (Aho
§9.6.1). Retroceso significa destino que domina al origen (§9.6.4); algoritmo
9.46 (§9.6.6) para el lazo natural. Une lazos de igual cabecera antes de contar
profundidad. Los inalcanzables tienen profundidad cero y no participan. Si al
retirar retrocesos por dominancia queda un ciclo alcanzable, diagnostica grafo
irreducible. El análisis sirve en HIGH e ISA y no depende de las marcas.

La consistencia es en un solo sentido: una marca **con retroceso alcanzable**
requiere su lazo natural; puede haber lazos sin marca (R y copias de arreglos).
Las marcas sin latch o con latch inalcanzable quedan señaladas **sin retroceso**
y son válidas, por ejemplo cuando el cuerpo termina en E. Las marcas de una
cabecera inalcanzable se identifican aparte. El unrolling ignora ambos casos.
El estado se consulta en el resultado sin mutar la marca ni el CFG.

`tac_loop_marks_discard` libera la colección; también se libera al destruir la
función. El gestor de E la descartará después de unrolling, o antes de la primera
pasada invalidante si está desactivado. Las otras pasadas no mantienen marcas;
nunca deben quedar referencias a bloques liberados. El descarte explícito existe
en C1; no se implementa ni simula todavía el unrolling.

### Edición y propiedad

`tac_insert_isa_after` y `tac_insert_pseudo_after` comparten inicialización con
append. NULL como previo inserta al inicio; previo ajeno devuelve NULL. No
expanden ni asignan registros. `tac_replace_reads` prepara y valida una copia:
solo cambia posiciones reads, excluye bases simbólicas y nunca cambia etiquetas,
objetivos ni definiciones. Si coincide una lectura ligada a definición
(preserves de ifmov/feis), rechaza todo sin cambios. Devuelve éxito y cantidad;
sin coincidencias es éxito con cero.

`tac_block_remove` verifica todo antes de editar. Rechaza entry, LABEL que
quedarían apuntándolo (incluido isa.target), marcas y caídas alcanzables
alteradas; tampoco deja al último bloque sin terminación válida. Las referencias
internas del bloque retirado desaparecen con él. Desconecta aristas y conserva
extremos; retirar regiones completas requiere que el cliente retire/redirija
referencias primero. Después de editar control: tac_rebuild_edges y tac_validate.
La clonación de bloques/instrucciones corresponde a C3.1, no a C1.

### Infraestructura y entregas pendientes

| Tarea | Responsabilidad / estado |
| --- | --- |
| C2.3 C1, implementado | Vivacidad genérica y efectos separados; marcas; dominadores/lazos/profundidad; clasificación; edición y descarte |
| C2.3 C2, implementado | Intervalos reutilizables, linear scan LRU, restricciones/preservación y reserva de área de derrame |
| C3.1, pendiente | Clonación y unrolling con residuo |
| C3.2, pendiente | Eliminación con raíces observables y prueba de seguridad de errores |
| C3.3a/b, pendiente | Renombramiento local y DAG/list scheduling virtual |
| C3.4, pendiente | Spilling sobre intervalos y profundidad natural |
| C2.3 D, implementado (emisor compartido) | Emisión, ABI, preservación y expansión P1 |
| C2.3 E, implementado | Gestor con firma/contexto común, cinco pasadas registradas vacías, validación tras cada pasada, banderas, métricas y control final de ROM |
| C4.2 / I2, pendiente | Ciclos del simulador a CSV/gráficas por prueba / ciclos RTL |

El área de derrame es una extensión **no contigua** propia de cada función,
separada de temp_base y agregada después de las áreas existentes, sin reubicar
símbolos ni imagen global. Solo -O1/-O2; el gestor deberá tomar la nueva dirección
de pila y repetir el chequeo de 256 bytes libres después de reservar. R reutiliza
el área; no hay recursión ordinaria. C1 no reserva ni modifica FrameLayout.
Sin spilling, la asignación falla con `sin registros temporales disponibles`,
con archivo:L:C. C2 elige los auxiliares y prepara el plan de preservación;
D hará los guardados/restauraciones requeridos, no elegirá tarde ocho auxiliares.

### §5. Banderas y errores: contrato de E

E implementa estas banderas y métricas; las cinco pasadas están registradas
con cuerpos vacíos hasta C3. Sin bandera de nivel, sigue el camino actual `-O0`, con salida
idéntica byte a byte. `-O1` activa construcción, pasadas HIGH, bajada, pasadas ISA
virtual, asignación, emisión D, calendarización y salidas existentes.

- `--opt=lista` selecciona `unroll,rename,dce,reorder,spill` e implica `-O1`
  aunque no se escriba ese nivel. Con `-O0` explícito es error de uso,
  independientemente del orden de los argumentos.
- Repetir el mismo nivel es válido; especificar niveles distintos es error de
  uso. Varias opciones `--opt` unen sus listas sin repetir pasadas. La lista
  vacía y cualquier nombre desconocido son errores de uso.
- El orden de ejecución es fijo: `unroll`, `rename`, `dce` en HIGH, `reorder`
  en ISA virtual y `spill` como política dentro de la asignación; nunca depende
  del orden de `--opt`. E registra las cinco pasadas vacías y valida después de
  cada una. Un fallo de validación es error interno, código 1, con el nombre
  de la pasada y el archivo real.
- `-O2` significará todas las pasadas, pero se rechaza como error de uso hasta
  C3.5. `--unroll=N` exige `unroll` activo y un entero entre 2 y 16 inclusive.
  Sin factor explícito se aplica la heurística del §11; no se fija un factor
  predeterminado nuevo en E.
- `--no-schedule` se permite en `-O1`: omite el empaquetado final y conserva la
  salida conservadora. **También se comprueba el máximo de 4096 bundles sobre
  esa salida**, porque el límite de ROM se aplica a lo entregado al ensamblador,
  aunque no se haya calendarizado. Con calendarización se comprueba después de
  ella, incluyendo expansiones, preservación y NOPs. No es el límite de región
  de `cg_schedule`, ni agrega un chequeo nuevo al camino `-O0`.
- `-i`/`--tac` conserva su significado: muestra el HIGH inicial, antes de las
  pasadas, aunque haya pasadas HIGH activas.
- `--ast-json` junto con `-O1` implica compilar aun sin otras opciones de salida.
  El AST se sigue escribiendo antes del análisis semántico; no se retrasa su
  escritura. El comportamiento actual sin `-O1` se conserva.
- `-v` informa construcción, pasadas, bajada, asignación y emisión mediante
  `report_phase`. `-m` muestra las etiquetas realmente emitidas, incluidas
  `.L_o1_*`, funciones, expansiones y terminales, con sus direcciones finales:
  `cg_ir_count`/`cg_ir_record` y `cg_label_address` sobre la salida final.
  `-p` muestra los bundles de esa salida. El camino `-O0` no cambia.
- `-x` junto con `-O1`, `-O2`, `--opt`, `--unroll` o `--metrics` es error de uso.
  Todos los errores de uso de este contrato devuelven código 2.

Los errores del camino `-O1` llevan siempre el nombre real del archivo.

### §6. Métricas y JSON: contrato de E

`--metrics archivo.json` exige el nivel efectivo `-O1`, incluido el obtenido
mediante `--opt`; sin él es error de uso, código 2. Una compilación fallida no
escribe JSON de métricas. `tiempo_ms.total` cubre toda la compilación hasta
terminar las salidas, excluyendo la escritura del propio JSON.

Las claves del objeto son fijas:

```text
programa, nivel, pasadas,
alto_nivel: {inicial, despues: {<pasada>: n}},
isa_antes_expandir, emitidas, bundles, nops, bytes, demanda, presion,
mascara_registros,
tiempo_ms: {total, pasadas: {<pasada>: ms}},
contadores: {desenrollados, renombradas, eliminadas, bloques_eliminados,
             reordenadas, derramados, cargas_derrame, guardados_derrame},
ciclos_sim
```

`nivel` vale `"O1"`. `pasadas` enumera solo las ejecutadas, en orden fijo; los
mapas por pasada contienen solo las ejecutadas que correspondan a ese mapa
(`alto_nivel.despues` solo las de HIGH). No se agregan entradas para pasadas
inactivas. `demanda` es el máximo de vivos antes de asignar, obtenido mediante
una consulta única de C1; `presion` es el máximo de intervalos activos con
registro en el asignador, tomando el máximo entre funciones. La máscara
`"0x...."` de registros usados es informativa y no sustituye la presión.

`emitidas` cuenta las operaciones no-NOP que llegan a `cg_schedule`, incluidos
los slots no-NOP de bundles fijos, expansiones y preservación; si se desactiva
el calendario, cuenta el mismo IR de entrada. `nops = 4 × bundles − emitidas`
y `bytes = 16 × bundles` describen la salida final elegida. Los bytes no son el
tamaño completo del `.bin` con cabecera e imagen. Las pasadas vacías dejan
`desenrollados` como `[]` y los demás contadores en 0; `ciclos_sim` vale `null`.
C4.2/I2 agregarán ciclos y presentación gráfica.

El intérprete comprueba pasadas HIGH; sim_asm contra -O0 comprueba las de ISA;
tac_validate se ejecutará tras cada pasada. El gestor E aplica este contrato; las transformaciones y el derrame efectivo
siguen pendientes de C3.

## Guía y sexto ejemplo, paso a paso

Para Gabriel, Saddy y Kevin: crear Scope global obligatorio (puede estar vacío), programa con
`tac_program_new`, Symbol de función prestado y `tac_function_new`. Crear
bloques con `tac_block_new`, temporales con `tac_temp_new`, operandos con los
constructores y cuádruplas con `tac_append`. Para ISA, fijar stage y usar
`tac_append_isa`. Los selectores nuevos son -1; asignar los aplicables antes
de reconstruir/validar. Imprimir antes/después de la pasada y liberar el
programa antes de destruir los símbolos. No comparar identidades por texto.

`pruebas/representacion/tac/ejemplo.cls`, compilado por check-tac, contiene:

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

`include/representacion/tac_build.h` recibe un AST que ya pasó el semántico y el ámbito real
`scopes_global()`. Usa los Symbol anotados, nunca vuelve a resolver nombres.
`TacBuildResult` posee el TacProgram, la imagen inicial y los Symbol auxiliares;
se libera antes de `semantic_free`. No modifica AST, ámbitos, direcciones,
etiquetas ni estado del generador. La construcción ocurre con `-i`/`--tac` y con `-O1`.

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

`pruebas/generador_o0/programas/factorial_r.cls` contiene el factorial con dos acumuladores.
Su AST tiene FUNCTION factorial, PARAMS n/acc, RETURN_TYPE r y un BLOCK con
CONDITIONAL: condición LESS_EQUAL(n,1), rama ASSIGNMENT(r,acc) y EMIT,
y RESIDUAL con REWIND factorial(n-1,acc*n).

1. B0 inicializa r a cero y salta a B1, entrada del cuerpo.
2. B1 captura n y calcula la condición; la caída B2 devuelve acc.
3. B3 captura n-1 y acc*n antes de escribir n y acc, y vuelve a B1.
4. B4 conserva el RETURN implícito del final, aunque aquí no tiene predecesores.
5. main inicializa f, pasa 3/1 y conserva el resultado inmediato del CALL.

El esperado completo está en `pruebas/representacion/tac_build/esperados/factorial_r.tac`.
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

`include/generador_o1/tac_lower.h` expone `tac_lower(high, error, capacity)`. Recibe el
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

E enlaza esta etapa con ccom para `-O1`. `-i` conserva HIGH y el intérprete
no cambia. C1 agrega vivacidad y lazos; C2 asigna registros; D implementa
conservación emitida de vivos/rl, expansión P1 y emisión.
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

El emisor de -O1 preserva los vivos afectados **antes** de
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

`make check-tac-values` ejecuta `pruebas/representacion/tac_build/probar_valores.py` y el auxiliar
`build/interpretar_tac`. Importa la tabla CASES de `pruebas/generador_o0/programas/probar.py`,
sin duplicar sus valores, y compara las variables finales de los 22 programas
sin CRYPTO. P09 y P11 se listan como omitidos, sin sumarlos a aprobados.

`pruebas/representacion/tac_build/interpretar.h` recibe un TacBuildResult en memoria, nunca el
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

## C2: intervalos y asignación a registros físicos

`tac_intervals.h` deriva intervalos de la vivacidad de C1 en ISA virtual. El
algoritmo linear scan sigue a Poletto y Sarkar (1999), **fuera de Aho**:
[Linear Scan Register Allocation](https://web.cs.ucla.edu/~palsberg/course/cs132/linearscan.pdf).
La política LRU entre registros libres corresponde al contrato del proyecto
(Aho §10.2.3); las restricciones de selección se describen en Aho §8.8.

La numeración recorre la lista de bloques de cada función y sus instrucciones:
lectura `2*i`, escritura `2*i+1`. Un bloque vacío consume también un índice y
sus fronteras tienen posiciones propias. Los intervalos son cerrados; expiran
si `fin < inicio_siguiente`. Cada TEMP tiene una sola envolvente que incluye
usos, definiciones, `before/after`, `live_in/live_out` y retrocesos. Se conservan
huecos, varias definiciones, definiciones muertas y bloques inalcanzables: no hay
DCE, SSA ni partición de intervalos en C2.

Las fuentes de MUL/DIV/MOD/POW ocupan también la posición de escritura. Así, el
destino recibe un registro distinto incluso cuando las fuentes mueren allí. La
bajada, si HIGH pide escribir el mismo TEMP que una fuente, crea un destino
nuevo para la PSEUDO y después `add destino_original, nuevo, rz`. La validación
ISA virtual rechaza destino TEMP igual a cualquiera de las fuentes TEMP.
Una instrucción ISA ordinaria puede reutilizar una fuente muerta.

`tac_allocate_registers(program, source_name, error, capacity)` asigna r4–r15;
rz, rl, rx y rg quedan excluidos. Ordena intervalos por inicio/ID y activos por
fin/ID. De los registros libres elige el que quedó libre primero; si empatan,
el menor número físico. Los nunca usados son inicialmente los más antiguos.
El punto interno estático `no_register_available` produce
`archivo:L:C: error de generacion: sin registros temporales disponibles`.
C3.4 reemplazará su cuerpo para integrar el derrame **dentro del recorrido**;
no hay callback ni una pasada que derrame después del error.

La asignación reescribe el mismo programa: TEMP pasa a PHYS en todos los
operandos, incluidas lecturas ligadas de ifmov/feis; se escriben las máscaras
`scratch_clobbers` y se cambia la etapa a ISA_PHYSICAL. Finalmente se ejecuta
`tac_validate`. Si falla se abandona la compilación: el contrato permite que
el programa quede inválido. El resultado posee planes y mapas, pero no el IR;
se libera antes del programa y de sus símbolos. `source_name` es obligatorio,
prestado y no NULL; se rechaza su ausencia antes del recorrido.

Cada sitio jas/CRYPTO/aritmética tiene un plan para D. Los ocho auxiliares
aritméticos son distintos y no coinciden con destino ni fuentes; se prefieren
registros sin un valor vivo que preservar. Se registra únicamente la
intersección de los candidatos `preserve` de C1 con los registros destruidos.
Cada entrada conserva ID TEMP original y registro físico, en orden físico.
Los clobbers nunca se convierten en definiciones de virtuales.

C2 no emite guardados ni movimientos: D debe guardar en `temp_base` antes de
preparar argumentos o expandir una plantilla y restaurar después. Para CRYPTO,
D prepara los registros fijos 6/7/9/10, resolviendo movimientos cruzados. La
métrica pública por función contiene solo máscara y cantidad de registros
asignados a valores; no incluye auxiliares ni todos los clobbers.

`FrameLayout.spill_base` (-1 sin reserva) y `spill_size` (bytes) describen un
tramo independiente, no necesariamente contiguo al frame original ni a
`temp_base`. `data_area_reserve_spill` usa `data_area_alloc`, solo en O1/O2;
cero bytes no reserva. Tras la última reserva el gestor debe repetir
`data_area_stack_short` y solo entonces inicializar rx desde `data_area_end`.
Una reserva fallida termina la compilación, sin restauración. Una compilación
nueva reinicia el cursor mediante el `data_area_reset` existente del semántico.
R reutiliza la misma área estática y la imagen inicial no se desplaza; C3.4
escribirá cada casilla antes de leerla. C2 ofrece y prueba la reserva, pero
como todavía no derrama no solicita casillas automáticamente.

La CLI `-O1` llama al asignador y a D, sin reservar derrame todavía;
O0 e `-i` conservan su significado. D implementa expansión/emisión y preparación
física de CRYPTO. Quedan pendientes
de C3.4 los derrames; E integra banderas y JSON.


## D: emisión de ISA física hacia codegen

`include/generador_o1/tac_emit.h` recibe ISA_PHYSICAL y su plan de C2 sin editar, imagen
inicial, LabelTable y nombre real obligatorio. `tac_emit_program` devuelve 1
en éxito y 0 con diagnóstico: no modifica IR ni símbolos. Reinicia codegen,
carga palabras con `cg_data_set`, repite `data_area_stack_short` y emite
`mov rx, data_area_end()`. D no reserva derrame: C3.4 hará las reservas antes.

Se emite main primero, luego las otras funciones; los bloques mantienen su
orden. El mapa por puntero asigna `.L_o1_N` único en el programa. Las entradas
de función y los terminales se consultan en LabelTable, conservando la ABI P1.
La inicialización del retorno ya está en TAC; no se repite y R sigue entrando
al cuerpo. `tac_validate_for_emission` sigue rechazando PSEUDO en IR expandido;
D usa `tac_validate` y expande directamente hacia codegen, sin cambiar ese IR.

Las formas ISA se adaptan a `cg_set_line`/`cg_op`; parser y semántica siguen en
codegen_instruction/isa_table. `include/generador_o0/generate_emit.h` presta `gen_reg_name`,
`gen_stack_push`, `gen_stack_pop` y `gen_arith_emit_op`, con la lógica P1 intacta.

El único ayudante D de preservación usa `temp_base + 4*(r-4)` para cada vivo
del plan. Jas guarda preservados, apila rl, llama, desapila rl y restaura.
MUL/DIV/MOD/POW llaman `cg_arith_emit` con los ocho registros de scratch_mask.
Un CGArithContext comparte next_label entre todas las funciones y CRYPTO.

CRYPTO guarda antes de preparar registros fijos 6/7/9 o 10. Las copias paralelas
eliminan identidades, consumen destinos no necesarios como fuentes y rompen
ciclos con r4. Las fuentes r4 se consumen antes de romper ciclos; los vivos
externos ya están guardados. Un `cg_gap` explícito separa la plantilla incluso
si no hubo movimientos. `cg_crypto_emit_template` conserva los bundles P1.

Finalmente se emiten ambos terminales fijos, se llama `cg_schedule` y se verifica
ROM mediante `tac_emit_check_rom`: 4096 bundles pasan, 4097 fallan con el archivo
real. Con `--no-schedule` se omite `cg_schedule` y se comprueba el mismo límite
sobre el flujo conservador. No se agrega validación de conocidos.
Tras éxito no se emite más. Se ensamblan `cg_line` y `cg_data` con el ensamblador
existente, pasando también el nombre real para sus diagnósticos.

E imprime -m sin tabla propia: recorrer `cg_ir_count`/`cg_ir_record`, seleccionar
CG_IR_LABEL y consultar `cg_label_address(record->data.label)` después de schedule.
Esto incluye bloques y expansiones. Consultar antes de cg_free o cg_init.
Liberar los planes antes del ISA, ISA antes de TacBuildResult, y conservar
símbolos/AST/LabelTable mientras se emite. La CLI invoca D con `-O1`.

## E: integración, gestor y medición

`src/optimizador/tac_passes.c` contiene la tabla fija y una firma única con contexto prestado
(IR, construcción, ámbitos, nombre real, selección y factor de unroll). El factor
cero indica la heurística del §11; E no la ejecuta porque unroll está vacío.
Las marcas se descartan después de unroll, o al entrar al punto HIGH si está
inactivo. La política spill se registra antes de asignar, sin recorrido propio;
no reemplaza el error de agotamiento del recorrido C2. C3.4 ampliará ese punto.

`src/optimizador/tac_metrics.c` es el único escritor JSON. Reutiliza `json_string` de AST
(publicado sin cambiar su cuerpo). `tac_live_max_temporaries` es la consulta única
de demanda, compartida por C2 y su auxiliar del corpus. El asignador registra el
máximo de `active_count`; E solo agrega máximos entre funciones. La máscara puede
incluir muchos registros usados sucesivamente aunque la presión sea baja.

La CLI conserva una captura del HIGH inicial para `-i`, anterior a las pasadas.
El JSON se escribe tras las salidas exitosas; un fallo no publica métricas.
`check-o1` verifica opciones, gestor, JSON y salidas. `check-o1-equivalence` usa
`ccom -O1 -s`; su adaptador lee dirección/ancho de GLOBAL y ámbitos de main en
la tabla de símbolos del listado. `compare()` y la proyección observable de D
siguen intactos. Se retiró el auxiliar de emisión desde fuente ya sin consumidor.

### Pendientes de limpieza después de integrar C2.3

- Búsqueda lineal de `block_label` en `src/generador_o1/tac_emit.c:80`, mismo patrón que
  `tac_live_instruction`, `tac_intervals_positions` y `tac_allocation_site`.
- Guardado/restauración de `src/generador_o1/tac_emit_pseudo.c:17` frente a
  `src/rutinas/codegen_crypto.c:78` (save_live) y `:98` (restore_live).
- Terminal de `src/generador_o1/tac_emit.c:181` frente a `src/generador_o0/generate_stmt.c:752`
  (gen_end_of_main) y `src/generador_o0/generate.c:216`. Se concentra una sola vez en D;
  no se reorganizan estas duplicaciones conocidas durante esta entrega.

Referencias: Aho §7.2, §8.6, §8.8 y §10.2.3.
