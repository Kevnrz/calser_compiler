<p align="center">
  <img src="assets/branding/calser-logo.png" alt="Logo de Calser" width="240">
</p>

<h1 align="center">Calser Compiler</h1>

<p align="center">
  Compilador de Calser desarrollado en C con Flex y Bison.<br>
  Proyecto del curso de Compiladores · CE1108
</p>

## Estado del proyecto

El ejecutable `ccom` realiza análisis léxico, sintáctico y semántico. Construye
el árbol sintáctico abstracto (AST), la tabla de símbolos con direcciones de
memoria y la tabla de etiquetas. Los errores muestran archivo, línea y columna.

`ccom` genera el ensamblador (`-s`), el binario `.bin`, el `.hex` y la imagen
inicial de datos `.mem`. Organiza las instrucciones en bundles VLIW y puede
ensamblar un `.asm` ya escrito (`-x`). La integración CRYPTO se valida
compilando y simulando `p09_crypto.cls`. Los entregables de los tres ejercicios
están en [ejercicios/](ejercicios/README.md).

El AST también se puede explorar como un árbol gráfico local; consultar
[el visor del AST](docs/visor_ast.md).

## Requisitos

Para compilar `ccom` solo se necesitan:

- GCC o un compilador de C compatible.
- GNU Make 4.3 o posterior.
- Flex y Bison.

Python 3.9 o posterior se usa únicamente para `make check`, `make ejercicios`,
el simulador (`tools/sim_asm.py`) y el paquete de VS Code; el compilador no lo
necesita. Bash se usa para el script de pruebas `pruebas/correr_pruebas.sh`.

Los comandos siguientes se ejecutan desde la raíz del repositorio, en Linux
o un entorno compatible como WSL.

## Compilar y ejecutar

```bash
make
./ccom -v pruebas/p02_factorial.cls
```

Las operaciones correctas muestran una confirmación en `stderr` y enumeran las salidas generadas. `--check` confirma el análisis sin crear archivos; `--ast-json` usado solo confirma la exportación, sin afirmar validación semántica. `stdout` conserva los formatos de AST, tablas y bundles.

Una entrada correcta devuelve código `0`. Los errores de análisis devuelven
`1`; los errores de uso de la línea de comandos devuelven `2`.

```bash
./ccom pruebas/p02_factorial.cls       # análisis; muestra errores si existen
./ccom -t pruebas/p02_factorial.cls    # árbol sintáctico abstracto
./ccom -m pruebas/p02_factorial.cls    # tablas de símbolos y etiquetas
./ccom -t -m -v pruebas/p02_factorial.cls
```

| Opción | Función |
| --- | --- |
| `-h`, `--help` | Mostrar todas las banderas, ejemplos y códigos de salida |
| `-t` | Mostrar el AST en terminal |
| `-m` | Mostrar las tablas de símbolos y etiquetas |
| `-v` | Mostrar el avance de cada fase |
| `-o <archivo>` | Nombre del binario de salida; es también el nombre base del `.hex` y el `.mem` |
| `-s` | Escribir además el ensamblador `.asm` |
| `-i`, `--tac` | Escribir además el listado de alto nivel `.tac` |
| `-O0` | Generador predeterminado |
| `-O1` | Camino HIGH/ISA y asignación de registros; pasadas todavía vacías |
| `-O2` | Reservado; devuelve error de uso 2 |
| `--opt=lista` | Implicar O1 y seleccionar unroll,rename,dce,reorder,spill |
| `--unroll=N` | Factor 2..16; requiere unroll activo |
| `--metrics <json>` | Escribir métricas tras una compilación O1 correcta |
| `-p` | Mostrar cada bundle en ensamblador y en binario |
| `-x` | Ensamblar una entrada `.asm` y generar `.bin`, `.hex` y `.mem`, sin compilar Calser |
| `--ast-json <archivo>` | Exportar el AST para el visor gráfico |
| `--check` | Analizar léxico, sintaxis y semántica sin generar archivos |
| `--check-json` | El mismo análisis con diagnósticos JSON v1 |
| `--tokens-json` | Tokens, comentarios y errores de Flex en JSON |
| `--syntax-json` | Solo léxico y sintaxis completos en JSON |
| `--suggest-json <cursor>` | Sugerencias LAC en un cursor expresado en bytes UTF-8 |
| `--repair-json` | Reparaciones de cierres verificadas sobre copias |
| `--clones-json <mínimo>` | Clones I/II de un paquete CCL1; mínimo 5..100000 tokens |
| `--time` | Mostrar en stderr la duración total del trabajo en milisegundos |
| `--no-schedule` | Usar emisión conservadora para comparar bundles y ciclos |

