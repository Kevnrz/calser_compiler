# CRYPTO para `ccom` — contrato entre la unidad criptográfica y el compilador

**Grupo 6 · 11 de octubre de 2026 · para el agente de Calser**

**Estado: A-01 CERRADA (11 oct).** Opción A, con las 7 operaciones de §6, que
en Calser son **palabras reservadas con regla gramatical propia** (D-04 de
`gramatica.md`). Fases 1 a 3 implementadas en `ccom` (cambio 11). El diseño de
CRYPTO de Saddy no cambia.

Este documento reúne **todo lo que el compilador necesita saber de CRYPTO**: qué
instrucciones hay, qué hacen, cómo se calendarizan y qué restricciones imponen a
Calser. No describe el interior del hardware.

| Fuente | Qué manda |
|---|---|
| `isa.md` §5.5, §6.4, §7, §11 y §12 (C-15 a C-17) | Semántica y codificación. **Si este documento lo contradice, manda `isa.md`** |
| `memoria_decisiones.md` §3 | Mapa de memoria de datos |
| Enunciado CE 4301 §5.2 y §6.2 | Pruebas obligatorias y validación conjunta |

---

## 1. Las instrucciones

Todas van **solo en el slot 1**, con `sel = 1`. Ese bundle no puede llevar
ALU1. Formato único:
`[1][funct 5][rd 4][rs1 4][rs2 4][k 2][i 2][reservado 10]`.

| Ensamblador | Hace | Escribe registro | Privilegio | Ejemplo codificado |
|---|---|---|---|---|
| `feis rd, rs1, rs2, k, i` | `rd = rs1 XOR F(rs2, subllave[k][i])`: **una** ronda de Feistel4 | Sí, `rd` | `AUTH = 1` | `feis r5, r3, r4, 0, 2` = `0x854D0800` |
| `auth rs1` | Si `H(rs1) == ancla`: `AUTH = 1` y `ancla = rs1`. Si no: `AUTH = 0`, `EXC = 1` | No | Ninguno | `auth t0` = `0x88100000` |
| `logout` | `AUTH = 0` | No | Ninguno | `0x8C000000` |
| `kv_write rs1, k, w` | Palabra `w` de la llave `k` ← `rs1` | No | `AUTH = 1` y `KVLOCK = 0` | `kv_write t1, 0, 3` = `0x90140C00` |
| `kv_lock` | `KVLOCK = 1` hasta el reset | No | `AUTH = 1` | `0x94000000` |
| `sanc rs1` | `ancla = rs1` (renueva la cadena de contraseñas) | No | `AUTH = 1` | `sanc r4` = `0x98100000` |

`F(x, K) = ((x <<< 5) + K) XOR (x <<< 13)`, la función de ronda del enunciado.
`H` es Ascon-Hash256 truncado a 32 bits (C-15); al compilador no le importa cómo
se calcula.

**Sin privilegio**, una instrucción no tiene efecto (no escribe `rd`, no toca la
bóveda ni el ancla) y pone `EXC = 1`. Solo un `auth` exitoso o el reset limpian
`EXC`.

**Lo que el programa no puede ver.** Ninguna instrucción lee `AUTH`, `KVLOCK` ni
`EXC`, y nada sale de la bóveda hacia registros o memoria. El programa **no
puede saber** si un `auth` funcionó; eso lo comprueba el *testbench*.

---

## 2. Reglas de calendarización

| Regla | Detalle |
|---|---|
| Separación entre registros | La de siempre: **1 bundle** entre quien escribe un registro y quien lo lee (`isa.md` §9.1) |
| `feis` encadenadas | Cada ronda lee el resultado de la anterior: 1 bundle de separación entre rondas |
| `auth` | Ocupa 14 ciclos y el **hardware congela el pipeline** (C-16). Para `ccom` es **transparente**: no se inserta nada, y el bundle siguiente ya ve `AUTH` actualizado |
| `kv_write` → `feis` | `feis` puede ir en el bundle siguiente y ya usa la palabra nueva: la bóveda se escribe al final de EX, como un `sw` (verificado en el modelo) |
| Un CRYPTO por bundle | Solo hay una unidad, en el slot 1 |
| `k`, `i`, `w` | Son **campos de la instrucción**, no registros: se fijan en compilación |

---

## 3. Valores de 32 bits

Tokens, palabras de llave y datos son de 32 bits y llegan **en registros**. Si
el valor es una constante que no cabe en el inmediato de 18 bits, `ccom` emite
la secuencia de C-02 (`isa.md` §4.3):

```
{ mov rd, altos_17 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll rd, rd, t11  | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi rd, rd, bajos_15 | nopi | nopi | nopi }
```

En Calser los literales son decimales hasta 2 147 483 647. Los valores con el
bit 31 en uno se escriben con menos unario: `0x89ABCDEF` es `-1985229329`.

