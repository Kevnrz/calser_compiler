# Ejercicios: ensamblador, binario y evidencia de ejecución

Entregables de los tres ejercicios (enunciado CE1108 §10.4 y §10.5). Cada
carpeta tiene el fuente en Calser, el ensamblador y el binario que genera
`ccom`, la salida de la compilación, la traza de ejecución (§8.4) y el resumen
de la ejecución contra los valores esperados.

| Carpeta | Programa | Fuente única |
|---|---|---|
| `ejercicio1_factorial/` | Factorial de 5 | `pruebas/p02_factorial.cls` |
| `ejercicio2_matriz/` | Búsqueda de un valor en una matriz | `pruebas/p12_ejercicio2.cls` |
| `ejercicio3_cifrado/` | Cifrado y descifrado de un archivo por tramos | `pruebas/p11_ejercicio3.cls` |

La fuente única sigue siendo la de `pruebas/`: `make ejercicios` la copia a
cada carpeta como `ejercicioN.cls`. No se edita la copia.

## Qué pide el enunciado y dónde se cumple

Los requisitos son los de CE1108 §7, textuales; a la derecha, la línea o la
construcción del `.cls` donde se cumple (los números son líneas del fuente de
cada carpeta).

### Ejercicio 1, factorial (§7.1)

| Requisito | Dónde se cumple |
|---|---|
| Variables locales | `Vm i : int = 2 ;` en la línea 6; también `res`, la variable de retorno (línea 1), y `Vm f : int ;` en `main` (línea 16) |
| Condicionales (if) | `C [ n < 0 ] ;` en la línea 2 |
| Ciclos (while o for) | `L [ i <= n ] ;` en la línea 7 (un while) |
| Operaciones aritméticas | `res = res * i ;` en la línea 8 y `i = i + 1 ;` en la línea 9; la multiplicación se resuelve por software con la rutina interna `__ccom_arith_*` |

Además: una función con parámetro y valor de retorno (`factorial [ int n ] : int res`, línea 1), llamada como expresión en `main` (línea 17).

### Ejercicio 2, búsqueda en un arreglo de 2 dimensiones que retorna su posición (§7.2)

| Requisito | Dónde se cumple |
|---|---|
| Declaración y uso de arreglos (de tamaño n) | Declaración: la matriz global `Vi m : int [3][4]` con sus 12 valores (líneas 1 a 3), el parámetro `Vi int mat [3][4]` y el arreglo de salida `int pos [2]` (línea 5), y `Vm p : int [2]` en `main` (línea 22). Uso: lectura `mat[i][j]` (línea 10) y escritura `pos[0] = i`, `pos[1] = j` (líneas 11 y 12) |
| Ciclos con condición de salida | Dos ciclos `S` anidados, `S [ i, 0, 3, 1 ]` (línea 8) y `S [ j, 0, 4, 1 ]` (línea 9), que terminan cuando la variable llega al límite (3 y 4); y la salida anticipada con `E ;` en la línea 14, que corta los dos al encontrar el valor |
| Operaciones de comparación | `mat[i][j] == x` en la línea 10 |

La posición se retorna en el arreglo `pos` (`[fila, columna]`, líneas 11 y 12), y
la función devuelve además `hallado` (línea 13). Si no está, `pos` queda en
`[-1, -1]` (líneas 6 y 7).

### Ejercicio 3, programa propuesto: cifrado de un archivo por tramos (§7.3)

| Requisito | Dónde se cumple |
|---|---|
| Uso de funciones con parámetros y valores de retorno | `bloques_de [ int total , int tam , int i ] : int b` (línea 5), `direccion [ int base , int tam , int i ] : int d` (línea 15), `cifrar_archivo [ int desde , int hacia , int total , int tam ] : int n` (línea 33) y `descifrar_archivo` (línea 46) |
| Llamadas a múltiples funciones | `main` llama a `preparar_boveda` (línea 63), `cifrar_archivo` (línea 64) y `descifrar_archivo` (línea 65); `cifrar_archivo` llama a `bloques_de` (línea 36) y a `direccion` (líneas 38 y 40) |
| Estructuras de control anidadas (condicionales dentro de ciclos) | `C [ i % 2 == 0 ] ; … A ; …` (líneas 37 a 41) dentro de `S [ i , 0 , n , 1 ]` (línea 35); lo mismo en `descifrar_archivo` (líneas 48 a 54) |
| Operaciones complejas o algoritmo significativo | Cifrado y descifrado de un archivo de 1024 bytes en 8 tramos de 16 bloques, alternando entre dos llaves según la paridad del tramo (`cifrar [ … , 0 ]` y `cifrar [ … , 1 ]`, líneas 38 y 40). La aritmética: `( total + tam - 1 ) / tam` (línea 34, el número de tramos con redondeo hacia arriba), `i % 2` (línea 37) y `base + i * tam * 8` (línea 16, la dirección de cada tramo). Se usan seis de las siete operaciones de cifrado: `autenticar` (línea 62), `cargar_llave` (líneas 21 a 28), `sellar` (línea 29), `cifrar`, `descifrar` y `cerrar_sesion` (línea 66); `cambiar_ancla` no se usa. El archivo se carga en 0x1000 (zona de `load_file`) |

