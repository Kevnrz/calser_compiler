# E7: archivos externos, proyecto y preferencias

Implementado en `feat/calside`, 2026-11-05. Independiente de los motores de sugerencias y reparaciones. El parser se conserva sin cambios.

## Archivos que cambian fuera de CALSIDE

Cada documento abierto registra la versión leída: dispositivo/inodo, tamaño, fecha en nanosegundos y SHA-256 de los bytes. La lectura comprueba que el archivo no fue reemplazado durante la operación. La huella detecta cambios incluso si otro editor conserva la fecha y el tamaño.

Un trabajador comprueba las fuentes abiertas cada 1,5 segundos. También se solicita una comprobación al volver a la ventana. Solo hay un sondeo activo; el hash y el recorrido de archivos no se ejecutan en el hilo gráfico. Los resultados llevan documento, ruta y versión base; se ignoran después de guardar, recargar, Guardar como o cerrar la pestaña.

Un cambio, borrado o error de lectura produce un aviso para la pestaña activa y un indicador `⚠` en su título. **No se recarga ni se descarta el texto automáticamente.**

- **Recargar:** lee el disco. Si hay ediciones locales, pide confirmación antes de descartarlas. Un fallo de lectura/UTF-8 conserva íntegramente el editor. Atajo: **Ctrl+Shift+R**.
- **Conservar:** mantiene el buffer y lo marca como pendiente de guardar. Oculta ese aviso concreto, pero no adopta la versión ajena como base ni autoriza sobrescribirla. Otro cambio externo vuelve a avisar.
- **Guardar como:** permite guardar una copia sin reemplazar el archivo original. Un destino existente exige confirmación; un archivo abierto en otra pestaña no puede usarse como destino.

Al guardar, se comprueba el disco aunque el sondeo aún no haya detectado el cambio. Si difiere de la versión leída, se ofrece sobrescribir/recrear explícitamente, recargar, guardar una copia o cancelar. La opción por defecto y Escape cancelan. Si el archivo cambia otra vez durante la decisión o la preparación de la escritura, se cancela el guardado conservando las dos versiones.

Si una fuente abierta fue sustituida por un enlace simbólico, el guardado condicionado se rechaza y pide Guardar como, para no sobrescribir otro destino indirectamente.

El guardado prepara un temporal en el mismo directorio, vacía sus datos, conserva los permisos de un destino existente, comprueba de nuevo la versión y hace un reemplazo atómico. Ante fallo conserva el archivo anterior y elimina el temporal. La versión registrada al guardar corresponde a lo escrito por CALSIDE; un escritor inmediatamente posterior se detectará como cambio externo.

**Límite del sistema de archivos:** la comprobación y el reemplazo son operaciones distintas; no hay un bloqueo exclusivo o compare-and-swap portable frente a todos los programas externos. Existe una pequeña ventana entre la comprobación final y el reemplazo. No se promete exclusión mutua con otro editor que escriba simultáneamente.

F5 también comprueba que disco y editor correspondan a la misma versión antes de compilar, incluso si el documento no tiene cambios locales. Una compilación cuyo archivo cambió durante el proceso no publica diagnósticos como si fueran actuales. El análisis automático de E4 continúa trabajando sobre el buffer del editor.

## Explorador de proyecto

`Explorador` usa `QFileSystemModel` y un filtro: carga directorios mediante el modelo asíncrono de Qt y expande subcarpetas según se necesitan. No hace un recorrido recursivo completo desde Python. Muestra carpetas y archivos `.cls`; usa iconos nativos de archivos/carpetas.

Se excluyen `.git`, `.venv`, `venv`, `__pycache__`, `build`, `dist`, `node_modules`, `.pytest_cache`, `.mypy_cache`, `.ruff_cache`, `.idea` y `.tox`. No sigue ni muestra enlaces simbólicos. Una carpeta que el usuario abrió explícitamente puede servir como raíz aunque su nombre esté excluido; también se permite llegar a una raíz dentro de un ancestro excluido.

El modelo detecta archivos nuevos, borrados y cambios de directorio. Abrir o cambiar de proyecto no cierra las pestañas existentes. Los errores de acceso de la raíz se muestran sin cambiar el proyecto; un fallo de acceso al expandir una carpeta se informa en la barra de estado.