**Tokens de `auth`.** Ninguna instrucción calcula `H`, así que la cadena de
Lamport (secreto → `H` → … → ancla) se calcula **antes de simular** con un script
auxiliar (responsable: Saddy). El ancla se graba en el hardware y el token entra
al programa como **literal**.

---

## 4. Plantillas verificadas

Se simularon con un modelo ciclo a ciclo del pipeline sobre 64 B, 1 KB y 4 KB de
datos aleatorios: el cifrado coincide con la referencia y el descifrado devuelve
el original (`sim_programa_crypto.py`). Registros: `t0` = L, `t1` = R, `t2`
origen, `t3` destino, `t4` contador, `t5` límite.

**Cifrar `n` bloques de 8 bytes** (llave 0, subllaves 0, 1, 2, 3). El `bge` del
tercer bundle salta el lazo si `n <= 0`; va en el hueco de separación que ya
existía, así que no cuesta bundles:

```
  { mov t4, 0 | mov t5, n | nopi | nopi }
  { mov t2, ORIGEN | mov t3, DESTINO | nopi | nopi }
  { nopi | nopi | bge rz, t5, fin_cifra | nopi }
cifra:
  { nopi | nopi | nopi | lw t0, 0(t2) }
  { nopi | nopi | nopi | lw t1, 4(t2) }
  { saddi t2, t2, 8 | nopi | nopi | nopi }
  { nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }
  { saddi t4, t4, 1 | nopi | nopi | nopi }
  { nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }
  { nopi | nopi | nopi | nopi }
  { nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }
  { nopi | nopi | nopi | nopi }
  { nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }
  { nopi | nopi | nopi | sw t0, 0(t3) }
  { saddi t3, t3, 8 | nopi | blt t4, t5, cifra | sw t1, 4(t3) }
fin_cifra:
```

**Descifrar**: subllaves en orden inverso (3, 2, 1, 0) y **mitades
intercambiadas** al cargar y al guardar (C-17). Sin el intercambio, el resultado
no es el texto original. Lleva las mismas tres líneas de entrada, con
`fin_descifra`.

```
descifra:
  { nopi | nopi | nopi | lw t0, 4(t2) }      # intercambio al cargar
  { nopi | nopi | nopi | lw t1, 0(t2) }
  { saddi t2, t2, 8 | nopi | nopi | nopi }
  { nopi | feis t0, t0, t1, 0, 3 | nopi | nopi }
  { saddi t4, t4, 1 | nopi | nopi | nopi }
  { nopi | feis t1, t1, t0, 0, 2 | nopi | nopi }
  { nopi | nopi | nopi | nopi }
  { nopi | feis t0, t0, t1, 0, 1 | nopi | nopi }
  { nopi | nopi | nopi | nopi }
  { nopi | feis t1, t1, t0, 0, 0 | nopi | nopi }
  { nopi | nopi | nopi | sw t0, 4(t3) }      # intercambio al guardar
  { saddi t3, t3, 8 | nopi | blt t4, t5, descifra | sw t1, 0(t3) }
fin_descifra:
```

Son 12 bundles por bloque, más 1 ciclo por salto tomado (unos 13 ciclos por
bloque). Los `saddi` ocupan huecos de separación. Probado con 0, 1, 3, 8, 128 y
512 bloques. El cifrado coincide con la Fig. 1 del enunciado. El descifrado de
la Fig. 1 es la misma función: solo cambia qué registro guarda cada mitad.

**Al expandir varias veces**, las etiquetas se renombran (`cifra_1`,
`fin_cifra_1`…). La plantilla usa `t0`–`t5` (y `t11` en las constantes de 32
bits); lo que esté vivo en esos registros se guarda antes, como en una llamada.
El hardware no obliga a usar esos registros: `ccom` puede elegir otros si
respeta la separación de 1 bundle y no escribe el mismo registro desde dos
slots de un bundle.

---

## 5. Memoria

| Rango | Uso |
|---|---|
| 0x0000 – 0x0FFF | Variables de `ccom` |
| 0x1000 – 0xFFFF | Archivos inyectados por `load_file.py` |

Calser no tiene punteros, y los archivos no son arreglos de Calser: **las
funciones de cifrado reciben direcciones como enteros**.

---

## 6. A-01: operaciones de CRYPTO en Calser (CERRADA)

Decidido el 11 de octubre: **opción A, rutinas completas**. `ccom` expande cada
operación en línea con las plantillas de §4. Las 7 operaciones son **palabras
reservadas con su propia regla gramatical** (no terminal *crypto*, decisión D-04
de `gramatica.md`); **no** pasan por la tabla de símbolos ni por el grafo de
llamadas. Así quedó en `calser_diseno.md` (nota del 11 oct) y en el código
(cambio 11 de `ccom_pendientes.md`, S-13).