La autenticación del ejercicio 3 usa el token Lamport `x99 = 0xF47BD70B`,
escrito como `-193210613` en Calser. Su hash Ascon truncado es `0xFEFF95FF`,
el ancla de fábrica. El simulador verifica ese hash; después de consumir x99
la siguiente autenticación debe presentar x98 (`0xA45326BA`). Una nueva
instancia del simulador comienza con el ancla de fábrica; no representa el
reset normal del hardware, que conserva el ancla activa.

## Qué contiene cada archivo

| Archivo | Contenido |
|---|---|
| `ejercicioN.cls` | Copia del fuente |
| `ejercicioN.asm` | Ensamblador (`ccom -s`), anotado: dirección y línea del `.cls` de cada bundle, tabla de etiquetas con sus direcciones y tabla de variables. Todo lo anotado va en comentarios con `#` |
| `ejercicioN.bin` | Binario: encabezado de 4 palabras, los bundles en 16 bytes y los datos |
| `ejercicioN.hex` | Un bundle por línea, `slot3_slot2_slot1_slot0` en hexadecimal |
| `ejercicioN.mem` | Imagen de datos (`@0` y una palabra por línea) |
| `compilacion.txt` | Línea de comando y salida completa de `ccom -v -m -s -p`: fases, tablas de símbolos y de etiquetas (con direcciones) y los paquetes en ensamblador y en hexadecimal |
| `traza.txt` | Traza de ejecución del simulador (`--traza`) |
| `ejecucion.txt` | Resumen: bundles ejecutados, ciclos, avisos de separación, valores finales de las variables y el resultado esperado contra el obtenido |
| `datos.txt` | Solo ejercicio 3: el texto de 1024 bytes que se carga en 0x1000 |

`make_check.txt`, en esta carpeta, es la salida de `make check`: la evidencia
de que las pruebas del compilador pasan (§10.5).

## Cómo leer la traza

Una línea por evento, `[bundle N · línea L del .cls] evento`:

```
[bundle 0038 · línea 1] res = 0  (prólogo)
[bundle 0094 · línea 8] res = 2
[bundle 0026 · línea 17] llamada → .L_func_factorial (bundle 38)
[bundle 0104 · línea 7] jef → .L_while_start_2 (bundle 60)
[bundle 0106 · línea 11] retorno → bundle 27
```

Muestra las escrituras a variables (`x = 5`, `p[1] = 1`; un `bool` como
`verdadero` o `falso`), los saltos tomados, las llamadas y los retornos, y las
operaciones de cifrado (`auth`, `kv_write`, `kv_lock`, y una línea por cada
`cifrar` o `descifrar` con sus bloques y direcciones). `(prólogo)` marca el
cero que cada función escribe en su variable de retorno al entrar. Al final
viene el estado de todas las variables. La cabecera del archivo dice qué no se
muestra: las escrituras a la pila de llamadas, a las casillas ocultas del `S`
y a los temporales, y los saltos dentro de las rutinas internas.

## Cómo regenerar todo

Desde la raíz del repositorio:

```
make
make ejercicios
```

Compila, simula y escribe todos los archivos de estas carpetas, y falla si
algún valor esperado no se cumple. Es determinista: correrlo dos veces seguidas
no cambia ningún archivo. No forma parte de `make check`.

## Resultados esperados

| Ejercicio | Esperado |
|---|---|
| 1 | `f = 120` |
| 2 | `ok = verdadero` y `p = [1, 1]` (el 42 está en la fila 1, columna 1) |
| 3 | `n1 = 8` y `n2 = 8`; el descifrado (0x3000–0x33FF) es igual al original (0x1000–0x13FF); el cifrado (0x2000–0x23FF) es distinto |

En los tres, 0 avisos de separación y fin por salto a sí mismo.