Las siete consultas del IDE son excluyentes y solo admiten `-v` y `--time`.
Combinarlas entre sí, con `-i`/`--tac`, niveles `-O0/-O1/-O2`, `--opt`,
`--unroll`, `--metrics` u opciones de generación/listados devuelve código 2.
La compilación normal conserva las opciones O1 y TAC.

Para consultar todas las opciones sin compilar:

```bash
./ccom --help
```

Para medir cuánto tarda la compilación del programa:

```bash
./ccom --time pruebas/p02_factorial.cls
```

El tiempo incluye análisis, generación, organización, ensamblado y escritura
de salidas; también se informa si el trabajo falla. Se mide tiempo de pared
con un reloj monotónico. Con `-x` se mide el ensamblado y con `--ast-json`
solo, sin opciones que continúen compilando, se mide la exportación del AST.

Consulta los detalles en [Uso y banderas de ccom](docs/ccom_banderas.md).

## Ver el árbol gráfico del programa

Desde la raíz del repositorio, con comandos compatibles con Fish y Bash:

```sh
make
./ccom --ast-json build/p09_crypto.ast.json pruebas/p09_crypto.cls
xdg-open tools/visor_ast.html
```

Si `xdg-open` no está disponible, abrir `tools/visor_ast.html` con un navegador
desde el administrador de archivos. El visor funciona sin servidor, sin
instalar paquetes y sin conexión a internet.

Al abrirlo se muestra un ejemplo de p09. Para visualizar el archivo que acabás
de generar, pulsar **Abrir AST JSON** y seleccionar
`build/p09_crypto.ast.json`. Para otro programa, cambiar la entrada `.cls` y
el nombre del JSON en el comando anterior.

El árbol se dibuja con nodos y ramas, desde la raíz hasta sus hojas. Las
declaraciones y los bloques empiezan plegados; pulsar **Expandir todo** para
mostrarlo completo.

- **Rueda del mouse:** acercar o alejar.
- **Arrastrar el fondo:** desplazarse por el árbol.
- **Clic en un nodo:** ver tipo, texto/valor y ubicación en el código.
- **Doble clic en un nodo:** expandir o plegar su rama.
- **Ajustar árbol:** encuadrar los nodos visibles; **Ir a la raíz:** centrar la raíz.

`--ast-json` usado solo exporta el árbol después del análisis sintáctico y
termina allí: permite inspeccionar programas sin ejecutar el resto de las
fases, pero no certifica que pasen el análisis semántico. Para
exportar y continuar la compilación, combinarlo con `-s`.

Más detalles y formato del JSON en [Visor del AST](docs/visor_ast.md).

## Un programa Calser

Los programas usan la extensión `.cls`. Este ejemplo declara una variable,
realiza una suma y termina la función principal:

```calser
F main [] : void ;
    Vm resultado : int = 2 + 3 ;
    E ;
.
```

La sintaxis se define en [grammar/parser.y](grammar/parser.y) y los tokens en
[grammar/lexer.l](grammar/lexer.l). En `pruebas/` hay ejemplos válidos y casos
que ejercitan errores del compilador.

## Estructura

