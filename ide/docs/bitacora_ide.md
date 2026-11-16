# Bitácora de CALSIDE

Registro consolidado · 2026-11-11. Este archivo guarda decisiones, entregas, verificaciones y documentos sustituidos. **Las secciones históricas pueden describir estados anteriores; no son instrucciones vigentes ni pendientes actuales.** Para trabajar usar [plan actual](plan_ide.md), [guía del sistema](guia_sistema_calside.md) y [revisión manual](revision_manual.md).

## Fondo hexagonal redondeado para logo_programa — 2026-11-11

El fondo cuadrado se sustituye por un hexágono oscuro #191c24 con seis
vértices redondeados. Solo el exterior queda transparente, para que el icono
tenga esa silueta; el fondo y centro siguen opacos. El emblema segmentado se
conserva idéntico al de inicio, incluidos el segundo fragmento morado y sus
medidas. Se actualizan `assets/branding/logo_programa.svg`, su PNG 512×512 y
la descripción en `ide/README.md`; Qt y el escritorio ya usan esas rutas.

Verificación puntual: SVG válido, emblema vectorial idéntico al anterior,
exterior transparente, centro opaco #191c24 e inspección visual correcta.
Sin cambios ejecutables, suite completa ni commits.

## Icono propio logo_programa — 2026-11-11

