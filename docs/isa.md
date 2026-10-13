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

**Entrega 1:** 25 de setiembre de 2026
**Revisión 1:** 6 de octubre de 2026 — aclaraciones posteriores a la Entrega 1,
detalladas en la [§12](#12-registro-de-cambios-posteriores-a-la-entrega-1).
Ninguna modifica la codificación de las instrucciones.

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
   - [9.4 Dependencias a través de memoria](#94-dependencias-a-través-de-memoria)
10. [Estrategia frente a saltos](#10-estrategia-frente-a-saltos)
11. [Sintaxis del ensamblador](#11-sintaxis-del-ensamblador)
12. [Registro de cambios posteriores a la Entrega 1](#12-registro-de-cambios-posteriores-a-la-entrega-1)

---

## 1. Descripción general

Este documento especifica la arquitectura de un procesador **VLIW** (*Very Long Instruction Word*) con un set de instrucciones propio de 32 bits, capaz de ejecutar programas de propósito general e incorporar instrucciones dedicadas en hardware para el algoritmo de cifrado **Feistel4**.

El procesador trabaja con **paralelismo estático**: el software agrupa cuatro operaciones independientes en un solo paquete de 128 bits (*bundle*), y el procesador las despacha simultáneamente a sus respectivas unidades funcionales para ejecutarlas en paralelo en un mismo ciclo de reloj.

### 1.1 Modelo estático y riesgos de datos

Para mantener un diseño de hardware simple y eficiente, el procesador **no implementa mecanismos automáticos de control de riesgos** (sin adelantamiento o *forwarding*, sin *scoreboarding* ni pausas del pipeline por dependencias). La responsabilidad de respetar las dependencias entre datos y dejar los espacios necesarios recae completamente en el software (el compilador o el programador en ensamblador).

**Única pausa: `auth`.** La autenticación calcula un hash de 12 rondas y ocupa la unidad CRYPTO 14 ciclos. Mientras tanto el hardware congela el pipeline. Es una pausa por **riesgo estructural** (un recurso ocupado), que el enunciado §4.1 permite si se documenta y justifica (§9.2). No depende de ningún dato, así que no cambia la regla de separación que el software debe respetar.

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

**Codificaciones reservadas.** Un programa no debe usar un `funct` marcado como
«libre» ni un `sel = 1` en los slots 0, 2 o 3. El ensamblador las rechaza. Si
aun así llegan al procesador, un `funct` libre se ejecuta como `nopi` y el bit
`sel` de los slots 0, 2 y 3 se ignora.

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

Un literal de 32 bits no cabe en un inmediato de 18. Se construye con una
secuencia de instrucciones. `sll` es de formato R: la cantidad de desplazamiento
viene de un registro, no de un inmediato, de modo que el 15 se carga primero en
un registro temporal (aquí `t0`):

| Bundle | Slot 0 | Slot 1 | Efecto |
|---|---|---|---|
| 1 | `mov rd, altos_17` | `mov t0, 15` | Carga los 17 bits superiores y la cantidad de desplazamiento |
| 2 | `nopi` | `nopi` | Separación entre dependientes (§9.1) |
| 3 | `sll rd, rd, t0` | — | Desplaza 15 posiciones a la izquierda |
| 4 | `nopi` | — | Separación |
| 5 | `saddi rd, rd, bajos_15` | — | Suma con signo los 15 bits inferiores |

Las dos cargas iniciales van en el mismo bundle, en las dos ALU, así que la
secuencia ocupa los mismos bundles que si existiera un desplazamiento con
inmediato. Los bundles de separación pueden llenarse con otras operaciones
independientes.

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
| Motor de hash `auth_hash32` | 320 bits de estado | calcula $H(rs1)$ con Ascon-Hash256, una ronda por ciclo (§7.2) |
| Comparador de credencial | 32 bits | compara $H(rs1)$ contra `ancla_otp` en `auth` |
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

**Hash de autenticación.** El protocolo Lamport OTP (§7.2) usa una función $H$
separada de $F$: Ascon-Hash256 (NIST SP 800-232) con entrada fija de 4 bytes y
salida truncada a 32 bits. Se calcula con un motor secuencial de una ronda por
ciclo que reutiliza la misma lógica de ronda 12 veces, en lugar de encadenar las
12 rondas en una sola ruta combinacional. El detalle está en el diseño de la
unidad (`microarchitecture.md` §3.10).

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

Al terminar las 4 rondas, el bloque resultante queda disponible en el par (`r4` = $L_4$, `r5` = $R_4$).

**Descifrado.** Usa la misma alternancia de registros con las subllaves en orden inverso (`3, 2, 1, 0`), pero **con las mitades intercambiadas**: se carga `r4` = $R_4$ y `r5` = $L_4$, y al terminar queda `r4` = $R_0$ y `r5` = $L_0$, que se guardan también intercambiadas. Sin el intercambio el resultado no es el texto original (verificado contra la implementación de referencia del enunciado, Fig. 1).

**Aislamiento de la bóveda.** El multiplexor de subllave alimenta únicamente la
función de ronda. No existe ningún camino desde la bóveda hacia el banco de
registros ni hacia la interfaz de memoria, de modo que su contenido no puede
extraerse por ninguna instrucción del set.

**Latencia:** un ciclo en todas las operaciones salvo `auth`, que ocupa 14
ciclos en EX: uno de carga, doce de ronda y uno de finalización. Durante ese
tiempo el pipeline se congela (§1.1, §9.2).

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

**Precisiones.**
- Las comparaciones `slt` y `sle` son **con signo** (complemento a dos).
- En `sll`, `srl` y `sra`, la cantidad de desplazamiento son los **5 bits
  bajos** de `rs2`; los demás bits se ignoran.
- En `ifmov`, si `rs2 == 0` el registro `rd` **conserva su valor**: la
  instrucción no escribe.

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

**Precisiones.**
- `PC` es el índice del bundle que contiene la instrucción de salto.
- `blt` y `bge` comparan **con signo** (complemento a dos), igual que `slt` en
  la ALU.
- En `jas` y `jefR`, el enlace se escribe en el registro indicado por `rd`. Por
  convención se usa `rl` (§8), pero cualquier registro es válido; con `rd = rz`
  el enlace se descarta.
- `jefR` salta al bundle indicado por los **22 bits bajos** de `rs1`, el ancho
  del contador de programa. En uso normal `rs1` contiene un enlace guardado por
  `jas`, cuyos bits altos valen cero.

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
| `00010` | `auth rs1` | CRYPTO | Valida token de 32 bits en `rs1`: si $H(rs1) == \text{ancla}$, hace `AUTH = 1` y actualiza `ancla = rs1`. Si no, activa `EXC = 1`. `rd`, `rs2`, `k`, `i` deben valer cero | Ninguno |
| `00011` | `logout` | CRYPTO | Elimina autorización (`AUTH = 0`). Todos los campos restantes en cero | Ninguno |
| `00100` | `kv_write rs1, k, w` | CRYPTO | Escribe la palabra `w` (bits `i`) de la llave `k` desde `rs1`. `rd` y `rs2` en cero | `AUTH == 1` y `KVLOCK == 0` |
| `00101` | `kv_lock` | CRYPTO | Sella la bóveda contra escrituras (`KVLOCK = 1`). Permanece bloqueada de forma permanente hasta un reinicio físico (`reset`). Todos los campos restantes en cero | `AUTH == 1` |
| `00110` | `sanc rs1` | CRYPTO | Reemplaza el ancla activa por el nuevo valor de 32 bits en `rs1` (`ancla = rs1`). Permite renovar la cadena OTP. `rd`, `rs2`, `k`, `i` deben valer cero | `AUTH == 1` |
| `00111`–`11111` | — | — | Libres | — |

**7 instrucciones. 25 códigos libres.**

**Operaciones no autorizadas.** Toda instrucción que se ejecute sin el privilegio
requerido **no tiene efecto** (no escribe `rd`, no modifica la bóveda ni el
ancla) y activa `EXC = 1`. Esto aplica a:

| Instrucción | Condición de rechazo |
|---|---|
| `feis` | `AUTH == 0` |
| `kv_write` | `AUTH == 0` o `KVLOCK == 1` |
| `kv_lock` | `AUTH == 0` |
| `sanc` | `AUTH == 0` |

**`auth` fallido.** Si la credencial no es válida, `auth` pone `AUTH = 0`
(cierra cualquier sesión abierta) y `EXC = 1`. El ancla no cambia.

**Limpieza de `EXC`.** Solo un `auth` exitoso o el `reset` ponen `EXC = 0`. Así
un intento rechazado queda registrado hasta la siguiente autenticación válida.

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

Para impedir que una contraseña capturada se reutilice, el procesador usa contraseñas de un solo uso basadas en **cadenas de Lamport**.

**Generación fuera de línea (*Offline* — Script en Python):**

$$\underbrace{w_0}_{\text{Secreto}} \xrightarrow{H} w_1 \xrightarrow{H} w_2 \xrightarrow{H} \cdots \xrightarrow{H} w_{N-1} \xrightarrow{H} \underbrace{w_N}_{\text{Ancla grabada en HW}}$$

**Secuencia de autenticación (consumo en orden inverso):**

| Sesión | Token enviado por software (`auth rs1`) | Verificación en Hardware | Acción del Hardware |
|:---:|:---:|:---:|:---|
| **1** | $w_{N-1}$ | ¿$H(w_{N-1}) == w_N$? | $\text{ancla} \leftarrow w_{N-1}$, activa `AUTH = 1` |
| **2** | $w_{N-2}$ | ¿$H(w_{N-2}) == w_{N-1}$? | $\text{ancla} \leftarrow w_{N-2}$, activa `AUTH = 1` |
| **3** | $w_{N-3}$ | ¿$H(w_{N-3}) == w_{N-2}$? | $\text{ancla} \leftarrow w_{N-3}$, activa `AUTH = 1` |

#### ¿Cómo funciona?

1. **La función $H$:** Ascon-Hash256 (NIST SP 800-232), especializada a una entrada fija de 4 bytes (el token) y una salida truncada a 32 bits:

$$H(x) = \text{Ascon-Hash256}(x)\big[31{:}0\big]$$

   El token entra en el orden `x[7:0]`, `x[15:8]`, `x[23:16]`, `x[31:24]`, con el relleno estándar. El hardware la calcula en 14 ciclos (§5.5).

* **Alcance de esta elección.** Ascon es un hash estándar: a diferencia de la versión anterior basada en $F$, que se invertía con $2^8$ candidatos, $H$ no se invierte en la práctica. La limitación que se mantiene es el **ancho del token**: con 32 bits, una búsqueda exhaustiva de $2^{32}$ candidatos es viable con cualquier función $H$. Es inherente al ancho de palabra de la arquitectura y se declara como limitación conocida.

2. **Crear la cadena de contraseñas:** En un entorno de prueba o software del cliente (como un programa de Python) se toma una clave secreta inicial $w_0$ y se le aplica la función $H$ varias veces seguidas para crear una cadena ($w_1, w_2, \dots, w_N$). El último valor generado ($w_N$, llamado **Ancla**) es el que se le programa al procesador en su registro interno `ancla_otp` durante el arranque protegido; el reset normal conserva el ancla activa.

3. **Verificar cada inicio de sesión:** Las contraseñas se usan en orden inverso ($w_{N-1}, w_{N-2}, \dots$). Cuando el usuario quiere entrar, pone su token en un registro y ejecuta `auth rs1`:
   * El hardware calcula $H(rs1)$ y lo compara con el ancla que tiene guardada.
   * Si coincide, activa `AUTH = 1` y actualiza automáticamente su ancla interna con este nuevo valor (`ancla_otp <= rs1`).
   * Si no coincide, pone `AUTH = 0` y levanta la bandera de error `EXC = 1`.

4. **Qué protege y qué no:**
   * **Protege contra la reutilización directa.** Una contraseña ya consumida ($w_{k-1}$) no vuelve a servir: el ancla avanzó y el procesador ahora exige la anterior de la cadena ($w_{k-2}$).
   * **No protege contra una búsqueda exhaustiva.** Con tokens de 32 bits, quien intercepte $w_{k-1}$ puede probar los $2^{32}$ valores posibles hasta encontrar $w_{k-2}$. El mecanismo demuestra el protocolo de Lamport en hardware, no es una garantía de seguridad para uso real.

#### Cambiar de contraseña (`sanc rs1`)

Como la cadena de contraseñas es finita y en algún momento se agota, el usuario puede renovarla mientras esté autenticado:

1. Inicia sesión con su contraseña actual (`auth rs1`).
2. Genera en software una nueva cadena a partir de un secreto nuevo y obtiene la nueva ancla final.
3. Carga esa nueva ancla en un registro y ejecuta `sanc rs1`. El hardware sobreescribe `ancla_otp <= rs1`.
4. Ejecuta `logout`. A partir de ese momento la cadena vieja queda anulada y se empieza a usar la nueva.

*(Si alguien intenta ejecutar `sanc` sin estar autenticado previamente con `AUTH == 1`, la instrucción se ignora y se activa la excepción `EXC = 1`).*

#### Reset normal y arranque protegido

Un reset normal borra las llaves, limpia `AUTH`, `KVLOCK` y `EXC`, y cancela
el hash pendiente. **Conserva el ancla activa**, incluso si fue modificada
por `auth` o `sanc`, para que reiniciar no vuelva a habilitar tokens consumidos.

La inicialización de fábrica usa `rst = 1` e `init_anchor = 1` durante al
menos una subida de reloj; luego desactiva `init_anchor` antes de liberar
`rst`. Esta señal procede del arranque o configuración protegida, no de
una instrucción ni del reset normal. Carga `RESET_ANCHOR`, último eslabón de
la cadena de fábrica: para la semilla `0xCAFEDECE` y 100 tokens,
`RESET_ANCHOR = 0xFEFF95FF`; los primeros tokens son `0xF47BD70B` y `0xA45326BA`.

Si se pierde la lista de tokens, un reset normal no restaura la cadena:
se necesita el arranque/configuración protegida para volver al ancla de
fábrica, o una sesión válida para ejecutar `sanc`. La conservación del
ancla es volátil: no sobrevive a pérdida de alimentación o reconfiguración.
Estos mecanismos no modifican la codificación de las instrucciones.

---

## 8. Banco de registros

**Dieciséis registros de propósito general de 32 bits.**

| Registro | Codificación | Rol | Naturaleza |
|---|---|---|---|
| `rz` | 0 | Constante cero cableada | Hardware: las lecturas retornan cero y las escrituras se descartan |
| `rl` | 1 | Enlace de retorno | Convención: destino por defecto del enlace de `jas` y `jefR` (§6.2) |
| `rg` | 3 | Puntero global | Convención: base de la zona de datos estáticos |
| `rx` | 2 | Temporal general | Libre |
| `t0`–`t11` | 4–15 | Doce temporales generales: `tN` es el registro `N + 4` | Libres |

Los nombres son alias del ensamblador: `r0`–`r15` son válidos siempre, y
`t0` y `r4` designan el mismo registro.

| Nombre | `rz` | `rl` | `rx` | `rg` | `t0` | `t1` | `t2` | `t3` | `t4` | `t5` | `t6` | `t7` | `t8` | `t9` | `t10` | `t11` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Número | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |

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
| 2 | `EXC` | Operación no autorizada o credencial inválida (§6.4) | Señala un error de acceso. Solo lo limpian un `auth` exitoso o el `reset` |
| 3–31 | — | — | Reservados |

---

## 9. Organización segmentada

**Pipeline de cuatro etapas y flujo de datos.**

![Organización general del procesador y flujo de datos](img/organizacion_procesador.png)

Una etapa adicional dedicada al acceso a memoria aumentaría en uno la cantidad
de bundles de separación que el generador de código debe dejar entre
operaciones dependientes, sin beneficio funcional que lo compense.

### 9.1 Escritura y lectura del banco de registros en flancos opuestos

El banco de registros se **escribe en la primera mitad del ciclo y se lee en la
segunda**: la escritura ocurre en el flanco de bajada del reloj y la lectura es
combinacional. Como los registros de segmentación avanzan en el flanco de
subida, el valor que la etapa WB escribe a mitad del ciclo ya está disponible
para la etapa ID antes de que termine ese mismo ciclo, sin necesidad de lógica
de adelantamiento.

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

**Excepción: `auth`.** Ocupa CRYPTO 14 ciclos y el hardware congela el
pipeline completo mientras tanto (PC, IF/ID e ID/EX se conservan; EX/WB recibe
burbujas; los almacenamientos y saltos se aplican una sola vez al terminar). Es
una pausa por riesgo estructural, permitida por el enunciado §4.1, y es
transparente para el software: la separación de 1 bundle no cambia y el
compilador no inserta nada. El bundle siguiente a `auth` ya ve el estado de
acceso actualizado.

**Por qué pausa y no latencia expuesta.** Se evaluó la alternativa de no pausar
y que el compilador dejara 13 bundles sin operaciones CRYPTO después de cada
`auth`. Simulando un programa completo, un error de calendarización de un solo
bundle hacía que un `kv_write` se rechazara en silencio: el archivo se cifraba
con una llave equivocada y el descifrado igual devolvía el original, de modo
que una prueba de ida y vuelta no lo detectaba. La pausa cuesta 13 ciclos por
`auth` (menos del 1 % al cifrar 1 KB) y elimina ese riesgo.

### 9.3 Semántica de lectura dentro de un bundle

Los cuatro slots leen sus operandos del banco de registros durante la etapa ID,
antes de que cualquier resultado del mismo bundle se escriba. En consecuencia,
**un slot que lee un registro que otro slot del mismo bundle escribe obtiene el
valor previo a la ejecución del bundle**.

Esta semántica permite que un mismo bundle incremente una variable de control,
la compare contra un límite y la use como índice, con los tres slots operando
de forma coherente sobre el valor anterior.

**Dos slots del mismo bundle no deben escribir el mismo registro destino.** El
ensamblador lo rechaza; el hardware no lo verifica y el resultado queda
indefinido.

La regla aplica también a los saltos: la BRU lee sus operandos en ID como los
demás slots, de modo que un salto condicional que compara un valor producido
por otro slot necesita la misma separación de un bundle. Un salto **no** ve el
resultado de una ALU de su propio bundle.

### 9.4 Dependencias a través de memoria

La memoria de datos se escribe al final de la etapa EX y se lee durante la etapa
EX. Un `sw` (o `sb`) y un `lw` (o `lb`) a la misma dirección **no requieren
separación**: si el `lw` está en el bundle inmediatamente siguiente, ya obtiene
el valor almacenado. La separación de un bundle aplica a las dependencias entre
**registros**, incluido el uso del registro destino de una carga.

---

## 10. Estrategia frente a saltos

**Vaciado de pipeline, sin *delay slots* explícitos.** Los saltos se resuelven
en la etapa de decodificación, de modo que se descarta un único bundle por
salto tomado.

**Un bundle que contiene un salto se ejecuta completo.** Las operaciones de los
demás slots de ese bundle se ejecutan siempre, se tome o no el salto; lo que se
descarta es el bundle **siguiente**, que ya había ingresado a la etapa de
búsqueda. El generador de código puede, y debe, colocar trabajo útil junto al
salto.

Los *delay slots* se descartaron por tres razones:

1. La cantidad de huecos quedaría determinada por la profundidad del pipeline,
   atando la especificación del ISA a una decisión de microarquitectura
   posterior.
2. Complicarían la escritura manual de programas en ensamblador, requerida de
   forma independiente del compilador.
3. Llenar un hueco requiere una operación independiente, que es el mismo
   recurso escaso que el generador de código emplea para llenar los demás slots
   del bundle.

---

## 11. Sintaxis del ensamblador

El ensamblador propio traduce un programa escrito con esta sintaxis al archivo
de inicialización de la memoria de instrucciones (formato en
`microarchitecture.md` §1.5). Es la misma sintaxis que emite el compilador de
CE1108, de modo que ambos producen programas intercambiables.

### 11.1 Estructura

- **Un bundle por línea**, entre llaves, con los **cuatro slots siempre
  presentes**, separados por `|`, en orden: slot 0, slot 1, slot 2, slot 3.
- **Comentarios** con `#`, hasta el final de la línea.
- **Etiquetas** con `nombre:`, en su propia línea o antes de un bundle. Una
  etiqueta vale el **índice del bundle** que la sigue.
- Las líneas vacías se ignoran.

```
# Cuenta en r4 mientras r4 < r5.
# blt lee el r4 anterior al saddi del mismo bundle (§9.3): termina con r4 = 4.
inicio:
  { mov r4, 0        | mov r5, 3 | nopi             | nopi }
  { nopi             | nopi      | nopi             | nopi }   # separación
lazo:
  { saddi r4, r4, 1  | nopi      | blt r4, r5, lazo | nopi }
```

Salida del ensamblador para este programa (un bundle por línea, slot 3 a la
izquierda):

```
00000000_00000000_49400003_49000000
00000000_00000000_00000000_00000000
00000000_0D500000_00000000_41100001
```

### 11.2 Operandos

| Elemento | Forma |
|---|---|
| Registro | `r0` a `r15`, o los nombres de la §8: `rz`, `rl`, `rx`, `rg`, `t0`–`t11` |
| Inmediato | Decimal con signo (`-5`) o hexadecimal (`0x1F`) |
| Destino de salto | Una etiqueta, o un desplazamiento entero en bundles. Con etiqueta, el ensamblador calcula `etiqueta − PC` |
| `k`, `i`, `w` | Enteros de 0 a 3 |

### 11.3 Formas de cada instrucción

| Slot | Unidad | Formas |
|---|---|---|
| 0 o 1 | ALU | `add`/`sub`/`and`/`or`/`xor`/`sll`/`srl`/`sra`/`slt`/`sle`/`seq`/`sne`/`ifmov rd, rs1, rs2` · `saddi`/`xori rd, rs1, imm` · `mov rd, imm` |
| 1 | CRYPTO | `feis rd, rs1, rs2, k, i` · `auth rs1` · `logout` · `kv_write rs1, k, w` · `kv_lock` · `sanc rs1` |
| 2 | BRU | `beq`/`bne`/`blt`/`bge rs1, rs2, destino` · `jef destino` · `jas rd, destino` · `jefR rd, rs1` |
| 3 | LSU | `lw`/`lb rd, imm(rs1)` · `sw`/`sb rs2, imm(rs1)` |
| Cualquiera | — | `nopi` |

En el slot 1, el ensamblador pone `sel = 1` si la instrucción es de CRYPTO y
`sel = 0` si es de la ALU.

### 11.4 Errores que el ensamblador rechaza

- Un bundle sin exactamente cuatro slots.
- Una instrucción en un slot que no corresponde a su unidad (por ejemplo, `lw`
  en el slot 0).
- Un inmediato, desplazamiento o destino fuera de rango (§4.2).
- Una etiqueta no definida o definida dos veces.
- Dos slots del mismo bundle que escriben el mismo registro, salvo `rz` (§9.3).
- Un mnemónico desconocido.

---

## 12. Registro de cambios posteriores a la Entrega 1

El enunciado (§3) exige que todo cambio posterior a la Entrega 1 se justifique y
se comunique formalmente. Ninguno de estos cambios modifica la codificación
binaria: los vectores de la §6.5 siguen siendo válidos.

| # | Sección | Cambio | Justificación |
|---|---|---|---|
| C-01 | 4.1 | Comportamiento de las codificaciones reservadas: `funct` libre se ejecuta como `nopi`; `sel` se ignora fuera del slot 1 | El documento no definía qué ocurre con ellas |
| C-02 | 4.3 | La secuencia de constantes de 32 bits usaba `sll rd, rd, 15`, pero `sll` es de formato R y no admite inmediato. Se corrige cargando el 15 en un registro, en el mismo bundle que la parte alta | Error de la Entrega 1. La secuencia corregida ocupa los mismos bundles |
| C-03 | 6.1 | `slt` y `sle` son con signo; los desplazamientos usan los 5 bits bajos de `rs2`; `ifmov` no escribe si `rs2 == 0` | Precisión de semántica ya implícita |
| C-04 | 6.2 | `blt` y `bge` son con signo | Coherencia con `slt`; sin signo, un ciclo con límite negativo no se ejecutaría |
| C-05 | 6.2, 8 | `jas` y `jefR` escriben el enlace en el `rd` indicado; `rl` pasa de «hardware» a «convención» | El formato J ya tenía `rd` libre; la §8 contradecía a la §6.2 |
| C-06 | 6.2 | `jefR` usa los 22 bits bajos de `rs1` | El contador de programa tiene el ancho del sumador de destino (§5.3) |
| C-07 | 6.4 | `feis`, `kv_write` y `kv_lock` sin privilegio no tienen efecto y activan `EXC`, igual que `sanc` | El enunciado §4.4.1 exige que toda operación no autorizada genere excepción |
| C-08 | 6.4, 7.2 | Un `auth` fallido pone `AUTH = 0` y `EXC = 1` | La §6.4 y la §7.2 decían cosas distintas |
| C-09 | 6.4, 8.2 | `EXC` solo se limpia con un `auth` exitoso o con `reset` | No estaba definido |
| C-10 | 5.5, 7.2 | Se corrige la afirmación de que $F$ no es invertible y se declara el alcance real del mecanismo | La afirmación era falsa y verificable: $F$ se invierte con $2^8$ candidatos |
| C-11 | 9.1 | La escritura del banco de registros ocurre en el flanco de bajada y la lectura es combinacional | Con los registros de segmentación en flanco de subida, escribir en subida exigiría 2 bundles de separación. La separación de 1 bundle no cambia |
| C-12 | 9.3, 9.4, 10 | Se explicitan: el salto no ve resultados de su propio bundle; `sw`→`lw` no requiere separación; un bundle con salto se ejecuta completo | Semántica ya implícita, ahora explícita para el compilador |
| C-13 | 11 | Se agrega la sintaxis del ensamblador | El enunciado §6.1 exige entregarla como parte del contrato con CE1108 |
| C-14 | 8 | Los temporales 4–15 se nombran `t0`–`t11` | La Entrega 1 los nombraba «`ra`–`rt`» sin indicar qué número correspondía a cada nombre. Son alias del ensamblador; la codificación no cambia |
| C-15 | 5.5, 6.4, 7.2 | $H$ deja de ser $F(x, K_{\text{fija}})$ y pasa a ser Ascon-Hash256 truncado a 32 bits. `RESET_ANCHOR` pasa a ser el final de una cadena con esa $H$ | $F$ se invertía con $2^8$ candidatos (C-10). Ascon corrige esa debilidad; la limitación de 32 bits del token se mantiene y se declara |
| C-16 | 1.1, 5.5, 9.2 | `auth` ocupa 14 ciclos y congela el pipeline por riesgo estructural | El hash secuencial necesita 14 ciclos. El enunciado §4.1 permite pausas estructurales documentadas. Es transparente para el compilador |
| C-17 | 5.5 | El descifrado carga y guarda las mitades intercambiadas | La descripción anterior («misma alternancia») no devolvía el texto original |
| C-18 | 7.2 | El reset normal conserva el ancla; `init_anchor` la inicializa durante el arranque protegido. Ancla de demostración `0xFEFF95FF` | Evita que un reset normal habilite tokens ya consumidos; coincide con el diseño Ascon implementado por Saddy. La integración del arranque en el core queda pendiente de Luis |