```text
calser_compiler/
├── src/                 # main.c y componentes en subcarpetas
├── include/             # Cabeceras agrupadas por los mismos componentes
├── grammar/             # lexer.l (Flex) y parser.y (Bison)
├── pruebas/             # Suites agrupadas por componente
│   ├── conductor/       # ayuda, exito, tiempo y ejecutor
│   ├── isa/             # constantes del ISA
│   ├── analisis/        # check, ast, constantes y recuperacion
│   ├── representacion/  # TAC, construcción y análisis de flujo
│   ├── optimizador/     # gestor de pasadas
│   ├── generador_o0/    # programas, matrices, etiquetas y diagnósticos
│   ├── generador_o1/    # bajada, asignación y emisión
│   ├── rutinas/         # aritmetica, arreglos y crypto
│   ├── ensamblado/      # bundles, ensamblador y simulador
│   ├── servicios_ide/   # consultas JSON e integración del IDE
│   └── p*.cls  e*.cls  resultados.txt  correr_pruebas.sh
├── tools/               # Simulador, ejecutor de pruebas, generador de ejercicios y visor del AST
├── ejercicios/          # Entregables de los tres ejercicios
├── docs/                # Documentación del compilador
├── assets/branding/     # Logo del lenguaje e icono de los programas
├── editors/vscode/      # Extensión para Calser, Lex/Flex y Yacc/Bison
├── ide/                 # Núcleo del IDE y pruebas; guía: ide/nucleo/README.md
├── build/               # Archivos generados y objetos; ignorados por Git
├── makefile
└── README.md
```

Los módulos con interfaz propia tienen su `include/<componente>/<módulo>.h`. `main` no
tiene cabecera; `semantic_expr`, `semantic_crypto` y los `generate_*` comparten
`semantic_internal.h` y `generate_internal.h`; `codegen_instruction` y
`codegen_verify` se declaran en `codegen.h` y `codegen_schedule.h`.

| Grupo | Módulo | Responsabilidad |
| --- | --- | --- |
| Entrada | `main` | Opciones de ejecución; coordina las fases con un estado de compilación y una función por fase |
| | `diagnostics` | Reporte de errores con archivo, línea y columna |
| | `ast` | Construcción, impresión, exportación JSON y liberación del AST; consultas de hijos |
| Semántico | `semantic` | Recorrido del análisis: firmas, declaraciones, bloques y funciones |
| | `semantic_scope` | Ámbitos, función en análisis y resolución de nombres |
| | `semantic_expr` | Tipos de expresiones, usos con corchetes, constantes, condiciones, asignaciones e inicializadores |
| | `semantic_calls` | Grafo de llamadas y detección de recursión |
| | `semantic_crypto` | Verificación de las operaciones de cifrado |
| | `data_area` | Contador de la memoria de datos estática y su límite |
| | `symbol_table` | Símbolos, ámbitos y el área estática de cada función (`FrameLayout`) |
| | `label_table` | Tabla de etiquetas y sus direcciones |
| | `semantic_internal` | Cabecera interna: errores semánticos y tipos de expresión compartidos |
| Generación | `generate` | Recorrido del programa: globales, `main` primero y el resto de las funciones |
| | `generate_support` | Registros temporales, constantes, variables en memoria, pila y etiquetas |
| | `generate_expr` | Expresiones, aritmética por software, arreglos, llamadas y CRYPTO |
| | `generate_stmt` | Bloques, asignaciones, `C`/`A`, ciclos `L` y `S`, `E` y `R` |
| | `generate_init` | Inicializadores de variables, en la imagen de datos o en código |
| | `generate_internal` | Cabecera interna: `GenCtx`, el estado de una generación |
| | `codegen_arith` / `codegen_array` / `codegen_crypto` | Rutinas de aritmética, arreglos y CRYPTO, con pruebas independientes |
| Salida | `codegen` | Emisor: IR, líneas, bundles e imagen de datos |
| | `codegen_instruction` | Parseo y validación del texto; describe dependencias con la semántica de `isa_table` |
| | `codegen_schedule` / `codegen_verify` | Organizador de bundles y verificación de su calendario |
| | `isa_table` | Tabla única del ISA: formatos, unidad, lecturas, escrituras, preservación y efectos; lectura de registros y enteros |
| | `assembler` | Ensamblador de dos pasadas: etiquetas y codificación de bundles |

