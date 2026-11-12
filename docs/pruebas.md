# Ejecutor opcional de pruebas

`tools/probar.py` reúne las suites existentes sin incorporar pruebas al
compilador. Requiere Python 3.9 o posterior y no abre ventanas ni navegadores.
Sin argumentos corre todas las suites; también se pueden elegir varias:

```sh
python3 tools/probar.py --help
python3 tools/probar.py --listar
python3 tools/probar.py matrices tiempo ayuda
python3 tools/probar.py --detalle matrices
python3 tools/probar.py --detener-al-fallar
```

Por defecto prepara los binarios necesarios en una sola llamada a Make y luego
ejecuta directamente las pruebas con el mismo Python que inició el script.
Para usar binarios ya construidos, sin invocar Make:

```sh
python3 tools/probar.py --sin-compilar matrices programas
```

En ese modo los binarios deben existir y estar actualizados. Las suites siguen
necesitando sus auxiliares de `build/` cuando corresponda. Los errores de una
suite no impiden ejecutar las siguientes, salvo con `--detener-al-fallar`.
Una suite con varios comandos se detiene al fallar uno de ellos.

Al finalizar muestra cuántas suites aprobaron, fallaron o fueron omitidas y el
tiempo total. Son conteos de suites, no de sus casos internos. Por defecto solo
muestra los logs de comandos fallidos; `--detalle` muestra también los exitosos.
`--timeout SEGUNDOS` fija el límite por comando, incluida la construcción
(300 segundos por defecto). Si falla la construcción no se ejecutan las suites.

## Un archivo concreto

```sh
python3 tools/probar.py --caso pruebas/p02_factorial.cls
python3 tools/probar.py --caso pruebas/e01_no_declarado.cls --codigo-esperado 1
```

`--caso` compila con `-s -m` y compara el código de salida con 0, o con 1 si se
indica `--codigo-esperado 1`. Esto permite verificar aceptación o rechazo, pero
no valida el texto exacto del diagnóstico ni el resultado del programa en el
simulador. Para inspeccionar tablas y diagnósticos usar `--detalle`.
Las salidas de ese caso van a un directorio temporal y se eliminan al terminar.
No se combina con nombres de suites.

El script funciona desde cualquier directorio: localiza el repositorio a partir
de su propia ruta. Las rutas de `--caso` se interpretan desde el directorio donde
lo invocaste. Por ejemplo:

```sh
python3 /home/saddygr/Documentos/GitHub/calser_compiler/tools/probar.py --sin-compilar matrices
```

## Suites y archivos generados

`--listar` muestra aritmética, arreglos, crypto, AST, ensamblador, programas,
ayuda, tiempo, matrices, etiquetas, simulador, bundles, salidas, aceptación de
bundles y las pruebas del propio ejecutor. Estas últimas usan comandos simulados
para comprobar selección, fallos y resumen; no vuelven a lanzar todas las suites.

La suite `validar-bundles` conserva sus reportes en `build/validacion_bundles/`,
como la herramienta original. La construcción escribe los binarios en `build/`
y `ccom`; el resto de las salidas temporales mantiene el manejo de cada suite.
No se invoca `pruebas/correr_pruebas.sh` ni se sobrescribe `pruebas/resultados.txt`.

La suite `programas` cubre la corrección del error del P1: `R` debe pasar la
dirección de un arreglo declarado, en lugar de su primer elemento. Incluye
cuatro casos: arreglo global (`r_arreglos.cls`, `s = 51`), arreglo local a la
función que ejecuta `R` (`r_arreglos_local.cls`, `s = 52`), matriz de dos
dimensiones (`r_arreglos_2d.cls`, `s = 73`) y reenvío del mismo parámetro arreglo
(`r_arreglos_parametro.cls`, `s = 15`). Los tres primeros verifican que los
argumentos restantes lean el arreglo anterior antes de actualizar los
parámetros. El reenvío conserva la carga de la dirección y el ensamblador
anterior: usar `cg_array_base` también ahí cambiaría los alias de registros.
Los cálculos esperados están comentados en `pruebas/programas/probar.py`.

