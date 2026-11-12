# `ccom` — uso y banderas

```
ccom [opciones] <archivo> [-o <salida>]
```

Las opciones pueden ir antes o después del archivo. Sin opciones, `ccom`
escribe sus archivos de salida y sale con código 0, confirmando el éxito y las
rutas generadas en stderr. La salida clásica mantiene los errores en stderr;
`--check-json` entrega los diagnósticos individuales en JSON por stdout.

## Banderas

| Bandera | Qué hace | Dónde sale | Estado |
|---|---|---|---|
| `-h`, `--help` | Muestra todas las banderas, ejemplos y códigos de salida, sin requerir entrada | stdout | Disponible |
| `-O0` | Generador actual, también predeterminado sin nivel | archivos | Disponible |
| `-O1` | Construcción HIGH, selección ISA, asignación y emisión; pasadas vacías | archivos | Disponible |
| `-O2` | Reservado hasta C3.5; devuelve error de uso | stderr | Pendiente |
| `--opt=lista` | Implica O1; une unroll,rename,dce,reorder,spill en orden fijo | archivos | Disponible |
| `--unroll=N` | Factor 2..16, exige unroll; sin factor: heurística §11 | contexto | Disponible |
| `--metrics archivo.json` | JSON solo tras compilación O1 exitosa | archivo JSON | Disponible |

| `--clones-json <mínimo>` | Clones I/II de instantáneas CCL1; mínimo 5..100000 tokens | JSON v1 stdout | Disponible |
| `--repair-json` | Reparaciones de cierres sobre copias verificadas con Flex/Bison; sin semántica ni artefactos | JSON v1 stdout; logs stderr | Disponible |
| `--syntax-json` | Verificación independiente de léxico/sintaxis para la política del IDE; sin semántica | JSON v1 stdout; logs stderr | Disponible |
| `--suggest-json <cursor>` | Continuaciones sintácticas Flex/Bison-LAC en el cursor UTF-8; sin semántica ni artefactos | JSON v1 en stdout; logs en stderr | Disponible |
| `--tokens-json` | Solo lexer Flex: tokens clasificados, comentarios y errores; sin parser ni artefactos | JSON v1 en stdout; logs en stderr | Disponible |
| `--check-json` | Mismo front-end que `--check`, con diagnósticos estructurados v1; sin artefactos | JSON en stdout; logs en stderr | Disponible |
| `--check` | Solo análisis léxico, sintáctico y semántico, sin archivos de salida | diagnósticos en stderr | Disponible |
| `--time` | Muestra la duración total en milisegundos, con estado completado o fallido | stderr | Disponible |
| `--no-schedule` | Desactiva el organizador local y conserva la emisión de una operación más separación; útil para comparar | archivos | Disponible |
| `--ast-json <archivo>` | Exporta el AST sintáctico a JSON para el visor gráfico | archivo JSON | Disponible |
| `-t` | Imprime el árbol sintáctico abstracto (AST) | stdout | Disponible |
| `-m` | Imprime la tabla de símbolos con las direcciones de memoria y la tabla de etiquetas, esta última con la dirección de cada etiqueta (su índice de bundle) | stdout | Disponible |
| `-v` | Muestra el avance de cada fase (`ok` o `con errores`), incluido el ensamblado | stderr | Disponible |
| `-o <archivo>` | Nombre del binario de salida. Sin `-o`: el nombre del fuente con `.bin` (`prog.cls` → `prog.bin`). Es también el nombre base de las demás salidas | archivos | Disponible |
| `-s` | Escribe además el ensamblador (`.asm`) | archivo | Disponible |
| `-i`, `--tac` | Construye y escribe la representación de alto nivel (`.tac`); continúa la compilación normal | archivo | Disponible |
| `-p` | Muestra cada bundle en ensamblador y en binario | stdout | Disponible |
| `-x` | Toma un `.asm` y genera el binario, sin compilar Calser | archivos | Disponible |

