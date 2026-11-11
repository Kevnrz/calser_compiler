# CALSIDE · prototipo inicial

> **M1/M1B/M2/M3/M4/M5/M6 completados (2026-11-11):** el resaltado y léxico vivo usan Flex mediante `ccom --tokens-json`; el análisis automático usa el servicio `ccom --check-json`. Sugerencias y reparaciones usan motores C (`--suggest-json` / `--repair-json`); la política verifica sintaxis con C y los clones usan Flex y comparación C (`--clones-json`). El LL(1) y el simulador siguen en Python con la autorización aclarada por Saddy. [Plan y etapas pendientes](docs/plan_migracion_logica_c.md).

Para entender la arquitectura, los flujos y el papel de cada archivo, consulta la [guía unificada del sistema y recorrido de comentarios](docs/guia_sistema_calside.md).

Aplicación de escritorio PySide6. Conserva los controles nativos para mover, minimizar, maximizar y cerrar. La esquina morada es estática; la animación de papel queda para una iteración posterior.

Desde la raíz del repositorio, con Python 3.10 o posterior:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make  # construye ccom con --tokens-json y --check-json
make ide PYTHON=.venv/bin/python
```

Si PySide6 ya está instalado, basta con `make ide`. También se puede abrir un programa mediante `python3 -m ide.main pruebas/p02_factorial.cls`. Python 3.14.9 y PySide6 6.12.0 son la combinación verificada inicialmente; probar las demás versiones antes de la defensa. El empaquetado como ejecutable todavía está pendiente.

## Disponible

- Distribución clásica oscura con ámbar Calser y esquina morada.
- Editor, números de línea, pestañas, indicador de cambios, estado de cursor y búsqueda.
- Abrir archivo/carpeta, nuevo, guardar, guardar como y cierre con guardar/descartar/cancelar.
- Explorador de subcarpetas y `.cls` con carga asíncrona, exclusiones y navegación por teclado.
- Avisos por cambios/borrado externo, recarga explícita y protección al sobrescribir.
- Preferencias de proyecto, ccom, fuente y distribución persistentes; fuente configurable desde Ver.
- Resaltado y errores léxicos desde Flex (`ccom --tokens-json`) mientras escribes, incluidos comentarios multilínea y texto sin guardar. El análisis automático sintáctico/semántico usa `ccom --check-json` tras otra pausa breve. Ambos necesitan un ccom compatible; no se usa el lexer Python como respaldo.
- Compilación real con `ccom` (F5), sin bloquear la ventana; cancelar, límite de 30 s, salida completa y errores navegables (doble clic o Enter).
- Ejecutar (F6) mediante `tools/sim_asm.py`, separado de Compilar, con traza, registros y estado final de variables en la pestaña Ejecución.
- Salidas en `<carpeta del fuente>/build/ide/<identificador de ruta>/`, evitando colisiones entre fuentes.

El compilador se busca en la ruta elegida en Compilar → Configurar ccom, después en la raíz del repositorio y finalmente en PATH. Ejecutar `make` si hace falta construirlo. Compilar guarda antes el documento activo; los diagnósticos se retiran al editar y las compilaciones de versiones anteriores se identifican.

## Compilar y ejecutar

1. **Compilar ▾ / F5** ejecuta el modo seleccionado: compilar programa, analizar sin artefactos, exportar AST JSON o ensamblar una entrada `.asm`. El modo inicial compila con listado `.asm` para F6. **Opciones de compilación…** permite elegir destino y banderas compatibles; la selección se guarda en preferencias. Se solicita guardar el documento antes de operar. El modo AST comprueba sintaxis, sin certificar semántica. Ensamblar pide una entrada `.asm` si no hay una activa. Ningún modo ejecuta el programa.
2. **Ejecutar (F6)** ejecuta la última compilación vigente de ese documento en el simulador del repositorio. No guarda ni recompila automáticamente. Si no existe una compilación correcta de esta sesión, hay cambios pendientes/externos o faltan/cambiaron los artefactos, solicita compilar otra vez.

Solo una compilación completa correcta con listado `.asm` y salidas renovadas permite F6. Cambiar a análisis, exportar AST, ensamblar o compilar sin `-s` no registra un ejecutable. El destino automático sigue en `build/ide/<hash de fuente>/`; un destino personalizado pide confirmación antes de reemplazar archivos existentes. Las salidas no pueden sobrescribir la entrada ni documentos abiertos. Las opciones manuales no cambian el análisis automático. No se ofrecen IR/TAC u optimizaciones de otras ramas todavía.

La pestaña **Ejecución** muestra stdout/stderr del simulador (registros, memoria y resumen), la traza y el estado final de las variables. Por ejemplo, el programa de suma de 2 y 3 muestra `resultado = 5`. Las salidas de ccom permanecen en su pestaña independiente.

Cada corrida crea un subdirectorio `ejecucion-…` dentro de la carpeta de compilación y copia allí el `.asm` y el `.mem`, comprobando sus huellas. `traza.txt` queda en ese mismo subdirectorio; su ruta aparece al terminar. No reutiliza la traza de una corrida anterior.

**Cancelar** detiene el proceso. Se usan los límites existentes del simulador: 100000 bundles y 5000 eventos de traza, además de 30 s de tiempo. Los avisos/límite de bundles y los errores se distinguen del éxito. La vista conserva hasta 12000 bloques y presenta como máximo 2 MB de traza; el archivo guardado mantiene la traza generada por el simulador.

Durante la ejecución se puede editar y usar la interfaz; Compilar y Ejecutar quedan deshabilitados para evitar procesos superpuestos. Mover/eliminar el fuente, cerrar su pestaña o salir requiere detener primero la corrida. Si se modifica el código mientras corre, el resultado se identifica como correspondiente a la versión compilada anterior. El análisis automático de ccom se suspende durante la corrida y luego se reanuda.

La ejecución requiere `tools/sim_asm.py` y el Python con el que se abrió CALSIDE. Al reiniciar se necesita volver a compilar: los artefactos permanecen en disco, pero no se recuperan como compilaciones confiables. Las opciones avanzadas como `--load-data` siguen disponibles mediante la terminal; todavía no se configuran desde el botón.

## Diagnósticos de ccom · paso 2

Al compilar con F5, los errores léxicos, sintácticos y semánticos aparecen con origen **ccom** en Problemas y se marcan en su pestaña:

- Subrayado ondulado sobre el rango informado por el compilador y punto rojo en el margen.
- Descripción al colocar el puntero sobre la línea y una franja de detalle bajo el editor cuando el cursor está en ella.
- Un clic o Enter en Problemas selecciona el fragmento y centra la vista. Se muestra también categoría, archivo, línea y columna.
- Para terminadores faltantes al final de línea se marca la línea sin subrayar otro token. La longitud se obtiene del señalador que imprime ccom; si no existe, se utiliza una posición mínima.
- Cada pestaña conserva el resultado de su última compilación válida. Editarla o cambiar su ruta retira sus diagnósticos; las marcas de las otras pestañas permanecen. Los resultados de una revisión anterior se descartan.
- La conversión de columnas UTF-8 a posiciones UTF-16 conserva tildes y caracteres fuera del BMP. Los diagnósticos generales del archivo (por ejemplo, falta de `main`) aparecen en Problemas, con línea/columna «—» y descripción visible, sin inventar un subrayado. Las demás líneas se conservan en Salida.

Los diagnósticos con origen **ccom** se actualizan al compilar con F5. Los de **núcleo** y **ccom automático** se actualizan durante la escritura mediante E3/E4.

## Léxico en vivo · E3

Después de una pausa de **300 ms**, el lexer de `ide.nucleo` analiza una copia del texto de cada pestaña en segundo plano. No guarda el documento, no ejecuta ccom ni genera archivos. El resaltado usa sus tokens reales; los comentarios se colorean con un recorrido de presentación porque el lexer los omite.

- Errores con origen **núcleo** en Problemas, en el margen y en los subrayados. Funciona también en pestañas **Sin título**; un clic navega al carácter correspondiente.
- La franja bajo el editor y la descripción al pasar el puntero identifican el origen. La barra de estado indica espera, análisis, errores, resultado o fallo.
- Cada resultado lleva documento y revisión. Los resultados de texto antiguo o de pestañas cerradas se descartan; la cola mantiene una tarea pendiente por documento y un trabajador activo.
- El formato no cuenta como edición, no ensucia archivos guardados y no introduce operaciones adicionales de Deshacer.
- Las posiciones Unicode del núcleo se convierten a UTF-16; las de ccom siguen convirtiéndose desde UTF-8. Un error léxico del mismo rango informado por ambos se presenta una sola vez, con prioridad para ccom.
- Documentos de más de **2 millones de caracteres** pausan este análisis con un mensaje explícito; todavía se pueden compilar con F5.

Prueba rápida: crea una pestaña e introduce `Vm x : int = 3 ; @`. El error de `@` aparece sin guardar ni compilar; selecciónalo desde Problemas y bórralo. Desaparece tras la pausa. Una declaración sin `;` no se señala por el lexer; después la revisa el análisis sintáctico automático de ccom (E4). El parser predictivo ya está disponible como API Python; las sugerencias M3 consultan Bison/LAC en C; la política automática verifica original/candidato mediante el parser C; Python conserva permisos y aplicación.

## Parser predictivo disponible para los motores

`ide.nucleo.parser` ofrece `analizar_sintaxis(texto)` y `contexto_en(texto, cursor, inicio_fragmento=None)`: pila LL(1), primer error, terminales esperados y consulta de prefijos sin exigir un programa completo. No genera AST, no hace semántica ni edita el documento. Ver [API real, ejemplos y límites](docs/parser_predictivo.md). Se verifica con `make check-parser`; las sugerencias reales ya están conectadas a C; reparaciones M4 también está conectada al motor C.

## Sintaxis y semántica en vivo · E4

Tras el lexer (300 ms) y una pausa adicional de **600 ms**, CALSIDE ejecuta **`ccom --check-json`** en segundo plano sobre una copia temporal del buffer, incluidos los documentos sin nombre. La copia se elimina al terminar, cancelar o fallar. El archivo original y sus artefactos permanecen intactos.

- Los resultados se identifican como **ccom automático** y se presentan en Problemas, subrayados y descripciones, sin cambiar de pestaña ni robar el foco.
- Las rutas temporales se remapean al documento original o a Sin título. El directorio de trabajo corresponde al del fuente cuando existe; el enlazado de módulos futuro deberá respetar también el contexto de importación.
- Se omite el proceso si el documento está vacío o el lexer ya encontró errores. Un único proceso atiende la cola; editar/cerrar/cambiar de ruta descarta la revisión anterior y cancela su proceso.
- **Análisis de ccom** conserva el estado y los mensajes del último análisis, separados de **Salida de ccom**, que pertenece a F5. Un ejecutable antiguo sin `--check`, un fallo de inicio o una salida de error sin diagnóstico reconocible se explica ahí; no se presenta como un documento correcto.
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

El detector usa **Flex y comparación en C** mediante `--clones-json`; Python reúne copias sin guardar y presenta los fragmentos. Los errores léxicos bloquean el análisis; no hay un tokenizador alternativo. Consulta cancelable con límite de 15 s. [Protocolo y límites M5](../docs/ccom_clones_json.md).

## Protección de archivos y proyecto · E7

Si otro programa cambia o borra una fuente abierta, aparece un aviso con **Recargar**, **Conservar** y **Guardar como**. Tu texto no se reemplaza automáticamente. Guardar vuelve a comprobar la versión del disco; un conflicto requiere decisión explícita y un segundo cambio durante el guardado lo cancela.

El explorador muestra subcarpetas y archivos `.cls`; excluye dependencias/salidas y enlaces simbólicos. **Ctrl+Shift+E** le da foco; flechas y Enter permiten navegar y abrir archivos. **Ctrl+Shift+R** recarga con confirmación si hay ediciones locales.

Al cerrar normalmente se recuerdan carpeta, ccom, fuente, tamaño/visibilidad/distribución de paneles y análisis automático. **Ver → Fuente del editor…** cambia las pestañas actuales y futuras. La configuración reside en el perfil del usuario; no guarda documentos ni sesiones de terminal.

Ver [comportamiento, exclusiones, límites y prueba manual](docs/proyecto_archivos.md). E7 pasa 28 pruebas nuevas y la suite combinada de 124 pruebas. La validación en otra máquina permanece en E8/E9.

## Pendiente

Recuperación sintáctica M7 (M1–M6 completados), correcciones automáticas, preferencias adicionales y empaquetado. El lexer ya está conectado al editor. No hay resultados simulados.

El resaltado usa el lexer del núcleo; conserva la paleta inicial inspirada en la extensión y el icono Calser existente. No copia el tema Seti.

## Validación

```sh
make check-ide
```

Comprobación manual: abrir `pruebas/p02_factorial.cls`, compilar y ver el resultado; abrir `pruebas/e01_no_declarado.cls`, compilar y activar el error; editar y verificar el indicador de cambios; cancelar el cierre para conservar el texto. Verificar varias pestañas, búsqueda y redimensionamiento de paneles.

Plan completo y alternativas visuales: [plan](docs/plan_ide.md), [mockups](docs/mockups.html).

### Portapapeles y foco

Copiar, cortar, pegar y Deshacer/Rehacer actúan sobre el campo de texto con foco, incluido el editor, la salida de ccom, el análisis automático y la entrada de terminal. Los paneles de salida siguen siendo de solo lectura. Para copiar una selección de consola sirven **Ctrl+C** y **Ctrl+Shift+C**. En la entrada de terminal, **Ctrl+C sin selección interrumpe el proceso**; con selección copia el texto. También se conserva el botón Interrumpir.

El panel de clones permite arrastrar el separador entre la tabla y la comparación para dar más altura al código. Cada fragmento tiene una cabecera con archivo/líneas y un botón compacto «Abrir ↗»; la ruta completa aparece al colocar el puntero. El reparto elegido se conserva durante la sesión al cambiar de par o volver a analizar.

## Archivos, carpetas y jerarquía

Click derecho permite **Nuevo archivo**, **Nueva carpeta**, Renombrar, Enviar a la papelera, Copiar, Cortar, Pegar, Copiar ubicación, Abrir ubicación y Propiedades. Sobre una carpeta también permite Expandir/Contraer y Expandir/Contraer todo. El fondo del árbol usa la raíz del proyecto como destino para crear o pegar.

Las carpetas se copian con todo su contenido en segundo plano, incluidos archivos que el filtro del explorador no muestra. Los enlaces internos se conservan como enlaces, sin recorrer sus destinos. Hay una operación de carpeta a la vez; las copias se pueden cancelar desde **Archivo → Cancelar copia de carpeta** o desde el menú contextual. Si una copia falla o se cancela, informa dónde quedó el destino incompleto y conserva el origen; no borra automáticamente los parciales.

Renombrar o mover una carpeta actualiza todas sus pestañas sin perder texto, cursor, Deshacer ni cambios pendientes. Se conservan los conflictos externos previos. Copiar usa la versión guardada en disco; Cortar mueve al pegar. Los destinos existentes no se fusionan ni se sobrescriben: para una entrada se pide otro nombre; en lotes se rechaza la operación antes de iniciarla. El portapapeles admite varios archivos y carpetas locales. El traslado de carpetas funciona en el mismo sistema de archivos; entre discos se permite copiar y el traslado se deriva al gestor del sistema.

Eliminar envía toda la carpeta a la papelera con confirmación y conserva los buffers abiertos como cambios sin guardar. No hay borrado permanente alternativo si falla la papelera. La raíz del proyecto y sus ancestros están protegidos; no se mueve ni elimina una carpeta que contiene la fuente de una compilación activa. Mientras una carpeta se procesa se bloquean guardados, recargas y compilaciones sobre esas rutas; se puede seguir editando o guardar una copia fuera de ellas. El cierre espera la operación para aplicar las nuevas rutas antes de salir.

El árbol recuerda las ramas abiertas durante la sesión y al reiniciar normalmente. Expandir todo carga por tandas sin recorrido recursivo bloqueante; limita a 1000 carpetas y avisa si se alcanza ese límite. Las ramas guardadas se limitan también por el tamaño de configuración. **Ver → Revelar archivo activo en el explorador** muestra su posición si pertenece al proyecto.

La selección admite **Ctrl+clic** para añadir/quitar entradas, **Shift+clic** para seleccionar un rango y **Ctrl+A** para seleccionar todo. Se puede dibujar un rectángulo empezando con el botón izquierdo sobre el espacio vacío. El clic derecho sobre una entrada seleccionada conserva el grupo: Copiar, Cortar y Papelera se aplican al conjunto. Renombrar y Propiedades requieren una sola entrada.

Al arrastrar aparece una tarjeta semitransparente junto al cursor, con hasta tres nombres y el recuento adicional. Para mover, suelta una o varias entradas en el centro de una carpeta (recuadro de destino); soltar sobre el espacio vacío las mueve a la raíz del proyecto. Al mantener el cursor en el centro, la carpeta se despliega tras una breve espera. Los bordes superior/inferior de una fila muestran una línea de inserción: ahí se cambia el orden visual entre entradas de la misma carpeta, sin cambiar ubicaciones. El orden se conserva al cambiar de proyecto durante la sesión y al reiniciar normalmente el último proyecto. El menú de la carpeta contenedora ofrece **Restablecer orden alfabético**. El arrastre es interno al explorador; mover desde otro programa todavía no está habilitado. Si se selecciona una carpeta y uno de sus descendientes, se procesa la carpeta una sola vez. Los lotes se validan antes de comenzar y las carpetas se procesan secuencialmente. Si aparece un fallo durante la operación, se detiene el resto y se informa cuántas entradas se completaron; no se deshacen las ya completadas. Cortar limpia el portapapeles únicamente tras completar todo el lote.

Atajos con el foco en el explorador:

- **F2:** renombrar.
- **Supr:** enviar a la papelera, con confirmación.
- **Ctrl+C / Ctrl+X / Ctrl+V:** copiar, cortar y pegar archivos o carpetas.
- **Ctrl+Shift+N:** nueva carpeta.
- **Alt+Enter:** propiedades.

La barra superior del lateral reemplaza el botón Abrir carpeta; esa acción permanece en **Archivo → Abrir carpeta…** (Ctrl+Shift+O). **Ctrl+Shift+F** enfoca la búsqueda del proyecto. Al escribir muestra resultados y habilita filtros **Todo / Archivos / Carpetas / Código**, el botón **Aa** para distinguir mayúsculas y **↻** para actualizar. Busca texto literal en nombres/rutas de archivos `.cls`, carpetas y contenido `.cls`. Incluye cambios sin guardar de documentos con ruta dentro del proyecto; los documentos sin nombre no se incluyen. La búsqueda del documento actual sigue en Ctrl+F.

Doble clic o Enter sobre un resultado abre el archivo. Las coincidencias de código seleccionan el texto en su línea/columna; si esa línea cambió, se actualizan los resultados. Abrir una carpeta encontrada vuelve al árbol y la despliega. Flecha abajo desde la barra enfoca el primer resultado; **Esc** o limpiar el campo vuelve al explorador. Enfocar explorador y Revelar archivo activo también limpian la consulta para mostrar el árbol.

La búsqueda se realiza en segundo plano con espera breve al escribir y cancelación de solicitudes antiguas. Ignora enlaces simbólicos y carpetas de dependencias/compilación excluidas. Se limita a 500 coincidencias, 20000 entradas recorridas, 2 MB por archivo y 20 MB de contenido, con aviso si hay omisiones, límites o archivos ilegibles. **↻** permite volver a consultar cambios externos, incluso en subcarpetas todavía no cargadas por el árbol.

Ver [detalles y validación de proyecto](docs/proyecto_archivos.md). Suite combinada: **312 pruebas correctas**, incluidas 26 de preferencias/política de asistencia, 15 de reparaciones, 15 de carpetas, 12 de selección/arrastre, 10 de orden/presentación, 14 de búsqueda, 13 de ejecución, 23 de contratos/ediciones y 26 de coordinación/UI de asistencia.

## Preparación para los motores de asistencia

El [contrato compartido](docs/contrato_asistencia.md) fija tipos y firmas para sugerencias/reparaciones en `ide/nucleo/asistencia.py`, sin Qt. Los compañeros deben importarlos desde ahí. Las sugerencias ya están implementadas en C y conectadas mediante `ide/sugerencias_ccom.py`; reparaciones M4 también usa su proveedor C.

El aplicador ya está disponible mediante `editor.contexto_asistencia()` y `editor.aplicar_ediciones(...)`, además de `aplicar_sugerencia` / `aplicar_reparacion` en `ide/interfaz/ediciones.py`. Comprueba todo antes de modificar, convierte rangos Python a UTF-16 y conserva el historial con una operación de Deshacer por propuesta. No guarda el documento ni activa correcciones automáticas. Los resultados de documento, revisión, texto, cursor o selección anteriores se rechazan.

Los pasos **1–6** están preparados: contrato, aplicador, coordinador, ambas UI manuales y preferencias/política de asistencia. **Ctrl+Espacio dentro del editor**, **clic derecho → Sugerencias de código** o Editar → Sugerencias de código solicita opciones. El clic derecho consulta en la posición pulsada y conserva las operaciones estándar de edición. La lista muestra etiqueta/categoría/detalle; flechas y RePág/AvPág recorren, Enter/Tab o clic aceptan, y Esc cancela sin modificar. Escribir cierra la lista y deja pasar la tecla; puede volver a consultar por pausa si se habilita en preferencias. El foco permanece en el editor y las respuestas se descartan al editar, mover cursor/selección, cambiar pestaña o cerrar.

En uso normal no se muestran datos inventados: si falta el motor aparece el mensaje correspondiente en la barra de estado. El coordinador carga `ide/sugerencias_ccom.py:sugerir` en segundo plano, capturando el ccom configurado. Reparaciones carga `ide/reparaciones_ccom.py:proponer_reparaciones` del mismo modo. Sus respuestas pasan validación; los errores de importación/contrato se distinguen de la ausencia de opciones.

El coordinador serializa ambos motores, limita texto/cola/opciones y solicita cancelación después de 3 segundos. Un motor debe atender el callback: no se destruyen hilos por la fuerza ni se abre otro mientras el anterior sigue ejecutándose. El cierre espera hasta 1500 ms y se aplaza si aún trabaja. Las consultas manuales de reparaciones consumen sus respuestas sin aplicarlas automáticamente.

**Alt+Enter dentro del editor** o Editar → Reparaciones de código abre la pestaña **Reparaciones**. También permite Buscar reparaciones desde el panel. Selecciona una propuesta y revisa **Cambios** (texto anterior/posterior por edición) y **Motivo** (razones/validación del motor). **Aplicar reparación** modifica solo el documento vigente, sin guardar, con una operación de Ctrl+Z; **Cancelar/Escape** conserva texto e historial. Consultar o seleccionar no aplica cambios, incluso si el motor marca una propuesta como candidata automática.

Puedes leer/copiar la vista previa sin invalidarla. Editar el código, mover su cursor/selección o cambiar/cerrar pestaña deshabilita la aplicación de propuestas antiguas. El panel se adapta abajo/derecha. Las propuestas con más de 12000 caracteres afectados no se aplican desde esta vista: el cambio completo debe poder revisarse. El panel no añade diagnósticos nuevos a Problemas ni altera sus prioridades.

Sigue el **paso 7: integración real de los motores y revisión del catálogo**. Sin el módulo real se muestra que no está disponible; las propuestas controladas solo se usan en pruebas. Verificación actual: **312 pruebas correctas**, incluidas 26 nuevas de preferencias/política, 15 del panel de reparaciones, 26 de coordinador/UI y las 23 anteriores de contratos/ediciones.


## Preferencias de asistencia (paso 6)

**Editar → Preferencias de asistencia…** permite desactivar toda la asistencia, mostrar sugerencias por pausa y ajustar la espera entre 300 y 3000 ms (700 ms por defecto). Los ajustes se guardan en tu perfil y se aplican sin reiniciar. La activación por pausa está desactivada inicialmente; consulta opciones, pero no las acepta sola. Es independiente del análisis automático de ccom.

La corrección automática también está desactivada por defecto. Requiere reglas revisadas y seleccionadas; la marca `automatica_segura` del motor no basta. Su disparador inicial es Enter al final de una línea y una pausa, con una sola inserción local de delimitador, precondiciones y aceptación de todo el texto corregido por el parser en segundo plano. Ctrl+Z revierte la reparación, no guarda ni encadena otra corrección. Las propuestas que no cumplen la política pueden revisarse manualmente.

El catálogo M4 contiene una regla revisada de punto y coma. Para usarla, habilita corrección automática y selecciónala en preferencias; ambos permisos vienen desactivados por defecto. No hay motores ni reglas simuladas en la aplicación normal. Ver [política, límites, cancelación y conexión del catálogo](docs/politica_asistencia.md).


## Servicio estructurado del compilador · M1

El análisis automático obtiene categorías, mensajes y rangos desde `ccom --check-json`; no interpreta frases de stderr para construir esos problemas. La salida del panel sigue siendo legible y las fuentes sin guardar se analizan sobre copias temporales. Una versión antigua/incompatible del compilador produce una explicación, nunca un falso «sin errores». Reconstruir con `make` después de actualizar.

`--check` clásico y F5/F6 conservan su funcionamiento. El lexer en vivo y el LL(1) Python no se retiran en M1. El servicio transporta listas de errores léxicos/semánticos; la recuperación de varios errores sintácticos sigue pendiente.

Protocolo y arquitectura: [ccom_check_json.md](../docs/ccom_check_json.md). Prueba específica: `make check-analysis-json`. Verificación de M1: 18 pruebas nuevas y 312 combinadas del IDE, suite completa del compilador correcta y 91 fuentes reales con respuestas compatibles.

## Léxico y resaltado M2

El editor envía una copia temporal del buffer a `ccom --tokens-json`; C clasifica tokens/comentarios y decide los errores. Python valida el JSON y adapta bytes UTF-8 a posiciones UTF-16 para pintar. No se guardan buffers ni se altera Deshacer. El proceso es cancelable y tiene límites de 5 s, 2 millones de caracteres y tamaño de respuesta. Los errores visibles tienen origen **ccom léxico**. Ver [contrato y arquitectura](../docs/ccom_tokens_json.md).

## Sugerencias C · M3

Escribe `F main [] : vo` y pulsa **Ctrl+Espacio**: ofrece `void`; **Tab/Enter** acepta y **Ctrl+Z** deshace, sin guardar. Flechas/clic seleccionan y Escape cancela. Para consultar al escribir, habilita **Editar → Preferencias de asistencia → sugerencias por pausa**. La pausa no acepta propuestas automáticamente.

C obtiene fragmentos/comentarios de Flex y opciones válidas de Bison/LAC. Python solo valida el JSON y adapta posiciones para el aplicador. No se inventan identificadores ni números; el motor sugiere terminales concretos y no garantiza semántica. No propone dentro de comentarios ni cuando un error anterior bloquea el contexto. Ver [protocolo, arquitectura y límites](../docs/ccom_suggestions_json.md). Reparaciones M4 y su regla opcional de punto y coma también están conectadas a C.

## Reparaciones C · M4

Introduce un programa con `Vm x : int = 3` sin su punto y coma y pulsa **Alt+Enter** o **clic derecho → Reparaciones de código**. El panel ofrece una inserción validada por ccom; revisa Cambios/Motivo y pulsa Aplicar reparación. **Ctrl+Z** deshace y el archivo permanece sin guardar. El motor inicial propone únicamente cierres `; . ] )`, sin inventar operandos ni corregir semántica.

La automatización viene apagada. **Editar → Preferencias de asistencia** permite habilitarla y seleccionar la regla revisada INSERCION_PUNTO_COMA; exige confirmar una línea y que el parser C acepte el documento corregido completo. Nunca se encadenan arreglos ni se guarda automáticamente. [Arquitectura, protocolo y límites M4](../docs/ccom_repairs_json.md).


## Clones C · M5

Abre `ide/ejemplos/clones`, entra a **Clones** y pulsa **Analizar carpeta** con mínimo 20. Se muestran tres pares (I, II, II). Editar un buffer y volver a analizar utiliza ese texto sin guardarlo; seleccionar un par permite comparar y abrir sus rangos. El compilador configurado en la ventana también se usa para clones.

M5 elimina el regex y la comparación Python: `ide/clones.py` solo mantiene modelos, envío de instantáneas CCL1 y adaptación del JSON/rangos. El motor `src/clones.c` reutiliza Flex y conserva límites, cancelación y renombramientos consistentes. [Arquitectura y protocolo completos](../docs/ccom_clones_json.md).


## Revisión y comprensión · M6

La [revisión M6](docs/revision_m6.md) explica la frontera C/Python, qué código se conserva, memoria, posiciones y el orden para leer los archivos. El LL(1) permanece para estudio/API/pruebas; la interfaz no lo carga como backend. El contrato de modelos `nucleo/asistencia.py` sí es productivo. Se unificó la cabecera de los servicios JSON y se comentaron los servicios/adaptadores migrados.

`make check-recorrido-ide` prueba varios documentos, colores/diagnósticos, clones, sugerencias, reparación/Deshacer y F5/F6 sin permitir importar los analizadores Python. Usa proyectos temporales y C real. Para revisión visual en tu pantalla, abre con `make ide` y repite ese orden. Última verificación global: **382 pruebas IDE** y suite completa de ccom correctas.