Códigos de salida del ejecutor: 0 si todo lo seleccionado aprobó (también para
ayuda/listado), 1 ante fallo de construcción o pruebas, 2 por argumentos
incorrectos y 130 si se interrumpe con Ctrl+C.

`make check` sigue disponible e incluye las pruebas del ejecutor mediante
`make check-runner`. El uso del script es opcional durante la presentación.

`make check-diagnosticos-generacion`, incluido en `make check`, ejecuta
`pruebas/diagnosticos_generacion/probar.py`. Crea en una carpeta temporal un
fuente semánticamente válido que agota los doce temporales del generador.
Comprueba el nombre del archivo con ruta y espacios, línea, columna, texto
exacto, código de salida 1 y ausencia de salidas, con y sin `--no-schedule`.
`gen_report_error` recibe el nombre prestado mediante `GenCtx.source_name`,
pasado a `generate_code`; sin nombre usa `ccom`. Conserva el contador y el
formato de posición del diagnóstico. Este target usa Python; la compilación
habitual con `make` sigue sin necesitarlo.

`make check-constantes`, incluido en `make check`, ejecuta el binario C
`build/probar_constantes` desde `pruebas/constantes/probar.c`. Comprueba búsqueda
de inicializadores, literales, operadores, referencias Vi encadenadas, rechazo
de Vm y llamadas, profundidad 64/65, truncamiento a 32 bits, división y módulo
con signo y conservación de la política sin corto circuito del evaluador.
El módulo compartido vive en `include/ast_constant.h` y `src/ast_constant.c`;
lo usa `generate_init.c` y queda disponible para el futuro constructor TAC,
sin depender del generador ni del emisor. Este target no necesita Python.

`make check-constantes-isa`, incluido en `make check`, ejecuta el binario C
`build/probar_constantes_isa` desde `pruebas/constantes_isa/probar.c`. Prueba
los doce valores de frontera y normalización de `isa_constant_plan`, el orden
MOV/MOV/SLL/SADDI, los roles DEST/SCRATCH, los inmediatos de 18 bits y la
cantidad de auxiliares. Recompone con aritmética definida de 32 bits y comprueba
que dos entradas fuera de rango no modifican la salida. No depende del AST,
del emisor ni de Python; la función compartida vive en `isa_constant.c`.

La semántica de dependencias del ISA vive en `include/isa_table.h` y
`src/isa_table.c`, compartida por el descriptor del organizador y disponible
para la representación intermedia. `codegen_instruction.c` conserva el parseo y
la validación del texto. `preserves` identifica el destino cuyo valor anterior
se conserva en `ifmov` y `feis`; no se suma a las lecturas físicas del organizador,
para mantener sus distancias RAW/WAW y los bundles existentes.

`make check-semantica-isa`, incluido en `make check-bundles`, ejecuta un binario C
que comprueba los 34 mnemónicos y las 13 formas, registros repetidos, destino
`rz`, preservación del destino, llamadas/retornos y la marca de `auth`.
Los rangos de selectores literales y el resto de la sintaxis siguen probándose
en `build/probar_ir`. `build/probar_dependencias` comprueba además que un
productor seguido por `ifmov` con destino preservado mantenga el calendario
WAW de dos bundles; si el destino es también fuente textual, exige RAW de tres.
Este target no necesita Python. Las pruebas completas conservan sus ejecutores.