El icono de CALSIDE pasa a `assets/branding/logo_programa.png` (512×512,
sin canal alfa, fondo sólido #191c24). Se extraen directamente del SVG de
inicio los seis fragmentos del hexágono, conservando geometría, grosores y
segundo fragmento morado. La fuente editable se guarda en `logo_programa.svg`.
Se actualizan `ide/main.py`, README y la entrada local `calside.desktop`;
los logos anteriores y los iconos de archivos se conservan.

Inspección visual correcta; comprobación puntual de dimensiones, opacidad,
fondo y herencia del icono Qt correcta. Entrada de escritorio validada y base
actualizada; `git diff --check` sin errores. Reiniciar CALSIDE para aplicar a
las nuevas ventanas. Sin suite completa ni commits.

## Icono de aplicación y reconocimiento en Wayland — 2026-11-11

Se usa `assets/branding/calser-logo.png` como icono global de CALSIDE en
`ide/main.py`; los archivos `.cls` conservan su icono anterior. Se fija
`desktopFileName=calside` y se registra en este equipo
`~/.local/share/applications/calside.desktop`, con Icon/Path dirigidos al
repositorio y Exec al Python actual. Esto permite que Hyprland/Wayland resuelva
el icono del selector de ventanas y ofrece una entrada en el lanzador. El
registro es local de desarrollo, no el empaquetado E8; si cambian las rutas hay
que actualizarlas. Se documenta en `ide/README.md`.

Verificaciones puntuales: `desktop-file-validate` sin errores, base de
escritorio actualizada, rutas existentes y punto de entrada Qt sin pantalla
con identidad correcta e icono heredado por un QWidget idéntico al PNG
solicitado. `git diff --check` correcto. Hace falta reiniciar CALSIDE para que
las nuevas ventanas anuncien la identidad. Sin suite completa ni commits;
se conservan los cambios locales de otras tareas.

## Punto de cierre en línea posterior — 2026-11-11

A petición del usuario, C solo prioriza el punto después de `E;` si el cursor está en una línea posterior. Se conserva la posición del terminador reconocido por Flex y se comprueba el salto de línea del intervalo; no se interpreta Calser en Python. Enter muestra el punto gris en la nueva línea y Tab lo acepta. No se inserta automáticamente ni se recomienda un nuevo F.

Se reconstruye ccom y se ejecuta únicamente el recorrido de `E;` sin punto gris en la misma línea → Enter con punto gris → Tab y Deshacer, tanto con corrección automática activada como desactivada. Una prueba focalizada correcta; sin suite completa. Repetición humana pendiente.

## Diagnóstico de llamada y cierre después de E; — 2026-11-11

El usuario muestra `suma(30,10)` diagnosticado como falta de `;` y un cierre sin `.` después de `E;`. La llamada es inválida en Calser, que usa `suma[30,10]`, pero el mensaje era engañoso: Bison priorizaba `;` aunque LAC también esperaba `[`. Se añade un diagnóstico C específico para `(` después de identificador con `[` esperado, conservando la ubicación del paréntesis. No se cambia la gramática ni se reescribe código del usuario.

La preferencia nativa de punto pasa a ser contextual: después de los tokens explícitos `E;` puede mostrar `.` si LAC lo permite y no existe ya a la derecha. No se ofrece tras la cabecera vacía ni se encadena `F`. Enter puede consultar continuaciones; cuando la corrección con Enter es optativa y no aplica, también se permite mostrar la continuación sin aplicarla sola.

Tres comprobaciones focalizadas correctas: dos funciones con punto omitido tras `E;`, tanto con corrección automática activada como desactivada; mensaje/rango correcto de llamada con paréntesis y aceptación de la variante con corchetes; conservación de la parada después de la cabecera. ccom reconstruido correctamente; sin suite completa. Repetición humana pendiente.

## Intención de las propuestas y cabecera compacta de main — 2026-11-11

El usuario reporta bloqueo de tipos después de `:` y cadena automática `; → . → F`. Se corrige el criterio del motor C: tipos y puntuación local tienen preferencia, pero un cierre/inicio de función no adquiere preferencia por ser la única opción gramatical. Una lista válida sin opción preferida tampoco deriva a reparaciones, que antes podían ofrecer el punto para cerrar un cuerpo vacío.

Bison publica el contexto del nombre de función durante las consultas. C propone `[]: ` con cursor fuera para main, que semánticamente no recibe parámetros y retorna void; las funciones normales conservan el par editable y los tipos de retorno. Los tokens reales delimitan la declaración de main para no transformar corchetes de lazos, condiciones o llamadas de su cuerpo; las declaraciones locales conservan sus tipos. El caso de dos puntos ya existentes evita duplicarlos.

Validación focalizada: cuatro recorridos UI correctos (cabecera de main sin . ni F posteriores, tipos genéricos, salida de par genérico y terminador local), contexto nativo del cuerpo y repetición del flujo de main correctos; `ccom --check pruebas/p02_factorial.cls` correcto y reconstrucción de Flex/Bison/ccom sin errores. Sin suite completa. Repetición humana pendiente.

## Continuidad tras aceptar corchetes — 2026-11-11

El usuario observa que después de aceptar `[]` el cursor queda dentro y no aparece `:`. La zona interior corresponde a parámetros; el controlador no consultaba tras aceptar una propuesta ni al salir de esa zona sin escribir. Se añade seguimiento del snapshot y del final de la plantilla recibida de C: segundo Tab sale sin editar, la flecha derecha también rearma la consulta y ccom ofrece la continuación después del par. Aceptar explícitamente otras propuestas consulta su siguiente continuación (por ejemplo, `void` seguido de `;`), sin insertar más texto automáticamente.

El salto de plantilla se invalida con cambios de texto/cursor/selección, foco, pestaña o preferencias. No se añaden analizadores Python ni se encadenan reparaciones automáticas de Enter. Dos recorridos con ccom real correctos: cadena `main → [] → salir → : → void → ;` con Deshacer y salida por flecha/cancelación al editar. Sin suite completa; repetición humana pendiente.

## Continuaciones útiles: main y líneas vacías — 2026-11-11

El usuario observa una `C` arbitraria en la línea vacía del cuerpo y ausencia de `[]` después de `main`. La primera era la primera opción alfabética entre muchas continuaciones válidas; la segunda ocurría porque el motor seguía tratando el identificador completo como prefijo de una reservada. Se corrige en C: si no hay completación y el token terminó en el cursor, se consulta LAC después del token. Un `[` inequívoco puede presentarse como par `[]` con cursor dentro, sin duplicar puntuación existente.

El protocolo añade campos opcionales `inline_preferred` y `cursor_relative`, validados y convertidos por el adaptador Python. La vista gris solo presenta propuestas preferidas por C (prefijo escrito o continuación única); Ctrl+Espacio conserva la lista completa. Un resultado sin preferencia puede dar paso a la consulta de reparación local, conservando el `;` propuesto en una declaración. No se inventan identificadores ni cuerpos de funciones.

Compilación de ccom correcta y **4 comprobaciones focalizadas OK**: `F main`/Tab/posición/Deshacer, línea vacía sin `C` conservando lista y prefijos, reparación local de `;` y reemplazo Unicode dentro de palabra. Sin suite completa. Pendiente repetición humana.

## Letras un 10 % más lentas — 2026-11-11

Se reduce la velocidad del trazado al 90 %: la fase de letras pasa de 1800 a
2000 ms (1800/0,9). Se conservan las otras fases; la secuencia total dura ahora
4,4 s. La velocidad angular permanece en unos 43°/s y se ajusta el ángulo
inicial para terminar a 0° sin salto. Cambios en `ide/interfaz/inicio.py`,
la prueba del giro en el tramo final y los tiempos de README/guía/plan.
Verificación focalizada: **10 pruebas de arranque OK** y `git diff --check`
sin errores. Sin suite completa ni commits.

## Giro lento y continuo durante toda la presentación — 2026-11-11

A petición del usuario, se sustituye el giro rápido limitado al desplazamiento
por media vuelta a velocidad constante durante los 4,2 s completos (~43°/s).
Empieza mientras se dibujan los fragmentos y continúa durante el desplazamiento,
las letras y los 300 ms finales. Parte de −180° y llega a 0° para conservar la
composición aprobada al finalizar, sin recolocación brusca. Clic/Esc y la
preferencia conservan su funcionamiento; la duración total no cambia.

Se ajustan `ide/interfaz/inicio.py`, las pruebas de `ide/pruebas/test_inicio.py`
y la explicación en README/guía del sistema. Verificación focalizada:
**10 pruebas de arranque OK**, incluyendo giro durante letras y tramo final,
colores y transición al SVG estático en escalas 1×/2×. Vista previa regenerada;
`git diff --check` sin errores. Sin suite completa, commits ni cambios de E8.

## Animación con marca geométrica aprobada — 2026-11-11

Tras validar imágenes estáticas con el usuario, la presentación adopta el
hexágono de seis fragmentos iguales y la tipografía dibujada a medida. Solo el
**segundo fragmento completo**, contando desde arriba en sentido horario,
es morado; CALS es naranja e IDE morado. La D refleja exactamente la C y queda
abierta. El SVG aprobado se guarda como `assets/branding/calside-inicio.svg`.
La variante CALSER que el usuario pidió conservar queda en
`assets/branding/calser-marca-geometrica.svg`; los PNG previos no se modifican.

Se amplía el ritmo a **4,2 s**: 1200 ms para dibujar los seis fragmentos,
900 ms para desplazamiento/giro completo, 1800 ms para trazos de las letras y
300 ms de reposo final. `MarcaInicio`, en `ide/interfaz/inicio.py`, carga el SVG
una vez y lo revela con máscaras de trazos Qt. No depende de bitmaps ni fuentes
instaladas. Cada fragmento se recorta por sus propias divisiones; un margen de
antialiasing evita que el paso al SVG completo cambie los bordes.

Clic/Esc, preferencia persistente y barrera entre presentación e inicialización
conservan su comportamiento. La construcción de widgets sigue en el hilo Qt y
los servicios mantienen sus trabajadores. No se amplía el trabajo a E8.

Archivos de esta revisión: dos SVG nuevos en `assets/branding/`,
`ide/interfaz/inicio.py`, `ide/pruebas/test_inicio.py`, `ide/README.md`,
`ide/docs/guia_sistema_calside.md`, `ide/docs/plan_ide.md` y esta bitácora.
Los cambios locales de otras tareas se conservan; no se realizan commits.

Verificación: **37 pruebas focalizadas OK** con
`QT_QPA_PLATFORM=offscreen python3 -m unittest ide.pruebas.test_inicio ide.pruebas.test_proyecto`.
Incluye colores y transición al reposo sin diferencias a escalas 1×/2×.
Se revisaron fotogramas intermedios y se generó una vista previa animada.
El punto de entrada real, con archivo de ruta con espacios y configuración
provisional, mostró la ventana a los 4346 ms en el recorrido normal, 160 ms
al omitir y 158 ms desactivada. Son medidas locales, no garantías. La revisión
del ritmo en el escritorio del usuario queda pendiente. `git diff --check`
sin errores; no se ejecutó la suite completa por este ajuste visual.

## Presentación de inicio de CALSIDE — 2026-11-11

Se implementa la secuencia acordada de unos 2 s: logo por seis lados, giro con
desplazamiento a la izquierda y trazos progresivos de CALSIDE. Se reutiliza
`assets/branding/calser-logo.png`; Qt dibuja letras y máscara de revelado en
coordenadas lógicas. Clic/Esc omiten el tiempo restante. Ver → Animación al
iniciar CALSIDE permite desactivar/reactivar los próximos arranques con
persistencia inmediata en `estado_v1.animacion_inicio`.

`Inicio` coordina la presentación con la preparación real mediante una barrera
de dos condiciones y pasos en el bucle Qt, sin sleeps ni bucles reentrantes.
Los widgets se construyen en el hilo gráfico y los servicios conservan sus
trabajadores; los argumentos se abren uno por turno. La presentación permanece
con «Preparando el editor…» si se omite antes de terminar la carga.

Archivos de implementación: `ide/interfaz/inicio.py` (nuevo), `ide/main.py`,
`ide/interfaz/ventana.py`, `ide/interfaz/proyecto.py`, `ide/preferencias.py` y
`ide/pruebas/test_inicio.py` (nuevo). Uso y verificación en `ide/README.md`;
estado y arquitectura actualizados en plan y guía del sistema. Se conservan
los cambios locales previos; no se realizan commits ni trabajo de E8.

Verificación focalizada: **36 pruebas OK**, con
`QT_QPA_PLATFORM=offscreen python3 -m unittest ide.pruebas.test_inicio ide.pruebas.test_proyecto`.
Se inspeccionaron fotogramas del revelado y composición final sin pantalla y
se verificó renderizado a escala Qt 2×. El punto de entrada real, con bucle Qt,
preferencias temporales y archivo con espacios, mostró la ventana a los 2166 ms
con animación, 177 ms al omitirla y 166 ms desactivada en esta máquina. Estos
valores son observaciones locales, no garantías de rendimiento. El constructor
de Ventana midió unos 52 ms; Qt conserva la construcción de widgets en su hilo.
La validación visual humana en el escritorio sigue pendiente; no equivale a
completar la revisión manual del IDE ni el empaquetado.

## Vista gris conservada al aparecer un diagnóstico — 2026-11-11

El usuario informa que la sugerencia aparece y luego desaparece cuando llega el error. Se reproduce con ccom automático real: el mensaje de diagnóstico modifica el tamaño del editor y el filtro trataba `Resize` como invalidación de contexto. Se conserva ahora la propuesta cuando solo cambia la geometría; una llamada posterior al layout la recoloca sin consultar de nuevo C, editar el documento ni reiniciar la opción elegida. El desplazamiento conserva propuestas vigentes mientras su cursor permanece visible. Edición, cursor/selección, cambio de pestaña, foco y Esc siguen descartándolas.

Comprobación focalizada: reproducción del diagnóstico tardío con Tab posterior y descarte con Esc/Enter/foco, **2 casos OK**. No se ejecutó la suite completa. Repetición humana pendiente.

## Asistencia en gris y correcciones más directas — 2026-11-11

A petición del usuario se presenta asistencia sobre el editor sin abrir un panel por cada propuesta. Tras una pausa de escritura (350 ms iniciales; se conserva la espera del perfil), el motor C propone y el widget `SugerenciaInline` dibuja solo el sufijo gris al final de la línea. Tab acepta, Esc descarta, Alt+[ / Alt+] recorre opciones; Enter continúa escribiendo líneas. No cambia documento, revisión, historial ni disco hasta aceptar. Contextos obsoletos, selección, comentarios y pérdida de foco impiden aplicar la vista. Ctrl+Espacio y Alt+Enter conservan sus vías manuales.

Si no hay una continuación, se puede solicitar una reparación nativa con origen interno `vista_inline`: se muestra solo una inserción local en el cursor, aplicable con Tab y precondiciones. No se inventan identificadores, números ni instrucciones completas; la lógica del lenguaje sigue en C. Para reemplazos a mitad de línea se usa la lista manual, evitando tapar código existente.

Se simplifica la activación de corrección con Enter: el diálogo selecciona la primera regla revisada cuando el usuario activa la opción y no tenía selección. La candidatura automática C de `;` conserva unicidad, aceptación completa, ausencia de errores léxicos y bloqueo de EOF, pero deja de exigir que el siguiente token sea reservado; una asignación puede comenzar con un identificador. No se habilita la corrección automática por defecto.

Validación focalizada: **7 casos UI, OK** (seis recorridos nuevos con ccom real y lista manual anterior), **2 casos de compatibilidad de preferencias/lista por pausa, OK**, compilación de ccom y revisión visual de `/tmp/calside-sugerencia-inline.png`. Sin suite completa. R12/H03 quedan ajustados y pendientes de repetición humana; los OK anteriores de la lista/panel manual se conservan.

## Asistencia manual aprobada y alcance informado por el profesor — 2026-11-11

El usuario confirma que todo el bloque C salvo R12 funciona: R09–R11 quedan OK y R12 conserva el fallo ya registrado. Informa que el profesor acepta un error a la vez; la recuperación de varias fases no se exige como trabajo adicional de esta entrega. Los demás requisitos y recorridos no se marcan aprobados por esta confirmación.

Se explica que E8 puede producir nuevas versiones empaquetadas durante el desarrollo: se continúa editando y probando el proyecto fuente y se vuelve a construir el paquete cuando se desee. Deben incluirse interfaz, recursos y dependencias, ccom y simulador según la plataforma. No se ha implementado el empaquetado con esta actualización documental.

## R12: fallo y observación de usabilidad — 2026-11-11

El usuario informa que la corrección automática no funciona y considera rígida y poco útil su interacción. Se marca R12 como Fallo y se registra H03, pendiente de diagnóstico y replanteamiento de la experiencia. No se atribuye una causa técnica sin investigar ni se modifica/retira la función. R09–R11 conservan sus estados anteriores, sin inferir aprobación. Actualización documental sin ejecutar pruebas.

## Revisión manual del bloque B aprobada — 2026-11-11

El usuario confirma como exitoso el bloque B: R05–R08 (diagnósticos sintácticos múltiples, actualización entre ediciones/pestañas, errores léxicos y comprobación semántica). Se conserva como límite vigente que una fase con errores bloquea las posteriores; la recuperación entre fases queda en mejoras futuras. Los bloques A y B están aprobados; continuar desde R09. Actualización documental sin nuevas pruebas automatizadas.

## Mejora futura: diagnósticos de varias fases — 2026-11-11

A petición del usuario se agrega al plan la recuperación de ccom para informar errores léxicos, sintácticos y semánticos de zonas independientes en una misma revisión. Incluye continuar tras tokens inválidos, analizar semánticamente partes fiables del AST, evitar cascadas y conservar los límites y el bloqueo de generación con errores. La lógica pertenece a C; CALSIDE muestra los resultados. Solo se documenta como trabajo futuro, sin cambiar el comportamiento actual ni ejecutar pruebas.

## Revisión manual R01–R04 aprobada — 2026-11-11

El usuario confirma que R01, R02, R03 y R04 pasan. Se marca completo el bloque A (archivos y edición): apertura y pestañas, guardado/cierre, operaciones del explorador con eliminación recuperable y búsqueda. Esta confirmación sustituye los estados pendientes anteriores de esos casos. El siguiente bloque pendiente es R05–R08: diagnósticos y recuperación. No se ejecutaron pruebas automatizadas para esta actualización documental.

## F5 sin código: aviso informativo — 2026-11-11

Compilar un documento que solo contiene comentarios válidos muestra «El archivo solo contiene comentarios; no hay código para compilar» en Salida, sin añadir un diagnóstico sintáctico ni generar salidas. Un documento vacío recibe un mensaje equivalente. Se usa `contiene_codigo` de Flex para la revisión actual; Python no reconoce comentarios. Si falta ese resultado, F5 lo espera de forma asíncrona y reanuda solo para el mismo documento, revisión y perfil, sin otra edición intermedia. ccom por consola conserva su comportamiento.

Se actualizó y ejecutó únicamente la regresión de F5 sin código: comentario de línea, comentario de bloque, documento vacío, comentario sin cerrar y transición a programa válido. **Una prueba focalizada, OK**. No se repitió la suite completa.

## Corrección de Ctrl+Z con papelera real — 2026-11-11

El usuario reprodujo un fallo que no detectaban los dobles de papelera: en esta versión de PySide, `QFile.moveToTrash(ruta)` devuelve bool aunque su documentación anuncie una tupla. La entrada se movía antes de fallar el desempaquetado y no se registraba en el historial. `papelera.py` usa ahora la API de instancia: `archivo.moveToTrash()` y `archivo.fileName()`, que proporciona la ubicación real. Archivo individual, carpeta y selección comparten este acceso.

Además, eliminar tiene prioridad para Ctrl+Z incluso al volver al editor. Una edición posterior devuelve la prioridad a deshacer texto; el menú explícito y Ctrl+Z en Archivos conservan acceso al historial de eliminaciones. Recargar se deshabilita cuando no existe el archivo y el comando directo explica cómo restaurarlo, sin mostrar solo una ruta. El aviso distingue una eliminación conocida de un cambio externo.

A petición del usuario se hizo solo una comprobación puntual: **dos casos, OK**, incluyendo papelera real con archivo temporal, Ctrl+Z desde el editor, Recargar ausente y deshacer escritura. No se repitió la suite completa. Las eliminaciones anteriores que fallaron antes de registrar el historial deben restaurarse desde la papelera del sistema.

## Eliminación visible y restauración durante la sesión — 2026-11-11

- Se renombra «Enviar a la papelera…» del explorador a «Eliminar archivo…», «Eliminar carpeta…» o «Eliminar selección…». La confirmación sigue indicando que se utiliza la papelera, sin borrado permanente alternativo.
- Se agrega eliminar al menú contextual del editor y de las pestañas con ruta guardada. Los documentos sin título no ofrecen una eliminación habilitada.
- `historial_archivos.py` registra origen, ubicación recuperable y estado de las pestañas. Ctrl+Z en Archivos y las acciones explícitas del menú restauran el último grupo eliminado; dentro del editor Ctrl+Z sigue deshaciendo texto. Los grupos conservan las eliminaciones que sí se completaron aunque falle otra entrada del lote.
- Se conservan buffers, historial de escritura y conflictos previos con el disco. No se sobrescriben destinos ni se recrean carpetas padre ausentes o sustituidas por enlaces. Una restauración parcial conserva pendientes para reintentar.
- El historial pertenece a esta sesión. Si el sistema no devuelve una ubicación recuperable se informa que debe restaurarse desde el gestor de archivos. Rehacer de eliminación no está implementado.
- Se agregan diez pruebas sobre archivos temporales: menús, Ctrl+Z real en árbol/editor, carpeta con subcarpetas, selección múltiple, buffers modificados, conflictos y fallos. Las 37 pruebas previas de menú, carpetas y selección también pasan. Suite completa: **400 pruebas IDE, OK** (40,265 s); `git diff --check` sin errores. Repetición manual pendiente.

## Recorrido de lo implementado

| Etapa | Trabajo registrado |
| --- | --- |
| Diseño y prototipo | PySide6, nombre CALSIDE, distribución clásica, menús bajo la cabecera, ámbar y esquina morada; animaciones aplazadas. |
| Editor y proyecto | Pestañas, edición, guardado, jerarquías, menú contextual, selección múltiple, arrastre, orden visual y búsqueda. |
| Primeras integraciones | ccom directo, lexer/LL(1) iniciales, análisis automático, problemas, clones, terminales y panel inferior/derecho. |
| Protección E7 | Huellas de disco, cambios externos, recarga/conservar/copia, operaciones de carpetas y preferencias. |
| Compilación/ejecución | Confirmaciones de éxito ccom, perfiles gráficos F5, ejecución F6 separada, artefactos vigentes, registros y traza. |
| Preparación de asistencia | Contratos, coordinador, sugerencias, reparación con vista previa, aplicador, Deshacer y permisos automáticos. |
| Decisión de arquitectura | Interfaz y gestión del IDE Python; decisiones del lenguaje en C reutilizando ccom. LL(1) y simulador Python autorizados; LL(1) queda para consulta/pruebas. |
| M1 / M1B | Diagnósticos JSON reales y perfiles/opciones de compilación por interfaz. |
| M2 | Tokens y colores desde Flex, sin lexer Python productivo. |
| M3 | Sugerencias Flex/Bison-LAC C, Ctrl+Espacio y clic derecho. |
| M4 | Reparaciones C de cuatro cierres, reanálisis y regla automática optativa de `;`. |
| M5 | Clones C Tipo I/II, varias instantáneas CCL1 y conservación del panel. |
| M6 | Validación JSON común, limpieza de duplicados, comentarios y recorrido gráfico sin analizadores Python. |
| M7 | Recuperación sintáctica conservadora C; varios errores, límites, progreso y bloqueo de semántica/generación con AST parcial. |

## Evidencia de cierre M7

- 384 pruebas del IDE correctas y suite completa de ccom correcta.
- Recuperación probada con 120 mutaciones deterministas, bloques/ramas, EOF, Unicode y límites.
- ASan/UBSan/LSan sin incidencias detectadas en el corpus M7 y 91 fuentes `.cls` del repositorio.
- Captura revisada con dos problemas sintácticos y navegación por línea.
- No se ejecutaron commits por iniciativa del asistente; los nombres de commit fueron sugeridos a petición del usuario.

Los conteos anteriores son evidencia de esa ejecución, no un objetivo fijo ni una garantía de validación manual o multiplataforma.

## Consolidación de documentación — 2026-11-11

Se separa el estado actual del historial. Se crea esta bitácora por solicitud del usuario. Plan y README se reescriben sin estados E5/E6 obsoletos; referencias de motores/política se fusionan en asistencia; seis documentos de servicios ccom se reúnen en una referencia; material LL(1) se agrupa en su carpeta. Se conservan documentos de compilador, ISA, crypto, ejercicios y pruebas porque pertenecen a otros componentes y aportan contenido propio.

La revisión manual queda **pendiente de ejecución humana**, con ejemplos copiables y resultados esperados. Las pruebas automáticas previas no se registran como pruebas manuales aprobadas. Empaquetado E8 y validación/defensa E9 siguen pendientes.

## Hallazgos iniciales de revisión manual — 2026-11-11

- El usuario informa un cierre inesperado de la aplicación. Se facilitó el comando de reapertura de la copia temporal; causa pendiente de reproducción/log. No se declara resuelto.
- El usuario informa falta de respuesta visual en Compilar/F5. Se acotó el fondo de la barra a su contenedor y se definieron estados normal, hover, pulsado, foco, deshabilitado y flecha del menú. La regla de fondo heredada y el selector por ID tapaban los estados genéricos.
- Verificación del arreglo: 18 pruebas de compilación correctas y capturas del botón real con diferencias comprobadas entre normal/hover/pulsado. No se guarda ni compila una fuente del usuario para revisar el estilo. La repetición humana sigue pendiente.

### Ajuste posterior de Archivos

Al acotar el fondo de la barra, el botón Archivos mostró el relieve nativo de su estado marcado. Se fijó fondo transparente/sin borde y autoRaise para conservar el aspecto plano, manteniendo hover/pulsación y sincronización del explorador. Capturas y alternancia mostrar/ocultar verificadas; repetición humana pendiente.

### Flecha de Compilar y menús redondeados

El usuario reporta un bloque irregular en la flecha al pasar el mouse/pulsar y pide que Archivo, Editar, Ver y Compilar compartan el resaltado redondeado de Archivos. Se hace transparente y se posiciona explícitamente el subcontrol del desplegable, conservando separador y flecha; los elementos de QMenuBar reciben radio de 4 px y colores de hover/pulsación coherentes. Verificados desplegable real, capturas y 18 pruebas de compilación. Repetición humana pendiente.

### Revisión R01/R02

El usuario confirma R01 correcto. En R02 reporta un problema y muestra un documento con solo `# prueba manual` y diagnóstico de EOF sintáctico: ese texto no constituye un programa completo. Se aclara la guía y se ofrece un programa mínimo válido para aislar guardado/cierre de los diagnósticos de lenguaje. R02 no se declara aprobado ni se presupone un fallo de guardado sin más evidencia.

### R02: comentarios válidos y URI en el buffer

El usuario solicita corregir el diagnóstico automático de un documento con solo comentario y reporta una URL añadida tras guardar. La copia guardada seguía conteniendo solo `# prueba manual`; se reprodujo la inserción de URL mediante MIME de archivo admitido por QPlainTextEdit. Se bloquean esos MIME en inserción/pegado/arrastre, conservando texto plano y el portapapeles. No se elimina automáticamente texto ya insertado en documentos del usuario.

El adaptador conserva si Flex reconoció tokens distintos de comentarios; Python usa ese metadato para omitir sintaxis automática sin duplicar reglas. Comentarios abiertos/caracteres inválidos siguen dando errores; F5 conserva comprobación explícita. Seis pruebas nuevas cubren guardado intacto, URL, texto, Deshacer, comentarios, transición de errores y compilación explícita. Suite completa: 390 pruebas IDE correctas; captura revisada con comentario guardado, buffer intacto y estado sin código. R02 sigue pendiente de revisión humana.

## Documentos sustituidos, preservados íntegramente como registro

Cada bloque conserva el contenido que había al consolidar. Las referencias se redirigen a su ubicación vigente; las afirmaciones internas describen el momento de aquel documento. No utilizar sus listas de pendientes para dirigir el trabajo actual.


<a id="hist-plan-ide"></a>

<details>
<summary>plan_ide.md — documento sustituido</summary>

# Plan de CALSIDE · IDE de Calser

> **Actualización M1 (2026-11-11):** el análisis automático usa `ccom --check-json` y conserva el front-end real en C. Saddy aclaró que el LL(1) y el simulador pueden seguir en Python; no se exige migrarlos. Las demás etapas se describen en el [plan de integración](plan_ide.md).

Guía de arquitectura, conexiones e inventario para revisar/comentar el código: [guia_sistema_calside.md](guia_sistema_calside.md).

Versión 2 · actualizado el **5 de noviembre de 2026**.

Documento de trabajo para completar el IDE. Sustituye el orden inicial y los anexos de avances de la versión anterior. Los estados describen código existente: una propuesta o una API recibida no equivale a una función conectada al editor.

## 1. Objetivo, alcance y referencias

Construir una aplicación de escritorio propia para editar Calser, recibir asistencia durante la escritura, compilar con `ccom` y detectar clones entre archivos. Debe poder entregarse como aplicación ejecutable, sin exigir VS Code al usuario final.

Referencias:

- Enunciado `proyecto-ii-grupal-perf-cache.pdf`: página 9, punto 4 (sugerencias, correcciones automáticas, errores gráficos con descripción y clones Tipo I/II entre archivos); página 13, §7.2 (iteraciones); página 18, §9.2 (IDE: 15 % de CE1108).
- [Guía del núcleo](guia_sistema_calside.md), [gramática original](ll1/gramatica_calser.md), [gramática LL(1)](ll1/gramatica_ll1.md) y [API del núcleo](../nucleo/README.md).
- [Uso, instalación y límites actuales](../README.md), [banderas de ccom](../../docs/ccom_banderas.md) y código de `ide/`, `grammar/` y `src/main.c`.
- [Mockups](mockups.html): referencias visuales con escenarios simulados; no constituyen la implementación.
- `plan_ide_interfaz.md` recibido del equipo: antecedente del reparto. Las decisiones posteriores de esta conversación prevalecen cuando cambian biblioteca, arquitectura, responsabilidades o secuencia.

**Separación de objetivos:** completar los requisitos del IDE no equivale a terminar el proyecto completo de Compiladores/Arquitectura. Optimizaciones, IR, cachés y simulador tienen sus propias tareas. Las métricas pueden integrarse después; no son condición para la primera versión completa del IDE.

## 2. Estado real al actualizar este documento

Actualización M7 (2026-11-11): recuperación sintáctica conservadora C, varios errores en Problemas, sincronización con progreso y límites, sin semántica/exportación/generación sobre AST parcial. 384 pruebas IDE y suite ccom verificadas. [Diseño y limitaciones](../../docs/ccom_servicios_ide.md#recovery).

Actualización M6 (2026-11-11): revisión de dependencias, LL(1) de consulta explícitamente fuera de producción, cabecera JSON común y comentarios de servicios/adaptadores. Recorrido gráfico con varios documentos, C real y bloqueo de imports de analizadores Python; 382 pruebas IDE y suite de ccom verificadas. [Revisión y orden de lectura](guia_sistema_calside.md).

Actualización M5 (2026-11-11): clones Tipo I/II con Flex/comparación C, instantáneas múltiples CCL1 y panel existente. Sin tokenizador/normalizador Python. Límites, cancelación y bloqueo explícito de errores léxicos. [Contrato M5](../../docs/ccom_servicios_ide.md#clones).

Actualización M4 (2026-11-11): reparaciones reales C (`--repair-json`), cuatro cierres validados sobre copias, precondiciones verificables y panel conectado. La política automática usa `--syntax-json` en C. Una regla revisada de punto y coma, desactivada hasta permiso explícito en preferencias. 371 pruebas IDE y suite del compilador correctas. [Contrato M4](../../docs/ccom_servicios_ide.md#repair).

Actualización M3 (2026-11-11): sugerencias reales en C con Flex + Bison/LAC, `--suggest-json`, Ctrl+Espacio y pausa opcional. La entrega se adaptó a C sin usar sus modelos duplicados. Reparaciones sigue pendiente M4. [Contrato M3](../../docs/ccom_servicios_ide.md#suggest).

Actualización M2 (2026-11-11): resaltado/errores léxicos vivos desde Flex mediante `ccom --tokens-json`, sin recorrido léxico ni búsqueda de comentarios Python en el editor. Se conservan snapshots, cancelación y Deshacer. Ver [contrato léxico](../../docs/ccom_servicios_ide.md#tokens).

Actualización M1B (2026-11-11): botón Compilar dividido, F5 según modo seleccionado, diálogo de destino/opciones compatibles y preferencias. Cuatro modos reales: análisis, compilación, AST JSON sintáctico y ensamblado .asm; F6 requiere compilación completa con listado y artefactos renovados. Ver [plan de integración](plan_ide.md).

Actualización M1 (2026-11-11): `ccom --check-json` implementado y conectado al análisis automático. Diagnósticos tipados y listas/rangos validados, sin regex de mensajes en ese recorrido. 312 pruebas del IDE y suite completa del compilador correctas. F5/F6, lexer en vivo, LL(1) y simulador se conservan. Ver [contrato del servicio](../../docs/ccom_servicios_ide.md#check).

Rama: `feat/calside`. El merge de `origin/develop` se completó en `c8b1997`. En esa revisión pasaron **44 pruebas** mediante `make check-ide`: 24 de CALSIDE y 20 del núcleo. Es una referencia de validación de esta revisión, no un conteo que deba mantenerse fijo.

### Implementado y probado

- Aplicación PySide6/Qt Widgets llamada **CALSIDE**, con tema oscuro, acento ámbar y esquina morada estática.
- Cabecera sobre la barra de menús; menús centrados verticalmente.
- Editor con números de línea, resaltado basado en tokens reales del núcleo, pestañas, búsqueda y operaciones de edición.
- Nuevo, abrir, guardar, guardar como, protección de cambios al cerrar y rutas canónicas para evitar pestañas duplicadas.
- Explorador asíncrono de subcarpetas y `.cls`, con exclusiones y sin enlaces simbólicos; ocultar/recuperar con botón o Ctrl+B.
- E7: versiones de disco con SHA-256, avisos no destructivos y guardado condicionado a la versión confirmada; preferencias de fuente, ccom y distribución persistentes.
- Panel de resultados movible abajo/derecha y terminales PTY independientes en Linux, con entrada por líneas.
- Compilación real asíncrona con `ccom`, salida, cancelación y límite actual de 30 segundos.
- Análisis automático con `ccom --check` sobre copias temporales, pausa de 600 ms después del lexer y límite de 5 segundos; sin guardar ni generar binarios.
- Errores léxicos/sintácticos/semánticos de `ccom` al compilar: subrayado, punto en margen, descripción, navegación y selección del fragmento.
- Diagnósticos por documento y descarte de resultados antiguos al editar o cambiar de ruta.
- Clones Tipo I/II independientes de LL(1), en segundo plano, con comparación, navegación, buffers sin guardar y ejemplos reproducibles.

### Núcleo incorporado y conexión actual

- `ide.nucleo.lexico.analizar(texto)`: conectado al resaltado y a errores léxicos en vivo (E3). Análisis por pausa, documento y revisión; funciona sin guardar.
- Gramática Calser transformada a LL(1), PRIMERO/SIGUIENTE y tabla predictiva.
- Parser predictivo Python implementado por Saddy: análisis completo, primer error y contexto en el cursor, sin árbol/AST. API real en `parser_predictivo.md`.
- Pruebas y documentación de la API.

### Todavía pendiente

- Motores de sugerencias y reparaciones a cargo del mismo compañero; UI manual preparada; conexión real posterior. El parser ya está implementado por Saddy.
- Entrega empaquetada y validación en otra máquina; preferencias adicionales y mejoras visuales posteriores.
- Animación de papel y controles de ventana en la esquina, como mejora posterior.

**Estado actual:** el editor usa el lexer del núcleo. La gramática y la tabla ya alimentan un parser predictivo disponible como API independiente; todavía no se invoca desde los widgets. Los motores de sugerencias y reparaciones continúan pendientes; ambas interfaces manuales están preparadas.

## 3. Decisiones vigentes

| ID | Decisión | Estado / consecuencia |
|---|---|---|
| D00 | Nombre **CALSIDE**, el IDE de Calser | Aceptado y aplicado |
| D01 | Aplicación propia con Python y PySide6/Qt Widgets | Aceptado e implementado; reemplaza Tkinter |
| D02 | Dependencias externas permitidas | PySide6 aceptado; no añadir otras sin necesidad |
| D03 | Reutilizar recursos útiles del plugin | Patrones de resaltado e icono existentes; conservar avisos aplicables |
| D04 | `ccom` es el compilador y se invoca directamente | Implementado; LL(1) no es intermediario obligatorio |
| D05 | Núcleo para asistencia durante la edición | Lexer conectado; parser disponible como API; motores de sugerencias/reparaciones pendientes; UI manual preparada |
| D06 | `QPlainTextEdit` para código | Implementado |
| D07 | `QProcess` para ccom, sin bloquear la UI | Implementado |
| D08 | Distribución clásica adaptable | Implementada; explorador plegable y resultados abajo/derecha |
| D09 | Ámbar + esquina morada de papel | Esquina estática implementada; animación pospuesta |
| D10 | Detector de clones independiente del parser | Implementado; motor e interfaz separados correctamente |
| D11 | Mantener controles nativos de ventana por ahora | Implementado; revisar movilidad y redimensionamiento antes de sustituirlos |

## 4. Responsabilidades y dependencias

| Componente | Responsable según el reparto actual | Dependencia pendiente |
|---|---|---|
| Interfaz, archivos, paneles, terminales y adaptadores | Saddy / trabajo de CALSIDE | E7 implementado; continuar con empaquetado y validación |
| Integración con `ccom` | Saddy / CALSIDE, coordinado con responsable del compilador | Modo --check implementado; coordinar futuras extensiones del compilador |
| Léxico, gramática y tabla LL(1) | Compañero / C1.1a | Entrega 1 incorporada |
| Parser predictivo | Saddy | Implementado; API, ejemplos y pruebas disponibles |
| Sugerencias | Compañero (también reparaciones) | Motor sobre la API real del parser |
| Reparaciones | Mismo compañero de sugerencias | Motor, contrato de ediciones y casos seguros |
| Detector y panel de clones | Implementados en CALSIDE por solicitud de Saddy | Mantenimiento y validación final |

Mantener el parser de Saddy separado de los widgets; los motores del compañero se adaptan a su API implementada. Las funciones que no dependan de esa entrega pueden avanzar en paralelo lógico: `ccom`, archivos, clones, pruebas y empaquetado. Esto no requiere crear agentes ni chats adicionales.

## 5. Funcionalidades y criterios de aceptación

Estados: **implementado** = existe en el prototipo y tiene verificación; **parcial** = falta una parte del criterio; **pendiente** = requiere implementación/integración. P0: base; P1: cierre del IDE; P2: mejoras posteriores. Una función implementada todavía debe pasar el guion final en otra máquina.

| ID | Prioridad | Función | Estado actual y criterio para cerrarla |
|---|---|---|---|
| F01 | P0 | Abrir carpeta/proyecto | Implementado: modelo asíncrono de Qt, subcarpetas, exclusiones, archivos nuevos y errores de acceso. No sigue enlaces simbólicos. Incluye creación y operaciones de carpetas, expansión lazy, ramas persistentes, selección múltiple, arrastre interno con tarjeta semitransparente y orden manual persistente entre hermanos. |
| F02 | P0 | Nuevo/abrir/guardar/guardar como | Implementado: UTF-8, rutas canónicas, identidad actualizada y sin pestañas duplicadas. Protección de cambios externos en F17. Menú contextual para crear, renombrar, copiar/cortar/pegar y enviar archivos o carpetas a la papelera, conservando buffers abiertos; ubicación y propiedades disponibles. |
| F03 | P0 | Pestañas y cambios pendientes | Implementado: texto/cursor por documento y guardar/descartar/cancelar al cerrar. |
| F04 | P0 | Edición y búsqueda | Implementado: deshacer/rehacer, portapapeles, búsqueda en documento (Ctrl+F) y proyecto (Ctrl+Shift+F), y margen sincronizado. La barra lateral busca nombres/rutas .cls, carpetas y código, incluye buffers y navega a línea/columna; filtros, cancelación y límites con aviso. |
| F05 | P0 | Compilar documento activo | Implementado: F5 guarda y compila; cancelar guardado cancela la operación. Ejecutar F6 es una acción separada: usa una compilación vigente y el simulador del repo, con traza y variables finales en Ejecución. No recompila automáticamente. |
| F06 | P0 | Resolver/configurar ccom | Implementado: configuración persistente y recuperación de ruta inválida mediante la búsqueda habitual. |
| F07 | P0 | Salida y estado del proceso | Implementado para compilación y ejecución: stdout/stderr, código, fallo de inicio, cancelación y timeout. La ejecución tiene pestaña independiente, copias verificadas de artefactos y traza única por corrida. Tiempo límite configurable pendiente en F18. |
| F08 | P0 | Errores gráficos y navegables | Implementado al compilar: origen, categoría, archivo, rango, subrayado, descripción y navegación. Los diagnósticos generales sin posición se cuentan en Problemas, muestran «—» y abren el archivo sin inventar rangos; la salida completa se conserva. |
| F09 | P1 | Resaltado con lexer del núcleo | Implementado: tokens reales, comentarios coloreados aparte y rangos UTF-16. El formato conserva revisión, estado de guardado y Deshacer. |
| F10 | P1 | Análisis mientras se escribe | Implementado: lexer en vivo (300 ms) y ccom --check después de 600 ms adicionales. Snapshots por revisión, cancelación, temporales y resultados independientes de F5. Parser predictivo disponible; integración gráfica pendiente como motor complementario. |
| F11 | P1 | Sugerencias | Parcial: coordinador y UI manual con Ctrl+Espacio, flechas, Enter/Tab, clic, Escape y reemplazo validado. Proveedor real ausente se informa; datos controlados solo en pruebas. Pausa configurable preparada; falta motor real. |
| F12 | P1 | Reparaciones propuestas | Parcial: contrato y aplicador con una operación de Deshacer implementados. Panel manual con explicación, vista previa y aplicar/cancelar implementado. Faltan motor y catálogo para terminadores/delimitadores Calser. |
| F13 | P1 | Corrección automática | Parcial: permisos persistentes, disparador Enter/pausa, reglas seleccionadas y validación con el parser implementados y probados. Catálogo real vacío hasta revisar positivos/negativos del motor; no se declara completada la función sin esa integración. |
| F14 | P1 | Clones Tipo I entre archivos | Implementado: ignora formato/comentarios y permite comparar/navegar entre ubicaciones. |
| F15 | P1 | Clones Tipo II entre archivos | Implementado: renombramiento consistente y literales del mismo tipo; preserva operadores/reservadas; umbral y límites documentados. |
| F16 | P1 | Recuperación ante fallos | Implementado en las funciones disponibles: errores de lectura/escritura, procesos, cancelación y archivo borrado; texto conservado ante fallos. Los motores pendientes tendrán sus propias pruebas. |
| F17 | P1 | Cambios externos | Implementado: sondeo de huella en segundo plano, cambio/borrado/reemplazo, recargar/conservar/copia y comprobación al guardar. Ver límite de concurrencia del sistema de archivos en `proyecto_archivos.md`. |
| F18 | P2 | Preferencias | Parcial: ruta de ccom, fuente, proyecto, paneles, análisis automático y asistencia (control general, pausa, reglas) persistentes, con configuración validada. Tema y timeout configurables quedan para mejoras posteriores. |
| F19 | P1 | Artefactos y opciones del compilador | Requisito confirmado por el profesor (2026-11-11): selector de modo en Compilar/F5, destino y opciones compatibles. Parcial: la compilación fija actual funciona; falta la selección gráfica. Ver M1B y sección 13 de [migración](plan_ide.md). |
| F20 | P2 | Métricas dentro del IDE | Pendiente y opcional para el IDE: depende del contrato de datos de las optimizaciones. |
| F21 | P0 | Ocultar/recuperar explorador | Implementado: ×, botón Archivos, Ctrl+B y menú Ver sincronizados. |
| F22 | P0 | Mover resultados abajo/derecha | Implementado: conserva pestañas, contenido y procesos. |
| F23 | P0 | Varias terminales | Implementado en Linux: PTY, directorios independientes, historial, interrupción y cierre con confirmación. Pantalla completa fuera de esta entrega. |
| F24 | P0 | Alineación de menús | Implementado: barra bajo cabecera y menús centrados verticalmente. |
| F25 | P1 producto | Ejecutable de escritorio | Pendiente: abrir desde icono sin VS Code ni comando Python; incluir recursos/dependencias y ccom compatible o configuración guiada. |
| F26 | P2 | Esquina de papel animada | Pendiente, pospuesta: revelar controles mediante clic/teclado y conservar mover/redimensionar/restaurar/cerrar correctamente. |

F09, F10, F17 y F25 son compromisos del producto y del flujo acordado; no todos figuran literalmente en el apartado de IDE del enunciado. Los cuatro requisitos explícitos de ese apartado corresponden a F08, F11, F13 y F14/F15.

## 6. Arquitectura existente y cambios propuestos

Estructura real relevante:

```text
ide/
  main.py                       inicio y composición
  adaptador_nucleo.py            API real → estilos y errores
  diagnosticos.py                diagnóstico común con origen/unidad
  archivos.py                   lectura consistente y guardado atómico condicionado
  ediciones.py                  validación de ediciones y posiciones Python/UTF-16
  preferencias.py               configuración validada
  politica_asistencia.py        permisos, catálogo revisado y evaluación automática con el parser
  ccom.py                       ejecutable, diagnósticos y categorías
  clones.py                     adaptador JSON/CCL1 y modelos, sin Qt
  interfaz/
    ventana.py                  ventana, acciones, procesos y coordinación
    ejecucion.py                compilaciones vigentes, QProcess del simulador, trazas y cancelación
    proyecto.py                 conflictos de disco, recarga, fuente y preferencias
    disco.py                    sondeo de archivos en segundo plano
    explorador.py               árbol lazy, ramas persistentes, atajos y exclusiones
    acciones_archivos.py         menús y operaciones con pestañas conservadas
    operaciones_carpetas.py      copia/traslado/papelera en segundo plano
    editor.py                   editor, margen, resaltado y marcas
    ediciones.py                aplicación contextual en Qt y bloque único de Deshacer
    analisis.py                 cola serial y trabajador léxico
    asistencia.py               coordinador serial de ambos motores y validación de respuestas
    sugerencias.py              lista manual junto al cursor y control de interacción
    reparaciones.py             panel de propuestas, motivos, vista previa y aplicación explícita
    preferencias_asistencia.py   diálogo de permisos, pausa y reglas automáticas
    comprobacion.py             ccom --check, QProcess y temporales
    clones.py                   trabajador y panel de clones
    terminal.py                 terminal de comandos
    terminal_pty.py             proceso auxiliar Linux
  nucleo/
    lexico.py                   tokens y errores léxicos
    gramatica.py                gramática como datos
    tabla.py                    PRIMERO/SIGUIENTE y tabla LL(1)
    parser.py                   parser con pila, primer error y contexto del cursor
    asistencia.py               contrato compartido de sugerencias, reparaciones y ediciones
  ejemplos/clones/
  pruebas/
  docs/
```

Componentes de E3 ya creados: `adaptador_nucleo.py`, `diagnosticos.py` e `interfaz/analisis.py`. El modelo común explicita origen, unidad de columna e identidad del documento; el coordinador procesa snapshots sin tocar widgets desde el trabajador. La cola reemplaza solicitudes pendientes del mismo documento y rechaza respuestas de revisiones antiguas.

No refactorizar todo el IDE solo para igualar un árbol de carpetas. Mantener `ide/clones.py` e `ide/interfaz/clones.py`: uno es adaptador del motor C y el otro presentación. No hace falta LSP para completar el alcance.

## 7. Contratos de integración

### 7.1 Léxico recibido: API comprobada

```python
from ide.nucleo.lexico import analizar
tokens, errores = analizar(texto)
```

- `Token`: `tipo`, `lexema`, `linea`, `columna`, `columna_fin` inclusiva y `valor`.
- `ErrorLexico`: `mensaje`, `linea`, `columna`, `largo`, `fatal`.
- Líneas/columnas desde 1; columnas en caracteres Python; tabulación cuenta una columna.
- Comentarios y espacios no producen tokens. Un comentario de bloque abierto produce un error fatal y corta el análisis.
- `construir_tabla(CALSER)` produce la tabla, no un diagnóstico sintáctico del documento.

Procedimiento de adaptación:

1. Capturar identificador del documento, texto y revisión.
2. Invocar el lexer sobre esa copia de texto, no sobre el widget desde un trabajador.
3. Transformar token a categoría de color y error a rango/mensaje/origen `núcleo`.
4. Mantener el coloreado de comentarios como presentación; no crear otro parser.
5. Publicar solo si el documento sigue abierto y su revisión coincide.

### 7.2 Posiciones y resultados

| Fuente | Unidad original | Conversión hacia Qt |
|---|---|---|
| ccom/Flex | Columnas y longitudes en bytes UTF-8 | Decodificar prefijo y rango sin partir caracteres; convertir después a UTF-16 |
| Núcleo | Caracteres Python, fin inclusivo de token | Convertir prefijos con `len(prefijo.encode('utf-16-le')) // 2`; fin exclusivo a partir de `columna_fin` |
| Detector de clones | Offsets Unicode, inicio inclusivo y fin exclusivo | Convertir ambos prefijos a unidades UTF-16 |

Resultado mínimo del coordinador: documento, revisión, origen, estado, tokens/diagnósticos y duración cuando se mida. Cambiar de pestaña no permite que un resultado termine sobre otra. Los diagnósticos sin rango válido conservan su mensaje; no inventar un token. Los errores al final de línea mantienen su señal de línea.

Separar resultados `núcleo`, `ccom automático` y `ccom compilación`. Definir deduplicación o prioridad para que el mismo error no aparezca varias veces; no sumar mensajes sin identificar su fuente. Editar invalida los resultados de la revisión anterior, pero conserva el historial de salida identificado.

### 7.3 Parser implementado y contratos de los motores

El parser expone `analizar_sintaxis` y `contexto_en` en `ide/nucleo/parser.py`; API y ejemplos en [parser_predictivo.md](ll1/parser_predictivo.md). Los tipos y firmas de ambos motores están fijados en [contrato_asistencia.md](asistencia.md), con implementación compartida en `ide/nucleo/asistencia.py`. Los motores siguen pendientes; no deben volver a definir sus tipos.

- Sugerencias: `sugerir(texto, cursor, *, cancelar=None) -> ResultadoSugerencias`.
- Reparaciones: `proponer_reparaciones(texto, *, cancelar=None) -> ResultadoReparaciones`.
- Estados explícitos, opciones inmutables, rangos Python `[inicio, fin)` y precondiciones sobre el original. Ambas APIs son Python puro y propagan cancelación al parser.
- El IDE conserva documento, revisión, texto, cursor y ancla; aplica solo sobre el mismo contexto. La automatización exige política/catálogo/disparador además de un indicador del motor.
- El aplicador está implementado: rangos, límites, UTF-16, cursor/selección, precondiciones y una operación de Deshacer. No guarda ni crea propuestas.
- Coordinación en segundo plano y UI manual de sugerencias implementadas; carga diferida de módulos reales y proveedores inyectables para pruebas. Panel manual de reparaciones implementado; preferencias, pausa y política automática preparadas; faltan motores reales, catálogo revisado y prioridad de sus diagnósticos frente a ccom.

### 7.4 Análisis automático con ccom

`ccom` y el núcleo son rutas complementarias: CALSIDE puede invocar al compilador directamente, sin atravesar LL(1).

Implementado `ccom --check`: ejecuta léxico/sintaxis/semántica y libera AST/símbolos sin generar código ni escribir artefactos. Solo se combina con -v/--time; las opciones de salida se rechazan antes de abrir archivos. `--ast-json` sigue siendo otra operación: por sí solo termina tras sintaxis y no valida semántica completa.

La copia temporal debe reflejar el buffer sin guardar y conservar contexto de rutas cuando se incorpore enlazado. No sobrescribir el fuente ni generar artefactos de compilación por cada tecla. Mapear los diagnósticos de la copia al documento original. Limpiar temporales al completar/cancelar; limitar concurrencia y tiempo; no competir con Compilar por un mismo destino.

## 8. Procedimiento de implementación, en orden

Cada entrega termina con pruebas y actualización de este documento. Las tareas independientes pueden adelantarse sin bloquearse por APIs que todavía no existen.

### E0 · Base y merge — completado

- [x] PySide6, identidad, distribución y comandos `make ide` / `make check-ide`.
- [x] Incorporar la entrega 1 del núcleo mediante merge, conservando CALSIDE.
- [x] Ejecutar pruebas combinadas: 44 correctas en `c8b1997`.

La presencia del paquete ya está resuelta; no volver a planificar otro merge para esa misma entrega. Futuras entregas se incorporarán por separado cuando existan. Publicar commits/PR queda como operación Git aparte.

### E1 · Editor, compilación y diagnósticos — prototipo completado

- [x] Archivos/pestañas, cambios pendientes y edición.
- [x] Ejecutar ccom, mostrar salida y gestionar proceso.
- [x] Rangos, subrayados, descripciones, navegación y resultados por documento.
- [x] Terminales y controles de distribución.

**Salida comprobada:** abrir → editar → guardar → compilar → activar error → corregir → compilar correctamente. Las mejoras de proyecto y cambios externos siguen en E7.

### E2 · Clones — implementado

- [x] Tipo I/II, comparación entre archivos y normalización documentada.
- [x] Trabajo en segundo plano, cancelación, buffers abiertos y rangos Unicode.
- [x] Comparación y navegación; invalidación ante cambios.
- [x] Ejemplos positivos/negativos y límites explícitos.

**Salida comprobada:** `ide/ejemplos/clones` produce tres pares (uno I y dos II) con mínimo 20 tokens. Su detector no depende del núcleo y no necesita esperar otra entrega.

### E3 · Conectar el lexer al editor — completado

1. Crear el adaptador descrito en §7.1 con conversiones de §7.2.
2. Introducir coordinación por documento y revisión; pausa inicial aproximada de 300 ms, medible y ajustable.
3. Sustituir el resaltado provisional por categorías de tokens y conservar comentarios coloreados.
4. Presentar errores léxicos del buffer sin guardar con origen `núcleo`, subrayados y descripción.
5. Reutilizar navegación y panel Problemas sin mezclar unidades ni duplicar errores.
6. Gestionar apertura/cierre, texto vacío, comentarios abiertos, edición rápida y respuestas antiguas.

**Criterio cumplido:** escribir `@` produce el error sin guardar ni compilar; quitarlo retira el diagnóstico; tipos, reservadas, comentarios y Unicode se muestran correctamente; editar una pestaña no contamina otra. No se anuncian todavía errores sintácticos del parser LL(1).

Implementación histórica E3 (sustituida por M2 para usar Flex): adaptador puro, comentarios de presentación, timer de 300 ms y cola serial con snapshots por revisión; errores con origen `núcleo`. Desduplicación por rango frente a ccom. Límite explícito de 2 millones de caracteres. Verificación E3: 56 pruebas combinadas, incluidos cierre durante análisis, resultados antiguos, recuperación ante fallos, Unicode y preservación de guardado/Deshacer.

### E4 · Análisis automático con ccom — completado, independiente de LL(1)

1. Coordinar y comprobar el modo de solo análisis del compilador (§7.4), con pruebas que demuestren ausencia de salidas binarias.
2. Conectar copias temporales del buffer a un proceso separado, con pausa, cancelación y revisión.
3. Remapear rutas y publicar errores sintácticos/semánticos sobre el documento original.
4. Deduplicar/priorizar frente a resultados del lexer y conservar la compilación manual como acción diferenciada.
5. Probar que no cambia el archivo en disco y que temporales/procesos se limpian incluso al fallar.

**Criterio cumplido:** una variable no declarada y una sentencia incompleta se señalan sin pulsar F5; corregirlas retira el mensaje y el archivo original continúa sin guardar. Usa copias temporales eliminadas tras éxito, cancelación o fallo, y orígenes separados. F5 tiene prioridad y reanuda otros documentos.

Verificación E4: 70 pruebas combinadas de CALSIDE/núcleo; `make check-analysis` y `make check` del compilador correctos. Se probaron original/artefactos intactos, errores sintácticos/semánticos, edición/cierre/Guardar como durante el proceso, timeouts, ejecutable antiguo, fallos de inicio, salida excesiva y prioridades entre pestañas. La prioridad es compilación manual sobre ccom automático y, para errores léxicos equivalentes, ccom sobre núcleo. Los mensajes de sintaxis conservan la explicación y posición de Bison; mejorar su contexto sigue pendiente.

### E5 · Parser predictivo y sugerencias — parcial

- [x] Parser implementado por Saddy en `ide/nucleo/parser.py`, reutilizando lexer/gramática/tabla existentes.
- [x] Decisiones 1B, 2A, 3A, 4A y 5B: análisis completo y contexto del cursor; pila explícita; primer error; sin árbol/AST; API fijada después de implementar. Recuperación por sincronización posterior.
- [x] Contrato real documentado en `parser_predictivo.md`, con posiciones Python, estados, límites y ejemplos para ambos motores.
- [x] 26 pruebas nuevas; 96 combinadas correctas. Comparación sintáctica con Bison (`--ast-json`): 105 de 110 fuentes sin errores léxicos, sin discrepancias; cinco excluidas por errores léxicos.
- [x] Contrato de motores y aplicador común implementados; ver preparación de integración más abajo.
- [x] M3: comportamiento/casos de la entrega adaptados a C sobre Flex/Bison-LAC.
- [x] M3: proveedor C conectado al contrato; no añade otra fuente de diagnósticos en vivo.
- [x] UI manual con Ctrl+Espacio y coordinador conectados al contrato; sin motor ficticio de producción.
- [x] Activación por pausa opcional, permisos y cancelación preparados en el paso 6; falta verificar contextos con el motor real.
- [x] Lista con flechas, Enter/Tab/clic, Escape y reemplazo por rango, con descarte de contexto anterior.
- [x] M3: verificado con el motor real que no propone dentro de comentarios; los errores/bloqueos se presentan sin lista.

**Terminado cuando:** se puede escribir una declaración o construcción Calser apoyándose en opciones válidas; aceptar reemplaza solo el fragmento previsto; cancelar no cambia el texto. El parser disponible no equivale todavía a sugerencias integradas.

#### Preparación de integración de E5/E6

1. [x] **Contrato compartido:** modelos y firmas en `nucleo/asistencia.py`; guías de ambos motores alineadas. Decisiones documentadas en `contrato_asistencia.md`.
2. [x] **Aplicador de ediciones:** validación completa previa, rangos del original, precondiciones, Unicode, contexto obsoleto, conservación de selección y Deshacer/Rehacer. Verificado dentro de la ventana; sin guardado ni motores simulados en producción.
3. [x] **Coordinador de asistencia:** ambos tipos de motor, una tarea activa, pendientes por documento/tipo, cancelación, límites, validación y carga diferida de proveedores reales.
4. [x] **UI de sugerencias:** Ctrl+Espacio, lista junto al cursor, teclado/ratón, Escape y aceptación mediante el aplicador. Verificada con proveedores controlados solo de prueba.
5. [x] **UI de reparaciones:** Alt+Enter, explicación, vista previa de cambios, aplicar/cancelar, una operación de Deshacer e invalidación de propuestas antiguas. Conserva la prioridad actual de diagnósticos; el panel no publica una fuente nueva ni modifica el contador de Problemas.
6. [x] **Preferencias y política automática:** diálogo persistente, pausa opcional, desactivación general, permiso por regla y comprobación real del parser en segundo plano. Disparador conservador: Enter al final de una línea y pausa. Catálogo de producción vacío hasta revisar las reglas de la entrega real; ver [política y conexión](asistencia.md).
7. [ ] **Integración real:** adaptar/conectar entregas y verificar con el parser y ccom reales.

Preparación 1/2 verificada: **23 pruebas nuevas; 227 combinadas correctas**. Las opciones controladas de las pruebas no son un motor de sugerencias/reparaciones. Los pasos 3/4 también están preparados: **26 pruebas nuevas y 253 combinadas correctas**. El paso 5 añade **15 pruebas; 268 combinadas correctas**. El paso 6 añade **26 pruebas; 294 combinadas correctas**. Sigue el **paso 7, integración real de los motores** y revisión de su catálogo.

### E6 · Reparaciones y corrección automática — depende del motor del compañero

1. El contrato de ediciones y su aplicador ya están implementados. Falta el catálogo del motor por `;`, `.`, `[]` y `()` conforme a la gramática real.
2. Implementado: panel de propuestas con explicación, vista previa acotada y aplicación explícita mediante el aplicador común.
3. Implementado: cada reparación aplicada es una sola operación de deshacer; no se guarda automáticamente.
4. Política y controles implementados: desactivado por defecto, reglas seleccionadas, Enter al final de línea, pausa y aceptación completa por el parser. Falta aprobar el catálogo real; la marca del motor no basta.
5. No aplicar respuestas antiguas, no tocar comentarios por error ni guardar automáticamente el documento.
6. Probar positivos y negativos por categoría; permitir deshacer y desactivar la automatización.

**Terminado cuando:** se demuestran correcciones realmente automáticas, no solo un botón manual, con deshacer y sin cambios incorrectos en casos ambiguos. Si un caso no es seguro, queda como propuesta explicada.

### E7 · Protección de archivos y experiencia de proyecto — implementado y verificado localmente

- [x] Versiones de disco al abrir/guardar: SHA-256, tamaño, fecha y dispositivo/inodo. Detectar cambios, borrados, reemplazos y errores de lectura en un trabajador con pausa de 1,5 s.
- [x] Aviso no destructivo por pestaña, recargar/conservar/copia y confirmación antes de descartar ediciones. Guardado con comparación fresca y nueva comprobación antes del reemplazo atómico.
- [x] F5 comprueba la versión de disco antes de compilar; los resultados de una fuente cambiada durante el proceso quedan identificados como antiguos.
- [x] Explorador de subcarpetas asíncrono con exclusiones, iconos nativos, actualización de archivos y navegación por teclado, sin seguir enlaces simbólicos.
- [x] Preferencias persistentes: proyecto, ccom, fuente, tamaño/visibilidad/distribución de paneles y análisis automático. Recuperación de JSON/campos inválidos y rutas desaparecidas.
- [x] Pruebas de foco, atajos, mínimo 800×520, cierre con monitor activo y revisión visual local al 100 % y 150 %. Las pruebas previas de terminales/análisis siguen correctas.
- [x] 28 pruebas nuevas; **124 combinadas correctas**. Funciones disponibles de F01–F10, F14–F17 y F21–F24 verificadas; F11–F13 dependen de los motores y no se declaran terminadas.

Revisión posterior (2026-11-06): creación y operaciones completas de carpetas, árbol expandible por tandas, ramas persistentes, atajos y revelado del archivo activo. Incluye selección con Ctrl/Shift/Ctrl+A, rectángulo desde el fondo, arrastre interno y operaciones de lotes que conservan buffers. Incluye tarjeta de arrastre con alfa, inserción visual entre hermanos, orden persistente y restauración alfabética. La barra lateral reemplaza Abrir carpeta y busca archivos .cls, carpetas y código con filtros, buffers, tareas cancelables y navegación. Ejecutar F6 está conectado al simulador, separado de F5, con validación de fuente/artefactos, copias por corrida y traza/variables finales. Contrato y aplicador de asistencia también implementados, con tipos compartidos y documentación para ambos motores. Coordinador y UI manuales de sugerencias y reparaciones también preparados; preferencias y política automática también preparadas; los motores reales y su catálogo revisado siguen pendientes. Suite combinada actual: **294 pruebas correctas**.

Detalle y prueba manual en [proyecto_archivos.md](guia_sistema_calside.md). No se promete un bloqueo exclusivo frente a escritores externos entre la última comprobación y el reemplazo: son operaciones distintas del sistema de archivos. La validación del equipo de defensa permanece en E8/E9. Preferencias de tema y timeout quedan fuera de esta entrega.

### E8 · Empaquetado y entrega de aplicación — compromiso de producto

1. Confirmar sistema operativo y arquitectura del equipo de defensa. Primera plataforma propuesta: Linux. Windows requiere su propia validación, incluida la terminal que hoy es Linux.
2. Elegir y documentar herramienta de empaquetado; generar un paquete reproducible desde un entorno definido.
3. Incluir iconos, recursos y dependencias de la UI. Decidir si ccom se distribuye junto a CALSIDE o se configura; probar su compatibilidad y rutas con espacios.
4. Abrir desde icono/acceso directo y verificar recursos sin depender del directorio de trabajo del repositorio.
5. Probar en otra máquina sin VS Code y sin pedir ejecución manual de Python. Documentar bibliotecas del sistema necesarias, instalación y límites reales.

**Terminado cuando:** el paquete abre como aplicación normal, edita/guarda/compila y conserva las funciones entregadas. No afirmar soporte multiplataforma sin probarlo.

### E9 · Validación final, documentación y defensa

1. Ejecutar `make check-ide` tras los cambios pertinentes y comprobar los tests del compilador si se modificó ccom.
2. Corregir fallos y completar la matriz manual de §9 en la plataforma acordada.
3. Ejecutar el guion de §10 con un compañero y un conjunto adicional de programas.
4. Actualizar instalación, arquitectura, funcionalidades, responsabilidades y límites. Preparar los diagramas requeridos por el proyecto; usar la arquitectura final, no un mockup antiguo.
5. Separar cambios ajenos al IDE al preparar revisión. `.gitignore` no oculta cambios ya versionados; no retirar pruebas compartidas del repo para limpiar un commit.
6. Preparar revisión/PR y publicación de la entrega cuando el usuario indique esas operaciones.

**Terminado cuando:** se cumple la lista de cierre de §12, el recorrido es reproducible en otra máquina y las limitaciones están declaradas. Un merge o una prueba aislada no sustituye esta validación.

### E10 · Mejoras posteriores, sin bloquear el cierre funcional

- Texto de sugerencia en gris dentro del editor (mejora de interfaz sobre M3): mostrar sin modificar el buffer, aceptar con Tab y cancelar al cambiar contexto. Reutilizar el motor C. Completar varios terminales, como `void;`, requiere definir una propuesta conjunta justificada por el backend; no añadir puntuación automáticamente desde Python. Pendiente de decisión/implementación.
- Esquina morada que se dobla y revela minimizar/maximizar/restaurar/cerrar; clic y teclado, animación breve y opción de movimiento reducido. Conservar comportamiento de ventana antes de sustituir controles nativos.
- Tema claro, refinamientos visuales y preferencias adicionales. Los iconos `.cls` compartidos con el plugin de VS Code ya están incorporados en explorador, pestañas, búsqueda y arrastre.
- Abrir artefactos del compilador y exponer opciones nuevas cuando estén realmente implementadas.
- Métricas por optimización cuando exista contrato de datos; integración con enlazado/IR cuando sus tareas estén listas.
- Emulación completa de terminal para aplicaciones curses, redimensionamiento PTY y otras plataformas.

#### Animación de apertura de CALSIDE — propuesta pendiente

Mejora visual de E10; se implementará después del cierre funcional. No es una tarea de implementación activa.

Secuencia propuesta:

1. Dibujar el contorno del hexágono, lado a lado, hasta completar los seis segmentos.
2. Al cerrarlo, iniciar un giro suave y desplazarlo hacia la izquierda.
3. Mientras gira y se desplaza, dibujar las letras **CALSIDE** de forma progresiva, una tras otra, a su derecha. La intención final es revelar los trazos de cada letra, no solo hacer aparecer caracteres completos.
4. Acomodar el conjunto en la cabecera y dar paso al editor, conservando la identidad ámbar y morada.

Duración objetivo inicial: aproximadamente **2 segundos** para la secuencia completa, ajustable durante el prototipo. La aplicación debe cargar en paralelo; la animación no añadirá una espera obligatoria. Se podrá saltar con clic o Esc, desactivar en preferencias y sustituir por una presentación estática cuando se use movimiento reducido. Al saltarla se mostrará directamente el estado final, con foco y documentos de apertura conservados.

Implementación prevista: geometría vectorial y animaciones de Qt/PySide6. El hexágono usa seis segmentos; la escritura real necesita definir los trazos vectoriales de cada letra. Primero se podrá validar el ritmo con letras que aparecen en secuencia, dejando el trazado completo para el acabado. La transición a la cabecera debe funcionar al redimensionar y con distintos escalados de pantalla.

Estimación orientativa, sujeta al diseño final: unas horas para un prototipo de dibujo/giro/desplazamiento y aparición de letras; **1–3 días de trabajo** para trazado de letras, acabado y transición. No constituye un plazo comprometido.

Antes de implementarla, decidir el estilo y recorrido del giro, los trazos de las letras y si se muestra en cada inicio o únicamente en el primer arranque. Validar con un prototipo visual. Para cerrarla, comprobar apertura normal y con archivo recibido por consola, salto durante cada tramo, preferencia desactivada, movimiento reducido, foco correcto y ausencia de retraso artificial en la carga.

## 9. Matriz de pruebas y evidencias

| Área | Evidencia mínima |
|---|---|
| Archivos | UTF-8/CRLF, rutas con espacios, nuevo, guardar como, duplicados, permisos, disco cambiado/borrado y cancelar cierre |
| Núcleo y adaptador | Tokens/categorías, comentarios, error fatal, texto incompleto, rangos Unicode y resultados por revisión |
| ccom manual/automático | Válidos, léxicos, sintácticos, semánticos, varios errores, E/S, timeout, cancelación y ausencia de escritura del fuente por análisis automático |
| Concurrencia | Escribir/cambiar/cerrar pestaña durante análisis; resultados tardíos descartados; widgets modificados solo desde hilo principal |
| Sugerencias | Contexto, comentarios, aceptar/cancelar, rango correcto y respuesta antigua |
| Correcciones | Casos positivos/negativos por delimitador, automatismo demostrable, deshacer y conservación de texto/disco |
| Clones | I/II entre archivos, renombramiento consistente, operadores distintos, negativos, buffers, exclusiones, límites y navegación tras cambio externo |
| Terminales | Sesiones/directorios independientes, historial, interrupción y cierre; declarar falta de emulación de pantalla completa |
| Interfaz | 1366×768 y 1920×1080, escalado, teclado, foco, paneles, descripciones y señales que no dependan solo de color |
| Empaquetado | Arranque desde icono, rutas de recursos, ccom y recorrido en otra máquina de la plataforma acordada |

Usar `unittest` para lógica y eventos relevantes, más revisión manual de la UI. Añadir pruebas que cubran riesgos reales; no duplicar implementación con tests triviales. Una modificación de documentación no exige volver a ejecutar toda la suite si no cambia código.

## 10. Guion final de demostración

1. Abrir CALSIDE desde su icono; abrir carpeta y dos archivos.
2. Mostrar pestañas, ocultar/recuperar archivos y mover el panel a la derecha.
3. Escribir un carácter inválido; mostrar diagnóstico en vivo sin guardar y corregirlo.
4. Mostrar sintaxis/semántica automáticas según E4 y distinguirlas de Compilar/F5.
5. Solicitar sugerencias, aceptar una con teclado y cancelar otra sin cambios.
6. Mostrar reparaciones y una corrección automática inequívoca; deshacerla y revisar casos ambiguos.
7. Compilar un programa válido y uno con error semántico; navegar al fragmento y volver a compilar correctamente.
8. Analizar `ide/ejemplos/clones`, comparar I/II y abrir ambos fragmentos; mostrar también un caso sin coincidencias.
9. Mostrar dos terminales independientes e interrumpir un comando de prueba.
10. Modificar un archivo externamente, conservar o recargar sin pérdida, y cancelar el cierre de un documento editado.

El guion debe ejecutarse contra funciones reales. No presentar datos simulados ni una biblioteca recibida como integración terminada.

## 11. Decisiones todavía abiertas y riesgos

| Tema | Resolución necesaria |
|---|---|
| Plataforma de entrega | Confirmar equipos, arquitectura y versiones; actualmente comprobados Python 3.14.9/PySide6 6.12.0 en Linux |
| ccom automático | Implementado y probado con --check. Mantener el contrato de cero artefactos al evolucionar el compilador |
| API de los motores | Parser implementado; compañero adapta sugerencias y reparaciones a `parser_predictivo.md`; verificar sus ediciones y mantener adaptadores |
| Correcciones seguras | Catálogo, disparadores, deshacer y casos que deben seguir siendo propuestas |
| Diagnósticos solapados | Lexer/ccom manual/automático ya tienen prioridad. Revisar de nuevo al incorporar el parser predictivo |
| Mejoras del proyecto 1 | Inventariar observaciones y aclarar qué mejoras exige §7.2 con el equipo/profesor antes de cerrar |
| Paquete ejecutable | Herramienta, recursos, dependencias del sistema y distribución de ccom |
| Plazo | El plan recibido proponía 6/11, 12/11 y 17/11 como hitos; reestimar con entregas y trabajo real. No son compromisos nuevos de este documento |

Riesgos a vigilar: divergencias entre gramáticas, resultados antiguos, diferencias UTF-8/Unicode/UTF-16, reparación de construcciones aún incompletas y pérdida por cambios externos. Mantener corpus compartido, revisión por documento y separación entre motor y presentación. La integración pendiente de los motores no bloquea el resto del IDE.

## 12. Lista de cierre del IDE

- [x] Base de aplicación propia, editor, archivos y paneles.
- [x] Compilación manual con ccom y diagnósticos gráficos.
- [x] Clones I/II reales entre archivos.
- [x] Terminales básicas y distribución adaptable.
- [x] Núcleo entrega 1 incorporado al repositorio y suite combinada correcta.
- [x] E3: lexer conectado al resaltado y a errores en vivo.
- [x] E4: análisis automático de ccom sin guardar ni generar artefactos.
- [x] E5 (motor base): parser de Saddy implementado, documentado y probado.
- [ ] E5 (restante): motor de sugerencias recibido y conectado a la interfaz.
- [ ] E6: reparaciones y corrección automática verificables.
- [x] E7: protección de disco, proyecto y preferencias implementados y verificados localmente; falta validación final en otra máquina.
- [ ] E8: paquete ejecutable probado en otra máquina.
- [ ] E9: corpus, documentación, diagramas y guion final completados.
- [ ] Aclaradas e incorporadas las mejoras pendientes del proyecto 1.

La animación de la esquina y las mejoras de E10 se mantienen separadas de esta lista de cierre funcional.

## 13. Mantenimiento del plan

Al terminar una etapa: actualizar estado y criterio de aceptación, registrar evidencia relevante y anotar la siguiente tarea disponible. Si cambia la arquitectura, reescribir la sección afectada; no acumular anexos que contradigan tareas anteriores. Conservar IDs F01–F26 para seguir requisitos y revisión.

Siguiente tarea de integración: **E5/E6, paso 7, conectar motores reales y revisar su catálogo**; los pasos 1–6 del IDE están preparados, incluidas preferencias, pausa y política automática. Sin catálogo revisado no hay reglas automáticas habilitadas en producción. En paralelo, el compañero implementa sugerencias sobre el parser disponible y el contrato compartido; la conexión real sigue pendiente. El mismo compañero implementará reparaciones para E6. E7 ya está implementado; mientras llegan los motores, puede avanzar E8 (empaquetado).


</details>


<a id="hist-plan-migracion-logica-c"></a>

<details>
<summary>plan_migracion_logica_c.md — documento sustituido</summary>

# CALSIDE: migración de la lógica del lenguaje a C

Fecha inicial: **2026-11-10**. Actualizado: **2026-11-11**. Este documento revisa la conexión inicial de motores y propone reutilizar el front-end C de ccom, conservando las excepciones de lenguaje aclaradas después por Saddy. Registra la indicación del profesor comunicada por Saddy: la aplicación/interfaz puede ser Python, pero la lógica del lenguaje debe estar en C y reutilizar el compilador existente.

Estado: **M1 implementado y verificado (2026-11-11)**: `ccom --check-json` y análisis automático estructurado. No se han fusionado los motores ni retirado componentes del IDE; M1B también implementado y verificado (2026-11-11): modos gráficos y preferencias. M2 también implementado y verificado (2026-11-11): léxico y resaltado desde Flex. M3 también implementado y verificado (2026-11-11): sugerencias C desde Bison/LAC conectadas al IDE. M4 también implementado y verificado (2026-11-11): reparaciones C, panel y validación automática nativa; regla opcional de punto y coma. M5 también implementado y verificado (2026-11-11): clones Tipo I/II en C y envío de múltiples buffers. M6 implementado y verificado (2026-11-11): limpieza, comentarios, documentación y recorrido gráfico sin analizadores Python. M7 implementado y verificado (2026-11-11): recuperación conservadora C, varios errores sintácticos y bloqueo de fases sobre AST parcial.

**Aclaración posterior de Saddy (2026-11-11):** el simulador y el LL(1) pueden seguir en Python. Migrarlos no es una obligación del enunciado ni de este plan. Las propuestas de retirar duplicación se evalúan por responsabilidad y mantenimiento, sin convertir esa autorización en otra restricción.

## 1. Qué se revisó y qué se encontró

Copia de trabajo: rama `feat/calside`, commit `8fbad0c` al comenzar la revisión. Árbol de trabajo limpio. Se actualizó únicamente la referencia remota de `feat/reparaciones-sugerencias` mediante fetch para inspeccionarla; no se cambió de rama ni se hizo merge.

Entrega encontrada en `origin/feat/reparaciones-sugerencias`, hasta `56a7347`:

- `ide/nucleo/sugerencias.py`: sugerencias predictivas basadas en el parser Python.
- `ide/nucleo/correccion.py`: propuestas de inserción de `;`, `.`, `]` y `)`, con reanálisis y clasificación conservadora.
- Documentación y pruebas de ambos motores.

Hay material reutilizable: casos de aceptación, catálogo de cierres, criterios de ambigüedad, filtrado de prefijos, rangos y comportamiento esperado. El código entregado no se puede conectar directamente como si ya cumpliera el contrato del IDE:

| Diferencia observada | Consecuencia |
| --- | --- |
| Los motores declaran sus propias dataclasses en lugar de importar `ide/nucleo/asistencia.py`. | El coordinador actual rechazaría sus respuestas por tipo. |
| Sugerencias usa estados del parser como `prefijo_valido` y una estructura de resultado distinta. | Hay que traducir estados/metadatos al protocolo de servicio; no basta importar la función. |
| Reparaciones usa `propuestas`, `sin_error`, `sin_propuesta`, precondiciones descriptivas y un objeto de validación propio. | El aplicador requiere precondiciones verificables por rango y una respuesta normalizada. |
| `proponer_reparaciones` no admite `cancelar`. | La llamada actual del trabajador produciría un error de argumento. |
| Ambos dependen del lexer/parser Python paralelo. | Una conexión temporal completa en Python perpetuaría la duplicación que se quiere retirar. |
| Las pruebas de reparación son funciones pytest; `make check-ide` descubre unittest. | No quedan ejecutadas automáticamente por esa suite sin adaptar el arnés. |
| La inserción automática entregada se coloca ante el siguiente token, por ejemplo antes de `E`; la política actual del IDE exige el final de la línea confirmada. | Hay que acordar y probar una ubicación de edición; no habilitar reglas solo por coincidir el nombre del código. |

Las 294 pruebas de la entrega anterior son una referencia histórica. No constituyen validación de esta entrega remota ni de una migración a C todavía inexistente.

### Comprobaciones ejecutadas durante la auditoría

Se ejecutaron consultas `--check` con el binario disponible y fuentes temporales, sin reconstruir ni modificar el compilador:

| Entrada de prueba | Resultado observado |
| --- | --- |
| Dos asignaciones a nombres no declarados en un programa sintácticamente válido. | 2 diagnósticos semánticos, retorno 1. |
| Dos declaraciones con expresiones incompletas. | 1 diagnóstico sintáctico, retorno 1; no se llega al segundo. |
| Dos caracteres `@` antes de una función válida. | 2 diagnósticos léxicos, retorno 1. |

Las consultas no generaron artefactos adicionales. Esto prueba esos casos concretos; no garantiza recuperación general ni que todas las combinaciones de errores lleguen a reportarse.

También se cargaron los motores remotos en un directorio temporal, sin fusionarlos:

- Las **9 pruebas unittest de sugerencias pasaron** con el núcleo Python disponible.
- Se reprodujo el rechazo de su respuesta por `validar_respuesta`: «El motor devolvió un tipo ajeno al contrato compartido».
- Se verificó la firma de reparaciones, que no incluye `cancelar`.
- Las pruebas pytest de reparaciones **no se ejecutaron** porque pytest no está instalado. No se instalaron dependencias. Su código se revisó; no se presentan como pruebas aprobadas.

## 2. Frontera de responsabilidades propuesta

### Permanece en Python/PySide6

- Ventana, pestañas, editor, explorador, arrastre y orden visual.
- Abrir/guardar, versiones de disco, conflictos, preferencias y diálogos.
- Búsqueda de nombres/rutas/texto literal como función del gestor de proyecto.
- Procesos, temporizadores, cancelación, instantáneas y control de vigencia.
- Renderizado de colores/diagnósticos y vistas previas.
- Portapapeles, terminales y navegación.
- Conversión UTF-8/índices Python/UTF-16 y validaciones genéricas de rangos.
- Aplicación de las ediciones recibidas y un bloque de Deshacer por propuesta.
- Preferencias y autorización del usuario: si permite consultar/aplicar una regla.

Estas tareas administran la aplicación o presentan datos. No deciden qué significa Calser, qué token corresponde, qué espera la gramática ni qué reparación resuelve un error.

### Debe estar en C, reutilizando el compilador

- Tokenización y clasificación del lenguaje: `grammar/lexer.l`.
- Sintaxis, tokens esperados y contexto de análisis: `grammar/parser.y` y Bison/LAC.
- Semántica, declaraciones, tipos y alcances: módulos existentes de `src/semantic*`.
- Generación de diagnósticos del lenguaje, no su renderizado en Qt.
- Selección y validación de sugerencias contextuales.
- Generación, clasificación y reanálisis de reparaciones.
- Reglas que deciden si una reparación es automática según el lenguaje.
- Comparación de clones basada en tokens Calser, reutilizando el flujo léxico C.

No se propone traducir `ide/nucleo/gramatica.py`, `tabla.py` y `parser.py` a otro parser C paralelo. Se deben aprovechar las reglas Flex/Bison existentes. El contexto predictivo de escritura se obtendrá desde esas reglas; Bison ya usa LAC para consultar tokens esperados en errores.

`tools/sim_asm.py` y el LL(1) tienen autorización para seguir en Python, según la aclaración posterior de Saddy. Se conservan; cualquier sustitución del LL(1) se decidirá por reutilización y necesidad de contexto, no por una obligación de traducirlo a C.

## 3. Arquitectura de destino recomendada

```text
PySide6: editor, proyecto, preferencias y paneles
    │ instantánea, operación y cursor
    ▼
Adaptador de transporte Python
    │ QProcess + argumentos + fuente temporal
    ▼
Servicio de análisis en C, integrado con ccom
    ├── lexer Flex existente → tokens/colores y posiciones
    ├── parser Bison existente → sintaxis/contexto/esperados
    ├── semántico existente → problemas de declaraciones/tipos
    ├── sugerencias → opciones concretas del contexto
    ├── reparaciones → ediciones verificadas sobre copias
    └── clones → comparación de tokens entre documentos
    │ respuesta estructurada y versionada
    ▼
Adaptador Python: validar protocolo y convertir posiciones
    ▼
Paneles, resaltado y aplicación con Deshacer
```

### Alternativas de conexión

| Opción | Ventajas | Costes |
| --- | --- | --- |
| **Proceso C + JSON versionado (recomendada)** | Encaja con `QProcess`, aísla fallos y estado global de Flex/Bison, permite cancelar el proceso y depurar desde consola. | Inicio de proceso y preparación de instantáneas; requiere un protocolo claro. |
| Biblioteca C cargada desde Python | Evita iniciar procesos y permite llamadas en memoria. | Gestión de memoria/ABI, errores que pueden cerrar el IDE, reinicio del estado global y cancelación más complejos. |
| Adaptar primero todo a Python y migrarlo después | Permite probar antes la UI con el comportamiento entregado. | Duplica trabajo y mantiene temporalmente la arquitectura descartada. No es el recorrido recomendado. |

La recomendación inicial es mantener un proceso por solicitud y analizar su instantánea. No introducir todavía un servidor persistente con estado de varios documentos. Si el rendimiento medido exige persistencia, se podrá ampliar sin cambiar los modelos de la UI.

No se debe vincular un parser nuevo solo porque el servicio tenga otro ejecutable: la implementación debe enlazar o compartir los módulos reales de ccom.

## 4. Contrato del servicio: decisiones que deben fijarse primero

M1 concreta la operación `check` con `--check-json`; véase el [protocolo implementado](../../docs/ccom_servicios_ide.md#check). M2–M5 ya concretan las demás operaciones; los resultados y contratos vigentes aparecen al final de este documento.

- Versión explícita del protocolo y operación identificada.
- Resultado con estado y **listas** de tokens, diagnósticos, sugerencias o reparaciones según la operación.
- stdout reservado para respuesta estructurada; logs separados en stderr.
- Mensaje de fallo de transporte/servicio distinto de un error del programa del usuario.
- Códigos y categoría de diagnóstico estables; no clasificar ni reconstruir reglas a partir de una frase humana.
- Rangos del original `[inicio, fin)` en bytes UTF-8, con unidad declarada. Línea/columna, cuando se incluyan, también deben declarar sus convenciones.
- El adaptador Python convierte bytes a posiciones Python/Qt antes de construir los modelos de presentación. En **ediciones**, una frontera dentro de una secuencia UTF-8 se rechaza: nunca se corta un carácter ni se redondea un cambio.
- Los **diagnósticos** necesitan otra política: el lexer actual reporta errores por byte y puede señalar solo parte de un carácter UTF-8. Conservar el rango original y ampliar explícitamente solo su marca visual al carácter completo evita rechazar toda la respuesta. Esta conversión de presentación debe tener pruebas y no modifica la edición propuesta.
- Documento y revisión se conservan en el contexto del solicitante, aunque el compilador solo vea una copia temporal. Una respuesta no autoriza a modificar un documento distinto o más reciente.
- Propuestas con código, explicación, ediciones, precondiciones por rango, validación y elegibilidad automática. La preferencia del usuario sigue siendo necesaria además de la elegibilidad del motor.
- Estado de límite/cancelación y señal de resultados truncados. Ninguna respuesta incompleta se presenta como análisis correcto.
- Tamaños, duración, cantidad máxima de opciones y limpieza de temporales acotados.

La primera operación ya comprueba un buffer con el front-end C real mediante `--check-json`, sin crear binarios. `--check` y la CLI clásica conservan su comportamiento. El contrato de las operaciones futuras se ampliará explícitamente.

Antes de modificar `src/main.c`, revisar su evolución en las ramas de integración: actualmente hay cambios remotos de representación intermedia/TAC que son ajenos a esta tarea. No fusionarlos como efecto lateral de traer motores.

## 5. Varios errores: soporte del contrato y recuperación son trabajos distintos

La UI actual ya muestra listas de problemas y ccom puede producir varios diagnósticos, especialmente semánticos. Eso no significa que recupere múltiples errores sintácticos independientes.

Al comenzar M1, Bison informaba el primer fallo sin recuperación. M7 incorpora sincronización conservadora en el parser real, sin prometer recorrer todo programa estructuralmente dañado. El parser LL(1) Python tampoco lo resuelve: su contrato se limita al primer error.

Se debe preparar desde M1 una respuesta `diagnosticos: [...]`. Más adelante, en M7, añadir recuperación **en C** con puntos de sincronización adecuados a Calser. No ejecutar artificialmente el mismo parser sobre líneas aisladas ni aplicar parches al documento para fabricar una lista de errores.

La recuperación deberá tener:

- avance garantizado, límites de errores y protección frente a bucles;
- sincronización con conocimiento de delimitadores/bloques;
- supresión de cascadas previsibles;
- política explícita para AST parcial y ejecución del semántico;
- prohibición de generar código si hay errores;
- pruebas con varios errores reales, errores anidados, EOF y comentarios sin cerrar.

Preparar una lista JSON ahora **no completa** esa recuperación. Es una decisión de formato que evita rehacer la UI cuando llegue esa iteración.

## 6. Etapas de trabajo

### M0 — Auditoría y nuevo reparto

- [x] Localizar y actualizar la referencia de los motores entregados.
- [x] Identificar dependencias, modelos incompatibles y problema de cancelación.
- [x] Revisar las entradas de ccom, diagnósticos y Bison/LAC existentes.
- [x] Comprobar casos concretos de diagnósticos múltiples y la incompatibilidad del contrato; ejecutar las pruebas unittest entregadas de sugerencias.
- [x] Definir en este documento frontera y recorrido propuestos.
- [x] Aclarado por Saddy: simulador y LL(1) pueden continuar en Python.

### M1 — Servicio C y diagnósticos estructurados

- [x] Fijar esquema, estados, rangos, límites y compatibilidad de CLI.
- [x] Exponer diagnósticos desde `src/comun/diagnostics.c` sin inferirlos con regex en Python.
- [x] Reutilizar las fases existentes de parseo/semántica sin generación de código.
- [x] Adaptar `ide/ccom.py` y `interfaz/comprobacion.py` al servicio y sus fallos explícitos.
- [x] Probar fuente sin guardar, rutas con espacios, Unicode, falta de main, varios errores semánticos, cancelación y cero artefactos.

Criterio de cierre: el IDE recibe problemas estructurados del compilador real y no contiene reglas semánticas propias. F5/F6 y la CLI previa siguen funcionando.

Verificado: 312 pruebas del IDE, suite completa `make check`, contrato `make check-analysis-json` y 91 fuentes reales con respuestas compatibles. Protocolo, límites y alcance en [ccom_check_json.md](../../docs/ccom_servicios_ide.md#check). F5 mantiene el lector textual; M1 cambia el análisis automático. No añade recuperación de errores sintácticos ni sustituye el LL(1).

### M1B — Opciones de compilación desde la interfaz (requisito del profesor)

- [x] Convertir Compilar en un control con acción principal y menú de modos; F5 ejecuta el modo seleccionado y conserva uno predeterminado útil.
- [x] Permitir elegir análisis sin salidas, compilación completa, exportación de AST y ensamblado de una entrada `.asm`, utilizando solo opciones existentes y verificadas de ccom.
- [x] Exponer destino de salida y opciones compatibles de listados, tiempo, progreso y organización de bundles.
- [x] Mantener el mismo comportamiento desde el botón y el menú Compilar; mostrar modo activo, resultados y artefactos generados.
- [x] Impedir combinaciones incompatibles y distinguir los modos que no producen una compilación ejecutable.
- [x] Preparar la incorporación de IR/TAC y optimizaciones únicamente cuando estén integradas y comprobadas en el ccom utilizado.
- [x] Probar los comandos reales generados por cada perfil, rutas con espacios, cancelación, fallos y disponibilidad de F6.

Criterio de cierre: el usuario puede seleccionar y ejecutar los modos desde la interfaz sin escribir comandos. Los modos no se simulan en Python: cada uno solicita la operación correspondiente a ccom. Detalles y ejemplos en la sección 13.

Es una tarea de interfaz que puede adelantarse sin esperar a los motores, una vez fijado su modelo de ejecución y compatibilidad. No depende de que se complete toda la migración.

### M2 — Tokens y resaltado desde Flex

- [x] Instrumentar el lexer existente para exponer clasificación y rangos, incluidos comentarios para presentación, sin otra tabla de reglas Python.
- [x] Transportar tokens/diagnósticos léxicos en la respuesta C y convertir posiciones en el adaptador.
- [x] Reemplazar el recorrido Python de análisis vivo y la dependencia de `ide/nucleo/lexico.py` en ese recorrido vivo (clones y asistencia tienen sus fases posteriores).
- [x] Mantener revisión, cancelación, colores, Deshacer y comportamiento sin guardar.

Criterio de cierre: una corrección en `grammar/lexer.l` es la fuente de las reglas usadas por compilación y editor.

### M3 — Sugerencias contextuales en C

- [x] Trasladar comportamiento y casos de la entrega, no sus dataclasses duplicadas.
- [x] Obtener fragmento/comentario del lexer C y contexto/esperados del parser existente.
- [x] Distinguir prefijo válido de programa completo; no considerar cualquier EOF como error previo.
- [x] Filtrar/rankear opciones en C, sin inventar `id`, `num` o símbolos declarados que no se hayan consultado.
- [x] Conectar respuesta al panel actual de sugerencias y preferencias de pausa.
- [x] Migrar pruebas de prefijos, cursor en medio de palabra, comentarios, límites, errores previos y cancelación.

Criterio de cierre: Ctrl+Espacio y pausa funcionan con el motor C real; no requieren la gramática/tabla/parser Python.

### M4 — Reparaciones en C y autorización de reglas

- [x] Usar diagnósticos y expectativas estructuradas del parser C para construir candidatos.
- [x] Reanalizar copias con el mismo parser, reiniciando y liberando su estado entre intentos.
- [x] Trasladar pruebas de `;`, `.`, `]`, `)` y los negativos del compañero.
- [x] Normalizar precondiciones y validación; no convertir instrucciones descriptivas en una precondición vacía que siempre pase.
- [x] Resolver la diferencia entre inserción ante el siguiente token e inserción al final de la línea previa.
- [x] Conectar panel, vista previa y aplicador existentes.
- [x] Mover a C la validación del lenguaje que hoy hace `politica_asistencia.py`; conservar en Python preferencias, vigencia y aplicación.
- [x] Habilitar solo reglas revisadas, con permiso del usuario, sin guardar ni encadenar reparaciones y con un Deshacer.

Criterio de cierre: consultas manuales y ruta automática usan el motor C y sus reglas comprobadas; no hay análisis del lenguaje en Python.

### M5 — Clones con el flujo léxico C

- [x] Reemplazar el tokenizador y comparación Python de `ide/clones.py` por lógica C.
- [x] Reutilizar tokens reales para Tipo I/II y preservar correspondencia consistente de nombres/literales.
- [x] Definir envío de varias instantáneas, incluidas las de buffers sin guardar, sin perder límites ni cancelación.
- [x] Mantener el panel de comparación/navegación en Python.

Criterio de cierre: no existe un segundo tokenizador Calser productivo para clones.

### M6 — Retiro de duplicados, comentarios y comprensión

- [x] Revisar duplicaciones y retirar solo las sustituidas por recorridos comprobados; el LL(1) está autorizado en Python y no se elimina por obligación de lenguaje. Documentar su papel si se conserva como apoyo.
- [x] Decidir explícitamente si los archivos anteriores se eliminan o conservan como material histórico fuera de producción; no seguir presentándolos como backend actual.
- [x] Actualizar pruebas del IDE: casos del lenguaje contra el servicio C; dobles solo para aislar transporte/UI.
- [x] Comentar cada archivo C y adaptador: propósito, consumidores, memoria, unidades, estado global, errores y cancelación.
- [x] Actualizar `guia_sistema_calside.md`, contratos, plan y guías de motores.
- [x] Ejecutar las suites afectadas del compilador y del IDE, y un recorrido gráfico automatizado de varios documentos, con captura inspeccionada y guía de repetición.

Criterio de cierre: mapa y comentarios describen el sistema real; el análisis principal y los motores reutilizan C, el papel del LL(1) Python permitido queda explícito y cada recorrido tiene pruebas.

### M7 — Recuperación sintáctica conservadora

- [x] Añadir sincronización al parser C para producir varios errores fiables en una misma consulta.
- [x] Definir semántica sobre entrada recuperada y evitar generación con errores.
- [x] Probar precisión, cascadas, progreso y límites.

Criterio de cierre: un archivo con varios errores sintácticos independientes produce una lista útil en una sola consulta, sin depender de corregir uno para descubrir el siguiente.

## 7. Qué se conserva y cómo se verifica

Se conserva la mayor parte de la experiencia construida: ventana, editor, explorador, documentos, protección de disco, búsqueda, terminales, F5/F6, problemas, propuestas, vista previa, preferencias y Deshacer. Cambia la fuente de decisiones del lenguaje y el transporte de sus resultados.

La migración debe realizarse por recorridos completos, manteniendo el IDE utilizable entre etapas. Cada etapa requiere una prueba que pase por el servicio C real, además de las pruebas aisladas de widgets. No basta sustituir una función por un mock y afirmar que el backend fue trasladado.

Matriz mínima de regresión: documento sin nombre, fuente guardada/modificada, cambio de pestaña durante consulta, cierre, Unicode, comentarios, varias propuestas, cancelación/timeout, motor o ccom incompatible, error sintáctico, error semántico, compilación y simulación correctas, origen/artefactos intactos durante análisis y Deshacer de una reparación.

## 8. Orden de comentarios durante la migración

1. API/contrato del servicio y `diagnostics.h`/`diagnostics.c`.
2. Entradas de lexer/parser y control de estado/memoria.
3. Adaptador Python y comprobador.
4. Motores C de sugerencias y reparación, con sus ejemplos y pruebas.
5. Clones C.
6. Coordinadores y paneles que reciben respuestas.
7. Retiro y actualización de referencias antiguas.

Los comentarios añadidos a los módulos de archivos, preferencias, diagnóstico y conversión del IDE siguen siendo útiles. Los que describen un motor Python productivo se actualizarán cuando se cambie su consumidor; mientras tanto las guías deben advertir que esa arquitectura está en migración.


## 9. Detalles técnicos que afectan al diseño

### 9.1 El estado del compilador no es todavía una biblioteca reentrante

`grammar/lexer.l` conserva columna, contador de errores, error fatal y datos de comentario; Flex agrega sus buffers, línea y estado de escaneo. `grammar/parser.y` mantiene `root` y observación de bloques. El semántico conserva ámbitos y función actual.

Un proceso por solicitud aísla solicitudes de documentos distintos, pero no soluciona por sí solo el reanálisis de varios candidatos **dentro de una misma solicitud** de reparación. Hace falta un ciclo de vida explícito: inicializar, analizar, extraer datos, liberar y reiniciar para el candidato siguiente. `yyrestart` no debe suponerse un reinicio de todas las variables propias del lexer.

`compile_release` libera la raíz solo cuando se llegó a registrar un AST correcto. Al incorporar consultas parciales, errores recuperados y candidatos repetidos se debe revisar también la propiedad de los nodos/lexemas descartados. Las pruebas de memoria y fallos son parte de la integración, antes de considerar un servidor persistente.

### 9.2 Bison ya calcula expectativas; hay que exponerlas como datos

En `grammar/parser.y`, `yyreport_syntax_error` consulta `yypcontext_expected_tokens` y luego elige un mensaje humano. Los detalles de ese contexto pertenecen al parser generado y no son una API que Python pueda llamar directamente.

La extensión correcta se escribe en las fuentes de la gramática o en una interfaz C que esta invoque. No editar `build/generated/parser.tab.c` o `lex.yy.c`: se regeneran con Bison/Flex.

Para sugerencias, el caso `F main [] : vo|` requiere identificar `vo` como fragmento, consultar el contexto **antes** de ese fragmento y filtrar continuaciones como `void`. No se analiza siempre el documento completo ni se interpreta el prefijo incompleto como una reparación. Si el cursor está en medio de una palabra, el reemplazo debe incluir el sufijo del mismo token, sin duplicarlo.

Una posibilidad de implementación es analizar el prefijo con un EOF de consulta y distinguir un bloqueo anterior de las expectativas al llegar al cursor. Otra es exponer el estado de un parser incremental generado por la misma gramática. La decisión se valida con una prueba técnica en M3: prefijo vacío, fragmento, programa ya completo, EOF y comentarios. No se requiere reconstruir FIRST/FOLLOW ni otra gramática para hacerlo.

### 9.3 Posición real del error y posición elegida para mostrarlo

El parser actual puede mostrar «falta ;» al final de la sentencia anterior, aunque el token que provoca la detección esté en la línea siguiente. En EOF también puede destacar la apertura de un bloque.

El servicio debe conservar por separado:

- token inesperado y su rango real;
- tokens esperados;
- rango que se quiere destacar al usuario;
- posición y precondiciones de cada edición propuesta.

Si el motor reutiliza únicamente el rango del mensaje visible para insertar, puede reparar en otro lugar. El JSON debe transportar esta diferencia desde C, sin que Python tenga que adivinarla por el texto del mensaje.

### 9.4 Resultado parcial no equivale a permiso automático

El motor entregado admite una reparación manual si el parser supera el error original, aunque encuentre otro después. Para automatización exige además aceptación completa y otros filtros. Ese comportamiento puede conservarse, pero la respuesta debe indicar con claridad «resuelve este error» frente a «programa sintácticamente aceptado».

La condición entregada `esperados.count(';') == 1` comprueba que `;` aparece una vez; no demuestra que sea el único terminal esperado. Debe contrastarse con contraejemplos antes de usarla como prueba de elección inequívoca en C.

Tampoco se debe relajar la política de ubicación del IDE para hacer pasar la marca automática sin revisar el caso de uso. La ubicación legible al final de la línea previa y la inserción ante el siguiente token pueden tener sintaxis equivalente y, aun así, producir una experiencia distinta.

### 9.5 Evitar que la dependencia antigua siga entrando indirectamente

Hoy `ide/preferencias.py` importa la validación de configuración de `ide/politica_asistencia.py`, y este último importa `ide/nucleo/parser.py`. Por esa cadena, incluso cargar preferencias puede cargar el parser Python.

Retirar el uso del parser no consiste solo en cambiar el botón de sugerencias: hay que separar configuración/permisos de evaluación del lenguaje y comprobar importaciones transitivas. Los modelos genéricos de edición/contexto pueden seguir siendo dataclasses Python, porque no implementan gramática ni semántica.

## 10. Mapa de cambios por archivo

Los nombres de módulos C nuevos son propuestas, no archivos creados. Se elegirán al fijar el contrato para que cada módulo tenga una responsabilidad verificable.

| Archivo o grupo actual | Tratamiento previsto | Se conserva en Python |
| --- | --- | --- |
| `ide/main.py`, `ide/__init__.py` | Mantener entrada y paquete. | Arranque de la aplicación. |
| `ide/archivos.py`, `ide/interfaz/disco.py`, `ide/interfaz/proyecto.py` | Mantener, adaptando referencias solo si cambian los servicios. | Guardado, versiones, conflictos y proyecto. |
| `ide/preferencias.py`, `interfaz/preferencias_asistencia.py` | Mantener sin dependencia transitiva del parser Python. | Configuración y permiso del usuario. |
| `ide/diagnosticos.py` | Mantener como modelo de presentación; revisar campos de códigos/rangos originales si se necesitan. | Datos y conversión para la UI, sin clasificación inferida del lenguaje. |
| `ide/ccom.py` | Sustituir lectura productiva de mensajes humanos por validación del protocolo estructurado. | Localización, transporte/adaptación de datos. |
| `ide/adaptador_nucleo.py` | Reemplazar llamada al lexer Python y recorrido propio de comentarios. | Conversión de posiciones/formatos de la respuesta C. |
| `ide/ediciones.py`, `interfaz/ediciones.py` | Mantener las validaciones y aplicación genéricas. | Rangos, Unicode, contexto, precondiciones textuales y Deshacer. |
| `ide/busqueda.py`, `interfaz/busqueda.py` | Mantener búsqueda literal del proyecto. | Recorrido de archivos y navegación, sin análisis del lenguaje. |
| `ide/clones.py`, `interfaz/clones.py` | Trasladar tokenización/comparación a C; conservar el panel y reunión de instantáneas. | Solicitud, progreso, comparación visual y navegación. |
| `ide/politica_asistencia.py` | Dividir configuración/autorización y evaluación. El reanálisis y los criterios de Calser pasan a C. | Ajustes, reglas permitidas y condiciones de vigencia de la UI. |
| `ide/nucleo/lexico.py`, `gramatica.py`, `tabla.py`, `parser.py` | Revisar duplicaciones al integrar servicios, sin obligación de migrar/eliminar el LL(1) Python autorizado. | Un apoyo LL(1) con papel explícito si se conserva. |
| `ide/nucleo/asistencia.py` | Conservar o reubicar como modelos genéricos del adaptador, evitando cambios cosméticos durante el primer paso. | Contexto, resultados y ediciones, no motores. |
| Motores Python de la rama entregada | Usar comportamiento/pruebas como referencia del traslado; no integrarlos como backend definitivo. | No quedan algoritmos del lenguaje en la UI. |
| `interfaz/analisis.py`, `interfaz/comprobacion.py`, `interfaz/asistencia.py` | Adaptar coordinadores a procesos/respuestas del servicio C y revisar prioridades para no duplicar consultas innecesarias. | Cola, timeout, cancelación y revisión. |
| `interfaz/sugerencias.py`, `interfaz/reparaciones.py` | Conservar widgets e interacción; cambiar el proveedor de datos. | Listas, vista previa, aceptar/cancelar y foco. |
| `interfaz/ventana.py`, `interfaz/editor.py` | Actualizar conexiones y recepción de diagnósticos/tokens sin replicar reglas. | Documento, pestañas, resaltado visual y acciones. |
| `interfaz/explorador.py`, `acciones_archivos.py`, `seleccion_archivos.py`, `orden_explorador.py`, `operaciones_carpetas.py`, `iconos.py` | Mantener comportamiento del gestor de proyecto. | Árbol, archivos, lotes, arrastre y recursos. |
| `interfaz/ejecucion.py`, `terminal.py`, `terminal_pty.py` | Mantener integración de herramientas; el simulador Python está autorizado y se conserva. | Control de procesos y presentación. |
| `src/comun/diagnostics.c`, `include/comun/diagnostics.h` | Añadir salida/colección estructurada reutilizable con lista de diagnósticos. | Nada de diagnóstico del lenguaje inferido por regex. |
| `grammar/lexer.l`, `grammar/parser.y` | Exponer rangos, tokens, contexto y reinicio; posteriormente recuperación. | No se copian sus reglas a Python. |
| `src/main.c`, `makefile` | Integrar servicio, opciones compatibles, construcción y pruebas. | No cambia la responsabilidad de F5 como invocación de ccom. |
| Módulos `src/semantic*` e interfaces existentes | Reutilizar y revisar ciclo de vida; no reconstruir semántica para el IDE. | Solo se presentan sus resultados. |

## 11. Ejemplos de aceptación para revisar el diseño

| Acción | Resultado esperado | Quién decide |
| --- | --- | --- |
| Escribir `F main [] : vo` y solicitar sugerencias. | Opción `void` con rango exacto del fragmento; aceptar no duplica el sufijo. | Lexer/contexto/motor C; Python presenta y aplica. |
| Solicitar opciones dentro de `#[ comentario`. | Bloqueo contextual, sin lista inventada. | Estado léxico C. |
| Omitir `;` antes de `E ;`. | Propuesta de inserción validada; vista previa explica qué resuelve. | Parser/motor C. |
| El motor devuelve dos reparaciones plausibles. | Revisión manual; no elegir automáticamente la primera. | Motor C informa ambigüedad; Python respeta permiso y elección. |
| La fuente tiene dos nombres sin declarar. | Lista con ambos problemas semánticos en una consulta. | Semántico existente C. |
| La fuente tiene dos errores sintácticos independientes. | En M1 se informa la capacidad actual sin fingir recuperación. En M7 se exige progreso y dos diagnósticos fiables. | Parser C. |
| Un carácter no ASCII inválido para Calser produce errores por byte. | La respuesta conserva rangos nativos y la UI marca caracteres completos de forma explícita. | C diagnostica; Python adapta la presentación. |
| El usuario escribe después de enviar una consulta. | Se descarta la respuesta anterior aunque el proceso termine con éxito. | Contexto/revisión de la UI. |
| Un resultado solicita una edición que corta una secuencia UTF-8. | Se rechaza la propuesta y se informa fallo de contrato, sin cambiar el documento. | Validación genérica del adaptador. |
| Buscar un nombre de carpeta o copiar un archivo. | Continúa funcionando en Python sin pasar por el compilador. | Gestor de proyecto. |

## 12. Decisiones a revisar antes de implementar

1. **Transporte:** proceso C + JSON (recomendado) frente a biblioteca C. La justificación está en la sección 3.
2. **Superficie CLI:** modo de servicio integrado en `ccom` (recomendado inicialmente) frente a ejecutable auxiliar que enlace los mismos objetos. Ambos deben reutilizar las fuentes, no copiar el parser.
3. **Unidad de intercambio:** bytes UTF-8 con distinción entre rangos diagnósticos y ediciones (recomendado) frente a convertir todas las posiciones en C a índices de caracteres. Se debe elegir una sola convención y probarla.
4. **Orden de integración:** análisis estructurado → opciones gráficas de compilación (M1B, adelantable) → tokens/resaltado → sugerencias → reparaciones → clones → retiro/documentación (recomendado). Conectar motores Python primero se deja como alternativa descartable, con coste adicional explicado.
5. **LL(1) Python:** está permitido; decidir su papel de apoyo y revisar duplicación antes de retirar componentes. No eliminar referencias antes de probar los reemplazos que se acuerden.
6. **Simulador y LL(1):** aclarado por Saddy, pueden permanecer en Python. No existe tarea obligatoria de migrarlos.

M1 ya implementa el proceso y contrato de análisis; las recomendaciones restantes son diseño y se revisarán antes de su etapa.


## 13. Opciones de compilación desde la interfaz

**Requisito añadido por Saddy el 2026-11-11, tras la indicación del profesor:** las opciones disponibles desde terminal también deben poder seleccionarse desde el botón/menú Compilar. M1B implementado: esta sección documenta el control disponible y sus criterios de validación.

### 13.1 Diseño propuesto del control

- **Compilar ▾**: la acción principal ejecuta el modo elegido; la flecha abre la selección.
- **F5** ejecuta ese mismo modo, sin una ruta de código distinta.
- Mostrar el modo activo junto al control o en su ayuda visible para que F5 no resulte ambiguo.
- **Opciones de compilación…** abre un diálogo con tipo de operación, destino y casillas compatibles; recordar la selección en preferencias y volver a validarla al cambiar el compilador.
- Mantener **Ejecutar/F6** como acción separada. Elegir análisis o exportar un AST no ejecuta el programa ni crea una compilación ejecutable.
- Perfil inicial recomendado: **Compilación completa con listado `.asm`**, equivalente al F5 actual, para que posteriormente pueda utilizarse F6.

### 13.2 Modos respaldados por el ccom de esta rama

| Modo de la interfaz | Opciones solicitadas | Qué hace y qué debe mostrar |
| --- | --- | --- |
| **Analizar sin generar archivos** | `--check fuente.cls` | Léxico, sintaxis y semántica. Resultado de análisis y diagnósticos; no crea binario, AST JSON ni listado. |
| **Compilar programa** | `fuente.cls -o destino.bin` | Compilación normal y salidas `.bin`, `.hex`, `.mem`. Casilla para añadir el listado `.asm` mediante `-s`. |
| **Exportar AST a JSON** | `--ast-json destino.json fuente.cls` | Perfil de exportación sintáctica: sin `-s`, `-m` o `-p`, esta bandera termina después de sintaxis. No certificar análisis semántico, generación ni ejecución. |
| **Ensamblar archivo `.asm`** | `-x entrada.asm -o destino.bin` | Ensambla una entrada elegida explícitamente; no intenta tratar el `.cls` activo como ensamblador. El selector de entrada debe admitir `.asm`. |

No añadir «objeto `.o`» como si fuera una salida existente: esta versión de ccom no expone un modo `-c` de ese tipo. Si se requiere, es una ampliación del compilador con su propia tarea y pruebas.

### 13.3 Opciones adicionales

| Control | Bandera o comportamiento | Compatibilidad que debe respetar |
| --- | --- | --- |
| Carpeta/nombre de salida | `-o` | Disponible para compilación/ensamblado; deshabilitado en análisis y sustituido por destino JSON en exportación de AST. |
| Generar listado ensamblador | `-s` | Compilación de `.cls`; predeterminado activado para el perfil inicial. No ofrecer «solo ensamblador»: `-s` también acompaña las salidas binarias. |
| Mostrar AST textual | `-t` | Presenta el AST en stdout; por sí sola no detiene la compilación. No confundir con exportar AST sin las demás fases. |
| Mostrar tablas | `-m` | Tablas de símbolos y etiquetas; no compatible con `--check` ni con el perfil AST sintáctico exclusivo. |
| Mostrar bundles | `-p` | Listado en ensamblador/binario; no compatible con `--check` ni con el perfil AST sintáctico exclusivo. |
| Mostrar progreso de fases | `-v` | Presentar log separado de los diagnósticos. Es compatible con `--check`. |
| Medir duración | `--time` | Presentar tiempo informado por ccom; compatible con `--check`. |
| Desactivar organización de bundles | `--no-schedule` | Comparación en compilación normal; no presentarlo como una optimización del IDE ni habilitarlo en `--check`. |

En ensamblado, el ccom actual ignora algunas opciones del front-end. La UI debe deshabilitarlas en vez de sugerir que sí tienen efecto. Toda combinación se comprobará contra el comportamiento real de las opciones, no solo contra nombres visibles.

**IR/TAC y optimizaciones:** hay trabajo relacionado en otras ramas, pero las opciones correspondientes no forman parte del ccom actual de `feat/calside`. Planificar su aparición en el diálogo cuando se integren y prueben; no inventar banderas ni mostrar controles funcionales para capacidades ausentes. La integración puede añadir una consulta de capacidades del servicio; debe distinguir una opción no soportada de un fallo del programa.

### 13.4 Implementación y responsabilidades

- Python define los controles, valida elecciones compatibles y construye una lista de argumentos para QProcess; no concatena una orden que pase por un shell.
- C ejecuta las fases reales y genera resultados/artefactos. No se replica en Python un «análisis» para hacer funcionar un perfil.
- Usar el mismo camino para botón, F5 y menú. El proceso conserva captura de stdout/stderr, cancelación, límites y resultados por revisión.
- Mostrar el resumen del modo seleccionado, destino y opciones efectivas antes/durante la operación. El usuario debe distinguir análisis correcto, AST exportado, ensamblado correcto y compilación completa.
- Conservar el destino por fuente sin colisiones entre nombres iguales de carpetas distintas; permitir elegir otro y proteger fuente/entradas frente a sobrescrituras accidentales.
- No mostrar artefactos antiguos como recién generados por un modo sin salidas. F6 solo se habilita por una compilación completa vigente con `.asm` y `.mem` verificados; un análisis o exportación no la registra ni renueva por sí solo.
- Mantener separados los ajustes de compilación manual y las consultas automáticas del editor: cambiar F5 a exportación no debe convertir el análisis en vivo en generación de archivos.
- Revisar `docs/ccom_banderas.md` y la ayuda al implementar: actualmente ese documento conserva referencias antiguas a compilación silenciosa, mientras el código ya confirma éxitos. La fuente operativa que debe comprobarse es el ccom utilizado.

Archivos afectados previsibles: `interfaz/ventana.py` (control/acciones/proceso), un módulo de configuración de compilación si conviene separarlo, `preferencias.py`, integración de vigencia en `interfaz/ejecucion.py`, pruebas y documentación. Esta funcionalidad no exige cambiar las reglas de Calser.

### 13.5 Pruebas de aceptación

1. Seleccionar **Analizar** y ejecutar con clic/F5: informa errores o éxito y no crea ni modifica artefactos preexistentes.
2. Compilar con y sin `-s`: los archivos y la disponibilidad de ejecución corresponden a lo generado; no se usa un `.asm` antiguo.
3. Exportar AST: produce JSON válido y lo identifica como resultado sintáctico, sin anunciar éxito semántico o binario.
4. Ensamblar una entrada `.asm`: ejecuta `-x` sobre esa entrada y permite elegir destino; rechaza una selección incompatible sin sobrescribir archivos.
5. Activar tiempo/progreso/listados: aparecen en sus paneles y se conservan los diagnósticos navegables.
6. Cambiar el modo: no permite combinaciones prohibidas como `--check -s`; F5 y botón envían los mismos argumentos.
7. Probar rutas con espacios, fuentes homónimas en carpetas diferentes, permisos, cancelación y salida fallida.
8. Cambiar de pestaña o editar durante el proceso: el resultado conserva su documento/revisión y no habilita ejecutar una versión obsoleta.
9. Reiniciar/cambiar ccom: se restaura únicamente configuración válida y no se reactivan capacidades no soportadas.

### Resultado de M1B (2026-11-11)

Implementados `ide/compilacion.py` (perfiles/argumentos), `ide/interfaz/opciones_compilacion.py` (diálogo) y su conexión con Ventana, preferencias y EjecucionVentana. Botón dividido y F5 comparten QAction. Los cuatro modos se ejecutan con ccom real, sin shell. Las opciones automáticas del editor permanecen independientes.

El modo manual opera sobre la fuente guardada y solicita guardar si hace falta, también para analizar. Analizar usa `--check`; el análisis automático conserva `--check-json` y sus snapshots. Exportar AST informa expresamente que no comprueba semántica. Ensamblar utiliza el .asm activo o pide elegir uno; su pestaña no se analiza como Calser.

Destinos automáticos por hash de ruta; destino personalizado relativo al fuente o absoluto. Los nombres se normalizan mediante extensión exigida (.bin/.json). Se rechazan salidas que coinciden con documentos abiertos/entrada, enlaces simbólicos o enlaces duros a documentos protegidos. Sobrescribir salidas de un destino personalizado requiere confirmación. La carpeta build automática se reutiliza de forma intencionada. Estas comprobaciones no proporcionan un bloqueo de disco frente a cambios externos concurrentes.

Cada operación captura perfil, revisión y versión de disco. Se descarta el registro ejecutable anterior para ese documento; solo una compilación completa correcta con -s y artefactos renovados registra F6, utilizando el nombre de salida elegido. Analizar/AST/ensamblar y compilar sin -s no registran F6. Cancelar, fallo al iniciar y límite de tiempo restauran los controles. Los diagnósticos manuales siguen usando la salida textual de ccom; M1 estructura el análisis automático.

Las preferencias solo conservan campos/banderas conocidos y se filtran al restaurar, seleccionar otro ccom y lanzar el trabajo. No hay negociación de capacidades con ejecutables ajenos: si no admiten una bandera, su error se presenta como fallo y no habilita ejecución. IR/TAC/optimizaciones quedan pendientes de integrarse en ccom antes de incorporarlas al diálogo.

Validación: `ide/pruebas/test_compilacion.py` cubre perfiles, interfaz, cuatro operaciones reales, AST con error semántico, rutas con espacios, destinos personalizados, sobrescritura, enlaces, preferencias, cancelación y rechazo de artefactos antiguos. Suite completa: `make check-ide`.

## 14. Resultado de M2 (2026-11-11)

Implementado `ccom --tokens-json`: Flex publica clasificación y rangos UTF-8 de tokens y comentarios; el escaneo independiente del parser llega hasta EOF aunque el programa esté incompleto. Comparte captura de diagnósticos con M1, manteniendo `--check-json` y la CLI clásica. No genera artefactos.

El adaptador conserva el nombre `ide/adaptador_nucleo.py` para sus consumidores, pero ya no importa `ide/nucleo/lexico.py` ni reconoce comentarios. Recibe el JSON, valida el transporte y convierte a UTF-16 para Qt. La tarea de fondo lanza ccom sin shell, usa instantáneas privadas y mata/recoge el hijo al cancelar. Se mantienen revisión, pestañas, archivos sin guardar y Deshacer. Los errores ahora tienen origen ccom léxico y las reglas/recuperación reales de Flex, incluidos errores individuales por byte no ASCII fuera de comentarios.

Límites: 2 millones de caracteres/8 millones de bytes de entrada, 100000 tramos, 128 diagnósticos, proceso de 5 s, stdout de 16 MB y stderr de 1 MB. Un límite o ccom incompatible se informa explícitamente sin fallback al lexer Python ni apariencia de análisis correcto. Configurar otro ccom vuelve a pedir el léxico.

Ver [protocolo M2](../../docs/ccom_servicios_ide.md#tokens). Pruebas: `make check-tokens-json`, `make check` y `make check-ide`; 341 pruebas del IDE correctas, incluidas 11 nuevas del transporte Flex y cancelación, además de los casos nativos. Clones, asistencia, LL(1) y simulador conservan sus recorridos actuales; no se anticiparon M3/M4/M5.

## 15. Resultado de M3 (2026-11-11)

Adaptado el comportamiento y los casos de la entrega a `src/servicios_ide/suggestions.c`, sin importar/fusionar sus dataclasses. `--suggest-json <cursor UTF-8>` usa los rangos Flex y el contexto real de Bison/LAC. Un marcador privado en el EOF temporal permite preguntar por continuaciones de programas completos/incompletos y distinguir errores anteriores. No se ejecuta semántica ni se inventan identificadores/literales. Filtrado y orden se realizan en C.

`ide/sugerencias_ccom.py` valida JSON y convierte rangos al contrato compartido. `ide/servicio_ccom.py` comparte transporte cancelable con el léxico de M2. El proveedor predeterminado ya es C; Ctrl+Espacio, pausa opcional, aceptación, Deshacer y vigencia mantienen su UI. La pausa conserva su valor predeterminado desactivado. Cambiar ccom cancela consultas/listas antiguas. Reparaciones sigue pendiente para M4.

Contrato, decisiones de fragmento/comentario, límites y archivos: [servicio de sugerencias](../../docs/ccom_servicios_ide.md#suggest). Límites: 200000 bytes de fuente, 50000 tokens y 64 opciones; proceso 2,5 s, stdout/stderr 1 MB por canal. El coordinador conserva 3 s. No se ofrecen listas parciales como completas.

Validación: `make check-suggestions-json`, `make check` completo y suite IDE; 14 pruebas nuevas del motor real/transporte/interfaz. Verificación adicional ASan/LSan: 65 consultas parciales sin incidencias detectadas. Se preserva la compatibilidad de M1, M1B y M2.

## 16. Resultado de M4 (2026-11-11)

Motor entregado adaptado a C: `repairs.c` enumera cierres LAC y `syntax_service.c` reanaliza cada copia con Flex/Bison, reiniciando/liberando estado entre intentos. `--repair-json` ofrece inserciones de ; . ] ) solo cuando superan el primer error; las parciales son manuales. `--syntax-json` permite verificación independiente desde la política, sin semántica ni artefactos. Se conserva la CLI clásica.

El proveedor `ide/reparaciones_ccom.py` sustituye la carga pendiente del motor Python. Alt+Enter, clic derecho, vista previa/aplicador y cancelación usan el motor C real. Precondiciones verificables no vacías y contexto completo sustituyen las instrucciones narrativas de la entrega. La inserción queda tras el último token válido, antes de espacios/comentarios y del salto de línea.

La política ya no llama al parser LL(1) Python: valida original y candidato en C. Catálogo revisado inicial: INSERCION_PUNTO_COMA, con configuración automática desactivada y ninguna regla seleccionada por defecto. Requiere permiso explícito por regla, una propuesta única aceptada completamente, línea confirmada/contexto vigente y un Deshacer. No guarda ni encadena reparaciones. Otros cierres, EOF y propuestas parciales siguen manuales.

[Contrato, arquitectura, límites y diferencias frente a la entrega](../../docs/ccom_servicios_ide.md#repair). Verificación: `make check-repairs-json`, `make check` completo y 371 pruebas IDE correctas, incluidas 12 nuevas del motor real/interfaz; ASan/LSan correcto en 124 consultas con reanálisis. Este apartado registra el cierre de M4; M5 se detalla a continuación. M6 revisará retiros y documentación global.


## 17. Resultado de M5 (2026-11-11)

Clones Tipo I/II compara en `src/servicios_ide/clones.c` los reconocimientos reales de Flex. `ide/clones.py` queda como modelos, empaquetado CCL1, transporte y adaptación de rangos, sin regex ni comparación Python. La ventana pasa la ruta de ccom configurada al trabajador; el panel, instantáneas sin guardar, comparación y navegación se conservan.

Se mantienen equivalencia biyectiva de nombres, variación de literales por clase, límites de archivos/bytes/tokens/candidatos/pares, cancelación y ausencia de resultados parciales; se añade un presupuesto de trabajo y plazo de 15 s. Las entradas con errores léxicos se bloquean explícitamente. [Protocolo, arquitectura, límites y pruebas M5](../../docs/ccom_servicios_ide.md#clones).

Verificación: `make check-clones-json`, suite completa `make check`, 379 pruebas IDE verificadas (378 en la primera ejecución completa más la prueba final de ruta configurada/error léxico), 250 comparaciones diferenciales con el detector previo y suite nativa con ASan/UBSan/LSan. Este apartado registra el cierre de M5; M6 se detalla a continuación y M7 sigue pendiente.


## 18. Resultado de M6 (2026-11-11)

Revisadas las importaciones productivas: solo se usan modelos de `ide.nucleo.asistencia`; lexer, gramática, tabla y parser LL(1) permanecen como API autorizada de consulta/estudio y pruebas, fuera del backend gráfico. No se necesita eliminar ni mover esos módulos para preservar la frontera.

Se unifica cabecera JSON en `ccom.informe_json` (seis operaciones), se elimina la segunda decodificación léxica y las copias de validación en adaptadores, se retira el vector inverso nunca leído de clones y se aclara `lexer_parse_text` para prefijos/copias completas. Comentarios de servicios C y adaptadores explican consumidores, memoria, UTF-8/Python/UTF-16, estado global/local, errores y cancelación. Guías de núcleo/motores/contratos describen el backend real. [Auditoría, decisiones, orden de lectura y reproducción](guia_sistema_calside.md).

Verificación: suite completa `make check`, **382 pruebas IDE**, recorrido gráfico con varios documentos y captura inspeccionada. `test_arquitectura_ccom` y `tools/probar_recorrido_ide.py` bloquean importar analizadores Python durante el recorrido real de M1–M5/F5/F6. El resultado de ejecución es 5; análisis, propuestas y Deshacer no guardan fuentes. M7 queda pendiente.


## 19. Resultado de M7 (2026-11-11)

El parser C recupera en separadores/cierres de cuerpos, globales y cabeceras. Dos errores independientes producen dos diagnósticos en `--check-json` y en Problemas. Los descartes están acotados, cada ancla retenida se reusa una vez y no se repiten diagnósticos sobre el mismo token rechazado. Las consultas M3/M4 permanecen en primer error.

Semántica sobre AST recuperado: **deshabilitada**. `phase_parse` comprueba el contador sintáctico además del retorno de Bison, bloqueando semántica/exportación/generación aun cuando `yyparse` termine en 0. Nodos/cadenas descartados y árboles parciales se liberan. Máximo 32 errores/4096 tokens descartados sin ancla; los límites se anuncian como informe truncado, no como resultado completo. [Diseño, límites y prueba rápida](../../docs/ccom_servicios_ide.md#recovery).

Verificación: `make check-recovery`, `make check`, 384 pruebas IDE, corpus de 120 mutaciones y casos M7 con ASan/UBSan/LSan. Se conservan las limitaciones explícitas ante falta de separadores/cierres; no se inventan expresiones ni se promete hallar todos los errores de entradas arbitrarias.


</details>


<a id="hist-revision-m6"></a>

<details>
<summary>revision_m6.md — documento sustituido</summary>

# Revisión y limpieza de CALSIDE · M6

Registro de M6, 2026-11-11. Actualización posterior: M7 ya implementó recuperación conservadora en C; ver [M7](../../docs/ccom_servicios_ide.md#recovery).
Este documento registra el mapa vigente y el alcance de la revisión. La explicación completa del sistema está en [guia_sistema_calside.md](guia_sistema_calside.md).

## Decisión sobre el código anterior

| Pieza | Decisión y motivo |
| --- | --- |
| `ide/nucleo/asistencia.py` | Productivo: modelos y validación estructural, sin importar analizadores ni Qt. Mantener su ubicación evita romper contratos del aplicador y las entregas. |
| `ide/nucleo/lexico.py`, `gramatica.py`, `tabla.py`, `parser.py` | Conservar API de consulta, estudio y pruebas del LL(1), autorizada por Saddy. Cabeceras/README explican que no es backend gráfico ni respaldo de ccom. |
| Sugerencias/reparaciones Python de la entrega | No se cargan en producción; los proveedores efectivos son `ide.sugerencias_ccom` y `ide.reparaciones_ccom`. No se fusiona ni se borra la rama del compañero desde esta revisión. |
| Regex/firma/comparación de clones Python | Sustituidos en M5 por Flex/`src/servicios_ide/clones.c`. El helper `tokenizar` restante consulta Flex y solo se conserva para inspección/pruebas, no para el detector. |
| `ide/adaptador_nucleo.py` | Conservar nombre por compatibilidad: hoy adapta Flex/JSON a Qt. No importa el lexer del núcleo. |
| Regex de `ide/ccom.py:diagnosticos` | Conservar para leer la salida textual de F5. No reconoce código Calser ni determina reglas del lenguaje. |
| `ide/servicio_ccom.py` y `interfaz/comprobacion.py` | Conservar ambos: transporte Popen bloqueante en trabajadores M2–M5 y QProcess dirigido por señales para M1. Sus ciclos de vida son distintos; no hay dos analizadores. |
| Simulador Python | Conservado como herramienta externa autorizada. F6 ejecuta los artefactos de F5. |

No se eliminan módulos por estar escritos en Python: se retira el uso redundante del lenguaje en los recorridos productivos, preservando lo que sigue teniendo consumidores reales.

## Cambios de limpieza

- `ccom.informe_json` centraliza UTF-8, duplicados, constantes no JSON, versión, operación, identidad de fuente y código real del proceso. Lo reutilizan los seis servicios; cada adaptador conserva su validación específica de estados y contenido.
- `lexico_json` reutiliza el informe decodificado para diagnósticos y tokens. Se elimina la segunda lectura JSON del mismo mensaje.
- Se retira `_informe` de reparaciones y las copias de la cabecera en sugerencias/clones.
- `lexer_parse_prefix` pasa a llamarse `lexer_parse_text`: su función sirve tanto al prefijo de M3 como a la copia completa de M4. Se actualizan declaración, definición y consumidores; no cambia la gramática ni los tokens.
- Se retira el vector inverso de valores de clones que nunca se leía. La marca `seen_b` ya garantiza la unicidad inversa de la biyección; sus pruebas negativas se conservan.
- Se actualizan cabeceras/comentarios de servicios C, interfaces públicas, observadores Flex/Bison y adaptadores Python: consumidores, estado global/local, propiedad de memoria, rangos, errores y cancelación.
- Se actualizan las guías antiguas para que no recomienden volver a integrar un backend Python ya sustituido.

## Recorridos productivos

| Acción | Entrada Python | Decisión en C | Salida/interfaz |
| --- | --- | --- | --- |
| Léxico/colores | `adaptador_nucleo.analizar_documento` | `--tokens-json`, Flex | `convertir_lexico`, `ResultadoLexico`, `Editor.aplicar_analisis`. |
| Sintaxis/semántica automática | `ComprobadorCcom` | `--check-json`, Flex/Bison/semántico | `ccom.diagnosticos_json`, Problemas y marcas. |
| Sugerencias | `AsistenciaEditor` → `sugerencias_ccom.sugerir` | `--suggest-json`, Flex y Bison/LAC | Modelos `Sugerencia`, popup, aplicador con revisión vigente. |
| Reparaciones | `ReparacionesVentana` → `reparaciones_ccom.proponer_reparaciones` | `--repair-json`, candidatos y reanálisis C | Vista previa, motivos, aplicación y Deshacer. |
| Validación automática de reparación | `politica_asistencia.evaluar_automatizacion` | `--syntax-json` original y candidato | Permisos/regla/vigencia en Python; no guardar ni encadenar. |
| Clones | `PanelClones` → `ide.clones.detectar` | `--clones-json`, Flex y comparación Tipo I/II | Fragmentos, tabla, comparación y navegación vigilada. |
| Compilación/ejecución | Ventana, perfil F5 y controlador F6 | ccom genera artefactos | Simulador externo Python, salida y traza. |

## Propiedad y unidades

C recibe copias; los servicios nunca editan documentos del usuario. En M2–M5 el transporte crea una carpeta privada y recoge el hijo antes de borrarla. QProcess hace lo equivalente mediante señales en M1. No se descartan silenciosamente errores de versión/transporte para inventar resultados vacíos.

Los servicios C producen bytes UTF-8 `[start,end)`, los contratos de ediciones usan caracteres Python y Qt usa UTF-16. Esas conversiones son adaptación de interfaz, no otra tokenización. No mezclar el token inesperado con el rango visual elegido para el diagnóstico ni con la posición de inserción de una reparación.

Los servicios C son seriales dentro de un proceso: Flex/Bison y observadores comparten estado global. CALSIDE usa hijos separados, así que varias tareas no comparten esos globales. Bison libera subárboles/cadenas de consulta mediante destructores; la compilación normal conserva la transferencia del AST a main. Clones libera sus tablas/textos y solo presta lexemas mientras vive ese contexto.

La autorización automática es independiente del resultado C: permiso de usuario, regla seleccionada, precondiciones y revisión vigente siguen en Python. C comprueba el lenguaje; Qt aplica una única operación de Deshacer sin guardar.

## Pruebas y recorrido gráfico

```sh
make check
make check-ide
make check-recorrido-ide
```

`test_arquitectura_ccom.py` verifica cabeceras de los seis servicios reales y respuestas cruzadas/UTF-16/duplicados inválidos. Ejecuta el recorrido gráfico en un proceso aislado donde importar `ide.nucleo.lexico`, `gramatica`, `tabla` o `parser` falla expresamente. No bloquea `asistencia.py`, que contiene los modelos productivos.

`tools/probar_recorrido_ide.py` realiza cinco grupos de acciones con Qt real, sin proveedores ficticios: dos pestañas con Unicode y análisis; clones y navegación; error sin guardar, reparación, Deshacer/Rehacer y disco intacto; sugerencias en documento sin nombre y cambio de pestaña; F5/F6 con resultado **5**. Cierra los trabajadores y limpia el proyecto temporal. Es un recorrido gráfico automatizado; no sustituye la valoración visual humana en otros tamaños de pantalla.

Para guardar una captura de ese recorrido:

```sh
QT_QPA_PLATFORM=offscreen python3 tools/probar_recorrido_ide.py --captura /tmp/calside-m6.png
```

Verificación final M6: **382 pruebas IDE correctas**, suite completa del compilador y recorrido gráfico. La compilación usa `-Wall -Wextra` sin advertencias nuevas. Las pruebas Python del LL(1) permanecen identificadas como estudio/diferenciales; las pruebas del lenguaje productivo consultan C. Los dobles de las suites de coordinación/transporte prueban fallos, plazos y descarte de respuestas, no demuestran reglas de Calser.

## Orden para leer y entender el código

1. `ide/main.py`, `ide/__init__.py`, `ide/interfaz/ventana.py`: arranque y conexiones.
2. `ide/ccom.py`, `ide/servicio_ccom.py`, `ide/interfaz/comprobacion.py`: ejecutable, cabecera común y procesos.
3. `src/main.c`, `include/comun/diagnostics.h`, `src/comun/diagnostics.c`, `src/servicios_ide/diagnostics_json.c`: despacho y datos reales.
4. `grammar/lexer.l`, `include/analisis/lexer_service.h`, `ide/adaptador_nucleo.py`, `ide/interfaz/analisis.py`: tokens y colores.
5. `grammar/parser.y`, `src/servicios_ide/suggestions.c`, `src/servicios_ide/syntax_service.c`, `src/servicios_ide/repairs.c`: LAC, candidatos y propiedad de AST.
6. `ide/nucleo/asistencia.py`, `ide/sugerencias_ccom.py`, `ide/reparaciones_ccom.py`, `ide/interfaz/asistencia.py`: contratos y coordinación.
7. `ide/politica_asistencia.py`, `ide/ediciones.py`, `ide/interfaz/ediciones.py`: permisos, vigencia y edición.
8. `src/servicios_ide/clones.c`, `ide/clones.py`, `ide/interfaz/clones.py`: paquete, equivalencia y comparación visual.
9. `ide/nucleo/README.md`: LL(1) de consulta/pruebas, separado del recorrido anterior.

Los comentarios de M6 cubren los servicios/adaptadores de la migración y sus puntos de integración. No implican una reescritura ni auditoría total del generador, semántico, simulador o todos los widgets.

## Pendientes reales

**M7 se completó después de esta revisión:** recuperación sintáctica conservadora C y varios errores independientes. Las consultas de sugerencias/reparaciones conservan primer error; el LL(1) de consulta no se amplía. [Diseño y limitaciones M7](../../docs/ccom_servicios_ide.md#recovery).

Empaquetado, animaciones, texto de predicción gris y mejoras visuales mantienen su planificación propia. No se añadieron como parte de M6.


</details>


<a id="hist-contexto-parser-predictivo"></a>

<details>
<summary>contexto_parser_predictivo.md — documento sustituido</summary>

# Contexto y entrega: parser predictivo LL(1)

Responsable: Saddy. Implementación disponible en `ide/nucleo/parser.py`. Un mismo compañero continuará con los motores de sugerencias y reparaciones.

La documentación de uso vigente es [Parser predictivo: API y entrega a los motores](ll1/parser_predictivo.md). Contiene tipos, estados, límites, posiciones y ejemplos ejecutables. Este documento reemplaza la propuesta inicial del parser; los otros motores deben adaptarse a la API implementada.

## Decisiones de Saddy

- 1B: programas completos y contexto en el cursor; recuperación múltiple después.
- 2A: pila explícita y tabla LL(1), reutilizando el núcleo recibido.
- 3A: primer error, sin sincronización ni cambios en el código. 3B queda para después.
- 4A: resultado, diagnósticos y estado; sin árbol/AST. Traza optativa limitada.
- 5B: implementar primero el parser y fijar después su contrato real.

## Qué se implementó

```python
from ide.nucleo.parser import analizar_sintaxis, contexto_en

r = analizar_sintaxis("F main [] : void ; E ; .")
assert r.aceptado
texto = "F main [] : void ; Vm total : "
c = contexto_en(texto, len(texto))
assert c.valido
assert c.esperados == ("bool", "int")
```

Los resultados son dataclasses congeladas. La cima de la pila está a la derecha. Los rangos usan offsets Python desde 0, fin exclusivo; línea/columna desde 1. Los errores léxicos mantienen su modelo original. Consultar la guía de API antes de convertirlos o de aplicar cambios en Qt.

El contexto no detecta automáticamente la palabra parcial: el motor de sugerencias indica `inicio_fragmento` para excluirla del análisis y recibe su prefijo. No hay nombres insertables, variables declaradas ni propuestas de reparación en este módulo.

## Dependencias y límites de la entrega

El parser depende de `lexico.py`, `gramatica.py` y `tabla.py`, todos existentes. Ninguno de los dos motores es una dependencia del parser. Ambos motores sí pueden consultar esta API.

La interfaz de CALSIDE mantiene E3/E4, basada en lexer y ccom. E5 está parcial: parser terminado, motor y UI de sugerencias pendientes. E6 sigue pendiente. No se modificaron la gramática ni la semántica del compilador.

No se creó `modelos.py`: los modelos reales del parser viven en `parser.py`. Evitar duplicarlos. Los tipos de propuestas y ediciones corresponden a los motores del compañero.

## Verificación y siguiente paso

- `make check-parser`: 26 pruebas nuevas; no depende de Qt.
- `make check-ide`: 96 pruebas combinadas correctas.
- Comparación con Bison mediante `--ast-json`: 105 fuentes sin errores léxicos, sin diferencias de aceptación; cinco fuentes con errores léxicos excluidas.

El compañero puede usar directamente los ejemplos de [la API](ll1/parser_predictivo.md) para construir sugerencias y reanalizar candidatos de reparación. Después se integrarán los motores mediante adaptadores y widgets, con revisión del documento y Deshacer. No implementar el trabajo del compañero en el parser.


</details>


<a id="hist-contexto-motor-sugerencias"></a>

<details>
<summary>contexto_motor_sugerencias.md — documento sustituido</summary>

# Contexto vigente del motor de sugerencias

Actualizado en M6, 2026-11-11. La entrega original Python se usó como referencia de comportamiento; el motor productivo ya está adaptado a **C** y conectado al IDE. El reparto de noviembre 5 que exigía Python puro queda sustituido por esta arquitectura. El LL(1) se conserva para consulta/estudio, no como dependencia de este motor.

## Dónde trabajar

- Backend: `src/servicios_ide/suggestions.c, include/servicios_ide/suggestions.h, grammar/parser.y y grammar/lexer.l`.
- Entrada nativa: `ccom --suggest-json <cursorBytes>`; stdout JSON v1 y stderr para auxiliares.
- Adaptador: `ide/sugerencias_ccom.py`. Valida datos y convierte coordenadas; no interpreta Calser.
- Modelos compartidos: `ide/nucleo/asistencia.py`, sin importar los analizadores Python.
- Contrato nativo detallado: [ccom_suggestions_json.md](../../docs/ccom_servicios_ide.md#suggest).
- Contrato Python/UI: [contrato_asistencia.md](asistencia.md).

## Frontera de responsabilidades

C obtiene contexto/expectativas del lexer/parser real y decide las propuestas. Python conserva archivos/pestañas, instantáneas, permisos, revisión vigente, interfaz y aplicación. No duplicar reglas de lenguaje en un proveedor Python ni crear otro modelo incompatible. El coordinador captura root/ccom configurado y usa el transporte cancelable compartido; no leer obligatoriamente la fuente original ni guardar buffers.

C usa bytes UTF-8 `[start,end)`, los modelos usan caracteres Python y Qt usa UTF-16. No convertir un comentario humano en precondición ni confundir el rango del mensaje visual con el de la edición. Los buffers/AST temporales pertenecen al servicio C y se liberan antes de retornar. Flex/Bison y observadores son seriales dentro del proceso; la concurrencia del IDE usa procesos separados.

## Comportamiento y límites

Límites nativos: 64 opciones, 200000 bytes y 50000 reconocimientos. El hijo tiene un plazo de 2,5 s en el transporte y el coordinador gráfico un presupuesto total de 3 s. Cero opciones válidas, error previo que bloquea, límite y fallo de servicio son estados distintos. La cancelación se propaga al transporte y termina/recoge el hijo sin cambiar documentos.

`F main [] : vo` ofrece `void`; el reemplazo incluye el token completo si el cursor está en medio. Bison/LAC produce esperados tras el prefijo; no se inventan nombres, números ni cuerpos de función. No ofrecer dentro de comentarios ni cuando un error anterior bloquea el contexto. Ctrl+Espacio y clic derecho consultan el mismo proveedor; la pausa opcional conserva permisos.

Texto gris/predicción de instrucciones sigue como mejora futura, no forma parte de M3/M6.

## Pruebas antes de cambiar el motor

`make check-suggestions-json`, `make check`, `make check-ide` y `make check-recorrido-ide`. Los casos del lenguaje deben usar C real; los dobles solo aíslan coordinación, transporte o fallos de interfaz.

[Revisión M6 y orden de lectura](guia_sistema_calside.md). La sincronización para varios errores sintácticos fiables se implementará en M7.


</details>


<a id="hist-contexto-motor-reparaciones"></a>

<details>
<summary>contexto_motor_reparaciones.md — documento sustituido</summary>

# Contexto vigente del motor de reparaciones

Actualizado en M6, 2026-11-11. La entrega original Python se usó como referencia de comportamiento; el motor productivo ya está adaptado a **C** y conectado al IDE. El reparto de noviembre 5 que exigía Python puro queda sustituido por esta arquitectura. El LL(1) se conserva para consulta/estudio, no como dependencia de este motor.

## Dónde trabajar

- Backend: `src/servicios_ide/repairs.c, src/servicios_ide/syntax_service.c y sus cabeceras`.
- Entrada nativa: `ccom --repair-json y --syntax-json`; stdout JSON v1 y stderr para auxiliares.
- Adaptador: `ide/reparaciones_ccom.py`. Valida datos y convierte coordenadas; no interpreta Calser.
- Modelos compartidos: `ide/nucleo/asistencia.py`, sin importar los analizadores Python.
- Contrato nativo detallado: [ccom_repairs_json.md](../../docs/ccom_servicios_ide.md#repair).
- Contrato Python/UI: [contrato_asistencia.md](asistencia.md).

## Frontera de responsabilidades

C obtiene contexto/expectativas del lexer/parser real y decide las propuestas. Python conserva archivos/pestañas, instantáneas, permisos, revisión vigente, interfaz y aplicación. No duplicar reglas de lenguaje en un proveedor Python ni crear otro modelo incompatible. El coordinador captura root/ccom configurado y usa el transporte cancelable compartido; no leer obligatoriamente la fuente original ni guardar buffers.

C usa bytes UTF-8 `[start,end)`, los modelos usan caracteres Python y Qt usa UTF-16. No convertir un comentario humano en precondición ni confundir el rango del mensaje visual con el de la edición. Los buffers/AST temporales pertenecen al servicio C y se liberan antes de retornar. Flex/Bison y observadores son seriales dentro del proceso; la concurrencia del IDE usa procesos separados.

## Comportamiento y límites

Límites nativos: 4 propuestas, 200000 bytes y 50000 reconocimientos. El hijo tiene un plazo de 2,5 s en el transporte y el coordinador gráfico un presupuesto total de 3 s. Cero opciones válidas, error previo que bloquea, límite y fallo de servicio son estados distintos. La cancelación se propaga al transporte y termina/recoge el hijo sin cambiar documentos.

Solo se insertan `;`, `.`, `]` y `)` esperados por LAC. Cada copia se reanaliza íntegramente; una mejora parcial es solo manual. Las precondiciones contienen texto original real y no vacío. No se inventan `E`, operandos ni reparaciones semánticas.

`automatic_safe` no es permiso: la política Python exige una única propuesta de la regla revisada INSERCION_PUNTO_COMA, habilitación/selección del usuario, contexto vigente y aceptación completa por C. Se verifican original/candidato con `--syntax-json`. No guardar, no encadenar y conservar Deshacer.

## Pruebas antes de cambiar el motor

`make check-repairs-json`, `make check`, `make check-ide` y `make check-recorrido-ide`. Los casos del lenguaje deben usar C real; los dobles solo aíslan coordinación, transporte o fallos de interfaz.

[Revisión M6 y orden de lectura](guia_sistema_calside.md). La sincronización para varios errores sintácticos fiables se implementará en M7.


</details>


<a id="hist-integrar-nucleo"></a>

<details>
<summary>integrar_nucleo.md — documento sustituido</summary>

# Conexión vigente del backend de CALSIDE

Actualizado en M6, 2026-11-11. La entrega y el merge originales ya ocurrieron; no hay que repetirlos. La arquitectura actual reutiliza ccom para las decisiones del lenguaje. La excepción acordada permite mantener LL(1) y simulador en Python; el LL(1) se conserva para consulta/pruebas, fuera del recorrido gráfico.

## Qué llama la interfaz

| Acción | Servicio real | Adaptación Python |
| --- | --- | --- |
| Léxico y colores | Flex, `--tokens-json` | `adaptador_nucleo.py`, `interfaz/analisis.py`. |
| Análisis automático | Flex/Bison/semántico, `--check-json` | `ccom.py`, `interfaz/comprobacion.py`. |
| Sugerencias | Bison/LAC y Flex, `--suggest-json` | `sugerencias_ccom.py`. |
| Reparaciones | Candidatos y reanálisis C, `--repair-json` | `reparaciones_ccom.py`, panel/aplicador. |
| Comprobación de regla automática | Parser C, `--syntax-json` | `politica_asistencia.py`: preferencias y contexto. |
| Clones | Flex y comparación C, `--clones-json` | `clones.py`: varias instantáneas y posiciones. |

Los adaptadores comparten `ccom.informe_json` para la cabecera; después validan su contenido. El transporte M2–M5 está en `servicio_ccom.py` dentro de trabajadores; el análisis automático M1 conserva QProcess. Los textos se envían como copias privadas sin guardar buffers. La ventana aporta el ccom configurado y descarta revisiones antiguas.

Los únicos imports productivos desde `ide.nucleo` son modelos/validadores de `asistencia.py`. No conectar de nuevo `lexico.analizar` al resaltado ni `parser.contexto_en` a las sugerencias gráficas. Para consultar el parser LL(1) directamente, ver [su API](ll1/parser_predictivo.md) y [su papel actual](../nucleo/README.md).

## Posiciones y memoria

C: bytes UTF-8 `[start,end)`. Contratos de ediciones: caracteres Python. Qt: unidades UTF-16. Los adaptadores convierten posiciones; el aplicador verifica el contexto y agrupa una propuesta en un Deshacer. Los diagnósticos visuales no sustituyen el rango real del token ni la posición de una edición.

Los servicios nativos atienden una consulta por proceso, liberan sus buffers/AST y no tocan archivos de usuario. Cancelar mata/recoge el hijo y elimina la copia; el IDE conserva documentos e historial. El LL(1) Python no se carga si ccom falla: se informa la ausencia/incompatibilidad.

## Verificación

```sh
make
make ide
make check
make check-ide
make check-recorrido-ide
```

[Guía unificada](guia_sistema_calside.md), [decisiones y limpieza M6](guia_sistema_calside.md), [etapas](plan_ide.md). La recuperación de varios errores sintácticos es M7; no la completa el hecho de transportar una lista JSON.


</details>


<a id="hist-politica-asistencia"></a>

<details>
<summary>politica_asistencia.md — documento sustituido</summary>

# Preferencias y política de asistencia — paso 6

> **M4 implementado (2026-11-11):** motor real `ide/reparaciones_ccom.py` sobre `ccom --repair-json`; la política verifica original/candidato con `--syntax-json` en C. Python conserva permisos, contexto y aplicación. Catálogo inicial INSERCION_PUNTO_COMA, optativo y desactivado por defecto. [Contrato M4](../../docs/ccom_servicios_ide.md#repair).

> **Actualización M1 (2026-11-11):** el análisis automático usa `ccom --check-json` y conserva el front-end real en C. Saddy aclaró que el LL(1) y el simulador pueden seguir en Python; no se exige migrarlos. Las demás etapas se describen en el [plan de integración](plan_ide.md).

Estado: preparación del IDE implementada. Motores reales y aprobación de su catálogo pendientes de integración (paso 7). No se declara E5/E6 completa sin esas entregas.

## Uso

Abrir **Editar → Preferencias de asistencia…**. Aceptar aplica y guarda los ajustes del perfil sin reiniciar; Cancelar no cambia ajustes ni consulta motores.

| Preferencia | Predeterminado | Efecto |
| --- | --- | --- |
| Habilitar asistencia | Sí | Control general de consultas manuales y automáticas. Al desactivar cancela pendientes y cierra sugerencias. |
| Mostrar sugerencias después de escribir | No | Solicita opciones tras una pausa de escritura; nunca las acepta sola. Ctrl+Espacio sigue disponible cuando el control general está habilitado. |
| Pausa de asistencia | 700 ms | Ajustable entre 300 y 3000 ms. Se reinicia al seguir escribiendo; también se usa después de confirmar una línea. |
| Corregir al confirmar una línea con Enter | No | Requiere catálogo revisado y reglas seleccionadas. Sin catálogo el control está deshabilitado. |
| Reglas permitidas | Ninguna | Permiso por código exacto; el indicador del motor no agrega permisos. |

Estos ajustes son independientes de **Compilar → Análisis automático** (`ccom --check`), del lexer y de F5/F6. Se persisten bajo `asistencia` en el estado JSON existente; campos inválidos recuperan valores predeterminados, sin interpretar cadenas como booleanos.

## Disparadores y cancelación

El Editor emite `entradaAsistencia` **después** de procesar una tecla que realmente cambia el texto. Escribir caracteres imprimibles sin Ctrl/Alt y sin reemplazar una selección puede programar sugerencias. Enter sin modificadores, sin repetición, sin selección y al final de una línea no vacía puede programar reparaciones.

Abrir, recargar, pegar, arrastrar texto, completar desde una propuesta, Deshacer/Rehacer y las ediciones programáticas no arman el temporizador. Una entrada mediante IME invalida lo anterior; esta versión conserva la asistencia manual para esa entrada, sin activación automática al confirmar la composición.

Se captura el contexto completo al armar la pausa. Modificar texto, mover cursor/selección, cambiar pestaña, cerrar, desplazar/redimensionar u ocultar el editor, perder foco, presionar Esc, cambiar proveedor, catálogo o preferencias cancela lo que corresponda. Las sugerencias/manuales de reparaciones conservan sus reglas de interacción del paso 5: leer el panel no invalida una reparación manual. Las solicitudes automáticas sí se cancelan al perder foco.

Antes de consultar y antes de aplicar se comprueba que el editor sigue activo, enfocado, editable, sin selección ni conflicto externo conocido, y que no hay compilación o simulación en curso. También se verifica que coincidan documento, revisión, texto, cursor y ancla. Una consulta manual cancela la automatización anterior y toma prioridad. El cierre detiene la pausa y conserva el protocolo cooperativo de cancelación de trabajadores.

## Política inicial de reparación automática

El motor conserva su firma: `proponer_reparaciones(texto, *, cancelar=None)`. No recibe widgets, preferencias ni autorización para editar. El IDE añade el origen y las reglas autorizadas a su solicitud interna; no modifica los tipos compartidos del motor.

Para aplicar automáticamente deben cumplirse **todas** estas condiciones:

1. El usuario habilitó la asistencia, la corrección automática y el código concreto de una regla revisada del IDE.
2. La solicitud provino de confirmar una línea con Enter y terminó la pausa sin nuevas acciones que la invalidaran.
3. La respuesta contiene exactamente una propuesta, marcada `automatica_segura`, con razones y validación no vacías.
4. Tiene exactamente una edición, sin borrar/reemplazar texto ni fijar un cursor nuevo. Solo inserta el delimitador de esa regla (`;`, `.`, `]` o `)`, con espacios opcionales y máximo 8 caracteres insertados) al final de la línea recién confirmada, antes de su espacio final.
5. Tiene al menos una precondición no vacía sobre esa línea; todas las precondiciones y rangos son válidos.
6. El texto no supera 100000 caracteres; no existe selección. El cursor sigue al comienzo de la línea siguiente, con solo espacios antes de él.
7. El parser real encuentra un error sintáctico en el original y **acepta todo el programa corregido**, sin errores léxicos, límites ni cancelación. Ambas comprobaciones se realizan en el trabajador, dentro del plazo y callback de cancelación del coordinador.
8. Antes de editar, el hilo gráfico vuelve a comprobar contexto, foco, permisos y regla vigente. El aplicador común verifica nuevamente precondiciones.

Esta política no intenta cerrar funciones o programas mientras se construyen. Si el resto del documento todavía está incompleto, la propuesta requiere revisión manual. La aceptación sintáctica no prueba intención ni semántica; por eso también se exige revisar y probar cada regla antes de incorporarla al catálogo.

La reparación aplicada se informa en la barra de estado y queda consultable en **Reparaciones**, con cambios, motivo del motor y comprobación del IDE. No abre el panel ni roba foco. Una operación de Ctrl+Z revierte la reparación; los diagnósticos anteriores se invalidan mediante la edición normal y vuelve a programarse su análisis. No guarda ni compila. No encadena otra corrección y Deshacer/Rehacer no dispara un nuevo ciclo.

Si una respuesta con propuestas no cumple la política, queda disponible en el panel para revisión manual con su motivo; una consulta posterior con Alt+Enter puede actualizarla. Bloqueos, límites y errores se informan sin diálogos automáticos. Nunca se toma la primera opción de una respuesta ambigua como autorización.

## Conexión y aprobación del catálogo real

Archivos de referencia:

- `ide/politica_asistencia.py`: validación de preferencias, `ReglaAutomatica`, catálogo `REGLAS_AUTOMATICAS` y coordinación de validación nativa en el trabajador.
- `ide/interfaz/preferencias_asistencia.py`: diálogo nativo.
- `ide/interfaz/sugerencias.py`: pausa, permisos, vigencia y aplicación en el hilo gráfico.
- `ide/interfaz/asistencia.py`: motor y validación del parser en segundo plano.
- `ide/pruebas/test_politica_asistencia.py`: proveedores controlados exclusivamente de prueba.

El catálogo M4 contiene **INSERCION_PUNTO_COMA**, revisada con casos positivos/negativos. La automatización y su selección vienen apagadas por defecto. No se importa como permiso un listado que venga del motor y no hay reglas ficticias activas en la aplicación.

Al integrar el motor:

1. Entregar el módulo y firma acordados, importando `Reparacion`, `PrecondicionTexto` y `ResultadoReparaciones` desde `ide.nucleo.asistencia`. Los widgets existentes consumen directamente sus resultados.
2. Documentar por código la forma exacta de edición, justificación, validación y casos ambiguos. Usar `automatica_segura=False` cuando no se pueda justificar la automatización.
3. Añadir positivos y negativos con el parser real: comentarios de línea/bloque, código incompleto, delimitadores anidados, Unicode, cursores, errores múltiples y precondiciones. Una regla puede quedar solo manual.
4. Para una regla que satisface esta política y ha sido revisada, añadir `ReglaAutomatica(codigo_real, descripcion, delimitador)` a `REGLAS_AUTOMATICAS`. No cambiar la evaluación para aceptar un código sin sus pruebas.
5. El usuario habilita corrección automática y marca esa regla en preferencias. Incorporar una regla al código no la habilita en el perfil por sí solo.
6. Verificar en la ventana: Enter al final de la línea, pausa, modificación esperada, Ctrl+Z, ningún guardado y descarte de respuesta antigua. Mantener las consultas manuales.

Para pruebas aisladas existe `ventana.asistencia.establecer_reglas(...)`; invalida tareas anteriores, pero no habilita preferencias. `establecer_motor(...)` sigue permitiendo proveedores controlados; ninguno se activa por defecto.

## Verificación de esta entrega

`make check-ide`: **294 pruebas correctas**, incluidas **26 nuevas** de política y preferencias. Cubren pausa reiniciable, contexto final, cancelación, foco, desactivación, pegado/carga/Deshacer sin disparador, catálogo ausente, selección de reglas, respuesta ambigua, límites de edición, parser real en otro hilo, conflictos, compilación, timeout, persistencia y diálogo aceptar/cancelar.

La validación con reglas y motores de los compañeros es parte del paso 7. Estos proveedores de prueba no sustituyen esa entrega.


</details>


<a id="hist-contrato-asistencia"></a>

<details>
<summary>contrato_asistencia.md — documento sustituido</summary>

# Contrato de asistencia de CALSIDE · versión 1

> **Estado M6, 2026-11-11:** contrato Python de datos vigente, consumido por los adaptadores de motores **C** y los widgets. No implica implementar reglas Calser en Python. `asistencia.py` no carga el lexer/parser LL(1). Sugerencias: `ide.sugerencias_ccom`; reparaciones: `ide.reparaciones_ccom`; validación de lenguaje automática: `--syntax-json`. [Frontera y ciclo de vida](guia_sistema_calside.md).

## Archivos y responsabilidades

| Archivo | Responsabilidad |
|---|---|
| `ide/nucleo/asistencia.py` | Tipos inmutables y protocolos de llamada. Python puro; no importa Qt. |
| `ide/ediciones.py` | Validación de rangos/precondiciones, transformación sobre copia y conversiones Python/UTF-16. Python puro. |
| `ide/interfaz/ediciones.py` | Capturar el contexto y aplicar cambios sobre un Editor en el hilo gráfico. |
| `ide/interfaz/asistencia.py` | Coordinador serial de ambos motores, tareas, cancelación, límites y validación de respuestas. |
| `ide/interfaz/sugerencias.py` | Lista junto al cursor y control manual de sugerencias en el editor. |
| `ide/interfaz/reparaciones.py` | Panel manual con motivos, vista previa, cancelación y aplicación explícita. |
| `ide/sugerencias_ccom.py` | Adaptador productivo del motor C Flex/Bison-LAC (`--suggest-json`). |
| `ide/reparaciones_ccom.py` | Adaptador productivo de reparaciones y comprobación sintáctica C. |

Importar los tipos compartidos; **no volver a definir** `EdicionTexto`, `Sugerencia`, `Reparacion` ni los resultados dentro de los motores. El LL(1) conserva sus propios tipos de consulta en `ide/nucleo/parser.py`, fuera de los modelos de propuestas productivas.

Las funciones Python del contrato reciben texto en memoria y devuelven datos. Sus adaptadores invocan los servicios de ccom sobre copias temporales; las decisiones del lenguaje están en C. No acceden a widgets ni guardan los documentos originales. Una reparación no equivale a validación semántica. La UI presenta propuestas manuales; los disparadores y permisos de automatización están implementados en la [política del paso 6](asistencia.md).

## Funciones de los adaptadores implementados

```python
from typing import Callable
from ide.nucleo.asistencia import ResultadoSugerencias, ResultadoReparaciones

# ide/sugerencias_ccom.py
sugerir(texto: str, cursor: int, *,
        cancelar: Callable[[], bool] | None = None) -> ResultadoSugerencias

# ide/reparaciones_ccom.py
proponer_reparaciones(texto: str, *,
        cancelar: Callable[[], bool] | None = None) -> ResultadoReparaciones
```

Estas funciones están implementadas por los adaptadores C, compatibles con los protocolos `MotorSugerencias` y `MotorReparaciones`. Añaden `root=None` y `configurado=""` para localizar el ejecutable; el coordinador los captura. `cancelar=None` significa que no se proporcionó cancelación; cuando existe se pasa al transporte que termina y recoge el hijo. Devolver `CANCELADO` si se interrumpe. Acotar pasos/candidatos/opciones y devolver `LIMITE` cuando no se pueda completar una respuesta fiable. No ocultar fallos internos como una lista vacía.

## Posiciones y textos

- Offsets **globales de caracteres Python**, desde 0. No son bytes UTF-8, columnas de pantalla ni unidades UTF-16.
- Rangos `[inicio, fin)` sobre el **texto original recibido**. Inserción: `inicio == fin`; eliminación: `texto == ''`.
- Todas las ediciones de una propuesta usan la misma versión original, aunque se apliquen juntas.
- El texto del editor se captura con `toPlainText()`: sus saltos de línea son LF. Emitir texto nuevo con `\n` y espacios normales.
- El aplicador rechaza texto insertado con CR, separadores U+2028/U+2029 o espacio U+00A0, que Qt normaliza al trabajar en texto plano. No se normalizan silenciosamente propuestas cuyos cursores dependan de esas posiciones. Se rechazan sustitutos Unicode aislados.
- Unicode fuera del BMP, acentos y tabulaciones cuentan según Python. La adaptación a Qt pertenece al IDE; una posición UTF-16 que parte un par sustituto se rechaza.
- Los archivos CRLF se normalizan al cargarse en el editor; el motor trabaja sobre esa copia, sin escribir el archivo.

## Contexto de cada solicitud

`ContextoDocumento(documento, revision, texto, cursor, ancla)` es una copia inmutable que captura el IDE. `cursor` es el extremo activo de la selección y `ancla` el otro extremo, ambos como offsets Python. No es obligatorio enviar estos metadatos al motor: el coordinador los conservará junto a la tarea.

Antes de aplicar se comparan **todos** esos campos con el editor actual. Otra pestaña, texto/revisión distintos, cursor movido o selección invertida invalidan la respuesta. Deshacer hasta recuperar el mismo texto no recupera la revisión anterior. Un editor destruido también invalida la solicitud. Guardar como no cambia por sí solo la identidad del documento: este contrato de motores no depende de rutas ni de símbolos de otros archivos.

## Resultados y estados

Usar `EstadoAsistencia`, no cadenas arbitrarias:

| Estado | Significado | Opciones aplicables |
|---|---|---|
| `OK` | Respuesta válida y no vacía. | Sí. |
| `SIN_OPCIONES` | Consulta completada sin opciones. | No. |
| `BLOQUEADO` | Contexto no disponible, comentario o error que impide proponer. | No. |
| `CANCELADO` | Solicitud interrumpida. | No. |
| `LIMITE` | No se completó una respuesta fiable dentro de los límites. | No. |
| `ERROR` | Fallo informado por el motor. | No. |

`ResultadoSugerencias(estado, sugerencias=(), explicacion='')` y `ResultadoReparaciones(estado, propuestas=(), explicacion='')` convierten las colecciones a tuplas. Solo `OK` acepta opciones y debe contener al menos una. `BLOQUEADO`, `LIMITE` y `ERROR` requieren explicación. Un constructor inválido genera un error de contrato; el futuro coordinador deberá presentarlo como fallo, no como análisis correcto.

### Sugerencia

```python
Sugerencia(etiqueta, texto, inicio, fin, categoria,
           detalle='', cursor_relativo=None)
```

- `etiqueta`, `categoria` y `detalle` son textos de presentación; etiqueta y categoría no pueden estar vacías.
- `texto` es código insertable real: sin marcadores de snippet ficticios.
- `inicio/fin` abarcan el fragmento completo que se reemplaza, incluido su sufijo cuando el cursor está en medio de una palabra.
- `cursor_relativo` es una posición Python dentro de `texto`, entre 0 y `len(texto)`. `None` coloca el cursor al final de la inserción. Aceptar colapsa la selección en ese punto.
- `.edicion` produce el `EdicionTexto` correspondiente; `.cursor_final` calcula la posición absoluta resultante.

Ejemplo de estructura de respuesta, sin implementar el razonamiento del motor:

```python
from ide.nucleo.asistencia import EstadoAsistencia, ResultadoSugerencias, Sugerencia

texto = 'Vm total : in'
opcion = Sugerencia('int', 'int', len(texto) - 2, len(texto), 'tipo', 'Tipo entero')
respuesta = ResultadoSugerencias(EstadoAsistencia.OK, (opcion,))
```

El motor real todavía debe justificar esa opción con el contexto del parser. No insertar literalmente `id`, `num`, `$` o `ε` como si fueran propuestas Calser.

### Reparación

```python
Reparacion(codigo, descripcion, ediciones,
           automatica_segura=False, razones=(), precondiciones=(),
           validacion='', cursor_final=None)
```

- `codigo`: identificador estable de la regla; `descripcion`: explicación visible del error y del cambio.
- `ediciones`: una o más instancias de `EdicionTexto(inicio, fin, texto)` sobre el original.
- `razones`: tupla de explicaciones; `validacion`: resumen del reanálisis, error objetivo resuelto y alcance comprobado. No afirma corrección semántica.
- `precondiciones`: tupla de `PrecondicionTexto(inicio, fin, texto_esperado)`; el aplicador comprueba esas porciones del original antes de tocarlo.
- `cursor_final`: posición Python **absoluta sobre el resultado completo**. Si es `None`, se trasladan cursor y selección existentes de acuerdo con las ediciones.
- `automatica_segura` es falso por defecto. Si es verdadero, el modelo exige razones y validación no vacías. Eso **no prueba seguridad ni autoriza aplicación automática**: se exige catálogo revisado con contraejemplos, permiso del usuario, disparador y política del IDE.

El catálogo debe explicar ambigüedades y comentarios, conservar texto ajeno al rango y reanalizar candidatos con el parser real. No basta con que un candidato pase sintaxis para volverlo automático.

## Validez del lote de ediciones

El aplicador valida todo antes de modificar:

- Tipos, rangos y cursores enteros; no admite bool como offset.
- Rangos dentro del original, precondiciones satisfechas y cursor explícito dentro del resultado.
- Sin solapamientos ni inserciones duplicadas en el mismo offset.
- Se permiten reemplazos adyacentes. Una inserción que comparte el inicio o fin de otro rango se considera ambigua y debe fusionarse en el motor en una sola edición con el orden deseado.
- Máximo **128 ediciones**, **128 precondiciones** y **2 millones de caracteres** en original/resultado.

El orden de llegada no importa. `ide.ediciones.aplicar_ediciones` transforma una copia del texto para validación y pruebas; no toca el editor. En Qt, las modificaciones reales se aplican de derecha a izquierda para no desplazar los offsets pendientes.

## Aplicador disponible para el IDE

```python
from ide.interfaz.ediciones import aplicar_sugerencia, aplicar_reparacion

contexto = editor.contexto_asistencia()  # Capturar antes de enviar la solicitud.
# ... conservar contexto junto al trabajo; recibir una respuesta válida ...
resultado = aplicar_sugerencia(editor, contexto, opcion)
# O bien: aplicar_reparacion(editor, contexto, reparacion)
```

Para aplicar datos de edición sin un tipo de opción:

```python
resultado = editor.aplicar_ediciones(
    contexto,
    ediciones,
    cursor_final=None,
    precondiciones=(),
)
```

Todas estas operaciones se hacen en el hilo gráfico; el aplicador rechaza llamadas desde trabajadores. El editor debe estar abierto y ser editable. No se usa `setPlainText`: se agrupan cambios locales con un bloque de edición de Qt. Una aceptación es **una operación de Deshacer**, mantiene el historial previo y permite Rehacer. El documento queda modificado; no se guarda ni se compila.

Sin cursor explícito, los extremos de una selección se trasladan conservando su dirección. Si un extremo cae dentro de texto reemplazado, se elige el borde correspondiente; un cursor sin selección queda en el borde derecho. Las inserciones anteriores desplazan los extremos. Con cursor explícito se colapsa la selección en el punto indicado.

`ResultadoAplicacion.estado`:

| Estado | Efecto |
|---|---|
| `aplicada` | Texto editado; `.aplicada` vale `True`. |
| `sin_cambios` | El resultado textual sería idéntico; no cambia cursor, selección, modificación ni historial. |
| `obsoleta` | Contexto anterior, documento distinto o editor cerrado. No cambia nada. |
| `invalida` | Datos, rangos, precondiciones, límite o hilo incorrectos. No cambia nada. |
| `solo_lectura` | Editor bloqueado. No cambia nada. |

Los rechazos incluyen `.explicacion`; no abren diálogos por su cuenta. Las señales normales del Editor actualizan la pestaña, invalidan diagnósticos anteriores y programan de nuevo el análisis. La UI muestra explicación, cancela listas obsoletas y aplica la política de asistencia.

## Coordinador y conexión disponibles

`CoordinadorAsistencia.solicitar(tipo, contexto)` acepta `sugerencias` o `reparaciones` y devuelve una `SolicitudAsistencia(numero, tipo, contexto)`. Su señal `resultado` entrega `RespuestaAsistencia(solicitud, resultado)`. El trabajo solo recibe datos inmutables y se ejecuta fuera del hilo gráfico.

Hay una tarea activa global y una pendiente por documento/tipo. La consulta más reciente reemplaza a la pendiente anterior de esa clave e interrumpe cooperativamente la activa si corresponde. `invalidar(documento=None, tipo=None)` retira pendientes y evita publicar respuestas anteriores. Cambiar el proveedor también invalida sus solicitudes. No se usan pilas mutables compartidas ni se llaman simultáneamente ambos motores.

Límites del coordinador: 2 millones de caracteres, 32 solicitudes pendientes, 64 sugerencias o 32 reparaciones por respuesta y plazo de 3 segundos por tarea. La respuesta completa se valida antes de publicarse: tipos compartidos, estados, rangos/precondiciones y cursores. Cada sugerencia debe abarcar el cursor solicitado. Los fallos de contrato y excepciones se convierten en `ERROR`; un resultado vacío no oculta un fallo.

El timeout solicita cancelación y emite `LIMITE` solo para la consulta vigente. Si un motor ignora el callback, su hilo sigue ocupado hasta retornar: no se lanza otro en paralelo ni se destruye el hilo por la fuerza. `detener()` invalida solicitudes y espera hasta 1500 ms; si no termina, el IDE aplaza el cierre y explica el motivo. Esto no desactiva futuras solicitudes si el cierre se cancela.

### Cómo conectar las entregas reales

La carga normal es diferida, dentro del trabajador:

- `ide.nucleo.sugerencias.sugerir` para sugerencias.
- `ide.nucleo.correccion.proponer_reparaciones` para reparaciones.

Si los compañeros entregan esos módulos, firmas y tipos, reiniciar CALSIDE permite usarlos sin cambiar el widget. La ausencia del módulo produce `BLOQUEADO` con mensaje «motor todavía no disponible». Un módulo existente que falla al importar una dependencia, no exporta la función o devuelve datos inválidos produce `ERROR`, no se oculta como motor ausente. No se incluye una lista de palabras ni motor ficticio como alternativa.

Para pruebas o un adaptador explícito se puede inyectar un proveedor:

```python
ventana.asistencia.establecer_motor('sugerencias', funcion_sugerir)
ventana.asistencia.establecer_motor('reparaciones', funcion_reparar)
# None restablece la importación normal del módulo real.
```

El coordinador también expone `establecer_motor`. El proveedor debe aceptar `cancelar` por nombre y respetar el contrato. Los proveedores controlados del repositorio viven únicamente en las pruebas; no se activan en el programa normal.

## UI manual de sugerencias disponible

- **Ctrl+Espacio dentro del editor**, o Editar → Sugerencias de código, captura el contexto y solicita opciones. El atajo no roba el foco a búsqueda, consolas ni terminales.
- La lista se coloca junto al cursor y se limita al área disponible de pantalla. Muestra etiqueta, categoría, detalle de la opción activa y ayudas de teclado.
- Flechas arriba/abajo y RePág/AvPág recorren opciones. **Enter/Tab** o clic en una opción aceptan mediante el aplicador común; no insertan un salto o tabulación adicional.
- **Esc** cierra sin modificar. Escribir otra tecla cierra y deja que la tecla actúe normalmente en el editor; solo vuelve a consultar tras una pausa si esa preferencia está habilitada.
- Mover el cursor/cambiar selección, editar, desplazar el editor, cambiar pestaña, perder foco o cerrar el documento invalida solicitudes y lista. Un clic fuera de la lista también la cierra.
- Se conserva el foco en el editor y se comprueba el contexto otra vez antes de aplicar. Aceptar es una operación de Deshacer; no guarda.
- Estados sin opciones, bloqueo, límite, cancelación y error se informan en la barra de estado sin abrir listas vacías ni diálogos por cada consulta.

## UI manual de reparaciones disponible (paso 5)

**Alt+Enter dentro del editor**, Editar → Reparaciones de código o **Buscar reparaciones** en la pestaña Reparaciones consulta el documento actual. El atajo no intercepta terminales ni búsqueda. El panel se adapta a la distribución inferior o derecha y permite ajustar sus divisiones.

La lista muestra las propuestas del motor. **Cambios** presenta cada edición con línea/columna del original, `−` para el texto anterior y `+` para el posterior; **Motivo** muestra regla, razones y validación declaradas por el motor. Seleccionar o consultar no modifica el documento. La etiqueta `automatica_segura` se informa, pero nunca activa una aplicación por su cuenta.

**Aplicar reparación** es la única acción que modifica el editor. Usa el aplicador común: valida de nuevo todo el contexto y las precondiciones, agrupa los cambios en una operación de Deshacer y no guarda ni compila. **Cancelar** o **Escape** descarta la consulta/propuestas sin modificar texto ni historial.

Editar, mover el cursor/cambiar selección, cambiar pestaña o cerrar el documento invalida la propuesta y deshabilita Aplicar. Dar foco al panel, seleccionar su texto o desplazar el editor no invalida una reparación; estos eventos sí cierran sugerencias. Una respuesta cancelada o antigua no vuelve a habilitar Aplicar.

Límite de vista previa: **12000 caracteres afectados**, sumando los eliminados y los insertados. Dentro de ese límite se muestra el cambio completo, con hasta 60 caracteres de contexto inalterado a cada lado en la misma línea. Las propuestas mayores se explican y quedan deshabilitadas para aplicar desde este panel; no se aprueban cambios ocultos por truncamiento. La metadata explicativa se limita por separado a 4000 caracteres.

El controlador emite `reparacionesRecibidas(RespuestaAsistencia)` y el panel consume los tipos compartidos sin inventar resultados. La ausencia del módulo, el error, el bloqueo y la falta de opciones se muestran explícitamente. Los proveedores controlados solo existen en pruebas.

El panel no publica diagnósticos ni reemplaza Problemas: conserva las prioridades actuales entre compilación manual, ccom automático y lexer. Aplicar ediciones invalida los diagnósticos antiguos mediante el flujo normal del editor y vuelve a programar su análisis. Los diagnósticos visibles del lenguaje proceden de ccom; las propuestas no crean otra lista de errores Python. La ruta léxica y el análisis completo se identifican por su origen y revisión.

## Integración vigente

1. [x] Proveedores C de sugerencias y reparaciones conectados con modelos compartidos.
2. [x] Coordinador serial con contexto, cancelación y descarte de respuestas antiguas.
3. [x] Popup manual de sugerencias y aplicación validada.
4. [x] Panel de reparaciones, vista previa y aplicación explícita.
5. [x] Preferencias, pausa opcional y regla automática revisada de punto y coma.
6. [x] Casos del lenguaje probados contra C real; dobles limitados a coordinación/transporte/UI.

La automatización viene apagada; exige habilitarla y seleccionar INSERCION_PUNTO_COMA. El catálogo ya no está vacío. Una propuesta parcial nunca se automatiza. [Política y catálogo](asistencia.md), [protocolo C de sugerencias](../../docs/ccom_servicios_ide.md#suggest), [protocolo de reparaciones/sintaxis](../../docs/ccom_servicios_ide.md#repair).

## Registro histórico de las entregas iniciales

Verificación: pasos 1/2, 23 pruebas nuevas y 227 combinadas en su entrega; pasos 3/4, **26 pruebas nuevas y 253 combinadas correctas** con `make check-ide`. El paso 5 añade **15 pruebas; 268 combinadas correctas**, incluyendo revisión sin editar, aplicar/Deshacer/Rehacer sin guardar, cancelación, foco del panel, contexto antiguo, pestañas cerradas, estados sin motor y distribución inferior/derecha. Se cubren proveedores ausentes/rotos, contratos inválidos, hilos seriales, sustitución de pendientes, cancelación, timeout, cierre aplazado, foco, teclado/ratón, Unicode, contexto obsoleto, geometría y ninguna aplicación implícita de reparaciones manuales. El paso 6 añade **26 pruebas; 294 combinadas correctas**, incluyendo automatización con reglas controladas de prueba, comprobación del parser en segundo plano, Deshacer sin bucles y cancelación al cambiar permisos/foco/contexto.

Las solicitudes internas del coordinador añaden `origen` (`manual`, `escritura`, `linea_confirmada`) y reglas revisadas capturadas. La respuesta interna añade `automatica` y `motivo_automatico` tras evaluar en el trabajador. Estos campos no cambian las firmas ni los modelos públicos de los motores. Las consultas manuales nunca activan esta ruta automática.


</details>


<a id="hist-proyecto-archivos"></a>

<details>
<summary>proyecto_archivos.md — documento sustituido</summary>

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


</details>


<details>
<summary>Referencias de protocolos antes de retirar sus notas históricas de entrega</summary>

# Servicios de ccom para CALSIDE

Referencia técnica vigente · 2026-11-11. Reúne los seis protocolos y la recuperación sintáctica. Los comandos reciben copias privadas; no editan documentos originales. Python adapta transporte/modelos/posiciones y C decide el lenguaje.

| Operación | Opción | Sección |
| --- | --- | --- |
| Análisis léxico/sintáctico/semántico | `--check-json` | [Diagnósticos](#check) |
| Léxico para colores | `--tokens-json` | [Tokens](#tokens) |
| Continuaciones en el cursor | `--suggest-json <bytes>` | [Sugerencias](#suggest) |
| Propuestas y comprobación completa | `--repair-json`, `--syntax-json` | [Reparaciones](#repair) |
| Comparación de varias copias | `--clones-json <mínimo>` | [Clones](#clones) |
| Varios errores sintácticos | Sin bandera adicional | [Recuperación](#recovery) |

La cabecera común se valida en `ide/ccom.py:informe_json`; los estados y el contenido específico se validan en cada adaptador. No confundir `syntax` (solo sintaxis) con `check` (también semántica). [Arquitectura del IDE](guia_sistema_calside.md), [asistencia y permisos](asistencia.md).


<a id="check"></a>

## Análisis completo y diagnósticos

Implementado el 2026-11-11. La interfaz llama al front-end existente en C y recibe datos estructurados. No hay otra gramática, parser ni semántico para esta operación.

### Invocación y compatibilidad

```sh
./ccom --check-json programa.cls
./ccom --check-json -v --time programa.cls
```

Ejecuta exactamente las fases léxica, sintáctica y semántica de `--check`. No genera ni modifica `.bin`, `.hex`, `.mem`, `.asm` o AST JSON. Solo admite las opciones auxiliares `-v`/`--time`; combinarlo con `--check` es redundante y válido. Las opciones de salidas, ensamblado y listados se rechazan antes de abrir la entrada.

- **stdout:** un objeto JSON UTF-8, versión 1, seguido por salto de línea.
- **stderr:** resúmenes, progreso, tiempo y errores del proceso. Los diagnósticos individuales van en el JSON sin duplicar sus señaladores textuales.
- **Retorno 0:** análisis correcto, sin diagnósticos.
- **Retorno 1:** errores del programa, límite de informe o fallo del servicio; `status` distingue esos casos.
- **Retorno 2:** uso incorrecto o versión de ccom que no admite la opción; no se promete JSON en estos casos.

Una ayuda solicitada con `--help` mantiene su funcionamiento normal. La compilación F5 y las invocaciones clásicas conservan sus mensajes de texto; esta opción no cambia el lenguaje ni introduce recuperación sintáctica.

### Esquema v1

Ejemplo de error semántico; los offsets son ilustrativos:

```json
{
  "schema_version": 1,
  "operation": "check",
  "offset_unit": "utf8",
  "source": "/ruta/instantanea.cls",
  "status": "errors",
  "exit_code": 1,
  "error_count": 1,
  "truncated": false,
  "message": "",
  "diagnostics": [
    {
      "code": "CCOM_SEMANTIC",
      "category": "semantic",
      "severity": "error",
      "message": "'a' no ha sido declarado",
      "range": {"start": 22, "end": 23, "line": 2, "column": 5}
    }
  ]
}
```

| Campo | Significado |
| --- | --- |
| `schema_version` | Entero 1; no booleano ni versión que el cliente deba adivinar. |
| `operation` | `check`. No implica que existan ya operaciones JSON de tokens, sugerencias o reparación. |
| `offset_unit` | `utf8`: offsets y columnas en bytes del fuente original. |
| `source` | Argumento de entrada tal como lo recibió ccom. El IDE envía una ruta absoluta de su copia temporal y verifica la coincidencia. |
| `status` | `ok`, `errors`, `error` o `limit`. |
| `exit_code` | Debe coincidir con el retorno del proceso. |
| `error_count` | Cantidad total reportada por el front-end. Si hay truncamiento puede superar el número conservado en la lista. |
| `truncated` | Booleano: el informe no incluye todos los mensajes. Nunca equivale a análisis correcto. |
| `message` | Explicación del fallo del servicio o límite; vacío para resultados normales. |
| `diagnostics` | Lista de diagnósticos, vacía en un análisis correcto. |

`ok` requiere retorno 0 y lista/contador vacíos. `errors` requiere retorno 1, lista no vacía y contador coincidente, sin truncamiento. `error` indica un fallo de servicio, por ejemplo fuente ilegible, memoria insuficiente para capturar mensajes o formato no representable. `limit` indica un informe incompleto.

Cada diagnóstico transporta categoría y código decididos en C:

| Categoría | Código de grupo estable | Presentación |
| --- | --- | --- |
| `lexical` | `CCOM_LEXICAL` | Léxico |
| `syntax` | `CCOM_SYNTAX` | Sintáctico |
| `semantic` | `CCOM_SEMANTIC` | Semántico |
| `compilation` | `CCOM_COMPILATION` | Compilación |

Son códigos de grupo, no un catálogo de reglas individuales de reparación. `severity` es `error` en v1. `message` es el detalle sin encabezado añadido; el cliente no deduce la categoría a partir de sus palabras ni recorta un prefijo supuesto.

### Posiciones y Unicode

`range` puede ser `null` cuando no hay una ubicación representable en la fuente cargada, incluidos los errores generales como falta de `main`.

Cuando existe:

- `start` y `end` son offsets globales en bytes UTF-8, base 0, con rango `[start, end)`.
- `line` y `column` comienzan en 1; la columna también cuenta bytes. Una tabulación cuenta un byte.
- Los offsets conservan CR/LF originales; el servicio no calcula rangos sobre otra versión normalizada.
- Un punto al final de una línea puede ser un rango vacío; no se extiende sobre el token siguiente.
- El final no sobrepasa el contenido de esa línea. No se inventa una posición cuando el compilador no puede relacionarla con el fuente cargado.

El lexer actual puede reportar por separado cada byte de un carácter no ASCII inválido para Calser. Python conserva el rango nativo en `Problema.rango_utf8`, y amplía **solo la marca visual** a fronteras de caracteres UTF-8 completos para que Qt no seleccione medio carácter. Esta adaptación no cambia las reglas del lexer ni fusiona diagnósticos.

Las futuras ediciones de reparación requerirán fronteras válidas: este permiso de ampliar una marca visual no autoriza redondear cambios del documento.

### Implementación y ciclo de vida

1. `main.c` reconoce la opción y activa la captura antes de compilar.
2. `diag_init` carga las líneas y sus offsets reales. Lexer, parser y semántico siguen invocando `diag_error`/`diag_verror`.
3. Un observador recibe categoría, posición y argumentos de formato prestados. El serializador copia solo el mensaje necesario; no retiene punteros del AST o de las líneas.
4. Se ejecutan y liberan las mismas fases de análisis de la compilación normal.
5. `diag_json_finish` escribe la respuesta, libera los mensajes capturados y desconecta el observador. Un fallo de escritura cambia el retorno a 1 y puede dejar una respuesta incompleta que el cliente debe rechazar.

Archivos C: `include/comun/diagnostics.h`, `src/comun/diagnostics.c`, `include/servicios_ide/diagnostics_json.h`, `src/servicios_ide/diagnostics_json.c` y la integración de CLI en `src/main.c`.

El servicio es un proceso por solicitud. Su estado global no se comparte entre documentos ni se presenta como una biblioteca C reentrante.

### Límites y cancelación

La captura conserva hasta **128 diagnósticos**, **4096 bytes por mensaje** y **65536 bytes de mensajes acumulados**. Si se supera cualquiera, se informa `limit`/`truncated`; el compilador conserva su conteo real y puede seguir analizando. El tamaño del informe se mantiene acotado.

En el IDE se mantienen el timeout de 5 segundos y el límite de 1 MB por canal. Cancelar una instantánea detiene su QProcess, limpia temporales y evita publicar resultados antiguos. Si el proceso se interrumpe antes de emitir JSON completo, no se interpreta como éxito.

### Adaptación en CALSIDE

- `ide/ccom.py:diagnosticos_json` valida UTF-8, versión, operación, fuente, estado/retorno, contadores, lista, categorías y rangos. Rechaza claves duplicadas, constantes no JSON, Unicode inválido y datos inconsistentes.
- `Problema` conserva `codigo`, `categoria_explicita` y `rango_utf8`. Los lectores clásicos siguen funcionando con los campos previos.
- `ComprobadorCcom` solicita `--check-json` sobre el buffer temporal, reasocia cada problema al documento original y presenta un log legible. No llama al lector de mensajes humanos como alternativa silenciosa.
- Una versión antigua de ccom, una respuesta incompatible o `status=error/limit` produce «No disponible» con explicación. No se publica una lista vacía como si el programa fuera correcto.
- F5 conserva su invocación y lector textual en M1; F6 conserva la simulación. El lexer Python y el LL(1) siguen presentes en esta etapa. El filtro léxico previo de E3 todavía decide cuándo programar la consulta automática; el modo C puede consultarse directamente para errores léxicos.

**Múltiples errores:** la lista ya transporta varios errores léxicos o semánticos emitidos por el front-end. El parser Bison continúa deteniéndose en el primer fallo sintáctico. La recuperación de varios errores sintácticos es una ampliación posterior, no una capacidad añadida por JSON.

### Verificación

```sh
make
make check-analysis-json
make check
make check-ide
```

M1: **312 pruebas del IDE correctas**, incluidas **18 nuevas** de protocolo y UI. La suite completa del compilador también pasó. Se validaron además las respuestas de las **91 fuentes `.cls` de la carpeta principal de pruebas** con el adaptador del IDE, sin incompatibilidades.

Las pruebas específicas cubren fases reales, varios errores, errores generales, tildes/emoji, CRLF, rutas con espacios/comillas, límites, opciones inválidas, artefactos intactos, respuestas malformadas/versiones incompatibles y navegación. Las pruebas previas de procesos conservan cancelación, timeout, cierre, Guardar como, limpieza de temporales y prioridad de F5.

### Operación léxica complementaria (M2)

El resaltado del editor consulta [--tokens-json](../../docs/ccom_servicios_ide.md#tokens): utiliza el mismo lexer, sin parser, y añade comentarios/rangos para presentación. Su operación es `lex`; no sustituye ni altera el contrato `operation: check` de este documento.

<a id="tokens"></a>

## Tokens y resaltado

Implementado el 2026-11-11. Reutiliza `grammar/lexer.l`: no llama al parser ni al semántico y no genera artefactos. Puede resaltar documentos incompletos y tokens posteriores a un error sintáctico. El lenguaje y los modos clásicos de compilación mantienen sus reglas.

```sh
./ccom --tokens-json programa.cls
./ccom --tokens-json -v --time programa.cls
```

Solo se admite una entrada y opciones auxiliares `-v`/`--time`. Se rechazan listados/salidas y combinaciones con `--check`/`--check-json`. Retornos 0 (léxico correcto), 1 (errores, fallo o límite) y 2 (uso incompatible, sin prometer JSON). `--help` conserva su funcionamiento.

### Transporte v1

Comparte el sobre y el contrato de diagnósticos de [--check-json](../../docs/ccom_servicios_ide.md#check), cambiando `operation` a `lex` y añadiendo `fatal` y `tokens`:

```json
{
  "schema_version": 1,
  "operation": "lex",
  "offset_unit": "utf8",
  "source": "/ruta/documento.cls",
  "status": "ok",
  "exit_code": 0,
  "error_count": 0,
  "truncated": false,
  "message": "",
  "diagnostics": [],
  "fatal": false,
  "tokens": [
    {"kind": "tipo", "start": 0, "end": 3},
    {"kind": "identificador", "start": 4, "end": 5}
  ]
}
```

Este ejemplo corresponde a `int x`. Rangos absolutos de bytes UTF-8 **[start,end)**, ordenados, no vacíos y sin solapamientos. No se envían lexemas: el receptor ya tiene la instantánea exacta. `kind` es una clasificación visual decidida en las acciones de Flex, no el código interno de token Bison:

| kind | Contenido |
| --- | --- |
| `reservada` | Palabras reservadas, incluidas las operaciones criptográficas. |
| `tipo` | int, bool, void. |
| `booleano` | verdadero y falso. |
| `numero` | Literales enteros. |
| `identificador` | Identificadores reconocidos por Flex. |
| `operador` | Operadores y delimitadores. |
| `comentario` | Comentarios de línea o bloque, incluidos sus delimitadores. |

Espacios fuera de comentarios y EOF no generan tramos. Un comentario de línea no incluye LF (sí el CR si la fuente usa CRLF); un bloque incluye sus saltos y su cierre. Un bloque abierto produce un tramo hasta EOF, `fatal: true` y el diagnóstico nativo. No se buscan comentarios de nuevo en Python.

Los caracteres inválidos producen diagnósticos, no tokens inventados. Flex mantiene su recuperación real: un byte desconocido se descarta, un identificador largo se trunca para Bison y un entero fuera de rango se sustituye por cero. Los rangos de color cubren siempre el lexema original. Los comentarios de bloque no se anidan.

### Implementación y límites

- `YY_USER_ACTION` conserva el contador absoluto de bytes; `LEX_RETURN` publica clasificación antes de devolver el token habitual. En compilación clásica el observador está inactivo.
- `lexer_scan_only` reinicia el estado de Flex, recorre hasta EOF y libera inmediatamente las cadenas de identificadores que de otro modo consumiría Bison.
- `diagnostics_json.c` captura los mismos errores y añade los tramos. Máximo 100000 tramos, 128 diagnósticos, 4096 bytes por mensaje y 65536 bytes de mensajes. Un exceso produce `status: limit`, retorno 1 y explicación; no equivale a un documento válido.
- La entrada de esta operación se limita a 8 millones de bytes. Un archivo ausente/demasiado grande devuelve `status: error`, logs en stderr y retorno 1.
- stdout contiene únicamente el objeto JSON; logs, progreso y duración van en stderr.

### Integración CALSIDE

`Editor` espera 300 ms después de escribir; `CoordinadorAnalisis` conserva una tarea activa y la última solicitud por documento. `TareaLexica` llama al adaptador en un QThread. El adaptador usa `subprocess.Popen` sin shell y un temporal privado UTF-8; los buffers del usuario no se guardan.

El servicio admite 2 millones de caracteres por instantánea en el IDE, 5 s para el proceso, 16 MB de stdout y 1 MB de stderr. Las salidas se recogen en archivos temporales y se vigila su tamaño. Cancelación, edición y cierre solicitan interrupción; se mata y recoge el hijo y se eliminan temporales. No se actualizan widgets desde el trabajador.

`ccom.lexico_json` valida versión, operación, código, fuente, listas, categorías y fronteras de UTF-8 antes de aceptar el informe. `convertir_lexico` transforma los rangos a UTF-16 por línea para Qt; solo divide comentarios multilínea para pintarlos. Los diagnósticos conservan offsets nativos y origen **ccom léxico**. Un error por byte de un emoji conserva su identidad; la selección visual se amplía al carácter completo.

Al recibir, el editor comprueba documento/revisión. Los colores no cambian texto, guardado ni Deshacer. `Compilar → Configurar ccom` vuelve a solicitar léxico con el ejecutable elegido. Sin una versión compatible se informa el fallo: no hay fallback al lexer Python.

El análisis sintáctico/semántico automático sigue usando `--check-json` después de otra pausa de 600 ms y solo si el léxico está libre de errores. F5/F6 mantienen los perfiles M1B. El LL(1), el tokenizador de clones y la validación de reparaciones Python se conservan en sus recorridos pendientes; M2 retira su uso del resaltado y diagnóstico léxico vivo.

### Verificación

`make check-tokens-json`, incluido en `make check`: clasificación, comentarios, Unicode/CRLF, error léxico, EOF fatal, continuación sin parser, límites, opciones y salidas intactas. `make check-ide`: transporte inválido, posiciones Qt, cancelación real, proceso antiguo, edición/pestañas y Deshacer. Pruebas nuevas en `ide/pruebas/test_tokens_ccom.py`; la suite de comprobación usa Flex real al simular fallos únicamente de `--check-json`.

M3 comparte este transporte en `ide/servicio_ccom.py` y añade [sugerencias desde Flex/Bison-LAC](../../docs/ccom_servicios_ide.md#suggest). Los límites léxicos y el contrato de tokens M2 se conservan.

<a id="suggest"></a>

## Sugerencias

Implementado el 2026-11-11. Adapta el comportamiento de `origin/feat/reparaciones-sugerencias` a **Flex + Bison/LAC**, sin importar ni fusionar las dataclasses duplicadas de esa entrega. Reutiliza el lexer y las producciones de ccom. No llama al parser LL(1) Python, no ejecuta semántica, no repara errores ni genera artefactos.

### Uso

```sh
./ccom --suggest-json 14 programa.cls
./ccom --suggest-json 14 -v --time programa.cls
```

El cursor es un offset absoluto **en bytes UTF-8**, en una frontera entre caracteres. Para `F main [] : vo`, el cursor 14 ofrece `void` con rango `[12,14)`. En el IDE esta conversión se realiza desde el cursor Unicode del documento, que a su vez fue convertido desde Qt/UTF-16.

Solo admite una fuente y `-v`/`--time`. No se combina con `--check`, `--check-json`, `--tokens-json`, listados, salidas ni ensamblado. Un cursor no numérico/negativo es uso incorrecto (retorno 2); fuera de la fuente o en medio de UTF-8 produce estado error (retorno 1).

### Respuesta JSON v1

```json
{
  "schema_version": 1,
  "operation": "suggest",
  "offset_unit": "utf8",
  "source": "/ruta/programa.cls",
  "cursor": 14,
  "exit_code": 0,
  "status": "ok",
  "explanation": "",
  "suggestions": [
    {
      "label": "void",
      "text": "void",
      "start": 12,
      "end": 14,
      "category": "palabra",
      "detail": "Continuacion permitida por el parser de ccom (Bison/LAC)."
    }
  ]
}
```

| Estado | Retorno | Significado |
| --- | --- | --- |
| `ok` | 0 | Una o más propuestas aplicables al fragmento consultado. |
| `sin_opciones` | 0 | Contexto consultable sin terminales concretos que ofrecer. |
| `bloqueado` | 0 | Cursor dentro de comentario o error anterior que impide determinar el contexto. |
| `limite` | 1 | Fuente/tokens/opciones por encima del límite. |
| `error` | 1 | Fallo de archivo, cursor o consulta interna. |

Solo `ok` contiene propuestas. Todos los demás estados incluyen explicación y lista vacía. Cancelar mata el proceso desde el adaptador; no exige que el hijo emita JSON al ser interrumpido. stdout se reserva para el JSON; progreso y tiempo van a stderr.

### Cómo calcula las propuestas

1. `suggestions_run`, en `src/servicios_ide/suggestions.c`, lee la fuente acotada e instala un observador de diagnósticos silencioso.
2. Flex recorre la fuente completa y publica sus rangos mediante `LexerObserver` (`include/analisis/lexer_service.h`). Se detectan comentarios, fragmento alrededor del cursor y errores anteriores. Los errores del sufijo posterior no bloquean el prefijo consultado.
3. Para palabras/números, el fragmento se toma cuando `inicio < cursor <= fin`; para operadores extensibles `= ! < >`, cuando `inicio <= cursor < fin`. Los delimitadores y operadores ya completos se consideran parte del prefijo. Un carácter ASCII de reservada incompleta, como `V`, puede observarse como fragmento inválido y completarse a `Vi`/`Vm`; no se modifica la regla de Flex ni se tolera ese error al compilar.
4. Se analiza únicamente el prefijo anterior al fragmento con el parser existente. En modo consulta, `syntax_lex` sustituye el EOF temporal por un **TOKEN_CURSOR privado**, que no pertenece a ninguna producción. `yyreport_syntax_error` consulta `yypcontext_expected_tokens` con **LAC**, distingue ese marcador de un error real anterior y recoge los terminales concretos.
5. Así un programa completo también puede ofrecer continuaciones, sin confundir EOF con un error del usuario. No se derivan reglas a partir de mensajes humanos ni se copia una gramática a Python.
6. C excluye identificadores genéricos, números genéricos, EOF y símbolos internos. Filtra por el prefijo escrito y ordena: palabras, delimitadores, operadores; alfabético dentro de cada grupo.
7. Cada propuesta reemplaza el fragmento completo. En `vo|id` reemplaza `void`, evitando duplicar el sufijo. Se omiten reemplazos idénticos al fragmento ya terminado; tras una palabra completa, un espacio permite consultar las continuaciones siguientes.

Los comentarios de línea incluyen el punto antes de LF; los bloques abiertos incluyen EOF. Después de un bloque cerrado sí se puede consultar. No hay sugerencias dentro de comentarios.

Las propuestas son **continuaciones sintácticas**, no garantías semánticas. No se inventan nombres declarados ni literales numéricos. Consultar ámbitos, completar símbolos declarados o añadir plantillas queda como ampliación explícita posterior.

### Memoria, límites y cancelación

Cada solicitud usa un proceso independiente. Límite nativo: **200000 bytes**, **50000 tokens/tramos** y **64 opciones**. Se informa límite sin publicar una lista parcial como completa. El máximo de opciones del contrato sustituye al límite opcional de 24 de la entrega Python; no se añadió una opción CLI de truncamiento arbitrario.

Flex libera las cadenas del escaneo completo que normalmente consumiría Bison. El escaneo del prefijo reinicia posiciones/estado y sus buffers. Destructores condicionales del parser liberan cadenas y subárboles pendientes al finalizar deliberadamente en el marcador del cursor. La compilación normal conserva su propiedad habitual del AST; el marcador y estos destructores solo actúan en modo consulta. No se reutilizan pilas entre procesos.

`ide/servicio_ccom.py` comparte con M2 el transporte cancelable, sin shell. Captura stdout/stderr en temporales, los vigila y siempre mata/recoge el hijo si se cancela o falla. M3 limita su proceso a **2,5 s**, stdout a **1 MB** y stderr a **1 MB**; el coordinador mantiene su plazo general de 3 s. Las fuentes sin guardar nunca se escriben en la ruta del usuario: se consulta una copia privada eliminada al terminar.

### Conexión al IDE

- `ide/sugerencias_ccom.py`: firma `sugerir(texto, cursor, *, cancelar=None, root=None, configurado='')`, devuelve los tipos de `ide/nucleo/asistencia.py`. Valida versión, operación, fuente, cursor, código, estado, cantidades, rangos, fronteras UTF-8, textos y duplicados. Solo convierte posiciones; no clasifica ni filtra lenguaje.
- `ide/interfaz/asistencia.py`: carga ese adaptador por defecto y captura root/ccom configurado para cada tarea. Reparaciones conserva su ruta pendiente para M4. Proveedores explícitos de pruebas siguen disponibles.
- `AsistenciaEditor`: mantiene **Ctrl+Espacio**, menú Editar y **clic derecho → Sugerencias de código**, lista, Enter/Tab/clic, Escape y consulta tras pausa. Las respuestas se descartan al editar, mover cursor/selección o cambiar/cerrar pestaña. Cambiar ccom invalida la consulta/lista anterior.
- Pausa permanece **desactivada por defecto**: habilitar **Editar → Preferencias de asistencia → sugerencias por pausa**. Mostrar una lista no aplica su primera opción.
- Aplicar usa el contexto completo capturado y una operación de Deshacer, sin guardar. `.asm` no consulta asistencia de Calser.

### Pruebas y pendientes

`make check-suggestions-json`, incluido en `make check`, migra los ejemplos de la entrega y cubre Bison/LAC, palabras parciales, medio de palabra, tipos válidos, EOF de programa completo, comentarios, errores anteriores/posteriores, Unicode/CRLF, determinismo, límites y opciones. Se consultan además múltiples posiciones de una fuente real.

`ide/pruebas/test_sugerencias_ccom.py` verifica el motor real y el contrato, rechazo de JSON incompatible, ausencia de consultas al lexer/LL(1) Python, cancelación/límites, Ctrl+Espacio, pausa, aceptación/Deshacer, configuración de ccom y cancelación del hijo por edición. `make check-ide` comprueba también las interfaces y descartes ya existentes.

Verificación de memoria adicional: compilación temporal con AddressSanitizer/LeakSanitizer, 65 prefijos consultados sin errores ni fugas detectadas. El parser LL(1), el simulador y el tokenizador de clones siguen disponibles en sus recorridos actuales. **M4 sigue pendiente:** motor de reparaciones, validación y autorización de reglas automáticas.

El menú contextual conserva copiar/cortar/pegar y consulta en el punto pulsado (teclado: cursor actual). La consulta se inicia tras cerrar el menú para que la devolución de foco no la invalide. Respeta asistencia desactivada, solo lectura y entradas .asm. El texto de continuación en gris se mantiene como mejora de interfaz pendiente; M3 actual muestra una lista y no inserta automáticamente puntuación adicional.

<a id="repair"></a>

## Reparaciones y validación sintáctica

Implementado el 2026-11-11. Adapta el motor de `origin/feat/reparaciones-sugerencias` a Flex/Bison-LAC, conservando su alcance conservador: **insertar `;`, `.`, `]` o `)`** en el primer error, validar la copia y ofrecer solo candidatos que lo superan. No se fusionó la implementación Python ni sus dataclasses duplicadas.

No inventa identificadores, operandos ni sentencias como `E;`. No repara semántica. Un programa sintácticamente válido que carece de E puede seguir teniendo un error semántico de ccom; en ese caso este motor no propone insertar E.

### Invocaciones

```sh
./ccom --repair-json programa.cls
./ccom --syntax-json programa.cls
```

Ambas admiten `-v` y `--time`, sin otros modos, listados, ensamblado o archivos de salida. No escriben ni modifican artefactos ni el fuente. stdout contiene JSON v1; logs/progreso/tiempo van a stderr. Opciones incompatibles devuelven 2 antes de abrir la entrada.

`--repair-json` devuelve `operation: repair`, `offset_unit: utf8`, `source`, `exit_code`, `status`, `explanation` y `repairs`. Estados:

| Estado | Retorno | Resultado |
| --- | --- | --- |
| `ok` | 0 | Hasta cuatro propuestas validadas para el primer error. |
| `sin_opciones` | 0 | Sintaxis ya válida o ningún cierre resuelve el primer error. |
| `bloqueado` | 0 | Hay errores léxicos; no se modifica esa tokenización. |
| `limite` | 1 | Fuente/tokens/candidato excedidos; sin lista parcial. |
| `error` | 1 | Archivo, memoria o parser no disponibles. |

Cada propuesta contiene:

- `code`: INSERCION_PUNTO_COMA, INSERCION_PUNTO_FINAL, INSERCION_CORCHETE_CIERRE o INSERCION_PARENTESIS_CIERRE.
- `description`, `reasons`: presentación y justificación nativa.
- `edits`: una inserción `{start,end,text}`, con start=end en bytes UTF-8 absolutos.
- `preconditions`: uno o más rangos **no vacíos** `{start,end,expected}` del texto original, no instrucciones narrativas.
- `automatic_safe`: solo candidata a automatización; no es permiso para aplicar.
- `validation`: `accepted`, `resolved`, `reached`, `target` y `description`. reached pertenece a la copia editada; target al error original. Se distingue aceptación completa de progreso parcial.

`--syntax-json` es la verificación independiente usada por la política automática. Devuelve `operation: syntax`, versión/unidad/fuente/código y `status`: `aceptado` (retorno 0), `error_sintactico`, `error_lexico`, `limite` o `error` (retorno 1). Solo comprueba léxico/sintaxis; nunca semántica.

### Motor y memoria

`syntax_service.c` escanea cada copia completa con Flex, observa los rangos de tokens reales y descarta candidaturas si hay errores léxicos. Después llama al parser real con EOF real. El hook `yyreport_syntax_error` consulta LAC y recoge posición del token inesperado, último token válido y los cuatro cierres esperados. No interpreta el texto humano de los diagnósticos.

`repairs.c` enumera únicamente esos cierres, construye una copia por candidato y vuelve a llamar `syntax_analyze`. La copia se acepta como propuesta si el parser acepta todo el programa o supera el token original que causaba el error. En EOF se exige consumir la inserción hasta el EOF posterior. Las propuestas parciales quedan **solo manuales**.

Orden: aceptación completa primero, mayor avance después y catálogo fijo `; . ] )` para desempatar. No se elige automáticamente la primera propuesta de una lista ambigua.

**Posición de inserción:** después del último token válido y antes de sus espacios/comentarios finales. La entrega original insertaba ante el token inesperado de la línea siguiente; aquí un `;` faltante en `Vm x : int` queda como `Vm x : int;`, sin moverlo delante de E ni introducirlo dentro de un comentario. Los rangos de comentarios proceden de Flex; Python no los reconoce de nuevo.

Las precondiciones usan el token válido anterior y su texto exacto; el IDE además exige contexto completo vigente (documento/revisión/texto/cursor/selección) al aplicar. Eso sustituye la huella redundante y las precondiciones narrativas de la entrega, sin convertirlas en rangos vacíos.

Cada análisis reinicia diagnósticos, posiciones, estados Flex, observadores y pila Bison. Los buffers y strings del escaneo se liberan. Los destructores condicionales del parser liberan subárboles/cadenas temporales tanto al aceptar la copia como al abortar; la compilación clásica conserva su propiedad habitual del AST. Las copias candidatas se liberan tras cada intento y no se reutilizan pilas corruptas.

Límites por análisis: **200000 bytes**, **50000 tokens/tramos**, cuatro candidatos. Si cualquier candidato supera límites o falla internamente, se devuelve límite/error sin propuestas parciales. El proceso del IDE tiene 2,5 s y 1 MB por canal, con cancelación que mata/recoge el hijo. El coordinador conserva su plazo general de 3 s, también durante las verificaciones automáticas posteriores.

### Integración CALSIDE

`ide/reparaciones_ccom.py` valida sobre, estados, reglas, inserción, razones y progreso. Convierte bytes UTF-8 a índices Python y comprueba que cada precondición no vacía coincide con la instantánea original. No contiene lexer/parser del lenguaje ni acepta mensajes narrativos como precondiciones.

`ide/interfaz/asistencia.py` carga el proveedor C por defecto y captura el ccom configurado para la tarea. **Alt+Enter**, **clic derecho → Reparaciones de código** o el botón del panel consultan el motor real. Consultar/seleccionar no aplica: se revisa Cambios/Motivo y se pulsa Aplicar reparación. Ctrl+Z deshace una propuesta; no se guarda, compila ni ejecuta automáticamente. Editar/mover cursor/cambiar pestaña invalida la aplicación. `.asm` no usa estas reparaciones.

#### Automatización opcional

El catálogo de `politica_asistencia.py` incluye una sola regla revisada: **INSERCION_PUNTO_COMA**. Incorporarla no la habilita. Por defecto `reparaciones_automaticas=False` y la selección de reglas está vacía. El usuario debe habilitar la opción y marcar la regla en **Editar → Preferencias de asistencia**.

C marca candidata automática únicamente una propuesta única de `;`, con aceptación sintáctica completa y antes de una reservada de inicio de sentencia, nunca en EOF. La interfaz exige además confirmar con Enter una línea no vacía, una inserción local al final de esa línea, precondición real en la línea, sin selección/conflicto/solo lectura/compilación/pérdida de foco.

La bandera del motor no basta: la política vuelve a consultar **C mediante --syntax-json** sobre original y copia concreta; exige error sintáctico original y aceptación completa del resultado, sin errores léxicos. Python conserva solamente permisos, contexto, restricciones de interacción y aplicación. Antes de editar, el hilo gráfico vuelve a comprobar todo lo vigente. No guarda ni encadena otra reparación y Deshacer/Rehacer no dispara un nuevo ciclo.

Corchetes, paréntesis, puntos finales, EOF, candidatos parciales/ambiguos y líneas que no satisfacen estas condiciones permanecen manuales. Una aceptación sintáctica no prueba intención ni corrección semántica.

### Pruebas

- `make check-repairs-json`, incluido en `make check`: ejemplos de la entrega, cuatro cierres, EOF parcial, progreso/aceptación, operandos no inventados, Unicode/CRLF, comentarios, límites y opciones/salidas intactas.
- `ide/pruebas/test_reparaciones_ccom.py`: 12 pruebas nuevas del motor/transporte real y la interfaz; precondiciones inválidas, timeout, cancelación con eliminación del snapshot, panel/Deshacer, regla optativa y aplicación automática sin cadena.
- Política/interfaz previas: verificadas con el parser C, sin invocar LL(1) para validar lenguaje. La API Python LL(1) se conserva autorizada; clones sigue pendiente M5.
- Suite completa IDE: 371 pruebas correctas; suite del compilador correcta.
- ASan/LSan: 124 consultas de reparación/sintaxis, incluidas copias reanalizadas, sin errores ni fugas detectadas.

<a id="clones"></a>

## Clones Tipo I/II

Estado: implementado el 2026-11-11. `ccom --clones-json <mínimo> consulta.ccl`.
El argumento de entrada es un paquete de instantáneas CCL1, no un archivo Calser.
Solo admite `-v` y `--time`; las consultas y las opciones de generación son excluyentes.
No ejecuta parser, semántica, generación ni simulador. No guarda documentos ni genera artefactos.

### Responsabilidades y conexiones

1. `ide/interfaz/clones.py` reúne archivos `.cls` permitidos del proyecto y reemplaza su contenido por los buffers abiertos con nombre. Conserva las exclusiones, la comparación visual, el mínimo, la cancelación y la navegación vigilada. Los documentos nuevos con ruta pueden participar sin existir en disco; los documentos sin nombre quedan excluidos.
2. `ide/clones.py` mantiene los modelos `Fragmento`/`ParClon`, empaqueta instantáneas y valida el protocolo. Ya no tiene regex, reservadas, comparación, firmas ni normalización del lenguaje. El helper de inspección `tokenizar` consulta `--tokens-json`; el detector productivo no lo llama.
3. `ide/servicio_ccom.py` crea un directorio privado, escribe el paquete y lanza el ejecutable configurado, con el mismo transporte cancelable de los otros servicios. No utiliza shell. Siempre recoge el hijo y elimina el paquete, incluso al cancelar o fallar.
4. `src/servicios_ide/clones.c` lee el paquete acotado, observa los reconocimientos de `grammar/lexer.l` mediante `lexer_scan_text` y compara en C. Las rutas de los documentos solo identifican fuentes: **nunca se abren desde C**. Cada texto analizado coincide con la copia enviada; no hay que guardar antes de analizar.
5. El adaptador convierte extremos UTF-8 a caracteres Python para el panel y el editor. No reconstruye tokens. La ventana descarta resultados si cambian buffers, carpeta, mínimo o ejecutable.

### Equivalencia

- **Tipo I:** misma clase y lexema, ignorando espacios y comentarios. `x = x + 1;` y `x=x+1;` coinciden.
- **Tipo II:** biyección de identificadores dentro del fragmento; variación de literales de la misma clase. `x=x+1;` y `contador=contador+2;` coinciden. `x=x+y;` y `a=b+b;` no coinciden completos.
- Los literales se normalizan por clase (enteros o booleanos), como en el detector anterior; no se exige una biyección entre valores literales. Las apariciones de identificadores sí deben ser consistentes en ambas direcciones.
- Tipos, operadores, delimitadores y reservadas conservan su lexema. Un cambio de `+` a `-` impide un clon que lo atraviese, pero puede haber un fragmento común más corto si supera el mínimo.
- Se comparan fragmentos contiguos entre documentos distintos, sin exigir programas completos. No se demuestra equivalencia semántica ni se detectan clones con sentencias insertadas/reordenadas.
- Se eliminan inclusiones dentro del mismo tipo. Una subparte exacta puede coexistir con un Tipo II más amplio. El orden es determinista: longitud descendente y después tipo, archivos e inicios.
- **Flex es la autoridad:** cadenas, identificadores inválidos, números fuera de rango y comentarios abiertos bloquean la consulta. El antiguo regex aceptaba formas que Calser no admite; no se conserva esa divergencia. No se comparan tokens recuperados como si no hubiese errores.

### Entrada CCL1, versión 1

Todos los enteros son `uint32` little endian, sin padding:

```text
4 bytes: "CCL1"
u32: cantidad de documentos
repetido por documento:
    u32: bytes del nombre
    u32: bytes del texto
    nombre UTF-8, sin terminador NUL
    texto UTF-8, exactamente los bytes indicados
EOF obligatorio, sin bytes adicionales
```

Se rechazan nombres vacíos, nombres con NUL, nombres repetidos, cabeceras truncadas y longitudes excesivas antes de reservar sus datos. No hay escapado ni separadores ambiguos: el texto puede contener nuevas líneas, emojis, comentarios y NUL (que Flex diagnosticará). Los nombres pueden contener espacios y saltos de línea. Python envía rutas absolutas únicas y un orden estable.

Ejemplo independiente del IDE para generar una consulta:

```python
import struct
from pathlib import Path
fuentes = [("/ejemplo/a.cls", "x=x+1;"), ("/ejemplo/b.cls", "y=y+2;")]
partes = [b"CCL1", struct.pack("<I", len(fuentes))]
for nombre, texto in fuentes:
    nombre, texto = nombre.encode("utf-8"), texto.encode("utf-8")
    partes += [struct.pack("<II", len(nombre), len(texto)), nombre, texto]
Path("consulta.ccl").write_bytes(b"".join(partes))
```

```sh
./ccom --clones-json 5 consulta.ccl
```

El panel genera este paquete automáticamente al pulsar **Clones → Analizar carpeta**.

### Respuesta JSON v1

```json
{
  "schema_version": 1,
  "operation": "clones",
  "offset_unit": "utf8",
  "source": "consulta.ccl",
  "exit_code": 0,
  "status": "ok",
  "minimum": 5,
  "explanation": "",
  "files": [
    {"id": 0, "path": "/ejemplo/a.cls", "bytes": 6},
    {"id": 1, "path": "/ejemplo/b.cls", "bytes": 6}
  ],
  "pairs": [{
    "type": "II", "tokens": 6,
    "a": {"file": 0, "start": 0, "end": 6, "line_start": 1, "line_end": 1},
    "b": {"file": 1, "start": 0, "end": 6, "line_start": 1, "line_end": 1}
  }]
}
```

Los rangos `[start,end)` son bytes UTF-8 del documento identificado por `file`; líneas desde 1, tanto la primera como la última incluidas. El umbral cuenta tokens, no líneas ni porcentaje de similitud. El panel admite 5..200 (inicial 20); la CLI 5..100000.

| Estado | Código | Resultado |
| --- | --- | --- |
| `ok` | 0 | Consulta completa: cero o más pares. |
| `lexical_error` | 1 | Una fuente contiene errores de Flex; sin pares. |
| `limit` | 1 | Se excedió un límite; sin resultados parciales. |
| `error` | 1 | Entrada ilegible/mal formada o fallo interno; sin pares. |

Los estados de fallo incluyen explicación. Si el paquete no se pudo leer íntegro, `files` está vacío; en otro caso identifica todas las copias. stdout contiene solo JSON; `-v` y `--time` escriben en stderr. Uso incorrecto de banderas: código 2, sin prometer JSON. Compiladores antiguos/ausentes producen un error visible, nunca un respaldo al regex ni un resultado vacío ficticio.

Python valida operación, versión, ruta del paquete, umbral, código/estado, identidades/tamaños de archivos, tipos, tokens, fronteras UTF-8, líneas, pares duplicados y ausencia de resultados parciales. No valida equivalencia léxica en Python: esa decisión corresponde a C.

### Índice y memoria del motor C

Los lexemas se internan en una tabla acotada; cada token guarda valor, clase, bytes, línea y ocurrencia previa del identificador en su documento. Las ventanas Tipo II usan distancias a ocurrencias previas internas a la ventana, por lo que los nombres concretos no afectan su firma. Las colisiones de hash siempre se verifican token a token con mapas directos/inversos antes de producir resultados.

Se publica el índice de un documento solo después de recorrer sus ventanas, evitando comparaciones dentro del mismo archivo. Cada candidato compatible se extiende a derecha/izquierda con la misma correspondencia. Se registran intervalos cubiertos por diagonal para no repetir extensiones. El filtro final conserva pares máximos por tipo.

El proceso atiende una sola consulta: un contexto estático prestado adapta el callback de Flex; no es una API concurrente dentro del mismo proceso. Reinicia y destruye el scanner para cada documento, libera sus cadenas de identificadores y desconecta observadores. Todos los buffers/tablas se liberan antes de devolver, incluso con error o límite. No construye AST.

### Límites y cancelación

| Recurso | Límite |
| --- | --- |
| Documentos | 200 |
| Texto individual / total | 2000000 / 20000000 bytes |
| Nombre de documento | 16384 bytes |
| Tokens significativos totales | 100000 |
| Candidatos entre ambas pasadas | 50000 |
| Pares antes del filtro final | 500 |
| Pasos de firma/verificación/cobertura | 20000000 |
| Tiempo del hijo desde el IDE | 15 segundos |
| stdout / stderr del hijo | 1000000 bytes cada uno |

Alcanzar un límite se explica en el panel y exige aumentar el mínimo o reducir la carpeta. La cancelación se comprueba al reunir/empaquetar textos, esperar al hijo y adaptar la respuesta. El transporte mata y recoge el proceso; el cierre de la aplicación conserva su espera acotada. Cancelar no modifica archivos ni el historial de edición.

### Verificación

- `make check-clones-json`, incluido en `make check`: equivalencias, negativos, Flex, comentarios, Unicode/CRLF, estados, paquete truncado/duplicado/excesivo, límites, exclusión de banderas y preservación de artefactos.
- `ide/pruebas/test_clones.py`: casos anteriores sobre C y recorrido real del panel, buffers sin guardar, comparación, navegación e invalidación.
- `ide/pruebas/test_clones_ccom.py`: contrato estricto, falta/incompatibilidad de ccom, límites y cancelación/plazo reales con recogida del hijo y limpieza del paquete; ausencia de llamadas al lexer/parser Python.
- Suite completa: 379 pruebas del IDE correctas y suite del compilador correcta; la última prueba de interfaz se verificó además de las 378 de la primera ejecución completa.
- 250 casos diferenciales con entradas Flex válidas producen los mismos pares/rangos que el detector previo. La suite nativa pasó ASan/UBSan/LSan. No se preservan las aceptaciones inválidas del antiguo regex.

<a id="recovery"></a>

## Recuperación sintáctica M7

Implementada el 2026-11-11. El parser Bison real puede continuar después de errores independientes y publicar varios diagnósticos en una sola consulta. No se ejecuta otro parser ni se modifican copias para fabricar la lista. El LL(1) Python conserva su API de primer error para estudio/pruebas; la recuperación productiva está en C.

### Prueba rápida en CALSIDE

Escribe sin guardar:

```text
F main [] : void ;
    Vm x : int = ;
    Vm y : int = ;
    E ;
.
```

El análisis automático debe mostrar **dos errores sintácticos**, en las líneas 2 y 3. Cada problema permite navegar al `;` inesperado. Completar la primera expresión deja solo el segundo; completar ambas limpia la lista. No hace falta F5 ni guardar.

También puede comprobarse con `./ccom --check-json archivo.cls` o `./ccom --check archivo.cls`. El esquema JSON v1 no cambia: `diagnostics` ya admitía listas. La CLI clásica y F5 reciben los mismos diagnósticos reales.

### Cómo se recupera

`grammar/parser.y` añade producciones de error para cuerpos, globales y cabeceras de función. Bison descarta los símbolos parciales hasta un estado de recuperación; sus destructores liberan los nodos/cadenas descartados.

- En un cuerpo, se descartan tokens hasta `;`, `.` o una rama `A` del condicional observado. `;` se consume; `.`/`A` se conservan para que la gramática real cierre/continúe el bloque.
- Declaraciones globales se sincronizan en `;`; una cabecera de función inválida puede descartarse hasta `.` y se omite del AST parcial.
- Cada ancla conservada se reutiliza **una sola vez**. Si la gramática la sigue rechazando, se descarta sin emitir otra vez el mismo diagnóstico. Así se garantiza avance y se evita insistir sobre una `A` mal formada.
- Aperturas incompletas de la sentencia descartada se retiran del observador de bloques. Se normaliza la profundidad de delimitadores antes de continuar; durante los descartes no se registran falsas aperturas.
- EOF o una nueva `F` inesperada dentro del cuerpo detienen la recuperación de ese cuerpo: no se adivina el cierre ausente de una función.
- Un comentario abierto conserva su diagnóstico léxico y no provoca una cascada de errores sintácticos de EOF.

Es una recuperación **conservadora**, no una promesa de encontrar todos los errores de cualquier entrada. Una falta de `;` puede llevar a descartar también la sentencia siguiente hasta un separador fiable. Una cabecera/bloque muy dañado puede impedir continuar. El objetivo es obtener varios errores independientes cuando hay sincronización real, evitando inventar estructura o intención.

### AST parcial y fases posteriores

`yyparse()` puede devolver 0 tras recuperar errores; eso solo significa que llegó al símbolo inicial. **No significa que el programa sea válido.**

`parser_syntax_errors()` conserva el contador de errores de la consulta. `src/main.c:phase_parse` exige tanto éxito de Bison como **cero errores sintácticos y léxicos** antes de continuar. Si falla cualquiera:

- no se ejecuta el análisis semántico sobre el AST recuperado;
- no se exporta AST JSON ni se imprime con `-t`;
- no hay generación, organización de bundles ni ensamblado;
- no se crean/truncan artefactos de salida;
- el código de salida es 1.

El árbol parcial no se expone como resultado válido. Si se construyó un nodo de programa, main recibe su propiedad y lo libera durante la limpieza; los símbolos descartados antes fueron liberados por Bison. `parser_finish()` libera el observador de bloques, sin borrar el contador ni liberar el árbol de main. El estado se reinicia en la siguiente consulta.

### Sugerencias y reparaciones

`--suggest-json` y `--syntax-json`/`--repair-json` conservan el **primer error** para contexto, edición y validación. No aceptan un documento porque Bison pueda recuperarse. Tras capturar el contexto LAC, el envoltorio del lexer termina la consulta; no reinyecta indefinidamente el marcador privado del cursor durante la búsqueda de un ancla.

Los errores posteriores se descubren con `--check-json`. La reparación sigue conservadora: no se convierten varias propuestas parciales en autorización automática ni se amplía el catálogo de reglas por M7.

### Límites y estados

- Máximo **32 errores sintácticos** por consulta.
- Máximo **4096 tokens descartados** sin alcanzar un punto de sincronización de un cuerpo.
- Se mantiene el límite global del protocolo de diagnósticos/mensajes.
- Al agotar recuperación, `parser_recovery_limited()` hace que `--check-json` devuelva `status: "limit"`, `truncated: true`, código 1 y una explicación. Los diagnósticos obtenidos no se anuncian como un informe completo.
- El IDE conserva su tratamiento de informes limitados: muestra el fallo del servicio en lugar de publicar una lista parcial como si fuese completa. Hay que corregir/reducir la entrada y volver a consultar.
- Se mantienen cancelación y timeout de los procesos del IDE.

Una consulta con dos errores y recuperación normal tiene `status: "errors"`, dos elementos y `truncated: false`. Eso informa los errores hallados; no certifica que se haya inspeccionado semántica ni que sea posible continuar después de todo daño estructural.

### Archivos y pruebas

| Archivo | Responsabilidad |
| --- | --- |
| `grammar/parser.y` | Producciones de recuperación, sincronización, progreso, contador y destructores. |
| `include/analisis/parser_service.h` | Contador, límite y limpieza del observador, sin transferir el AST a otros consumidores. |
| `src/main.c` | Bloqueo de fases posteriores y limpieza del AST parcial. |
| `src/servicios_ide/diagnostics_json.c` | Señalar recuperación agotada como límite/truncado. |
| `grammar/lexer.l` | Finalizar/liberar el contexto del parser en consultas de memoria. |
| `pruebas/analisis/recuperacion/probar.py` | Globales, funciones, cuerpos/ramas/cierres, Unicode, EOF, comentarios, límites, artefactos y 120 mutaciones deterministas. |
| `ide/pruebas/test_recuperacion_sintactica.py` | Dos problemas reales navegables y corrección progresiva sin guardar; validación/reparación no aceptan entrada recuperada. |

```sh
make check-recovery
make check
make check-ide
```

Verificación: **384 pruebas IDE**, suite completa del compilador y casos M7 bajo ASan/UBSan/LSan. Bison no añade conflictos shift/reduce ni reduce/reduce. El corpus de mutaciones comprueba límites, progreso y ausencia de diagnósticos repetidos en la misma posición.


</details>

## 2026-11-11 — Corpus ampliado para revisión manual

Se agregaron 12 programas independientes en `ide/ejemplos/revision_manual/recorridos`, con comentarios y una guía de resultados: operadores, llamadas adelantadas/anidadas, ramas, ciclos L/S, recursión con R, recorrido/copia/búsqueda de arreglos, suma/resta de matrices, transposición rectangular, producto matricial y corto circuito. La guía propone mutaciones individuales para asistencia y errores sin convertir el corpus válido en un conjunto de fuentes dañadas.

Verificación específica del corpus: 12/12 pasan `ccom --check`, generación de artefactos y simulación; los valores finales coinciden con los esperados, con terminación normal y sin avisos de separación. Se usaron los auxiliares existentes de `pruebas/generador_o0/programas/probar.py`, con salidas temporales. No se ejecutó la suite completa. La revisión humana sigue pendiente; los estados manuales anteriores se conservan.
