# Arquitectura VLIW — Grupo 6

**CE 4301 Arquitectura de Computadores I**  
Instituto Tecnológico de Costa Rica   
Escuela de Ingeniería en Computadores

**Integrantes**

* Luis Castro Montenegro
* Saddy Guzmán Rojas
* Gabriel Núñez Morales
* Kevin Ruiz Rodríguez
* Tatiana Chacón Mora

18 de setiembre de 2026

---

## Contenido

1. [Descripción general](#1-descripción-general)
   - [1.1 Modelo estático y riesgos de datos](#11-modelo-estático-y-riesgos-de-datos)
2. [Resumen de especificaciones](#2-resumen-de-especificaciones)
3. [Organización del bundle](#3-organización-del-bundle)
   - [3.1 Asignación de slots](#31-asignación-de-slots)
   - [3.2 Instrucción vacía](#32-instrucción-vacía)
   - [3.3 Combinaciones legales](#33-combinaciones-legales)
4. [Formatos de instrucción](#4-formatos-de-instrucción)
   - [4.1 Campo de selección](#41-campo-de-selección)
   - [4.2 Rangos](#42-rangos)
   - [4.3 Constantes de 32 bits](#43-constantes-de-32-bits)
5. [Unidades funcionales](#5-unidades-funcionales)
   - [5.1 Acceso compartido al banco de registros](#51-acceso-compartido-al-banco-de-registros)
   - [5.2 ALU](#52-alu)
   - [5.3 BRU](#53-bru)
   - [5.4 LSU](#54-lsu)
   - [5.5 CRYPTO](#55-crypto)
6. [Set de instrucciones](#6-set-de-instrucciones)
   - [6.1 ALU — slot 0, y slot 1 con sel = 0](#61-alu--slot-0-y-slot-1-con-sel--0)
   - [6.2 BRU — slot 2](#62-bru--slot-2)
   - [6.3 LSU — slot 3](#63-lsu--slot-3)
   - [6.4 CRYPTO — slot 1 con sel = 1](#64-crypto--slot-1-con-sel--1)
   - [6.5 Ejemplos de codificación](#65-ejemplos-de-codificación)
7. [Bóveda de llaves](#7-bóveda-de-llaves)
   - [7.1 Aislamiento y sellado de la bóveda](#71-aislamiento-y-sellado-de-la-bóveda)
   - [7.2 Control de acceso: Protocolo Lamport OTP](#72-control-de-acceso-protocolo-lamport-otp)
8. [Banco de registros](#8-banco-de-registros)
   - [8.1 Sobre la especialización de registros](#81-sobre-la-especialización-de-registros)
   - [8.2 Registro de estado](#82-registro-de-estado)
9. [Organización segmentada](#9-organización-segmentada)
   - [9.1 Escritura y lectura del banco de registros en flancos opuestos](#91-escritura-y-lectura-del-banco-de-registros-en-flancos-opuestos)
   - [9.2 Latencia uniforme](#92-latencia-uniforme)
   - [9.3 Semántica de lectura dentro de un bundle](#93-semántica-de-lectura-dentro-de-un-bundle)
10. [Estrategia frente a saltos](#10-estrategia-frente-a-saltos)

---

## 1. Descripción general

Este documento especifica la arquitectura de un procesador **VLIW** (*Very Long Instruction Word*) con un set de instrucciones propio de 32 bits, capaz de ejecutar programas de propósito general e incorporar instrucciones dedicadas en hardware para el algoritmo de cifrado **Feistel4**.

El procesador trabaja con **paralelismo estático**: el software agrupa cuatro operaciones independientes en un solo paquete de 128 bits (*bundle*), y el procesador las despacha simultáneamente a sus respectivas unidades funcionales para ejecutarlas en paralelo en un mismo ciclo de reloj.

### 1.1 Modelo estático y riesgos de datos

Para mantener un diseño de hardware simple y eficiente, el procesador **no implementa mecanismos automáticos de control de riesgos** (sin adelantamiento o *forwarding*, sin *scoreboarding* ni pausas del pipeline). La responsabilidad de respetar las dependencias entre datos y dejar los espacios necesarios recae completamente en el software (el compilador o el programador en ensamblador).

---

## 2. Resumen de especificaciones

| Parámetro | Valor |
|---|---|
| Tamaño de instrucción | 32 bits |
| Slots por bundle | 4 |
| Tamaño de bundle | 128 bits |
| Registros de propósito general | 16, de 32 bits |
| Tamaño de inmediatos | 18 bits |
| Direccionamiento | 32 bits |
| Memoria mínima | 64 KB |
| Endianness | Little endian |
| Etapas de pipeline | 4 |

---

## 3. Organización del bundle

| Slot 0 (Bits 31..0) | Slot 1 (Bits 63..32) | Slot 2 (Bits 95..64) | Slot 3 (Bits 127..96) |
|:---:|:---:|:---:|:---:|
| **ALU0** | **ALU1 / CRYPTO** | **BRU** | **LSU** |
| 32 bits | 32 bits | 32 bits | 32 bits |

### 3.1 Asignación de slots

Se adoptó un **esquema mixto**: tres slots están atados permanentemente a una
unidad funcional, y el cuarto se comparte entre la segunda ALU y la unidad
criptográfica mediante un campo de selección de un bit.

| Slot | Unidad | Selección |
|---|---|---|
| 0 | ALU0 | fija |
| 1 | ALU1 o CRYPTO | bit 31 de la instrucción |
| 2 | BRU | fija |
| 3 | LSU | fija |

**Justificación.** El esquema de slots completamente flexibles se descartó tras
verificar que la cantidad de bundles resultante es la misma con slots dedicados
que con slots libres. La flexibilidad completa costaría un campo de opcode en
cada instrucción sin reducir el conteo de bundles.

El esquema mixto sí aporta: la unidad criptográfica permanece ociosa en la
práctica totalidad de los programas de propósito general, de modo que un slot
dedicado desperdiciaría 32 bits en cada bundle. Compartirlo con la segunda ALU
recupera ese ancho.

### 3.2 Instrucción vacía

La instrucción vacía de cada slot es **`nopi`**, codificada con
`funct = 00000`. Un bundle completamente vacío son 128 bits en cero.

### 3.3 Combinaciones legales

| Contenido del bundle | Slot 1 |
|---|---|
| Hasta 2 operaciones de ALU, 1 de salto y 1 de memoria | ALU1 |
| 1 operación de ALU, 1 de salto, 1 de memoria y 1 criptográfica | CRYPTO |

---

## 4. Formatos de instrucción

Los campos se alinean en la misma posición en todos los formatos, de modo que
el decodificador puede extraerlos sin identificar primero el tipo de
instrucción:

| Formato | Bit 31 | Bits 30..26 | Bits 25..22 | Bits 21..18 | Bits 17..14 | Bits 13..12 | Bits 11..10 | Bits 9..0 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Tipo R** | `sel` | `funct` (5) | `rd` (4) | `rs1` (4) | `rs2` (4) | <td colspan="3" align="center">*(reservado 14 bits)*</td> |
| **Tipo I** | `sel` | `funct` (5) | `rd` (4) | `rs1` (4) | <td colspan="4" align="center">`imm` (18 bits con signo)</td> |
| **Tipo B** | `sel` | `funct` (5) | `rs2` (4) | `rs1` (4) | <td colspan="4" align="center">`off` (18 bits con signo)</td> |
| **Tipo J** | `sel` | `funct` (5) | `rd` (4) | <td colspan="5" align="center">`target` (22 bits con signo)</td> |
| **CRYPTO** | `1` | `funct` (5) | `rd` (4) | `rs1` (4) | `rs2` (4) | `k` (2) | `i` (2) | *(reservado 10)* |

| Tipo | Se usa para |
|---|---|
| R | Operaciones entre registros: tres registros, sin inmediato |
| I | Operaciones con valor inmediato y accesos a memoria |
| B | Saltos condicionales: dos fuentes comparadas y un desplazamiento |
| J | Saltos incondicionales y llamadas: destino largo y registro de enlace |
| CRYPTO | Operaciones de Feistel4, autenticación y gestión de la bóveda |

### 4.1 Campo de selección

El bit 31 solo tiene significado en el slot 1, donde selecciona entre ALU1
(valor 0) y CRYPTO (valor 1). En los slots 0, 2 y 3 es un campo **reservado que
debe valer cero**, para dejar abierta la posibilidad de compartir otro slot más
adelante.

### 4.2 Rangos

| Campo | Bits | Con signo | Rango |
|---|---|---|---|
| `sel` | 1 | no | 0 o 1 |
| `funct` | 5 | no | 0 a 31, independiente por unidad |
| `rd`, `rs1`, `rs2` | 4 | no | r0 a r15 |
| `imm` | 18 | sí | −131 072 a 131 071 |
| `off` | 18 | sí | ±131 072 bundles |
| `target` | 22 | sí | ±2 097 152 bundles |
| `k`, `i` | 2 | no | 0 a 3 |

### 4.3 Constantes de 32 bits

Un literal de 32 bits no cabe en un inmediato de 18. Se construye en tres pasos:

1. `mov rd, altos_17` — Carga los 17 bits superiores en el registro destino.
2. `sll rd, rd, 15` — Desplaza el valor 15 posiciones hacia la izquierda.
3. `saddi rd, rd, bajos_15` — Suma con signo los 15 bits inferiores restantes.

Como el inmediato tiene signo, solo 17 bits son útiles por paso. Dos pasos
cubren 34 bits, suficiente para cualquier valor de 32. Se descartó incluir una
instrucción dedicada a cargar la parte alta: la secuencia resuelve el caso y el
compilador solo la emite cuando el literal no cabe.

---

## 5. Unidades funcionales

| Unidad | Instancias | Slot | Función |
|---|---|---|---|
| ALU | 2 | 0 y 1 | Aritmética entera, lógicas, desplazamientos, comparaciones |
| BRU | 1 | 2 | Saltos condicionales e incondicionales, llamada y retorno |
| LSU | 1 | 3 | Carga y almacenamiento de palabra y de byte |
| CRYPTO | 1 | 1 | Ronda de Feistel4, control de acceso y gestión de la bóveda |

### 5.1 Acceso compartido al banco de registros

Las cuatro unidades leen sus operandos en la misma etapa y escriben sus
resultados en la misma etapa. El banco de registros debe atender todos esos
accesos simultáneamente.

| Unidad | Puertos de lectura | Puertos de escritura |
|---|---|---|
| ALU0 | 2 | 1 |
| ALU1 o CRYPTO | 2 | 1 |
| BRU | 2 | 1 |
| LSU | 2 | 1 |
| **Total** | **8** | **4** |

Ocho puertos de lectura y cuatro de escritura sobre dieciséis registros. Este
es el costo estructural principal del paradigma VLIW y crece linealmente con la
cantidad de slots, razón adicional para no extender el bundle más allá de lo
necesario.

Que el slot 1 se comparta entre ALU1 y CRYPTO no aumenta la cantidad de
puertos: solo una de las dos unidades se activa en un ciclo dado.

### 5.2 ALU

Dos instancias idénticas. Cada una recibe dos operandos y produce un resultado
de 32 bits.

**Bloques internos**

| Bloque | Ancho | Lo usan |
|---|---|---|
| Sumador/restador | 32 bits | `add`, `sub`, `saddi`, y las cuatro comparaciones |
| Bloque lógico bit a bit | 32 bits | `and`, `or`, `xor`, `xori` |
| Desplazador de barril | 32 bits | `sll`, `srl`, `sra` |
| Multiplexor de segundo operando | 32 bits | selecciona entre `rs2` y el inmediato con signo extendido |
| Multiplexor de resultado | 32 bits | selecciona la salida según `funct` |

**Las comparaciones reutilizan el sumador.** No hace falta un comparador
dedicado: las cuatro se derivan de las banderas de la resta `rs1 - rs2`.

| Instrucción | Condición sobre la resta |
|---|---|
| `seq` | resultado igual a cero |
| `sne` | resultado distinto de cero |
| `slt` | bit de signo del resultado, corregido por desbordamiento |
| `sle` | `slt` o `seq` |

El resultado se extiende a 0 o 1 en los 32 bits del registro destino.

**`ifmov`** no requiere unidad aritmética: es un multiplexor gobernado por la
condición «`rs2` distinto de cero», que es una compuerta OR reductora sobre los
32 bits de `rs2`.

**`mov`** conduce el inmediato directamente al multiplexor de resultado.

**El desplazador es de barril**, no iterativo: desplazar por un número variable
de posiciones en un solo ciclo requiere una red de multiplexores en cinco
niveles. Un desplazador iterativo obligaría a detener el pipeline, lo que este
diseño no contempla.

**No se incluyen multiplicador ni divisor.** Un circuito iterativo exigiría
detener el pipeline; uno combinacional determinaría el ciclo de reloj. La
multiplicación, la división, el módulo y la potencia se resuelven mediante
rutinas de software.

La multiplicación por una constante conocida en compilación se traduce a una
secuencia de sumas y desplazamientos, sin llamada a rutina. Ese caso incluye
todo cálculo de índice en arreglos bidimensionales, puesto que las dimensiones
forman parte del tipo.

**Latencia:** un ciclo, todas las operaciones.

### 5.3 BRU

Una sola instancia. Resuelve el salto en la etapa de decodificación, de modo
que sus bloques deben caber en esa etapa junto con la lectura de registros.

**Bloques internos**

| Bloque | Ancho | Lo usan |
|---|---|---|
| Comparador | 32 bits | `beq`, `bne`, `blt`, `bge` |
| Sumador de destino | 22 bits | `jef`, `jas`, y los cuatro saltos condicionales |
| Incrementador de PC | 22 bits | `jas`, `jefR`, para producir el valor de enlace |
| Multiplexor de destino | 22 bits | selecciona entre `PC + off`, `PC + target` y el valor de `rs1` |
| Lógica de decisión | — | determina si el salto se toma |

**El comparador es propio de la BRU**, no compartido con la ALU. Tenerlo
separado permite que un mismo bundle contenga una comparación que produce un
valor y un salto condicional, sin que ambos compitan por el mismo recurso.

Como los saltos condicionales integran la comparación de dos registros, un
ciclo no requiere una instrucción de comparación previa.

**Las direcciones se cuentan en bundles**, no en bytes. El sumador de destino
opera sobre el índice de bundle, de ahí que sea de 22 bits y no de 32. En los
saltos relativos (`beq`, `bne`, `blt`, `bge`, `jef`), el destino se calcula
directamente respecto al bundle actual: $PC_{\text{destino}} = PC_{\text{actual}} + \text{desplazamiento}$.
En las instrucciones de enlace (`jas`, `jefR`), el valor de retorno guardado en
`rd` es siempre $PC_{\text{actual}} + 1$ (el bundle inmediatamente siguiente).

**Señal de descarte.** Cuando un salto se toma, la BRU emite una señal que
invalida el bundle que ya ingresó a la etapa de búsqueda. No hay *delay slots*:
ese bundle se descarta sin ejecutarse.

**Latencia:** un ciclo. El destino queda determinado al final de la etapa de
decodificación.

### 5.4 LSU

Una sola instancia. Único modo de direccionamiento: base más desplazamiento.

**Bloques internos**

| Bloque | Ancho | Función |
|---|---|---|
| Sumador de dirección | 32 bits | calcula `rs1 + imm` |
| Selector de byte | — | elige el byte dentro de la palabra en `lb` y `sb` |
| Extensor de signo | 8 a 32 bits | completa el resultado de `lb` |
| Interfaz de memoria | — | dirección, dato de escritura, dato de lectura, habilitación de escritura, tamaño de acceso |

**El modo único de direccionamiento** cubre los tres casos que el generador de
código necesita: variables globales tomando el puntero global como base,
elementos de arreglo con una base calculada en la ALU, y las áreas estáticas de
las funciones. Modos adicionales no aportarían nada que no se resuelva con una
instrucción de ALU previa.

**Alineación de accesos a memoria.** Para las operaciones de palabra completa
(`lw` y `sw`), la dirección calculada (`rs1 + imm`) debe estar alineada a
múltiplos de 4 bytes; el hardware ignora los dos bits menos significativos
(`addr[1:0]`). Para las operaciones a nivel de byte (`lb` y `sb`), los bits
`addr[1:0]` seleccionan cuál de los cuatro bytes se lee o se escribe dentro de
la palabra de 32 bits. En `sb` solo se modifica ese byte y los tres restantes
conservan su valor.

**En `sw` y `sb` no se escribe ningún registro.** El campo que en los demás
formatos indica el destino contiene la fuente del dato.

**Latencia:** un ciclo. La memoria de la simulación responde dentro del mismo
ciclo, razón por la cual no se destina una etapa propia al acceso a memoria.

### 5.5 CRYPTO

Una sola instancia, en el slot 1, activa cuando el campo de selección vale uno.
Contiene la bóveda de llaves, la lógica de la función de ronda y el subsistema de
autenticación dinámica por contraseñas de un solo uso (Lamport OTP).

**Bloques internos**

| Bloque | Tamaño | Función |
|---|---|---|
| Bóveda de llaves | 4 × 128 bits | almacena cuatro llaves de cuatro subllaves cada una |
| Selector de subllave | — | multiplexor gobernado por los campos `k` e `i` |
| Función de ronda F | 32 bits | rotaciones fijas, suma modular y XOR |
| XOR de salida | 32 bits | combina el resultado de F con la mitad `rs1` en `feis` |
| Registro de ancla OTP (`ancla_otp`) | 32 bits | almacena el token esperado de la cadena de Lamport activa |
| Registro de estado | 3 bits útiles | `AUTH`, `KVLOCK`, `EXC` |
| Comparador de credencial | 32 bits | compara $F(rs1, K_{\text{fija}})$ contra `ancla_otp` en `auth` |
| Lógica de control de acceso | — | habilita o bloquea las operaciones privilegiadas |

**La función de ronda** implementa la referencia del enunciado:

$$F(x, K) = \big((x \lll 5) + K\big) \oplus (x \lll 13)$$

donde $\lll$ denota rotación circular a la izquierda de 32 bits (`ROL32`), $+$ es suma modular de 32 bits ($\bmod 2^{32}$) y $\oplus$ es la operación XOR bit a bit. Los bloques que requiere son un sumador de 32 bits, una compuerta XOR de 32 bits y las dos rotaciones fijas.

**Las rotaciones no cuestan hardware.** Rotar por una cantidad fija y conocida
es un reordenamiento de cables: la salida del bit `i` se conecta a la entrada
del bit `(i + r) mod 32`. Por esta razón no se incluyeron instrucciones de
rotación en el set: al encapsular la ronda completa dentro de la unidad, las
rotaciones quedan internas y no requieren codificación propia ni ocupan slots
del bundle.

**Reutilización de la función F para autenticación.** La función $F$ no solo
computa las rondas de Feistel4, sino que actúa como función unidireccional (*hash* de
un solo sentido) para el protocolo Lamport OTP. Cuando se ejecuta `auth rs1`,
la entrada de $F$ se alimenta con el token de 32 bits contenido en `rs1` y se
suma con una constante fija cableada en silicio ($K_{\text{fija}} = \text{0x6A09E667}$),
ahorrando compuertas adicionales.

**Una ronda por instrucción.** Un programa de cifrado o descifrado se construye
encadenando cuatro invocaciones. Una instrucción que ejecutara las cuatro
rondas exigiría cuatro sumadores, cuatro XOR y una máquina de estados para
secuenciar las subllaves, resultando en una unidad excesivamente compleja y
poco representativa del paradigma VLIW, donde el paralelismo se explota entre
slots.

**Un solo registro destino y encadenamiento de rondas.** La ronda calcula
`nuevoR = L XOR F(R, subllave)`; el valor de `nuevoL` es `R`, que ya reside en
un registro. Producir dos resultados obligaría a un quinto puerto de escritura
en el banco de registros, destinado a una sola instrucción.

El intercambio de mitades se resuelve en software alternando qué registro
recibe el resultado en cada ronda consecutiva. Para un bloque de 64 bits con
la mitad izquierda $L$ en `r4` y la mitad derecha $R$ en `r5`, la secuencia de
cifrado y descifrado para la llave 0 (`k = 0`) se organiza de forma simétrica:

| Ronda | Mitad actualizada | Instrucción de Cifrado | Instrucción de Descifrado | Transformación matemática |
|:---:|:---:|---|---|---|
| **0** | $R_1 \to \text{r4}$ | `feis r4, r4, r5, 0, 0` | `feis r4, r4, r5, 0, 3` | $r_4 \leftarrow L_0 \oplus F(R_0, K_{0,i})$ |
| **1** | $R_2 \to \text{r5}$ | `feis r5, r5, r4, 0, 1` | `feis r5, r5, r4, 0, 2` | $r_5 \leftarrow L_1 \oplus F(R_1, K_{0,i})$ |
| **2** | $R_3 \to \text{r4}$ | `feis r4, r4, r5, 0, 2` | `feis r4, r4, r5, 0, 1` | $r_4 \leftarrow L_2 \oplus F(R_2, K_{0,i})$ |
| **3** | $R_4 \to \text{r5}$ | `feis r5, r5, r4, 0, 3` | `feis r5, r5, r4, 0, 0` | $r_5 \leftarrow L_3 \oplus F(R_3, K_{0,i})$ |

Al terminar las 4 rondas, el bloque resultante queda disponible en el par (`r4` = $L_4$, `r5` = $R_4$). Para el descifrado se utiliza exactamente la misma alternancia de registros pero suministrando las subllaves en orden inverso (`3, 2, 1, 0`).

**Aislamiento de la bóveda.** El multiplexor de subllave alimenta únicamente la
función de ronda. No existe ningún camino desde la bóveda hacia el banco de
registros ni hacia la interfaz de memoria, de modo que su contenido no puede
extraerse por ninguna instrucción del set.

**Latencia:** un ciclo. Todas las operaciones criptográficas caben en la etapa
de ejecución.

---

## 6. Set de instrucciones

El campo `funct` es de 5 bits y su espacio de nombres es **independiente por
unidad funcional**: el mismo valor significa cosas distintas en slots
distintos. Cada unidad dispone de 32 códigos.

En las unidades de propósito general, **el bit más significativo de `funct`
distingue la familia de formato**: vale `0` en los formatos de tres registros y
de salto relativo, y `1` en los formatos con inmediato largo o destino extendido.
El decodificador puede seleccionar la interpretación de los bits bajos sin
esperar a los demás campos.

`funct = 00000` está reservado a `nopi` en las cuatro unidades, de modo que un
bundle completamente vacío son 128 bits en cero.

### 6.1 ALU — slot 0, y slot 1 con `sel = 0`

**Formato R** — `[sel][funct 5][rd 4][rs1 4][rs2 4][reservado 14]`

| `funct` | Instrucción | Operación |
|---|---|---|
| `00000` | `nopi` | Sin efecto. Relleno de calendarización |
| `00001` | `add rd, rs1, rs2` | `rd = rs1 + rs2` |
| `00010` | `sub rd, rs1, rs2` | `rd = rs1 - rs2` |
| `00011` | `and rd, rs1, rs2` | Conjunción bit a bit |
| `00100` | `or rd, rs1, rs2` | Disyunción bit a bit |
| `00101` | `xor rd, rs1, rs2` | Disyunción exclusiva bit a bit |
| `00110` | `sll rd, rs1, rs2` | Desplazamiento lógico a la izquierda |
| `00111` | `srl rd, rs1, rs2` | Desplazamiento lógico a la derecha |
| `01000` | `sra rd, rs1, rs2` | Desplazamiento aritmético a la derecha |
| `01001` | `slt rd, rs1, rs2` | `rd = 1` si `rs1 < rs2`, si no `0` |
| `01010` | `sle rd, rs1, rs2` | `rd = 1` si `rs1 <= rs2`, si no `0` |
| `01011` | `seq rd, rs1, rs2` | `rd = 1` si `rs1 == rs2`, si no `0` |
| `01100` | `sne rd, rs1, rs2` | `rd = 1` si `rs1 != rs2`, si no `0` |
| `01101` | `ifmov rd, rs1, rs2` | `rd = rs1` si `rs2 != 0` |
| `01110`–`01111` | — | Libres |

**Formato I** — `[sel][funct 5][rd 4][rs1 4][imm 18]`

| `funct` | Instrucción | Operación |
|---|---|---|
| `10000` | `saddi rd, rs1, imm` | `rd = rs1 + imm` |
| `10001` | `xori rd, rs1, imm` | `rd = rs1 XOR imm` |
| `10010` | `mov rd, imm` | `rd = imm`. El campo `rs1` no se usa y debe valer cero |
| `10011`–`11111` | — | Libres |

Con operandos invertidos, `slt` cubre el mayor estricto y `sle` el mayor o
igual, sin necesidad de instrucciones adicionales.

**17 instrucciones. 15 códigos libres.**

### 6.2 BRU — slot 2

**Formatos B y R** — `[0][funct 5][rs2 4][rs1 4][off 18]` para los saltos
condicionales, `[0][funct 5][rd 4][rs1 4][reservado 14]` para `jefR`

| `funct` | Formato | Instrucción | Operación |
|---|---|---|---|
| `00000` | R | `nopi` | Sin efecto |
| `00001` | B | `beq rs1, rs2, off` | Salta si `rs1 == rs2` |
| `00010` | B | `bne rs1, rs2, off` | Salta si `rs1 != rs2` |
| `00011` | B | `blt rs1, rs2, off` | Salta si `rs1 < rs2` |
| `00100` | B | `bge rs1, rs2, off` | Salta si `rs1 >= rs2` |
| `00101` | R | `jefR rd, rs1` | `rd = PC+1`; salta a la dirección en `rs1` |
| `00110`–`01111` | — | — | Libres |

**Formato J** — `[0][funct 5][rd 4][target 22]`

| `funct` | Instrucción | Operación |
|---|---|---|
| `10000` | `jef target` | Salta a `PC + target`. El campo `rd` no se usa y debe valer cero |
| `10001` | `jas rd, target` | `rd = PC+1`; salta a `PC + target` |
| `10010`–`11111` | — | Libres |

Los desplazamientos y destinos se expresan en bundles, relativos al PC.

**8 instrucciones. 24 códigos libres.**

### 6.3 LSU — slot 3

**Formato I** — `[0][funct 5][rd 4][rs1 4][imm 18]`

| `funct` | Instrucción | Operación |
|---|---|---|
| `00000` | `nopi` | Sin efecto |
| `00001` | `lw rd, imm(rs1)` | `rd = mem[rs1 + imm]`, palabra |
| `00010` | `sw rs2, imm(rs1)` | `mem[rs1 + imm] = rs2`, palabra. La fuente ocupa el campo 25..22 |
| `00011` | `lb rd, imm(rs1)` | Carga byte con extensión de signo |
| `00100` | `sb rs2, imm(rs1)` | Almacena el byte bajo de la fuente |
| `00101`–`11111` | — | Libres |

En `sw` y `sb` no se escribe ningún registro: el campo que en los demás
formatos es `rd` contiene la fuente del dato.

**5 instrucciones. 27 códigos libres.**

### 6.4 CRYPTO — slot 1 con `sel = 1`

Formato unificado para la unidad criptográfica:
`[1][funct 5][rd 4][rs1 4][rs2 4][k 2][i 2][reservado 10]`

| `funct` | Instrucción | Formato | Operación | Privilegio requerido |
|---|---|---|---|---|
| `00000` | `nopi` | CRYPTO | Sin efecto | Ninguno |
| `00001` | `feis rd, rs1, rs2, k, i` | CRYPTO | `rd = rs1 XOR F(rs2, subllave[k][i])` | `AUTH == 1` |
| `00010` | `auth rs1` | CRYPTO | Valida token de 32 bits en `rs1`: si $F(rs1, K_{\text{fija}}) == \text{ancla}$, hace `AUTH = 1` y actualiza `ancla = rs1`. Si no, activa `EXC = 1`. `rd`, `rs2`, `k`, `i` deben valer cero | Ninguno |
| `00011` | `logout` | CRYPTO | Elimina autorización (`AUTH = 0`). Todos los campos restantes en cero | Ninguno |
| `00100` | `kv_write rs1, k, w` | CRYPTO | Escribe la palabra `w` (bits `i`) de la llave `k` desde `rs1`. `rd` y `rs2` en cero | `AUTH == 1` y `KVLOCK == 0` |
| `00101` | `kv_lock` | CRYPTO | Sella la bóveda contra escrituras (`KVLOCK = 1`). Permanece bloqueada de forma permanente hasta un reinicio físico (`reset`). Todos los campos restantes en cero | `AUTH == 1` |
| `00110` | `sanc rs1` | CRYPTO | Reemplaza el ancla activa por el nuevo valor de 32 bits en `rs1` (`ancla = rs1`). Permite renovar la cadena OTP. `rd`, `rs2`, `k`, `i` deben valer cero | `AUTH == 1` |
| `00111`–`11111` | — | — | Libres | — |

**7 instrucciones. 25 códigos libres.**

**Uso de registros para credenciales de 32 bits:** Las instrucciones `auth` y `sanc` leen su token de 32 bits desde un registro general (`rs1`), eliminando la limitación de los inmediatos de 18 bits. El programador construye el valor de 32 bits utilizando la secuencia de carga de constantes descrita en la Sección 4.3 antes de invocar la instrucción.

### 6.5 Ejemplos de codificación

**`add r5, r3, r4` en el slot 0**

| Campo | Valor | Binario |
|---|---|---|
| `sel` | 0 | `0` |
| `funct` | `add` | `00001` |
| `rd` | r5 | `0101` |
| `rs1` | r3 | `0011` |
| `rs2` | r4 | `0100` |
| reservado | — | `00000000000000` |

`0 00001 0101 0011 0100 00000000000000` → `0x054D0000`

**`lw r7, 0(r6)` en el slot 3**

| Campo | Valor | Binario |
|---|---|---|
| `sel` | 0 | `0` |
| `funct` | `lw` | `00001` |
| `rd` | r7 | `0111` |
| `rs1` | r6 | `0110` |
| `imm` | 0 | `000000000000000000` |

`0 00001 0111 0110 000000000000000000` → `0x05D80000`

**`blt r3, r4, -2` en el slot 2**

| Campo | Valor | Binario |
|---|---|---|
| `sel` | 0 | `0` |
| `funct` | `blt` | `00011` |
| `rs2` | r4 | `0100` |
| `rs1` | r3 | `0011` |
| `off` | −2 | `111111111111111110` |

`0 00011 0100 0011 111111111111111110` → `0x0D0FFFFE`

**`feis r5, r3, r4, 0, 2` en el slot 1**

| Campo | Valor | Binario |
|---|---|---|
| `sel` | 1 | `1` |
| `funct` | `feis` | `00001` |
| `rd` | r5 | `0101` |
| `rs1` | r3 | `0011` |
| `rs2` | r4 | `0100` |
| `k` | 0 | `00` |
| `i` | 2 | `10` |
| reservado | — | `0000000000` |

`1 00001 0101 0011 0100 00 10 0000000000` → `0x854D0800`

**`sanc r4` en el slot 1**

| Campo | Valor | Binario |
|---|---|---|
| `sel` | 1 | `1` |
| `funct` | `sanc` | `00110` |
| `rd` | 0 | `0000` |
| `rs1` | r4 | `0100` |
| `rs2` | 0 | `0000` |
| `k` | 0 | `00` |
| `i` | 0 | `00` |
| reservado | — | `0000000000` |

`1 00110 0000 0100 0000 00 00 0000000000` → `0x98100000`

**Bundle vacío**

Los cuatro slots con `funct = 00000` y todos los demás campos en cero:

`0x00000000 00000000 00000000 00000000`

---

## 7. Bóveda de llaves

La bóveda almacena **cuatro llaves de 128 bits**, cada una dividida en cuatro
subllaves de 32 bits.

### 7.1 Aislamiento y sellado de la bóveda

La bóveda es memoria dedicada dentro de la unidad criptográfica. **No es
accesible como memoria tradicional**: ninguna instrucción de carga o
almacenamiento la alcanza, y sus datos no pueden escribirse en registros de
propósito general ni en memoria general.

La instrucción de ronda selecciona la subllave mediante los campos `k` e `i`
contenidos en la propia instrucción. **No provienen de registros**, de modo que
un programa no puede calcular índices de llave en tiempo de ejecución ni
recorrer la bóveda.

**Sellado permanente con `kv_lock`:** Cuando el software inicializa las llaves
durante el arranque, ejecuta `kv_lock`. Esta instrucción actúa como un cerrojo
permanente (*sticky latch*) que pone la bandera `KVLOCK = 1`. A partir de ese
momento, **toda escritura posterior (`kv_write`) queda totalmente bloqueada e
ignorada**, sin importar si se ejecutan posteriores instrucciones de `auth` o
`logout`. La única forma de volver a escribir en la bóveda es mediante un
reinicio físico (`reset`) del procesador.

### 7.2 Control de acceso: Protocolo Lamport OTP

Para proteger la bóveda contra espías en el bus de datos o intentos de reutilizar contraseñas capturadas, el procesador usa contraseñas de un solo uso basadas en **cadenas de Lamport**.
 
**Generación fuera de línea (*Offline* — Script en Python):**

$$\underbrace{w_0}_{\text{Secreto}} \xrightarrow{H} w_1 \xrightarrow{H} w_2 \xrightarrow{H} \cdots \xrightarrow{H} w_{N-1} \xrightarrow{H} \underbrace{w_N}_{\text{Ancla grabada en HW}}$$

**Secuencia de autenticación (consumo en orden inverso):**

| Sesión | Token enviado por software (`auth rs1`) | Verificación en Hardware | Acción del Hardware |
|:---:|:---:|:---:|:---|
| **1** | $w_{N-1}$ | ¿$F(w_{N-1}, K_{\text{fija}}) == w_N$? | $\text{ancla} \leftarrow w_{N-1}$, activa `AUTH = 1` |
| **2** | $w_{N-2}$ | ¿$F(w_{N-2}, K_{\text{fija}}) == w_{N-1}$? | $\text{ancla} \leftarrow w_{N-2}$, activa `AUTH = 1` |
| **3** | $w_{N-3}$ | ¿$F(w_{N-3}, K_{\text{fija}}) == w_{N-2}$? | $\text{ancla} \leftarrow w_{N-3}$, activa `AUTH = 1` |

#### ¿Cómo funciona?

1. **Aprovechar el hardware existente:** Para no gastar área metiendo un algoritmo de hash pesado (como SHA-256), usamos la misma función de ronda $F$ de Feistel4 sumándole una constante fija ($K_{\text{fija}} = \text{0x6A09E667}$).

$$H(x) = F(x, K_{\text{fija}}) = (\text{ROL32}(x, 5) + K_{\text{fija}}) \oplus \text{ROL32}(x, 13)$$

* **¿Por qué actúa como función de un solo sentido (no invertible)?** Aunque el cifrador Feistel completo es reversible por su estructura cruzada, la función interna $F(x, k)$ **en sí misma no es invertible**. Al ser una estructura ARX (*Addition-Rotation-XOR*), combina la propagación de acarreos no lineales de la suma modular ($\text{mod } 2^{32}$) con dos rotaciones distintas (5 y 13 bits) mezcladas mediante XOR. Esta combinación destruye la linealidad de los bits: calcular el resultado hacia adelante toma solo 1 ciclo de reloj, pero despejar algebraicamente la entrada $x$ a partir del resultado es imposible en tiempo real sin una búsqueda exhaustiva.

2. **Crear la cadena de contraseñas:** En un entorno de prueba o software del cliente (como un programa de Python) se toma una clave secreta inicial $w_0$ y se le aplica la función $H$ varias veces seguidas para crear una cadena ($w_1, w_2, \dots, w_N$). El último valor generado ($w_N$, llamado **Ancla**) es el que se le programa al procesador en su registro interno `ancla_otp` durante el encendido o reset.

3. **Verificar cada inicio de sesión:** Las contraseñas se usan en orden inverso ($w_{N-1}, w_{N-2}, \dots$). Cuando el usuario quiere entrar, pone su token en un registro y ejecuta `auth rs1`:
   * El hardware le aplica $F(rs1, K_{\text{fija}})$ y lo compara con el ancla que tiene guardada.
   * Si coincide, activa `AUTH = 1` y actualiza automáticamente su ancla interna con este nuevo valor (`ancla_otp <= rs1`).
   * Si no coincide, pone `AUTH = 0` y levanta la bandera de error `EXC = 1`.

4. **Protección contra espías:** Si alguien intercepta el bus y copia la contraseña que pasó ($w_{k-1}$), no le sirve de nada: esa contraseña ya se usó y el procesador ahora exige la anterior de la cadena ($w_{k-2}$), la cual no se puede calcular a partir de la que vio debido a la no-invertibilidad de $F$.

#### Cambiar de contraseña (`sanc rs1`)

Como la cadena de contraseñas es finita y en algún momento se agota, el usuario puede renovarla mientras esté autenticado:

1. Inicia sesión con su contraseña actual (`auth rs1`).
2. Genera en software una nueva cadena a partir de un secreto nuevo y obtiene la nueva ancla final.
3. Carga esa nueva ancla en un registro y ejecuta `sanc rs1`. El hardware sobreescribe `ancla_otp <= rs1`.
4. Ejecuta `logout`. A partir de ese momento la cadena vieja queda anulada y se empieza a usar la nueva.

*(Si alguien intenta ejecutar `sanc` sin estar autenticado previamente con `AUTH == 1`, la instrucción se ignora y se activa la excepción `EXC = 1`).*

#### Reinicio de fábrica (Hard Reset)

Si el usuario pierde su lista de contraseñas, no existe ninguna instrucción por software que permita saltarse la seguridad. La única forma de recuperar el procesador es activando la señal física de `reset`, la cual restaura el ancla inicial de fábrica (`0xCAFEBABE`), borra las llaves de la bóveda y limpia las banderas de estado (`AUTH = 0`, `KVLOCK = 0`, `EXC = 0`).

---

## 8. Banco de registros

**Dieciséis registros de propósito general de 32 bits.**

| Registro | Codificación | Rol | Naturaleza |
|---|---|---|---|
| `rz` | 0 | Constante cero cableada | Hardware: las lecturas retornan cero y las escrituras se descartan |
| `rl` | 1 | Enlace de retorno | Hardware: `jas` escribe aquí |
| `rg` | 3 | Puntero global | Convención: base de la zona de datos estáticos |
| `rx`, `ra`–`rt` | 2, 4–15 | Trece temporales generales | Libres |

### 8.1 Sobre la especialización de registros

Se consideró asignar roles semánticos a la mayoría de los registros:
iteradores de ciclo, banderas de condición, buffer hacia la bóveda y argumentos
de llamada. Esa asignación se descartó.

El motivo es cuantitativo: reservar registros para esos roles dejaba **cinco
temporales reales** disponibles para evaluar expresiones, mientras que una sola
expresión de complejidad media puede tener diez valores vivos simultáneamente.

El hardware no distingue entre registros: ve un campo de cuatro bits y lee ese
registro. Los roles de conveniencia son, por tanto, **convención del
ensamblador y del compilador**, no restricción del ISA.

### 8.2 Registro de estado

Fuera del banco de registros, igual que el contador de programa. Ninguna
instrucción lo nombra por número.

| Bit | Nombre | Lo enciende | Función |
|---|---|---|---|
| 0 | `AUTH` | `auth` con token válido | Habilita las operaciones sobre la bóveda |
| 1 | `KVLOCK` | `kv_lock` | Bloquea la bóveda contra escrituras |
| 2 | `EXC` | Operación no autorizada o credencial inválida | Señala un error de acceso o trampa de seguridad |
| 3–31 | — | — | Reservados |

---

## 9. Organización segmentada

**Pipeline de cuatro etapas y flujo de datos.**

![Organización general del procesador y flujo de datos](docs/images/organizacion_procesador.png)

Una etapa adicional dedicada al acceso a memoria aumentaría en uno la cantidad
de bundles de separación que el generador de código debe dejar entre
operaciones dependientes, sin beneficio funcional que lo compense.

### 9.1 Escritura y lectura del banco de registros en flancos opuestos

El banco de registros se **escribe en el flanco de subida y se lee en el flanco
de bajada** del mismo ciclo. Como dentro de un ciclo la subida precede a la
bajada, un resultado escrito por la etapa WB queda disponible para la etapa ID
en ese mismo ciclo, sin necesidad de lógica de adelantamiento.

| Esquema de acceso al banco | Separación requerida entre operaciones dependientes |
|---|---|
| Escritura y lectura en el mismo flanco | 2 bundles |
| **Flancos opuestos** | **1 bundle** |

El costo en hardware es prácticamente nulo y reduce a la mitad la cantidad de
`nopi` de un programa típico. Esta característica forma parte del contrato con
el generador de código, puesto que determina la separación que debe dejar entre
operaciones dependientes.

### 9.2 Latencia uniforme

Todas las unidades funcionales escriben sus resultados en la etapa WB. El
generador de código trabaja con un único valor de separación en lugar de una
tabla de latencias por unidad.

### 9.3 Semántica de lectura dentro de un bundle

Los cuatro slots leen sus operandos del banco de registros durante la etapa ID,
antes de que cualquier resultado del mismo bundle se escriba. En consecuencia,
**un slot que lee un registro que otro slot del mismo bundle escribe obtiene el
valor previo a la ejecución del bundle**.

Esta semántica permite que un mismo bundle incremente una variable de control,
la compare contra un límite y la use como índice, con los tres slots operando
de forma coherente sobre el valor anterior.

**Dos slots del mismo bundle no deben escribir el mismo registro destino.**

---

## 10. Estrategia frente a saltos

**Vaciado de pipeline, sin *delay slots* explícitos.** Los saltos se resuelven
en la etapa de decodificación, de modo que se descarta un único bundle por
salto tomado.

Los *delay slots* se descartaron por tres razones:

1. La cantidad de huecos quedaría determinada por la profundidad del pipeline,
   atando la especificación del ISA a una decisión de microarquitectura
   posterior.
2. Complicarían la escritura manual de programas en ensamblador, requerida de
   forma independiente del compilador.
3. Llenar un hueco requiere una operación independiente, que es el mismo
   recurso escaso que el generador de código emplea para llenar los demás slots
   del bundle.