La representación intermedia de tres direcciones tiene además `make check-tac`,
incluido en `make check`: ejecuta `probar_tac` (seis casos de alto nivel) y
`probar_tac_isa` (ISA virtual/física), y compara dos salidas esperadas con `diff`.
También ejecuta `probar_tac_pseudo`: las doce operaciones de destino pendientes
de expansión, sus propiedades genéricas, selectores, auxiliares físicos,
rechazos de forma/etapa y el control obligatorio previo a emisión. Comprueba
la impresión `pseudo.*` en un flujo temporal sin cambiar los dos esperados.
Los binarios C comprueban definiciones/usos, globales, ABI, argumentos, memoria,
edición, reconstrucción y rechazos. El target también compila
`pruebas/tac/ejemplo.cls` con ccom. No usa Python; la construcción habitual con
`make` tampoco lo necesita. Ver
[representacion_intermedia.md](representacion_intermedia.md).

## Construcción TAC desde el AST

`make check-tac-build`, incluido en `make check`, construye `probar_tac_build`
y ejecuta `pruebas/tac_build/probar_cli.py`. El binario C parsea y analiza cada
fuente, construye TAC en memoria y llama `tac_validate`. Verifica línea/columna
de las instrucciones sin cambiar el listado. Compara imagen inicial
con la del generador y verifica que AST, símbolos, área de datos y estado del
emisor no cambien durante la construcción. Se libera TAC antes del semántico.

Hay 22 esperados en `pruebas/tac_build/esperados/`: factorial L/R, corto circuito,
relaciones y unarias, C/A/residual anidados, S positivo/negativo/paso grande,
E dentro de S, arreglos 1D/2D, copia y parámetros reenviados, llamadas anidadas
como valor y sentencia, retorno cero, inicializadores y las siete CRYPTO.
Incluye los cuatro `r_arreglos*.cls` existentes. Solo los tres casos faltantes
se agregan en `pruebas/tac_build/casos/`.

La CLI comprueba `-i`/`--tac`, nombres por defecto y `-o`, combinaciones con
listados, JSON temprano y tiempo, el aviso con `-x`, errores sin TAC y borrado
de un parcial mediante un enlace temporal a `/dev/full` donde está disponible.
Compara ASM/BIN/HEX/MEM con y sin `-i`, y TAC con/sin `--no-schedule`.
`check-tac` comprueba además COPY hacia parámetros arreglo y sus definiciones.
La construcción C1 se verifica por separado del intérprete y los valores de C2.

## Selección HIGH a ISA virtual

`make check-tac-lower`, incluido en `make check`, ejecuta dos binarios que no
se enlazan con ccom. `build/probar_tac_lower` comprueba selección ALU y unaria,
fronteras de constantes/desplazamientos, bases simbólicas/calculadas, IF_LE/IF_GT,
PARAM fuera de orden, CALL/RETURN/EXIT, posiciones y las once PSEUDO complejas.

`build/bajar_tac` parsea, analiza, construye HIGH y llama `tac_lower`, sin usar
el generador. Captura HIGH e imagen antes/después y exige igualdad de bytes,
incluidos IDs internos, posiciones y aristas. Distingue rechazo del frontend
(código 3) de fallo de selección (código 1). `pruebas/tac_lower/probar.py` recorre
los 118 fuentes de pruebas/ y ejercicios/: baja y valida los 39 válidos, exige
los 79 rechazos esperados y comprueba doce listados de `pruebas/tac_lower/esperados/`.
El caso adicional de constantes grandes se crea en una carpeta temporal, sin
agregar otro .cls al corpus. Incluye CRYPTO y los cuatro R con arreglos.

No asigna registros, conserva vivos, expande PSEUDO ni emite ensamblador.
La función compartida `isa_constant_plan` produce los pasos de constantes;
la bajada les asigna roles virtuales y copia línea/columna. Se libera ISA antes
de TacBuildResult y del semántico. La compilación habitual conserva sus fuentes
y el binario ccom; las pruebas se verifican además con C11 estricto y ASan/UBSan.

## Valores del TAC en memoria