| Sentencia | Instrucción(es) | Argumentos que son número literal 0–3 |
|---|---|---|
| `autenticar [ expr ] ;` (token) | `auth` | — |
| `cerrar_sesion [ ] ;` | `logout` | — |
| `cargar_llave [ num , num , expr ] ;` (`k`, `w`, valor) | `kv_write` (una palabra por sentencia) | `k`, `w` |
| `sellar [ ] ;` | `kv_lock` | — |
| `cambiar_ancla [ expr ] ;` (valor) | `sanc` | — |
| `cifrar [ expr , expr , expr , num ] ;` (origen, destino, bloques, `k`) | plantilla de cifrado (§4) | `k` |
| `descifrar [ expr , expr , expr , num ] ;` (origen, destino, bloques, `k`) | plantilla de descifrado con C-17 (§4) | `k` |

**Reglas**

| Regla | Detalle |
|---|---|
| Son sentencias | No devuelven valor y no pueden ir dentro de una expresión. Sus nombres son palabras reservadas: no pueden usarse como nombre de variable ni de función |
| `k`, `w` | La gramática exige un número escrito tal cual (**num**): una variable o `-1` es **error sintáctico**. Un número fuera de 0–3 es **error semántico** |
| Las demás posiciones | Cualquier expresión `int` |
| `origen`, `destino` | Múltiplos de 4 (no hace falta 8). Pueden ser iguales (cifrado en el mismo lugar) |
| Comprobaciones si son constantes | Primero la zona 0x1000–0xFFFF; si falla, no se revisa la alineación (un solo error por dirección). Luego múltiplo de 4. Luego `dirección + 8·bloques` ≤ 0x10000 si `bloques` es constante. Si son variables, responde el programador |
| `bloques` | Cualquier expresión `int`; con `bloques <= 0` no se hace nada (el `bge` de §4) y no es error |
| Orden de uso | Cifrar sin `autenticar` o `cargar_llave` después de `sellar` no es error de compilación: el procesador rechaza la operación y marca `EXC = 1` |

**Programa de demostración** (cubre los casos 4 a 7 de CE 4301 §5.2 y la
validación conjunta de §6.2):

```
Vi origen: int = 4096 ;     # 0x1000, donde load_file deja el archivo
Vi cifrado: int = 8192 ;    # 0x2000
Vi vuelta: int = 12288 ;    # 0x3000
Vi negado: int = 16384 ;    # 0x4000

F main [] : void ;
    autenticar[-193210613] ;           # x99 = 0xF47BD70B, primer token Lamport

    # Las 4 llaves (caso 6: almacenamiento de las 4 llaves)
    cargar_llave[0, 0, 19114957] ;      # 0x0123ABCD
    cargar_llave[0, 1, -1985229329] ;   # 0x89ABCDEF
    cargar_llave[0, 2, 324508639] ;     # 0x13579BDF
    cargar_llave[0, 3, 610839776] ;     # 0x2468ACE0
    # ... igual para las llaves 1, 2 y 3 (16 llamadas en total)
    sellar[] ;

    cifrar[origen, cifrado, 128, 0] ;   # 1 KB
    descifrar[cifrado, vuelta, 128, 0] ;   # debe quedar igual al original
    cifrar[origen, cifrado, 1, 3] ;     # usar otra llave al menos una vez

    cerrar_sesion[] ;
    cifrar[origen, negado, 1, 0] ;      # sin sesión: feis se rechaza y en 0x4000
                                        # queda el texto SIN cifrar (visible en memoria)
    autenticar[999] ;                   # token falso: AUTH = 0, EXC = 1 (testbench)
    E ;
.
```

**Cadena de demostración:** ancla inicial `0xFEFF95FF`, token `x99 =
0xF47BD70B` (`-193210613` en Calser) y siguiente token `x98 = 0xA45326BA`.
Se generan con `tools/lamport.py` del proyecto de hardware. Cada token se
consume una sola vez; un reset normal no restaura el ancla inicial.

---

## 7. Lo que exige el enunciado sobre CRYPTO

| Requisito | Dónde | Quién lo cubre |
|---|---|---|
| Programa generado por `ccom` que cifre y descifre Feistel4 desde alto nivel | CE 4301 §6.2 | A-01 |
| Ronda Feistel4; cifrado y descifrado de 4 rondas contra la referencia en C | CE 4301 §5.2, casos 4 y 5 | Plantillas de §4 |
| Bóveda: guardar 4 llaves, `auth` exitoso **y fallido**, llaves no legibles | CE 4301 §5.2, caso 6 | Programa de demostración (las 4 llaves, `cifrar` sin sesión) + *testbench* (`EXC`, intento de leer una llave) |
| ALU, memoria, saltos y CRYPTO **en los mismos bundles** | CE 4301 §5.2, caso 7 | Las plantillas ya lo hacen (`saddi`, `lw`/`sw`, `blt` junto a `feis`) |