Las cabeceras y las implementaciones se agrupan en las mismas subcarpetas de
`include/` y `src/`: `comun`, `isa`, `analisis`, `representacion`, `optimizador`,
`generador_o0`, `generador_o1`, `rutinas`, `ensamblado` y `servicios_ide`. Flex y
Bison generan sus archivos en `build/generated/`; los objetos y dependencias
se guardan en `build/obj/`. El ejecutable se mantiene en `./ccom`.

## Entregables

Los entregables de los tres ejercicios (enunciado §10) están en
[ejercicios/](ejercicios/README.md). Cada carpeta (`ejercicio1_factorial/`,
`ejercicio2_matriz/`, `ejercicio3_cifrado/`) contiene:

- el fuente `.cls`;
- el ensamblador `.asm`, el binario `.bin`, el `.hex` y la imagen de datos `.mem`;
- `compilacion.txt` (salida de `ccom -v -m -s -p`), `traza.txt` (traza de
  ejecución) y `ejecucion.txt` (resumen contra los valores esperados).

[ejercicios/README.md](ejercicios/README.md) explica cada archivo y dónde se
cumple cada requisito. [ejercicios/make_check.txt](ejercicios/make_check.txt) es
la salida de `make check`. Para regenerar los archivos de los ejercicios:

```bash
make ejercicios
```

## Pruebas

El ejecutor opcional permite elegir suites y muestra un resumen de resultados:

```bash
python3 tools/probar.py --listar
python3 tools/probar.py matrices tiempo ayuda
python3 tools/probar.py                   # todas las suites
python3 tools/probar.py --sin-compilar matrices
python3 tools/probar.py --caso pruebas/p02_factorial.cls
python3 tools/probar.py --caso pruebas/e01_no_declarado.cls --codigo-esperado 1
```

Make se usa solo para preparar los binarios requeridos; `--sin-compilar` evita
ese paso. El script es independiente de `ccom` y no se ejecuta al compilar
normalmente. `--caso` comprueba el código de salida del compilador; las suites
`programas` y `matrices` también verifican valores finales en el simulador.
Consultar [la guía del ejecutor](docs/pruebas.md) para más opciones.

Para ejecutar los casos del compilador y guardar errores, tablas y códigos
de salida en un reporte:

```bash
make
bash pruebas/correr_pruebas.sh
```

El reporte se escribe en `pruebas/resultados.txt`. Este script recopila
resultados para su revisión; no compara automáticamente salidas esperadas.