`make check-tac-values`, incluido en `make check`, construye únicamente el
auxiliar `build/interpretar_tac` con `pruebas/tac_build/interpretar.c` y
`ejecutar.c`. No incorpora el intérprete a ccom. El auxiliar parsea, analiza y
construye TAC, lo ejecuta dos veces con memoria nueva para comprobar independencia
del estado, y publica estado y valores de los símbolos sin homónimos.

`pruebas/tac_build/probar_valores.py` importa CASES de
`pruebas/programas/probar.py`; sus esperados no se copian. Ejecuta los 22 casos
sin CRYPTO, incluidos los cuatro R con arreglos (51, 52, 73 y 15). Los dos casos
especiales CRYPTO (p09/p11) se listan como omitidos y no cuentan como aprobados.
Diez controles temporales comprueban valor antes que índices, argumentos de
izquierda a derecha, captura antes de llamada, 32 bits y signos, corto circuito,
división/módulo por cero, exponente negativo, rechazo CRYPTO y límite de pasos.
Los fallos de ejecución tienen estado propio y no se aceptan como retornos.

Para un fuente concreto: `build/interpretar_tac archivo.cls [limite_de_pasos]`.
El JSON contiene `status`, `steps`, `diagnostic` y `values`; el código 0 indica
que se pudo informar el estado, no necesariamente ejecución normal. Solo `ok`
publica valores. Para comprobar otro build, incluso sanitizado:
`python3 pruebas/tac_build/probar_valores.py --binario /ruta/interpretar_tac`.
Las fuentes de los controles y las salidas son temporales. No se modifican el
constructor ni el generador cuando aparece una diferencia; la prueba informa
caso, valor obtenido y esperado y termina con fallo.

## Plantillas CRYPTO y evaluación del AST

`make check-crypto` conserva los 184 escenarios de Feistel y los once escenarios
de integración del compilador, incluido el ejercicio 3. `probar.py` ejecuta
primero `build/emitir_crypto --contract`: verifica las siete plantillas mediante
`cg_crypto_emit_template` sin AST ni callback, contratos inválidos sin emisión,
registros físicos fijos, selectores, contador compartido y agotado, y slots de
los bundles indivisibles. Comprueba los registros IR del lazo, separaciones,
etiquetas y orden de rondas/desplazamientos del descifrado. Los fallos del
callback en el segundo y tercer argumento comprueban orden, destinos, máscaras,
guardado previo y ausencia de restauración o consumo de etiquetas.

La función directa no prepara argumentos ni preserva vivos. `cg_crypto` sigue
haciéndolo para el generador actual, con las mismas plantillas P1. El auxiliar
se verifica con C11 estricto (`-Wall -Wextra -pedantic -Werror`) y ASan/UBSan.
La equivalencia del generador se comprueba con capturas de los 118 fuentes en
modo normal y `--no-schedule`, incluyendo `-s -i`, sin `--time` ni normalización.

## Análisis C1: vivacidad, lazos y edición

`make check-tac-live` ejecuta `build/probar_tac_live` y el auxiliar
`build/analizar_tac live`. Los casos manuales verifican USE/DEF, LIVE_IN/OUT,
vivos por instrucción, ciclos, uniones, redefiniciones, globales en CALL/RETURN,
raíces HIGH e ignoradas como registros ISA, candidatos de preservación,
clobbers separados, ifmov/feis y clasificación de Vm por ámbitos.

`make check-tac-loops` ejecuta `build/probar_tac_loops` y el mismo auxiliar en
modo `loops`. Verifica dominadores, unión de retrocesos de igual cabecera,
profundidad anidada, inalcanzables, diagnóstico irreducible, marca válida sin
retroceso, descarte explícito y consistencia en un solo sentido. La edición
prueba inserciones ISA/PSEUDO, rechazo ligado de ifmov/feis y reemplazos inválidos
sin modificación parcial; quitar bloques rechaza LABEL (también objetivos ISA),
marcas, entry y caídas alcanzables. Quita un bloque inalcanzable sin referencias.