- **Ctrl+B:** mostrar/ocultar explorador.
- **Ctrl+Shift+E:** mostrarlo y darle foco.
- Flechas: navegar y expandir; **Enter** o doble clic sobre archivo: abrirlo.
- **Ctrl+Alt+J:** alternar resultados abajo/derecha.

## Preferencias

Se guardan al cerrar normalmente mediante `QSettings("Calser", "CALSIDE")`, bajo la configuración del usuario. En Linux normalmente corresponde a `~/.config/Calser/CALSIDE.conf`, respetando `XDG_CONFIG_HOME`. No se escriben dentro del repositorio ni hay que agregarlas a `.gitignore`.

Se recuerdan carpeta de proyecto, ruta de ccom, familia/tamaño de fuente, tamaño de ventana, visibilidad del explorador y resultados, orientación/tamaños de paneles y activación del análisis automático.

**Ver → Fuente del editor…** cambia la fuente de las pestañas actuales y futuras sin modificar su texto ni Deshacer. Se admiten tamaños de 6 a 40 puntos. El tema oscuro actual se mantiene; selector de temas y tiempo de compilación configurable siguen fuera de esta entrega.

El JSON de configuración tiene versión y validación de campos/rangos. Valores inválidos usan los predeterminados; una carpeta desaparecida vuelve a la raíz del proyecto y una ruta inválida de ccom vuelve a su búsqueda habitual. No se restauran buffers, pestañas de documentos, terminales ni procesos. Las pruebas usan configuración temporal o `Ventana(root, preferencias=False)` y no alteran las preferencias del usuario.

## Organización de código

- `ide/archivos.py`: estados de disco, lectura consistente y escritura condicionada/atómica.
- `ide/preferencias.py`: validación y persistencia de configuración.
- `ide/interfaz/disco.py`: trabajador y temporizador de sondeo.
- `ide/interfaz/explorador.py`: modelo de archivos y exclusiones.
- `ide/interfaz/proyecto.py`: avisos, decisiones, recarga, fuente y preferencias de la ventana.
- `ide/interfaz/ventana.py`: integración de esas operaciones con edición, procesos y cierre.

## Validación

```bash
make check-ide
```

Resultado inicial de E7: **124 pruebas combinadas correctas**, incluidas 28 nuevas de esa etapa; la revisión de carpetas posterior se detalla al final. Se cubren fechas/tamaños iguales, reemplazo/borrado, cambios durante guardado, recuperación tras errores de lectura, Guardar como, resultados tardíos, F5 con disco ajeno, cierre con sondeo activo, fuentes, exclusiones/enlaces, archivos nuevos, preferencias dañadas y atajos reales.

Revisión visual local: 1250×800 y mínimo 800×520, con escala normal y 150 %. Conserva los botones del aviso y la edición; los paneles se pueden redimensionar. La comprobación en otra máquina y el equipo de defensa permanece en E8/E9.

Prueba manual reproducible:

1. Abrir una carpeta con una subcarpeta y un `.cls`. Navegar hasta el archivo y abrirlo.
2. Editarlo en CALSIDE y cambiarlo desde otro editor. Esperar el aviso; comprobar que el buffer local no cambió.
3. Pulsar Conservar y luego Guardar: debe pedir resolver el conflicto. Cancelar debe conservar ambas versiones.
4. Guardar como copia; comprobar que el archivo modificado externamente conserva su contenido.
5. Volver a abrirlo, modificarlo externamente y Recargar. Con cambios locales, comprobar primero la cancelación de la confirmación.
6. Borrar un archivo abierto: conservar el buffer, guardar una copia o recrearlo solo con confirmación.
7. Cambiar fuente, mover paneles, ocultar explorador y cerrar/reabrir normalmente. Comprobar preferencias y navegación con teclado.

## Revisión manual: archivos y carpetas (2026-11-06)