Se pueden combinar: `ccom -t -m prog.cls` imprime el AST y después las tablas.

Las siete consultas (`--check`, `--check-json`, `--tokens-json`, `--syntax-json`,
`--suggest-json`, `--repair-json`, `--clones-json`) son excluyentes: solo admiten
`-v` y `--time`. Rechazan con código 2 cualquier otra consulta, `-i`/`--tac`,
`-O0/-O1/-O2`, `--opt`, `--unroll`, `--metrics`, salidas, ensamblado y listados,
antes de abrir la entrada o modificar archivos. No se ignoran opciones.
La ayuda `-h`/`--help` conserva su terminación inmediata.

La compilación normal admite O1, pasadas, métricas y TAC. `--ast-json` solo
termina tras sintaxis; `-s`, `-i`, `-m`, `-p` u O1 hacen continuar la compilación.
Pruebas de incompatibilidades: `make check-ide-options` (incluida en `check`).


## Solo análisis (`--check`)

```bash
./ccom --check programa.cls
./ccom --check -v --time programa.cls
```

Ejecuta las mismas fases léxica, sintáctica y semántica que la compilación normal. No genera código, no ensambla, no exporta AST y no crea ni modifica `.bin`, `.hex`, `.mem`, `.asm` o `.json`. Es el modo usado por CALSIDE para revisar copias temporales de texto sin guardar.

Códigos: **0** análisis correcto; **1** error de análisis o lectura; **2** uso incorrecto. Mantiene el formato de diagnósticos con archivo, línea, columna y señalador. Un análisis correcto confirma `ccom: Análisis correcto: sin errores.` en stderr; stdout permanece vacío.

Solo se combina con `-v` y `--time`; las opciones de salida, ensamblado y listados (`-o`, `-s`, `-i`/`--tac`, `-x`, `-p`, `-t`, `-m`, `--ast-json`, `--no-schedule`, niveles, `--opt`, `--unroll`, `--metrics`) se rechazan con código 2 antes de abrir salidas. La medición se identifica como `tiempo de analisis`. Un éxito no certifica generación de código ni ejecución: para eso se usa la compilación normal.

Prueba del contrato y conservación de archivos: `make check-analysis` (también forma parte de `make check`).

## Ejemplos

```bash
./ccom prog.cls              # compila y confirma el resultado
./ccom -t prog.cls           # árbol
./ccom -m prog.cls           # tabla de símbolos y de etiquetas
./ccom -v prog.cls           # avance por fase
./ccom -t -m -v prog.cls     # todo lo anterior
./ccom prog.cls -o prog.bin  # nombre del binario y base de .hex y .mem
./ccom -s prog.cls           # además escribe prog.asm
./ccom -p prog.cls           # bundles en ensamblador y en binario
./ccom -x prog.asm           # ensambla un .asm, sin compilar Calser
```

**Tabla de etiquetas de `-m`.** Cada fila trae el nombre, el rol, el nodo del
AST, su línea y la columna `Address`: el índice de bundle donde está la
etiqueta. El nombre y el nodo salen de la Fase 3 (enunciado CE1108 §2.3); la
dirección solo se conoce después de generar el código, así que esta tabla se
imprime al final, después del ensamblado, y se completa con la misma dirección
que muestra el `.asm` anotado (libro §6.7, backpatching). La tabla de símbolos
no se mueve. Si la compilación falla antes de generar código, la tabla sale
igual con `-` en `Address`; también sale `-` en una etiqueta que no llega al
`.asm` (un `&` o `|` constante dentro de un inicializador global).

```
.L_func_main | FUNCTION     | AST kind: FUNCTION | Line: 14 | Address: 2
.L_if_end_1 | END          | AST kind: CONDITIONAL | Line: 2 | Address: 52
```

## Ayuda (`-h`, `--help`)

```bash
./ccom --help
./ccom -h
```