El organizador local de bundles está activado por defecto y verifica cada
calendario antes de emitirlo. Su uso y la comparación de factorial con
`--no-schedule` están en [Organización de bundles](docs/ccom_banderas.md#organización-de-bundles).

Para verificar las rutinas de generación de código:

```bash
make check-tac  # 6 casos de alto nivel, ISA virtual/física y ejemplo .cls; sin Python
make check-arith
make check-arrays
make check-matrices
make check-crypto
make check-semantica-isa
make check-bundles
make check-salidas
# O ejecutar todas:
make check
```

Para ver la comparación de factorial escalar y paralelo y conservar los
ensambladores, trazas y métricas:

```sh
make validar-bundles
less build/validacion_bundles/reporte.md
```

El reporte está en `build/validacion_bundles/`, junto a los archivos de ambos
modos. Comprueba resultado 120, paralelismo realmente ejecutado, cero avisos
y reducción de ciclos, además de seis entradas auxiliares.

Las [pruebas matriciales](pruebas/generador_o0/matrices/README.md) verifican suma, resta y
productos con ciclos e índices, incluyendo AB distinto de BA, matrices
rectangulares, identidad y ceros, con y sin organización de bundles.

El simulador verifica autenticación Lamport mediante Ascon-Hash256 truncado
a 32 bits, consume los tokens válidos y rechaza su reutilización.

Estas pruebas usan un modelo de software de la arquitectura objetivo. Las
pruebas crypto incluyen el recorrido `.cls` → `ccom` → `.asm`/`.mem` →
simulador; no representan una simulación RTL.

Para probar el núcleo del IDE (no necesita compilar `ccom`; ver
[ide/nucleo/README.md](ide/nucleo/README.md)):

```bash
make check-ide
```

Para limpiar los productos de compilación:

```bash
make clean
```

## Soporte en VS Code

La extensión incluida registra tres lenguajes:

| Extensión | Lenguaje |
| --- | --- |
| `.cls` | Calser |
| `.l` | Lex/Flex |
| `.y` | Yacc/Bison |

Incluye resaltado de sintaxis, comentarios, cierre de parejas y el tema de
iconos **Calser (Seti)**, que muestra el hexágono ámbar en los programas `.cls`.
Los colores del código los determina el tema de colores de cada integrante.

Para generar e instalar el paquete local:

```bash
python3 editors/vscode/package.py
code --install-extension editors/vscode/calser-language-0.3.0.vsix --force
```

Después ejecuta **Developer: Reload Window** y selecciona **Preferences:
File Icon Theme → Calser (Seti)**. También puedes instalar el `.vsix` con
**Extensions: Install from VSIX…**.

Si `.cls` se reconoce como LaTeX, crea o ajusta `.vscode/settings.json`:

```json
{
  "files.associations": { "*.cls": "calser" },
  "workbench.iconTheme": "calser-seti"
}
```

Hacer `git pull` comparte el código de la extensión; cada integrante debe
instalarla en su VS Code. `.vscode/` y los paquetes `.vsix` están ignorados
por Git. Más detalles en [editors/vscode/README.md](editors/vscode/README.md).

## Identidad visual

- [Logo del lenguaje](assets/branding/calser-logo.png).
- [Icono de los programas](assets/branding/calser-file-icon.png).

La extensión conserva una copia del icono para que pueda instalarse de forma
independiente del repositorio. Los avisos de los recursos Seti están en
[ThirdPartyNotices.txt](editors/vscode/ThirdPartyNotices.txt).

### Representación intermedia desde AST (C2.1)

`./ccom -i programa.cls` (o `--tac`) agrega `programa.tac`; con `-o salida.bin`
usa `salida.tac`. La compilación continúa con el generador actual, conservando
los binarios. El listado contiene imagen inicial, funciones y bloques de alto
nivel. `make check-tac-build` prueba construcción y CLI con 22 esperados, sin
intérprete todavía. Ver [la documentación de TAC](docs/representacion_intermedia.md)
y [las banderas](docs/ccom_banderas.md).

## CALSIDE · IDE de Calser

Aplicación propia de escritorio con Python/PySide6 para edición, proyecto, búsqueda, terminales y procesos. ccom aporta léxico, sintaxis/semántica, sugerencias, reparaciones y clones en C; el LL(1) Python se conserva como API de consulta/pruebas. Incluye perfiles gráficos F5, ejecución F6 mediante el simulador, recuperación de varios errores sintácticos y protección de cambios externos. M1–M7 implementados; revisión manual, empaquetado y otra máquina siguen su plan de cierre.

Los motores del IDE permanecen dentro de ccom, en C. Desde la raíz:

```sh
make
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make PYTHON=.venv/bin/python ide
```

Pruebas gráficas: `QT_QPA_PLATFORM=offscreen make PYTHON=.venv/bin/python check-ide check-parser check-recorrido-ide`.
 [Uso e instalación](ide/README.md) · [Guía del sistema](ide/docs/guia_sistema_calside.md) · [Plan vigente](ide/docs/plan_ide.md) · [Revisión manual](ide/docs/revision_manual.md) · [Bitácora](ide/docs/bitacora_ide.md) · [Protocolos C](docs/ccom_servicios_ide.md).