El menú contextual incorpora creación de archivos/carpetas, renombrar, papelera, copiar/cortar/pegar y propiedades, además de copiar ubicación y abrir el gestor del sistema. Al seleccionar un archivo, crear o pegar usa su carpeta contenedora; sobre el fondo se usa la raíz del proyecto. Los nombres no admiten rutas ni saltos de línea. Crear nunca reemplaza una entrada existente; los archivos nuevos sin extensión reciben `.cls`. El filtro del árbol continúa mostrando `.cls` y carpetas.

Las carpetas ofrecen Expandir/Contraer rama y Expandir/Contraer todo. Las expansiones completas se procesan por tandas y esperan la carga asíncrona de Qt. Hay un límite explícito de 1000 carpetas expandidas, con aviso si se alcanza. Se recuerdan ramas abiertas al cambiar de proyecto y al cerrar/reabrir normalmente; la configuración guarda rutas relativas y respeta su límite total de tamaño.

Atajos del explorador: F2 renombra, Supr envía a la papelera, Ctrl+C/X/V opera sobre archivos/carpetas, Ctrl+Shift+N crea una carpeta y Alt+Enter muestra propiedades. Los atajos de texto continúan actuando en el editor y las consolas cuando tienen foco. Ver → Revelar archivo activo selecciona el archivo si está dentro del proyecto actual.

Al mover o renombrar una carpeta se actualizan todas las rutas de documentos abiertos que contiene, conservando texto, cursor, Deshacer y estado de modificación. Se invalidan diagnósticos/clones anteriores y se reinicia el análisis. Los conflictos externos previos se mantienen. Los sondeos de la ruta antigua se descartan. No se permite mover/eliminar la raíz del proyecto, sus ancestros ni una carpeta que contenga la fuente de una compilación activa.

Las operaciones de carpetas se ejecutan en `ide/interfaz/operaciones_carpetas.py`, fuera del hilo gráfico. La copia incluye el contenido completo guardado en disco, incluidos archivos ocultos y otras extensiones; conserva enlaces simbólicos sin recorrerlos y rechaza entradas especiales. El origen y el destino deben ser diferentes y el destino no puede ser descendiente del origen. No se fusionan ni sobrescriben destinos; ante colisión individual se pide otro nombre; en selección múltiple se rechaza el lote antes de iniciarlo. El traslado de carpetas se admite dentro del mismo sistema de archivos; mover entre discos requiere el gestor del sistema (copiar sí está disponible).

Copiar admite cancelación cooperativa por bloques/entradas desde Archivo o el menú contextual. Un error o cancelación conserva el origen e informa de un destino parcial si llegó a crearlo. No elimina ese parcial automáticamente porque podría contener cambios de otros programas. La copia no constituye una instantánea de escritores externos concurrentes.

Hay una operación de carpeta a la vez. El editor permanece utilizable; las rutas involucradas no se guardan, recargan, abren o compilan durante la operación. Guardar como fuera de ellas sigue permitido. El cierre espera la operación para no descartar la actualización de rutas y buffers; las copias se pueden cancelar. Papelera requiere confirmación y no tiene alternativa de borrado permanente. Los documentos de una carpeta eliminada conservan su texto y quedan pendientes de guardar.

Propiedades muestra ubicación, tipo, tamaño del archivo, modificación y permisos. Las carpetas no se recorren para calcular tamaño. Abrir ubicación usa el gestor predeterminado del escritorio. El portapapeles admite varias entradas locales y conserva marcadores de copiar/cortar compatibles con gestores de escritorio; Cortar solo retira el origen cuando se completa el traslado.

Selección y arrastre: Ctrl+clic añade/quita, Shift+clic selecciona rangos, Ctrl+A selecciona todo y el arrastre desde espacio vacío dibuja un rectángulo. Se pueden mover varias entradas hacia una carpeta o al fondo (raíz). El arrastre solo admite el propio explorador como origen. El menú conserva la selección sobre el clic derecho y permite copiar/cortar/papelera en grupo; abrir aplica a los archivos, y renombrar/propiedades siguen siendo individuales. `ide/interfaz/seleccion_archivos.py` normaliza las raíces seleccionadas y coordina los lotes. Los destinos se comprueban antes de iniciar, sin fusionar ni sobrescribir; los trabajos se procesan secuencialmente. Un fallo posterior detiene los pendientes, conserva los completados e informa su cantidad. Cancelar la copia de una carpeta también detiene el resto del lote. Cortar solo limpia el portapapeles si todo terminó correctamente y el contenido sigue coincidiendo.