Ambas formas muestran la misma ayuda en stdout y devuelven 0, sin requerir
archivo de entrada ni generar salidas. La ayuda enumera todas las opciones,
los archivos producidos, ejemplos y códigos de salida. Al encontrar la opción
de ayuda se termina inmediatamente: no se abre la entrada ni se inicia la
medición de `--time`. Un error de opciones anterior a la ayuda sigue siendo
un error de uso (código 2).

## Duración del trabajo (`--time`)

```bash
./ccom --time -s prog.cls
# ccom: tiempo de compilacion (completado): 1.234 ms
```

Mide tiempo de pared con `CLOCK_MONOTONIC`, desde antes de abrir la entrada
hasta terminar el trabajo y cerrar las salidas. Incluye análisis, generación,
organización de bundles, ensamblado, escritura y limpieza; con `-t`, `-m`,
`-p` o `-v` también incluye el costo de sus listados dentro de esas fases.
No mide la construcción del ejecutable `ccom` ni la ejecución del programa
compilado. El valor varía entre ejecuciones.

La línea se escribe al final en stderr. Si falla la compilación se informa
`(fallido)` y se conserva el código de salida original. Con `-x` se etiqueta
como `ensamblado`; con `--ast-json` sin `-s`, `-i`, `-m`, `-p` ni O1, como
`exportacion AST`, pues ese modo termina después del análisis sintáctico. Con
consultas se identifica como `analisis`, sin generación.
Los errores de uso no inician trabajo ni muestran una duración. Si el reloj
no está disponible se emite un aviso, sin cambiar el resultado del compilador.

## Archivos de salida

En compilación normal, cuando no hay errores, `ccom` escribe tres archivos con el nombre base
de `-o` (o el del fuente): `prog.cls` produce `prog.bin`, `prog.hex` y
`prog.mem`. Con cualquier error no se escribe ninguno de los tres.

| Archivo | Contenido |
|---|---|
| `.bin` | Encabezado de 4 palabras de 32 bits *little endian*: cantidad de bundles, punto de entrada (0), tamaño de datos en bytes y dirección de datos (0). Luego cada bundle en 16 bytes (slot 0 primero, cada palabra *little endian*) y al final los datos |
| `.hex` | Un bundle por línea: `slot3_slot2_slot1_slot0`, cada slot en 8 dígitos hexadecimales en mayúscula |
| `.mem` | `@0` y luego una palabra de datos por línea, en hexadecimal |
| `.asm` | Solo con `-s`: el ensamblador generado, en la sintaxis de `isa.md` §11, anotado (ver abajo) |

**`.asm` anotado (`-s`).** Además de las instrucciones, el archivo muestra lo
que pide el enunciado CE1108 §6.2: la dirección de cada bundle (su índice, que
es lo que cuenta el PC), la línea del `.cls` que lo generó (solo cuando
cambia; los bundles vacíos de separación no la llevan, y un bundle que no es
de ninguna sentencia, como el salto final de una función, lleva `linea -`), la tabla de etiquetas
con su dirección y la tabla de variables con su dirección de datos. Todo eso va
en comentarios con `#` (`isa.md` §11.1), así que `ccom -x` sobre ese mismo
archivo da exactamente los mismos bundles. Las etiquetas internas de las
rutinas aritméticas, de arreglos y de cifrado se marcan `(interna)`: no son de
la tabla de etiquetas de la Fase 3.

```
# ccom: pruebas/p02_factorial.cls
  { mov rx, 120 | nopi | nopi | nopi }                      # 0000
  { nopi | nopi | nopi | nopi }                             # 0001
.L_func_main:                                               # bundle 2
.L_body_main:                                               # bundle 2
  { mov t0, 5 | nopi | nopi | nopi }                        # 0002  linea 15
  { nopi | nopi | nopi | nopi }                             # 0003
  { nopi | nopi | nopi | sw t0, 64(rz) }                    # 0004
...
# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_factorial               38
#   __ccom_arith_0_mul              80  (interna)
#   fin_main                       108
#   .L_arith_error                 109

# Variables (direccion de datos)
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x0040 size=56 temp_base=0x0048
...
```

