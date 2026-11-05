# CALSIDE · prototipo inicial

Aplicación de escritorio PySide6. Conserva los controles nativos para mover, minimizar, maximizar y cerrar. La esquina morada es estática; la animación de papel queda para una iteración posterior.

Desde la raíz del repositorio, con Python 3.10 o posterior:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make  # construye ccom, incluida la bandera --check
make ide PYTHON=.venv/bin/python
```

Si PySide6 ya está instalado, basta con `make ide`. También se puede abrir un programa mediante `python3 -m ide.main pruebas/p02_factorial.cls`. Python 3.14.9 y PySide6 6.12.0 son la combinación verificada inicialmente; probar las demás versiones antes de la defensa. El empaquetado como ejecutable todavía está pendiente.

## Disponible

- Distribución clásica oscura con ámbar Calser y esquina morada.
- Editor, números de línea, pestañas, indicador de cambios, estado de cursor y búsqueda.
- Abrir archivo/carpeta, nuevo, guardar, guardar como y cierre con guardar/descartar/cancelar.
- Explorador de `.cls` de la carpeta actual (sin subcarpetas en este prototipo).
- Resaltado con tokens reales del núcleo y errores léxicos mientras escribes, incluidos comentarios multilínea y texto sin guardar. El análisis automático sintáctico/semántico usa ccom --check tras otra pausa breve.
- Compilación real con `ccom`, sin bloquear la ventana; cancelar, límite de 30 s, salida completa y errores navegables (doble clic o Enter).
- Salidas en `<carpeta del fuente>/build/ide/<identificador de ruta>/`, evitando colisiones entre fuentes.

El compilador se busca en la ruta elegida en Compilar → Configurar ccom, después en la raíz del repositorio y finalmente en PATH. Ejecutar `make` si hace falta construirlo. Compilar guarda antes el documento activo; los diagnósticos se retiran al editar y las compilaciones de versiones anteriores se identifican.

## Diagnósticos de ccom · paso 2

Al compilar con F5, los errores léxicos, sintácticos y semánticos aparecen con origen **ccom** en Problemas y se marcan en su pestaña:

- Subrayado ondulado sobre el rango informado por el compilador y punto rojo en el margen.
- Descripción al colocar el puntero sobre la línea y una franja de detalle bajo el editor cuando el cursor está en ella.
- Un clic o Enter en Problemas selecciona el fragmento y centra la vista. Se muestra también categoría, archivo, línea y columna.
- Para terminadores faltantes al final de línea se marca la línea sin subrayar otro token. La longitud se obtiene del señalador que imprime ccom; si no existe, se utiliza una posición mínima.
- Cada pestaña conserva el resultado de su última compilación válida. Editarla o cambiar su ruta retira sus diagnósticos; las marcas de las otras pestañas permanecen. Los resultados de una revisión anterior se descartan.
- La conversión de columnas UTF-8 a posiciones UTF-16 conserva tildes y caracteres fuera del BMP. Los mensajes sin posición se mantienen completos en Salida; no se inventa un rango.

Los diagnósticos con origen **ccom** se actualizan al compilar con F5. Los de **núcleo** y **ccom automático** se actualizan durante la escritura mediante E3/E4.

## Léxico en vivo · E3

Después de una pausa de **300 ms**, el lexer de `ide.nucleo` analiza una copia del texto de cada pestaña en segundo plano. No guarda el documento, no ejecuta ccom ni genera archivos. El resaltado usa sus tokens reales; los comentarios se colorean con un recorrido de presentación porque el lexer los omite.

- Errores con origen **núcleo** en Problemas, en el margen y en los subrayados. Funciona también en pestañas **Sin título**; un clic navega al carácter correspondiente.
- La franja bajo el editor y la descripción al pasar el puntero identifican el origen. La barra de estado indica espera, análisis, errores, resultado o fallo.
- Cada resultado lleva documento y revisión. Los resultados de texto antiguo o de pestañas cerradas se descartan; la cola mantiene una tarea pendiente por documento y un trabajador activo.
- El formato no cuenta como edición, no ensucia archivos guardados y no introduce operaciones adicionales de Deshacer.
- Las posiciones Unicode del núcleo se convierten a UTF-16; las de ccom siguen convirtiéndose desde UTF-8. Un error léxico del mismo rango informado por ambos se presenta una sola vez, con prioridad para ccom.
- Documentos de más de **2 millones de caracteres** pausan este análisis con un mensaje explícito; todavía se pueden compilar con F5.

Prueba rápida: crea una pestaña e introduce `Vm x : int = 3 ; @`. El error de `@` aparece sin guardar ni compilar; selecciónalo desde Problemas y bórralo. Desaparece tras la pausa. Una declaración sin `;` no se señala por el lexer; después la revisa el análisis sintáctico automático de ccom (E4). El parser predictivo ya está disponible como API Python; su conexión gráfica sigue pendiente en E5.

## Parser predictivo disponible para los motores

`ide.nucleo.parser` ofrece `analizar_sintaxis(texto)` y `contexto_en(texto, cursor, inicio_fragmento=None)`: pila LL(1), primer error, terminales esperados y consulta de prefijos sin exigir un programa completo. No genera AST, no hace semántica ni edita el documento. Ver [API real, ejemplos y límites](docs/parser_predictivo.md). Se verifica con `make check-parser`; los motores y su UI siguen pendientes.

## Sintaxis y semántica en vivo · E4

Tras el lexer (300 ms) y una pausa adicional de **600 ms**, CALSIDE ejecuta **`ccom --check`** en segundo plano sobre una copia temporal del buffer, incluidos los documentos sin nombre. La copia se elimina al terminar, cancelar o fallar. El archivo original y sus artefactos permanecen intactos.

- Los resultados se identifican como **ccom automático** y se presentan en Problemas, subrayados y descripciones, sin cambiar de pestaña ni robar el foco.
- Las rutas temporales se remapean al documento original o a Sin título. El directorio de trabajo corresponde al del fuente cuando existe; el enlazado de módulos futuro deberá respetar también el contexto de importación.
- Se omite el proceso si el documento está vacío o el lexer ya encontró errores. Un único proceso atiende la cola; editar/cerrar/cambiar de ruta descarta la revisión anterior y cancela su proceso.
- **Análisis de ccom** conserva el estado y los mensajes del último análisis, separados de **Salida de ccom**, que pertenece a F5. Un ejecutable antiguo sin `--check`, un fallo de inicio o un error sin posición se explica ahí; no se presenta como un documento correcto.
- F5 conserva su compilación normal y tiene prioridad: pausa/cancela el análisis automático y reanuda las solicitudes de otras pestañas después. Los diagnósticos equivalentes no se duplican.
- Puede activarse/desactivarse en **Ver → Análisis sintáctico y semántico automático**. Desactivarlo retira los resultados automáticos y conserva los de compilación manual y del lexer.
- Límite de tiempo actual: **5 s**; máximo de salida: **1 MB por canal**. Los errores mantienen sus posiciones y mensajes de ccom; E4 no reescribe todavía explicaciones como la sentencia incompleta `vs` seguida de `E`.

Requiere reconstruir ccom con `make`. Prueba: escribe una función con una variable no declarada y espera; aparece el error semántico sin guardar ni pulsar F5. Corrige el nombre y el mensaje desaparece. Quitar un `;` permite probar sintaxis. `ccom --check` ejecuta el front-end completo, pero no genera código ni certifica ejecución.

## Distribución y terminales

- Menús centrados verticalmente en la barra debajo de la cabecera.
- Ocultar explorador con **×**, botón **Archivos**, **Ctrl+B** o menú Ver; el botón permite recuperarlo.
- **⇥** en la esquina del panel mueve Problemas/Salida/Clones/Terminales a la derecha; **⇣** los devuelve abajo. Atajo: **Ctrl+Alt+J**. Los divisores permiten ajustar el espacio y se mantienen las sesiones y el contenido.
- **＋** o **Ctrl+Shift+T** crea otra pestaña de terminal. Cada sesión tiene shell y directorio independientes; inicia en la carpeta de proyecto.
- Entrada por líneas, Enter para ejecutar, ↑↓ para historial y **Interrumpir** para enviar Ctrl+C. Cerrar una pestaña o el IDE solicita confirmación antes de terminar sesiones activas.
- Terminal PTY real disponible en **Linux**. La vista es de comandos: todavía no emula pantalla completa ni soporta correctamente aplicaciones como Vim/top, secuencias ANSI complejas o redimensionamiento del PTY. No requiere paquetes adicionales de pip.

## Clones Tipo I y II

El panel **Clones → Analizar carpeta** (o **Ctrl+Shift+D**) compara los `.cls` de la carpeta de proyecto y sus subcarpetas, sin compilar y sin depender del núcleo LL(1).

- **Tipo I:** misma secuencia de tokens, ignorando espacios y comentarios.
- **Tipo II:** misma estructura con renombramiento consistente y uno-a-uno de identificadores y cambios de literales del mismo tipo. Las palabras reservadas, tipos y operadores no se normalizan. No equivale a demostrar que dos programas hagan lo mismo.
- Mínimo inicial: **20 tokens**, configurable de 5 a 200. Un umbral bajo puede encontrar fragmentos triviales. Se muestran pares máximos por tipo; una parte exacta puede coexistir con un clon Tipo II más amplio.
- Se usan los buffers de archivos abiertos con nombre, incluidos cambios sin guardar. Se excluyen documentos sin nombre, archivos fuera del proyecto, enlaces simbólicos en disco y carpetas `.git`, `.venv`, `venv`, `build`, `bin`, `obj`, `node_modules`, `__pycache__` y otras carpetas ocultas.
- Selecciona un par para comparar sus textos y usa **Abrir A/B** para seleccionar el fragmento en el editor. Las posiciones respetan Unicode. Editar un documento analizado, cambiar la carpeta o el umbral invalida el resultado; antes de navegar se comprueba que el texto siga siendo el analizado.
- Ejecución en segundo plano y botón Cancelar. Límites: 200 archivos, 2 MB por archivo, 20 MB de texto, 100000 tokens, 50000 candidatos y 500 pares. Al excederlos se explica cómo reducir el análisis; no se muestran resultados incompletos como definitivos.
- Es una comparación léxica de fragmentos contiguos: no exige programas completos ni AST, y un fragmento puede comenzar o terminar dentro de una sentencia.

Demostración: abre la carpeta `ide/ejemplos/clones`, pulsa Analizar carpeta con mínimo 20. Deben aparecer tres pares: uno Tipo I (`base`/`igual`) y dos Tipo II (cada uno con `renombrado`). Se incluyen archivos `.cls` reales y no se simulan los resultados.

## Pendiente

Motor y UI de sugerencias (E5; parser ya implementado), correcciones automáticas (E6), detección de cambios externos, preferencias persistentes, árbol recursivo, iconos de archivos y empaquetado. El lexer ya está conectado al editor. No hay resultados simulados.

El resaltado usa el lexer del núcleo; conserva la paleta inicial inspirada en la extensión y el icono Calser existente. No copia el tema Seti.

## Validación

```sh
make check-ide
```

Comprobación manual: abrir `pruebas/p02_factorial.cls`, compilar y ver el resultado; abrir `pruebas/e01_no_declarado.cls`, compilar y activar el error; editar y verificar el indicador de cambios; cancelar el cierre para conservar el texto. Verificar varias pestañas, búsqueda y redimensionamiento de paneles.

Plan completo y alternativas visuales: [plan](docs/plan_ide.md), [mockups](docs/mockups.html).