Presentación y orden: `ide/interfaz/orden_explorador.py` dibuja la tarjeta de arrastre con alfa y los nombres de hasta tres entradas; si hay más, añade su recuento. El centro de carpeta indica un traslado a disco con recuadro y apertura demorada; sus bordes indican inserción visual con línea. Solo se reordenan hermanos; las selecciones de varios padres no se reordenan. El filtro compara primero las posiciones manuales y después aplica su comparación habitual a entradas nuevas. El orden usa rutas relativas y se guarda en preferencias del último proyecto, con caché de sesión para los demás. Renombrar carpetas remapea sus órdenes y los de sus descendientes. El menú de la carpeta contenedora permite volver al orden alfabético. La configuración limita el orden a 200 directorios, 1000 nombres por directorio y 3000 nombres en total; orden y ramas se ajustan al límite global de 32768 caracteres sin perder las preferencias básicas.

Búsqueda del proyecto: `ide/busqueda.py` implementa una búsqueda literal independiente de Qt; `ide/interfaz/busqueda.py` coordina una tarea activa y la consulta más reciente. El campo sustituye el botón lateral Abrir carpeta, que permanece en Archivo. Ctrl+Shift+F enfoca el campo; Todo/Archivos/Carpetas/Código filtra resultados y Aa controla mayúsculas. La espera al escribir es de 240 ms. Los resultados se agrupan por tipo y archivo, con línea/columna para el código; abrir una carpeta restaura el árbol. Esc o limpiar el campo cancela y vuelve al explorador; Ctrl+F conserva la búsqueda del documento actual.

Se buscan nombres/rutas `.cls`, carpetas y texto `.cls` de todo el proyecto. Los buffers con ruta tienen prioridad sobre el disco, incluso si el archivo ya no existe; los documentos sin nombre y los de otros proyectos se excluyen. Editar, guardar, cerrar pestañas y completar operaciones de archivos actualiza la consulta activa. Las señales del modelo se atienden por cola, fuera de sus cambios estructurales. El botón ↻ permite actualizar cambios externos no observados en subcarpetas descargadas. Las consultas tienen generación: respuestas antiguas o de otro proyecto se descartan. El cierre cancela la tarea y espera de forma acotada. Al navegar se valida la línea encontrada y se convierte la selección a posiciones UTF-16 de Qt.

Límites de búsqueda: 500 coincidencias, 20000 entradas recorridas, 2 MB por archivo y 20 MB acumulados de contenido; se avisa de resultados incompletos, contenido ilegible o límites. La búsqueda no sigue enlaces, excluye los mismos nombres de dependencias/compilación que el explorador y lee por bloques cancelables. Es una lectura del estado actual, no una instantánea de escritores externos.

Ejecución: `ide/interfaz/ejecucion.py` registra compilaciones correctas vigentes por pestaña durante la sesión. F5 compila sin ejecutar; F6 verifica fuente, revisión, estado de guardado y huellas `.asm`/`.mem` antes de lanzar el simulador mediante QProcess y el Python actual. No guarda ni recompila implícitamente. Cada corrida copia y comprueba sus entradas en un subdirectorio único y conserva su traza. La salida tiene pestaña independiente, decodificación incremental, límites de vista, clasificación de avisos/errores y cancelación/timeout. Editar durante la corrida es posible y se indica que el resultado pertenece a una versión anterior. Compilar/ejecutar no se superponen; mover/eliminar el fuente o cerrar su pestaña/ventana requiere detener la ejecución.

Verificación: **204 pruebas combinadas**, incluidas 10 del menú de archivos, 15 de carpetas, 12 de selección/arrastre, 10 de orden/presentación, 14 de búsqueda y 13 de ejecución. Se cubren traslados con varias pestañas, contenido completo y enlaces, cancelación y parciales, raíz/compilación protegidas, conflictos externos, árbol lazy, preferencias y atajos. La papelera de las pruebas se simula en un temporal sin afectar archivos del usuario. La búsqueda está conectada y verificada, incluyendo buffers, Unicode, resultados obsoletos y cancelación.