La primera línea del `.asm` es el comentario `# ccom: <fuente>`. Los errores
de ensamblado del código generado (`<generado>:<línea>`) usan la numeración de
este archivo.

**`-x archivo.asm`** ensambla el archivo sin compilar Calser y escribe los
mismos tres archivos. No hay imagen de datos: el tamaño de datos es 0 y el
`.mem` queda solo con `@0`. Con `-x`, la bandera `-s` se ignora (el `.asm` ya
es la entrada) y sale `ccom: aviso: -s se ignora con -x`; `-t` y `-m` no
tienen efecto.

**`-p`** imprime una línea por bundle: su índice en 4 dígitos, el bundle en
ensamblador y el bundle como en el `.hex`:

```
0000  { mov r4, 0 | mov r5, 3 | nopi | nopi }  00000000_00000000_49400003_49000000
0001  { nopi | nopi | nopi | nopi }  00000000_00000000_00000000_00000000
0002  { saddi r4, r4, 1 | nopi | blt r4, r5, lazo | nopi }  00000000_0D500000_00000000_41100001
```

Los errores de ensamblado (los de `isa.md` §11.4) salen por stderr con la
línea del `.asm`, y `ccom` termina con código 1:

```
ccom: prog.asm:3: error de ensamblado: etiqueta no definida: 'lazo'
```

Para el código que genera el compilador, el nombre es `<generado>` y la línea
es la del `.asm` que escribe `-s`.

Salida de `-v` en un programa correcto (stderr):

```
ccom: fase 1-2: analisis lexico y sintactico ... ok
ccom: fase 3: analisis semantico ... ok
ccom: fase 3: tabla de etiquetas ... ok
ccom: fase 4: generacion de codigo ... ok
ccom: ensamblado ... ok
```

Con `-x` solo sale la línea del ensamblado.

Con un error semántico, primero salen los errores y el resumen, y la fase que
falló termina en `... con errores`; las siguientes no corren:

```
ccom: fase 1-2: analisis lexico y sintactico ... ok
prog.cls:2:32: error semantico: el argumento 'k' de 'cifrar' debe estar entre 0 y 3
     2 |     cifrar [ 4096 , 8192 , 1 , 4 ] ;
       |                                ^
Analisis semantico fallido: 1 error(es).
ccom: fase 3: analisis semantico ... con errores
```

Con `-t` y errores semánticos, el AST se imprime igual, porque se construye
antes del análisis semántico. Con errores léxicos o sintácticos no hay AST.

## Códigos de salida

| Código | Significado |
|---|---|
| 0 | Trabajo completado o ayuda mostrada |
| 1 | Errores léxicos, sintácticos, semánticos, de generación o de ensamblado, o un archivo de salida que no se pudo escribir |
| 2 | Error de uso: bandera desconocida, `-o` sin nombre, ningún archivo o más de uno |

Línea de uso que se imprime en los errores de uso:

```
uso: ccom [-h|--help] [-v] [-t] [-m] [-s] [-i|--tac] [-p] [-x] [--check|--check-json|--tokens-json|--suggest-json <cursor>|--repair-json|--syntax-json|--clones-json <min>] [--ast-json <json>] [--no-schedule] [--time] [-O0|-O1|-O2] [--opt=lista] [--unroll=N] [--metrics archivo.json] <archivo> [-o <salida>]
```

## Pruebas

`pruebas/correr_pruebas.sh` llama a `ccom -m`, así que `pruebas/resultados.txt`
guarda los errores, las tablas y el código de salida de cada programa. Pasa
`-o` a una carpeta temporal, para no dejar archivos de salida en `pruebas/`.