Ambos targets están en `make check`. `pruebas/tac_analysis/probar.py` recorre los
118 fuentes actuales: analiza los 39 válidos en HIGH y en ISA virtual, y exige
los 79 rechazos del frontend. Tres fuentes temporales adicionales comprueban S/L
anidados y ambos tipos con cuerpo terminado en E, sin retroceso. R y copias de
arreglos se analizan como lazos naturales sin exigir marcas. Las capturas de
objetos, marcas, ámbitos, símbolos e imagen son idénticas antes/después de analizar.

Para verificar otro auxiliar: `python3 pruebas/tac_analysis/probar.py live
--binario /ruta/analizar_tac`, o `loops`. Los tres binarios nuevos se verifican
con C11 estricto y ASan/UBSan. C1 no implementa intervalos, asignación, derrame,
optimizaciones, banderas ni emisión: esos estados quedan documentados aparte.

## C2: intervalos, linear scan y extensión de derrame

`make check-tac-intervals` prueba posiciones de lectura/escritura, retrocesos,
ramas, bloques vacíos e inalcanzables, huecos y definiciones repetidas o muertas.

`make check-tac-allocate` prueba 12/13 vivos, diagnóstico archivo:L:C, LRU,
reutilización de fuentes muertas, exclusión de fuentes en PSEUDO aritméticas,
ocho auxiliares y preservación, CALL, las siete CRYPTO, ifmov/feis y la bajada
cuando destino y fuente coinciden. También comprueba la reserva estática de
derrame: cero bytes, varias funciones, límites y los 256 bytes de pila.

El auxiliar `build/asignar_tac archivo.cls` mide y asigna en memoria. Devuelve
0 al asignar todas las funciones, 4 si hay presión y 3 ante rechazo del frontend.
`pruebas/tac_allocate/probar.py` recorre el corpus completo, continúa tras cada
fallo e imprime por función vivos reales, solapamiento de intervalos, cantidades
MUL/DIV/MOD/POW, sitios con preservación, registros asignados y resultado.
Los máximos incluyen los bloques inalcanzables conservados. La lista final de
fallos nunca se omite; reportar presión no equivale a una asignación aprobada.
Ambos targets forman parte de `make check`; estos auxiliares no se enlazan con
ccom ni modifican sus salidas. Referencias: Aho §9.2.5 y §10.2.3; Poletto y
Sarkar 1999, fuera de Aho, para intervalos y linear scan.


## D: emisión y equivalencia observable

`make check-tac-emit` comprueba permutaciones CRYPTO sin cruce, intercambio,
ciclo de tres, fuente repetida y rz; casillas de preservación, rl en llamadas
encadenadas, etiquetas únicas, nombre obligatorio y ROM 4096/4097 como función.
El rechazo de source_name NULL del asignador se prueba en check-tac-allocate.

`make check-o1-equivalence` usa `build/emitir_tac fuente base` para construir,
bajar, asignar, emitir, calendarizar y ensamblar. Imprime dirección y ancho de
globales/variables de main, sin identidades de ámbito. El comparador usa sim_asm
sin duplicar ISA: compara esos tramos, 0x1000–0xFFFF, auth/kv_lock/exc/anchor/vault
y terminal lógico fin/error. Cada programa corre con los mismos datos externos
en cero y con patrón de 1024 bytes, límite explícito de 2.000.000 bundles y cero
avisos de separación. Imprime bundles O1/O0 y todos los fallos antes de devolver 1.

E cambiará solo el adaptador `compile_new` de `pruebas/tac_emit/comparar.py`;
la función `compare` y el target permanecen. Los dos targets forman parte de
make check. D no agrega banderas ni entra en la CLI de ccom. Referencias: Aho
§7.2 y §8.6; el tiempo/ciclos no son condición de equivalencia funcional.
