# CALSIDE · prototipo inicial

Aplicación de escritorio PySide6. Conserva los controles nativos para mover, minimizar, maximizar y cerrar. La esquina morada es estática; la animación de papel queda para una iteración posterior.

Desde la raíz del repositorio, con Python 3.10 o posterior:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make ide PYTHON=.venv/bin/python
```

Si PySide6 ya está instalado, basta con `make ide`. También se puede abrir un programa mediante `python3 -m ide.main pruebas/p02_factorial.cls`. Python 3.14.9 y PySide6 6.12.0 son la combinación verificada inicialmente; probar las demás versiones antes de la defensa. El empaquetado como ejecutable todavía está pendiente.

## Disponible

- Distribución clásica oscura con ámbar Calser y esquina morada.
- Editor, números de línea, pestañas, indicador de cambios, estado de cursor y búsqueda.
- Abrir archivo/carpeta, nuevo, guardar, guardar como y cierre con guardar/descartar/cancelar.
- Explorador de `.cls` de la carpeta actual (sin subcarpetas en este prototipo).
- Resaltado visual basado en los patrones de la extensión VS Code, con comentarios multilínea. No valida el programa.
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

Los diagnósticos se actualizan **al compilar**. El análisis automático durante la escritura corresponde al paso 3 y todavía no está implementado.

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

Integración del núcleo LL(1), sugerencias, correcciones automáticas, detección de cambios externos, preferencias persistentes, árbol recursivo, iconos de archivos y empaquetado. El análisis automático de errores mientras escribes sigue pendiente. No hay resultados simulados.

El resaltado reutiliza los patrones de `editors/vscode/syntaxes/calser.tmLanguage.json`; no copia el tema Seti. El icono de aplicación usa el recurso Calser existente.

## Validación

```sh
make check-ide
```

Comprobación manual: abrir `pruebas/p02_factorial.cls`, compilar y ver el resultado; abrir `pruebas/e01_no_declarado.cls`, compilar y activar el error; editar y verificar el indicador de cambios; cancelar el cierre para conservar el texto. Verificar varias pestañas, búsqueda y redimensionamiento de paneles.

Plan completo y alternativas visuales: [plan](docs/plan_ide.md), [mockups](docs/mockups.html).