`make check-asm` corre `pruebas/ensamblador/probar.py`: los vectores de
`isa.md` §6.5, el ejemplo de §11.1, el formato del `.bin`, los errores de
§11.4 y el ensamblado del `.asm` que genera el compilador.

## AST gráfico local

```sh
./ccom --ast-json build/p09_crypto.ast.json pruebas/p09_crypto.cls
```

Abrir `tools/visor_ast.html` en un navegador y seleccionar el JSON con
**Abrir AST JSON**. También incluye un ejemplo de p09 para verlo inmediatamente.
No requiere servidor, paquetes ni conexión. Ver [visor_ast.md](visor_ast.md).

`--ast-json` solo, o junto a `-t`/`-v`, termina después del análisis sintáctico:
no comprueba la semántica ni genera ensamblador. Su código 0 significa que el
AST se pudo construir y exportar, no que el programa completo sea válido.
Permite inspeccionar el árbol de un programa sin ejecutar el resto de las fases.
Con `-s`, `-i`, `-m` o `-p`, exporta el AST y continúa el recorrido normal; puede
conservar un JSON válido aunque las fases siguientes fallen. Si falla el
análisis léxico o sintáctico, no escribe un JSON nuevo. Un archivo de una
exportación anterior puede seguir existiendo; revisar siempre el código de salida.

## Organización de bundles

La compilación organiza operaciones independientes dentro de bloques básicos
antes de resolver etiquetas y ensamblar. Puede aprovechar ambas ALU y compartir
bundle con memoria o salto, respetando las dependencias de la ISA.

```sh
./ccom -s pruebas/p02_factorial.cls -o build/factorial_paralelo.bin
./ccom --no-schedule -s pruebas/p02_factorial.cls -o build/factorial_escalar.bin
```

Las plantillas manuales mantienen sus slots. `-x` ensambla el `.asm` externo sin
reorganizarlo.

## Representación intermedia (`-i`, `--tac`)

```sh
./ccom -i programa.cls
./ccom --tac -s programa.cls -o salida.bin
```

Genera `programa.tac` sin `-o`, o `salida.tac` junto a las demás salidas con
`-o salida.bin`. Incluye la imagen inicial y el listado de alto nivel desde el
AST validado. `.bin`, `.hex`, `.mem` y `.asm` siguen saliendo del generador
actual; `--no-schedule` no cambia el TAC. `-t`, `-m` y `-p` conservan sus listados.

`--ast-json archivo -i` escribe el JSON durante la fase sintáctica y continúa
la compilación. Si después falla el semántico, puede quedar el JSON, pero no
se escribe un `.tac` nuevo. Sin `-i` se conserva el modo sintáctico existente.
`--time` incluye construcción y escritura y reporta compilación. Con `-x`, `-i`
se ignora y avisa `ccom: aviso: -i se ignora con -x`.

El `.tac` se escribe al completar la generación; si falla la escritura o el
cierre se informa error, se elimina el parcial y se devuelve 1. No puede
sobrescribir el fuente, tampoco mediante enlaces al mismo archivo. Los errores
anteriores no borran un `.tac` preexistente. El listado no genera binarios ni
incluye calendarización: ver [representacion_intermedia.md](representacion_intermedia.md).


## Camino optimizante (`-O1`)

El contrato de banderas y métricas está en §5/§6 de
[representacion_intermedia.md](representacion_intermedia.md). Niveles distintos,
listas vacías o desconocidas, `--opt` con O0, `--metrics` sin O1 y `-x` con
opciones nuevas son errores de uso (2). Varias listas se unen sin duplicados.
`--no-schedule` conserva la emisión conservadora y comprueba igualmente ROM.
`-i` escribe HIGH inicial; `--ast-json` sigue escribiendo antes del semántico,
pero O1 implica compilar. `-m` muestra todas las etiquetas emitidas con su
índice final de bundle, mediante el IR de codegen. O0 conserva su tabla anterior.

## Confirmación de éxito

Las operaciones completadas confirman su éxito en `stderr`, incluso sin `-v`:

- `--check`: «Análisis correcto: sin errores», sin archivos generados.
- Compilación O0/O1: «Compilación correcta», con rutas `.bin`, `.hex`, `.mem`, `.asm` si se pidió `-s`, `.tac` si se pidió `-i`/`--tac` y métricas si se solicitaron.
- `-x`: «Ensamblado correcto», con las tres imágenes de salida; no anuncia un `.asm` nuevo.
- Exportación AST sola: confirma únicamente la exportación JSON, sin afirmar semántica ni generación de código.

Un JSON combinado con compilación también figura entre los archivos generados. Los errores de análisis, uso o escritura no anuncian éxito; una escritura parcial tampoco se presenta como compilación correcta. La ayuda no añade una confirmación. Los códigos 0/1/2 y el contenido de `stdout` se conservan. Con `--time`, la medición sigue al final de `stderr`.


## Análisis estructurado para CALSIDE (`--check-json`)

```sh
./ccom --check-json programa.cls
```

Usa las fases C existentes sin generar archivos. Su respuesta versionada distingue análisis correcto, errores del programa, fallo del servicio e informe limitado; incluye una lista de diagnósticos con códigos, categorías y rangos UTF-8. Admite `-v` y `--time`, y rechaza opciones de generación/listados igual que `--check`.

Contrato completo y ejemplos: [Servicio de diagnósticos](ccom_servicios_ide.md#check). Verificación: `make check-analysis-json`, incluida en `make check`.

## Tokens para el editor (`--tokens-json`)

`./ccom --tokens-json programa.cls` recorre la entrada completa con Flex y devuelve clasificación y rangos UTF-8, incluidos comentarios. No comprueba sintaxis/semántica ni genera archivos. Compatible con `-v` y `--time`. Contrato y límites: [servicio léxico M2](ccom_servicios_ide.md#tokens).

## Sugerencias sintácticas (`--suggest-json`)

`./ccom --suggest-json 14 programa.cls` consulta continuaciones en la frontera UTF-8 solicitada. Ejemplo: `F main [] : vo` ofrece `void`. Solo admite `-v` y `--time` como auxiliares. [Protocolo, decisiones y límites M3](ccom_servicios_ide.md#suggest).

## Reparaciones y verificación sintáctica M4

`./ccom --repair-json programa.cls` propone cierres del primer error; `./ccom --syntax-json programa.cls` verifica sintaxis completa sin semántica. No generan artefactos y solo admiten -v/--time como auxiliares. [Contrato, garantías y límites](ccom_servicios_ide.md#repair).


## Clones Tipo I/II (`--clones-json`)

`./ccom --clones-json 20 consulta.ccl` compara los textos del paquete CCL1 mediante Flex y el motor C. Las rutas del paquete solo identifican documentos; no se abren. Solo admite `-v`/`--time`, no genera artefactos y devuelve JSON v1 con pares/rangos UTF-8 o un fallo explícito sin resultados parciales. El IDE crea el paquete con los buffers sin guardar y conserva su panel. [Contrato, ejemplo de paquete y límites M5](ccom_servicios_ide.md#clones). `make check-clones-json` forma parte de `make check`.


## Recuperación de errores sintácticos (M7)

`--check`, `--check-json` y la compilación habitual utilizan recuperación conservadora del parser C. Una consulta puede reportar varios errores independientes; cualquier error bloquea semántica, AST y generación. No hay una bandera para compilar programas recuperados. [Funcionamiento y límites](ccom_servicios_ide.md#recovery).


## CALSIDE con entorno virtual

Los servicios C forman parte de `ccom`; el IDE requiere PySide6. Desde la raíz:

```sh
make
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make PYTHON=.venv/bin/python ide
QT_QPA_PLATFORM=offscreen make PYTHON=.venv/bin/python check-ide check-parser check-recorrido-ide
```

## Problemas conocidos

### Cascadas de recuperación e71/e72/e74

En e71, e72 y e74, después del primer error correcto, la recuperación puede
agregar en las líneas 4 y 5 un `E` inesperado y un cierre sin bloque. Estas
cascadas se aceptan para la integración C6; no cambian el rechazo (código 1)
ni permiten exportar AST, ejecutar semántica o generar código del árbol parcial.

### Pruebas del IDE desactualizadas

La línea base de `feat/calside` y la integración registran 427 métodos: 29 métodos
fallidos (34 eventos por los subtests) y un error. De ellos, **28 métodos fallidos
y un error corresponden a defectos de las pruebas**, preexistentes en la rama de
Saddy. Predominan las esperas que comprueban el resultado antes de iniciar F5:
la preparación léxica de `ide/interfaz/ventana.py:925` deja `process is None`
hasta que `recibir_analisis` reanuda la compilación en las líneas 539–543. Los
helpers de `ide/pruebas/test_ejecucion.py:42–46`,
`ide/pruebas/test_compilacion.py:61–65` y
`ide/pruebas/test_prototipo.py:49–54` confunden ese estado con una operación
terminada. Deben esperar la preparación y la finalización de la operación.
El error JSON lee el contenido anterior antes de que termine la exportación.
El grupo de 28 también incluye una aserción de geometría obsoleta:
`ide/pruebas/test_prototipo.py:258` espera 68±2 píxeles y obtiene 55, con la
cabecera/barra definidas en `ide/interfaz/ventana.py:133,147,156`.
Estas pruebas no se corrigieron en la integración.

### Análisis automático perdido después de F5/F6

Defecto real del IDE, también presente en Saddy: F5 invalida la comprobación del
documento y retira su resultado en `ide/interfaz/ventana.py:982–984`. Al terminar,
`ventana.py:1070` solo reanuda el coordinador;
`ide/interfaz/comprobacion.py:103–114` procesa lo pendiente pero no vuelve a
programar el documento invalidado. La liberación de F6 en
`ide/interfaz/ejecucion.py:166–173` tampoco lo programa. El documento puede
quedar sin análisis automático actualizado hasta una nueva solicitud, aunque
compilar y ejecutar hayan terminado correctamente. Falta reprogramar la revisión
vigente al finalizar; no se modifica ese comportamiento en C6 fase 1.

### Protecciones de F6 pendientes de verificar

Los **12 métodos** de `ide/pruebas/test_ejecucion.py:58–186` relativos a ejecución,
cancelación, tiempo límite, edición y vigencia de artefactos fallan en el helper
`compilar` de las líneas 42–46, antes de alcanzar su escenario. Por tanto, esas
protecciones no quedan verificadas por la suite actual. La ejecución real de un
programa mediante F5/F6 no sustituye esa cobertura; hay que corregir la espera
del helper y volver a ejecutar los doce casos.

### Cancelar durante la preparación léxica de F5

`ide/interfaz/ventana.py:1078–1082` solo cancela `run_process` o `process` activos.
No retira `_compilacion_esperando_lexico`, creado en `ventana.py:925–933`, cuando
todavía no existe el proceso compilador. Así, Cancelar no detiene una F5 que
espera el análisis léxico y la compilación puede arrancar después. Defecto
preexistente; queda pendiente cancelar también esa solicitud.

### Recorrido gráfico no determinista

`tools/probar_recorrido_ide.py` puede fallar en la línea 121, al comprobar
`primero in ventana.compilados` antes de que arranque/termine F5, o en la línea
126, esperando «Análisis actualizado tras F5/F6». El primer punto comparte la
espera prematura de las pruebas; el segundo expone la pérdida de análisis
automático descrita arriba. Ocurre también en la referencia de Saddy y no se
considera una verificación gráfica aprobada de la integración.
