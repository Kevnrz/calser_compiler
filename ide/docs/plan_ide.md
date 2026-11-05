# Plan de CALSIDE · IDE de Calser

Fecha: 4 de noviembre de 2026. Estado: prototipo inicial PySide6 implementado; integración LL(1) y animaciones pendientes; clones Tipo I/II implementados.

Este documento es la referencia de trabajo para construir el IDE. Las propuestas se distinguen de las decisiones aceptadas. Los ejemplos visuales representan comportamiento esperado, no funciones implementadas.

## 1. Objetivo y fuentes

Crear una aplicación de escritorio propia para escribir programas Calser, recibir asistencia durante la edición, compilar mediante `ccom` y explorar clones entre archivos.

Fuentes:

- Enunciado `proyecto-ii-grupal-perf-cache.pdf`: página 9, punto 4 (IDE); página 13, sección 7.2 (iteraciones); página 18, sección 9.2 (evaluación). El IDE vale 15 % de CE1108.
- `plan_ide_interfaz.md`, recibido del equipo: reparto y contratos preliminares. Sus fechas y estados son información reportada, pendiente de confirmar contra la rama correspondiente.
- Repositorio local inspeccionado: `editors/vscode/`, `include/diagnostics.h`, `src/diagnostics.c`, `grammar/lexer.l`, `grammar/parser.y`, `src/main.c` y documentación de `ccom`.
- Acuerdos de esta conversación: aplicación independiente con PySide6; se permite reutilizar recursos de la extensión y agregar dependencias.

Los documentos de referencia describen requisitos y propuestas; no autorizan por sí solos a instalar, publicar, crear PR o modificar otras partes del proyecto.

## 2. Estado verificado y decisiones

| ID | Decisión o dato | Estado |
|---|---|---|
| D00 | Nombre de la aplicación: **CALSIDE**. Descripción: el IDE de Calser | Aceptado por Saddy |
| D01 | Aplicación independiente, construida con Python y PySide6 / Qt Widgets | Aceptado por Saddy |
| D02 | Se permite la dependencia externa PySide6 | Aceptado; reemplaza la restricción D5 del plan recibido |
| D03 | Reutilizar recursos útiles del plugin de VS Code | Aceptado; conservar atribuciones y avisos |
| D04 | `ccom` sigue siendo el compilador; la interfaz lo ejecuta | Base del plan recibido y propuesta mantenida |
| D05 | Núcleo LL(1) externo a la interfaz para asistencia durante la edición | Reportado por el equipo; revisar entrega y API |
| D06 | Usar `QPlainTextEdit` como base del editor | Propuesta técnica |
| D07 | Usar `QProcess` para `ccom` sin bloquear la ventana | Propuesta técnica |
| D08 | Distribución principal: proyecto lateral, editor central y resultados abajo | Propuesta; comparar mockups |
| D09 | Ámbar de Calser como acento principal y esquinita morada como guiño de identidad; tipografía monoespaciada en código | Esquinita morada solicitada por Saddy; tema definitivo pendiente |
| D10 | Clones en panel navegable y comparación de fragmentos | Implementado: detector léxico independiente de LL(1) |

En el checkout inspeccionado, rama `develop`, no existía `ide/`. La extensión actual registra lenguajes, gramáticas e iconos; no tiene un controlador de ejecución de la extensión. El compilador centraliza diagnósticos con posiciones, pero imprime mensajes humanos a stderr. No se verificaron las entregas LL(1) reportadas por el equipo. Existen modificaciones locales ajenas a este plan: preservarlas.

## 3. Alcance y responsabilidades

| Componente | Responsabilidad propuesta | Dependencia |
|---|---|---|
| Ventana, editor, pestañas, paneles y accesos | Saddy / esta tarea | PySide6 |
| Documentos, archivos y carpeta de proyecto | Saddy / esta tarea | Ninguna del núcleo |
| Ejecución y lectura de resultados de `ccom` | Saddy / esta tarea | Ejecutable disponible |
| Adaptadores y presentación de asistencia | Saddy / esta tarea | Contrato con el núcleo |
| Léxico, parser LL(1), sugerencias y reparaciones | Compañero / C1.1a, según el plan recibido | Confirmar alcance y entrega |
| Detector de clones Tipo I y II | Esta tarea, por solicitud de Saddy | Implementado y probado sin parser |
| Panel de clones y navegación | Saddy / esta tarea | Detector |

No duplicar el parser de Calser en la interfaz. La UI solicita sugerencias y reparaciones; el motor decide su contenido. Una demostración con respuestas simuladas debe identificarse como tal y no cuenta como integración terminada.

Primera versión: Linux o entorno gráfico compatible del equipo. Soporte para Windows nativo, distribución empaquetada y versiones concretas de Python/PySide6 se acuerdan después de comprobar los equipos de defensa.

## 4. Funcionalidades esperadas y criterios de aceptación

Prioridad P0: primera entrega funcional. P1: requisitos completos del IDE. P2: mejoras posteriores. P0 y P1 forman parte del objetivo final; la prioridad indica orden, no opcionalidad.

| ID | Prioridad | Funcionalidad | Criterio de aceptación |
|---|---|---|---|
| F01 | P0 | Abrir carpeta de proyecto | Muestra los `.cls` y permite abrirlos; errores de permisos se explican sin cerrar el IDE. Definir recursividad antes de implementar. |
| F02 | P0 | Nuevo, abrir y guardar | Lee UTF-8; nuevo solicita ruta al guardar; Guardar como actualiza identidad y pestaña; no crea pestañas duplicadas para una misma ruta canónica. |
| F03 | P0 | Pestañas y cambios pendientes | Cada documento conserva texto, cursor y cambios propios; cerrar permite guardar, descartar o cancelar; cancelar deja la aplicación abierta. |
| F04 | P0 | Edición básica | Deshacer/rehacer, cortar/copiar/pegar, selección y búsqueda local; números de línea siguen el desplazamiento. Los atajos funcionan desde el editor. |
| F05 | P0 | Compilar documento activo | Usa su versión guardada; un documento nuevo solicita ubicación; cancelar el guardado cancela la compilación. Resultado visible sin congelar la UI. |
| F06 | P0 | Configurar `ccom` | Ruta configurada, ejecutable del repo y PATH, en ese orden; si falta, indica cómo construirlo o configurar su ruta. |
| F07 | P0 | Mostrar salida | Conserva stdout, stderr, código de salida y errores del proceso; no presenta una falla de inicio como error del programa Calser. |
| F08 | P0 | Problemas navegables | Muestra origen, archivo, posición, categoría y descripción; activar un diagnóstico abre el archivo y selecciona su rango cuando existe. |
| F09 | P0* | Resaltado de sintaxis | Usa tokens del núcleo para distinguir palabras, tipos, literales, operadores y comentarios. *Integración sujeta a disponibilidad real de tokens. |
| F10 | P1 | Análisis durante la escritura | Tras una pausa breve muestra errores léxicos/sintácticos del documento actual sin guardarlo; nunca aplica resultados de una revisión anterior. |
| F11 | P1 | Sugerencias | Ctrl+Espacio solicita opciones en la posición del cursor; teclado permite elegir, aceptar y cancelar; sustituye únicamente el rango indicado. Activación automática posterior. |
| F12 | P1 | Correcciones | Soporta reparaciones del núcleo para `;`, `.`, `[]` y `()` donde la gramática lo permita; ediciones agrupadas y deshacibles; no modifica disco silenciosamente. |
| F13 | P1 | Corrección automática verificable | Acordar casos inequívocos y disparador; demostrar al menos un caso sin confirmación manual y su deshacer. Una acción de “aplicar arreglo” por sí sola no demuestra este requisito. |
| F14 | P1 | Clones Tipo I | Detector encuentra fragmentos equivalentes ignorando comentarios/formato entre archivos distintos; panel muestra ubicaciones y abre ambos. |
| F15 | P1 | Clones Tipo II | Detector identifica los cambios de identificadores/literales acordados; resultados incluyen rango y tipo. Documentar límites y negativos para evitar marcar fragmentos triviales. |
| F16 | P1 | Estado y recuperación | Mensajes claros ante lectura/escritura fallida, archivo eliminado, análisis fallido o proceso terminado; el texto editado se conserva. |
| F17 | P1 | Cambios externos | Detecta cambios en disco; permite recargar o conservar edición sin perder trabajo ni sobrescribir sin aviso. |
| F18 | P2 | Preferencias visuales | Tema claro/oscuro, tamaño de fuente y persistencia de paneles. El editor sigue siendo usable con teclado. |
| F19 | P2 | Artefactos y opciones | Abrir `.asm` generado; opciones documentadas de `ccom`; resultados identificados por compilación y documento. |
| F20 | P2 | Métricas | Gráficas de optimizaciones cuando exista una fuente de datos acordada. El enunciado permite integrarlas en el IDE, no obliga a que estén dentro de él. |

Ampliaciones de usabilidad solicitadas e implementadas en el prototipo:

| ID | Funcionalidad | Verificación |
|---|---|---|
| F21 | Ocultar/recuperar explorador con botón visible y Ctrl+B | Conserva documentos abiertos y sincroniza el menú Ver. |
| F22 | Mover panel de resultados abajo/derecha | Conserva contenido, pestañas y procesos; permite ampliar la altura del código. |
| F23 | Varias pestañas de terminal | Sesiones PTY Linux independientes, `cd` persistente por sesión, historial e interrupción. Cierre con confirmación. |
| F24 | Alineación de menús | Centro vertical de menús alineado con la fila de acciones bajo la cabecera. |

Terminales: interfaz de comandos por líneas; emulación de pantalla completa, aplicaciones curses y soporte para Windows quedan pendientes. Estas funciones no sustituyen ningún requisito de asistencia del núcleo.

El enunciado usa términos generales para delimitadores; las reparaciones concretas deben seguir la gramática Calser. No introducir sintaxis por semejanza con otros lenguajes.

## 5. Experiencia y mockups

Comparar tres alternativas con el mismo contenido:

1. **Editor clásico:** proyecto a la izquierda, código al centro, problemas/salida/clones abajo. Familiar, buen ancho de código; resultados largos compiten por altura.
2. **Diagnóstico lateral:** proyecto a la izquierda, código al centro, problemas a la derecha; salida bajo el código. Mantiene errores visibles, necesita más ancho.
3. **Enfoque y revisión:** código ancho con explorador plegable y una vista de revisión dedicada para clones o diagnósticos. Maximiza edición; requiere cambiar de vista.

Ver `mockups.html` en esta carpeta. Los mockups permiten comparar distribución y estados simulados; no ejecutan Python, no compilan y no usan el núcleo.

Evaluar cada alternativa en 1366×768 y 1920×1080, también con escalado del sistema. Comprobar ancho de código, visibilidad del error, facilidad para llegar a un archivo y comparación entre fragmentos. No decidir por el color solamente.

Detalle de identidad acordado: pequeña esquina morada en la parte superior derecha, como guiño visual. El ámbar sigue siendo el acento principal. La esquina es decorativa, no un control, y no tapa texto ni acciones. Incluida en las tres alternativas para conservarla al elegir distribución.

Estados que debe contemplar el diseño definitivo:

- Sin proyecto; proyecto sin archivos; archivo nuevo sin nombre.
- Documento limpio, modificado y cambiado externamente.
- Núcleo no disponible, analizando, sin errores y con errores.
- Compilador no configurado, compilando, cancelado, exitoso y fallido.
- Sugerencias visibles o sin resultados; corrección disponible, aplicada y deshecha.
- Clones analizando, sin coincidencias y con pares entre archivos.

Navegación inicial propuesta: Ctrl+N/O/S, Ctrl+Shift+S, Ctrl+W, Ctrl+Z, Ctrl+Shift+Z, Ctrl+F, Ctrl+Espacio y F5 para compilar. Revisar colisiones y equivalencias en los equipos del grupo. Evitar acciones que dependan únicamente de hover o color.

## 6. Arquitectura propuesta

```text
ide/
  main.py                 inicio y composición
  modelos.py              documentos, rangos, resultados y revisiones
  archivos.py             lectura, guardado e identidad de archivos
  ccom.py                 resolución del ejecutable y argumentos
  adaptadores/
    nucleo.py             traducción de la API LL(1) al modelo del IDE
    clones.py             traducción del detector al modelo del IDE
    diagnosticos_ccom.py   interpretación de stderr
  interfaz/
    ventana.py            QMainWindow, acciones y paneles
    editor.py             QPlainTextEdit y margen de líneas
    resaltado.py          presentación de tokens
    problemas.py          lista de diagnósticos y navegación
    proceso_ccom.py       QProcess y señales de compilación
    tareas.py             tareas de análisis y entrega de resultados
    estilos.py            colores, espaciado y tipografía
  nucleo/                 entrega del compañero; no recrearla aquí
  clones/                 detector; responsable pendiente
  pruebas/
  docs/
```

Los nombres son propuesta, no archivos existentes. Los modelos, argumentos y lectura de diagnósticos se prueban sin ventanas. La UI traduce eventos del usuario a operaciones y presenta resultados. El núcleo y el detector no importan widgets.

`QProcess` pertenece al adaptador de interfaz: permite leer stdout/stderr de forma asíncrona y recibir finalización sin un hilo extra para el proceso. El análisis de texto puede usar un trabajador si su costo lo requiere. Toda modificación de widgets ocurre en el hilo principal. No agregar LSP de inicio: el adaptador deja abierta una evolución futura.

## 7. Contratos por acordar antes de integrar

Estas estructuras son un borrador del IDE, no la API atribuida al compañero. Pedir ejemplos reales y adaptar los límites mediante un solo adaptador.

### 7.1 Documento y posiciones

- Documento: identificador estable, ruta opcional, texto, número de revisión, estado modificado y estado conocido del disco.
- Dentro del modelo del IDE: offsets en caracteres Unicode, inicio inclusivo y fin exclusivo; línea/columna para presentación calculadas a partir del texto.
- Confirmar cómo cuenta el núcleo. Flex usa `yyleng` y columnas basadas en bytes para UTF-8; Qt usa posiciones UTF-16 en sus cursores. Convertir explícitamente y probar tildes y caracteres fuera del BMP, además de tabulaciones y CRLF.
- Un diagnóstico sin rango válido se presenta en la lista; no inventar un subrayado sobre código ajeno.

### 7.2 Resultado de análisis

Documento + revisión + tokens + diagnósticos + estado del análisis. Cada diagnóstico requiere origen, categoría/severidad, mensaje y rango opcional. Solicitudes de sugerencias incluyen posición del cursor; respuestas incluyen texto visible, texto insertable y rango a reemplazar. Reparaciones contienen descripción y lista de ediciones con rango/texto de reemplazo.

Acordar: comentarios incluidos en tokens, análisis de texto incompleto, cancelación, parser recuperable, códigos estables de error, límites de tamaño y si el motor es seguro al usar varios trabajadores. No asumir reentrancia. Hasta confirmarla, serializar solicitudes.

### 7.3 Clones

Entrada propuesta: colección de rutas/identificadores y sus textos con revisión. Preferir el contenido abierto sin guardar; avisar de esa elección. Salida: grupos o pares de fragmentos con archivo, rango y clasificación I/II. Acordar longitud mínima, normalización, exclusiones y si se comparan archivos en subcarpetas. La UI no normaliza código por su cuenta.

### 7.4 Compilación

- Ejecutable resuelto y argumentos separados; sin shell.
- Guardar explícitamente el documento activo antes de compilar; no compilar una revisión distinta sin indicarlo.
- Directorio de trabajo y salidas definidos por proyecto; usar rutas absolutas en argumentos. Definir nombres para evitar colisiones de archivos con el mismo nombre.
- Bloquear compilación duplicada sobre el mismo destino. Contemplar cancelar y tiempo límite configurable.
- Resultado: documento/revisión, comando representable, cwd, stdout, stderr, código de salida, duración, diagnósticos y artefactos realmente existentes.
- Verificar `0/1/2` con la CLI actual; distinguir salida por señal, cancelación y fallo al iniciar.
- Leer muestras reales de errores; tolerar rutas con espacios y dos puntos, mensajes multilínea, errores sin posición y errores de E/S. No depender de una expresión regular codiciosa sin corpus.
- Al editar, marcar como antiguos o retirar diagnósticos de compilación; resultados tardíos no sustituyen los actuales. Mantener salida anterior identificada si sirve para consulta.
- El plan recibido menciona un error con prefijo `ccom:L:C`; comprobarlo antes de aplicar una excepción. Proponer JSON como evolución separada, no como condición para el prototipo.

## 8. Reutilización de la extensión

| Recurso | Uso en el IDE | Precaución |
|---|---|---|
| Iconos Calser y branding | Ventana y archivos | Conservar origen y avisos aplicables |
| Gramática TextMate | Referencia de categorías y palabras | No se ejecuta directamente en Qt; no crear un lexer rival |
| Configuración de comentarios y parejas | Comandos de comentar/cierre de parejas | Coordinar con las reparaciones del núcleo |
| README y programas de prueba | Ayuda y demostración | Verificar vigencia contra la gramática |
| Empaquetador VSIX | Sigue sirviendo a la extensión | No es el empaquetado de la aplicación PySide6 |

## 9. Secuencia de implementación

### E0. Diseño y coordinación

- Revisar este documento, elegir distribución y confirmar equipos de ejecución.
- Obtener rama/PR del núcleo y muestras de sus resultados; confirmar quién implementa clones.
- Verificar Python/PySide6 disponibles y fijar una combinación reproducible en un entorno virtual.
- Salida: decisiones actualizadas y contratos suficientes para empezar. Sin instalar globalmente ni cambiar el compilador como parte de la documentación.

### E1. Prototipo de interfaz y archivos

- Ventana PySide6 con divisores ajustables, editor, pestañas, explorador y panel inferior.
- Nuevo/abrir/guardar/guardar como/cerrar, identidad canónica y protección de cambios.
- Deshacer/rehacer, números de línea y estado del cursor.
- Salida: recorrido de archivos completo; todavía no se considera asistencia implementada.

### E2. Recorrido vertical con `ccom`

- Resolución del ejecutable, argumentos, QProcess y salida completa.
- Panel de problemas con navegación; distinción entre problemas de proceso y del fuente.
- Guardado previo y manejo de resultados antiguos, cancelación y fallos.
- Salida: abrir un ejemplo, introducir un error, compilar, navegar al error, corregir y volver a compilar con éxito.

### E3. Análisis y resaltado

- Adaptador real del núcleo; colorear tokens, marcar errores y mostrar su descripción.
- Debounce inicial aproximado de 300 ms; medir y ajustar en los equipos del grupo.
- Resultados por revisión, sin bloqueo de la UI y sin errores de otros documentos.
- Salida: asistencia real sobre texto sin guardar. Si solo existe lexer, registrar expresamente la ausencia del parser.

### E4. Sugerencias y reparaciones

- Lista de sugerencias, navegación con teclado y aplicación por rango.
- Reparaciones propuestas y automáticas según casos acordados.
- Agrupar cada reparación para deshacer de una vez; no forzarla mientras el usuario completa una construcción válida.
- Salida: demostraciones de cada requisito del enunciado, con casos negativos y deshacer.

### E5. Clones y revisión

- Conectar detector, lista de pares/grupos y apertura de fragmentos relacionados.
- Revisar contenido sin guardar, archivos externos y resultados antiguos.
- Salida: casos Tipo I y Tipo II entre archivos y ejemplos sin coincidencia.

### E6. Estabilización y defensa

- Documentación de instalación, versión soportada, uso, límites y responsabilidades.
- Incorporar `make ide` y `make check-ide` cuando exista implementación; preservar objetivos actuales del makefile.
- Instalar en otra máquina del grupo y ejecutar guion de demostración.
- Métricas dentro del IDE: iteración posterior sujeta a contratos de optimización.

El plan recibido propone 6/11 para la primera versión, 12/11 para asistencia/clones y aproximadamente 17/11 para métricas. Son hitos de referencia, no compromisos confirmados: reestimar tras conocer disponibilidad del núcleo y el tiempo de implementación. No etiquetar una integración simulada como entrega funcional para cumplir una fecha.

## 10. Pruebas y demostración

| Área | Verificación necesaria |
|---|---|
| Archivos | UTF-8/CRLF, rutas con espacios, Guardar como, archivo nuevo, duplicados, fallo de escritura y cancelación de cierre |
| Diagnósticos | Corpus real léxico/sintáctico/semántico/E/S, rangos ausentes, varios errores y salida multilínea |
| Proceso | Ejecutable ausente, éxito, fallo, timeout/cancelación, stdout y stderr, editor usable durante ejecución |
| Revisiones | Escribir/cambiar de pestaña mientras analiza o compila; ignorar resultados antiguos |
| Posiciones | ASCII, tabulaciones, tildes en comentarios y carácter fuera del BMP; conversión hacia QTextCursor |
| Correcciones | Inserción prevista, no cambio ambiguo, deshacer, no modificación de comentarios y no guardado silencioso |
| Clones | Tipo I/II entre archivos, negativos, rangos correctos y textos sin guardar |
| Interfaz | Teclado, escalado, paneles redimensionables, foco, lectura de errores sin depender de color |

Usar `unittest` para lógica independiente y comprobación manual para la UI inicial. Añadir pruebas Qt automatizadas cuando un comportamiento de eventos concreto lo justifique. No agregar dependencias de pruebas sin necesidad.

Guion de defensa:

1. Abrir carpeta con dos programas y cambiar entre pestañas.
2. Escribir un error; mostrar diagnóstico del núcleo sin guardar.
3. Solicitar una sugerencia y aceptarla con teclado.
4. Mostrar una reparación automática y deshacerla; reaplicarla cuando corresponda.
5. Compilar un ejemplo válido y uno con error semántico; navegar desde Problemas.
6. Detectar clones Tipo I y II entre archivos y abrir sus fragmentos.
7. Cerrar un documento modificado y demostrar que Cancelar conserva el trabajo.

## 11. Riesgos y mitigaciones

- **API del núcleo incompleta:** obtener muestras y aislar el adaptador; avanzar con archivos y compilación.
- **Fechas cercanas:** entregar primero el recorrido vertical y evitar funciones decorativas que retrasen requisitos.
- **Desacuerdo entre LL(1) y Bison:** compartir corpus válido/inválido; registrar divergencias y acordar responsable de corrección.
- **Asistencia sobre texto incompleto:** no confundir estado transitorio con reparación segura; validar casos por construcción.
- **Posiciones incompatibles:** contrato explícito y pruebas Unicode antes de pintar rangos.
- **Detector sin responsable:** resolver asignación temprano; panel preparado no cumple detección.
- **Dependencias en defensa:** versiones documentadas, entorno virtual y verificación en otra máquina.
- **Alcance creciente:** enlazado `P`, optimizaciones e IR dependen de tareas externas; agregar solo cuando existan contratos reales.

## 12. Registro de decisiones pendientes

| ID | Pregunta | Propuesta inicial | Evidencia para cerrar |
|---|---|---|---|
| A01 | ¿Qué distribución usar? | Editor clásico | Elegir tras comparar los tres mockups |
| A02 | ¿Qué plataformas/versiones soportar? | Entorno Linux del grupo | Comprobar equipos de desarrollo y defensa |
| A03 | ¿Cuál es la API real del núcleo? | Adaptador a modelos del IDE | Rama/PR y ejemplos del compañero |
| A04 | ¿Qué se corrige automáticamente? | Casos inequívocos, deshacibles | Catálogo de casos y acuerdo con núcleo |
| A05 | ¿Quién hace clones? | Implementado en esta tarea por solicitud de Saddy | Detector y panel entregados |
| A06 | ¿Qué son las mejoras del proyecto 1? | Inventariarlas antes del cierre | Aclaración del equipo/profesor |
| A07 | ¿Recorrer subcarpetas? | Sí, con exclusiones explícitas | Tamaño y estructura de proyectos reales |
| A08 | ¿Datos en disco o buffers para clones? | Preferir buffers abiertos | Contrato del detector y mensaje de alcance |
| A09 | ¿Destino de artefactos? | Carpeta build del proyecto | Política sin colisiones y ejemplos de CLI |
| A10 | ¿Tema definitivo? | Oscuro con ámbar discreto | Prueba de contraste y preferencia del usuario |

Para cada decisión cerrada anotar fecha, razón, consecuencias y estado reemplazado. Actualizar aquí el alcance si cambia; no mantener planes contradictorios.

## 13. Estado del prototipo inicial

Implementados: ventana clásica PySide6, esquina morada estática, pestañas, edición/búsqueda, abrir/guardar, carpeta de primer nivel y compilación real asíncrona mediante `ccom`. Resaltado provisional reutilizado de la extensión, reemplazable por tokens del núcleo. Ver `../README.md` para instalación y límites.

Se conserva la barra nativa de ventana. La esquina de papel animada y sus controles se posponen por acuerdo con Saddy. Todavía no hay ejecutable empaquetado. La integración con el núcleo y los clones no se simula.

## 14. Lista de arranque

- [x] Elegir PySide6 y permitir dependencias.
- [x] Inventariar base del repositorio y requisitos conocidos.
- [x] Crear plan, criterios de aceptación y alternativas visuales.
- [ ] Elegir distribución y revisar estados vacíos/error.
- [ ] Recibir API/entrega del núcleo y confirmar reparto.
- [ ] Comprobar entorno Python/PySide6.
- [ ] Implementar E1 y E2; después integrar asistencia.
- [ ] Completar P0 y P1 y verificar guion de defensa.

## 15. Referencias técnicas

- Qt for Python: https://doc.qt.io/qtforpython-6/gettingstarted.html
- QPlainTextEdit: https://doc.qt.io/qtforpython-6/PySide6/QtWidgets/QPlainTextEdit.html
- QProcess: https://doc.qt.io/qtforpython-6/PySide6/QtCore/QProcess.html
- QTextCursor: https://doc.qt.io/qtforpython-6/PySide6/QtGui/QTextCursor.html

Los contratos y componentes Qt anteriores son propuestas de diseño; confirmar firmas y detalles contra la versión elegida al implementar.

## Avance: diagnósticos de ccom (paso 2)

Implementados subrayados por rango, punto en el margen, descripción al pasar el puntero y detalle para la línea del cursor, navegación con selección del fragmento y categorías léxica/sintáctica/semántica. Los resultados se conservan por documento y se invalidan al editar, renombrar o cerrar; las revisiones antiguas no se aplican.

Se reutiliza stderr y el señalador del compilador sin modificar su interfaz. Los errores sin posición siguen en Salida. Las posiciones de fin de línea se representan con marca de línea, sin atribuirlas a otro token. Este avance sigue ejecutándose con Compilar/F5: el análisis en vivo es un paso posterior y no requiere pasar por LL(1).

Validación: pruebas automáticas sobre corpus real léxico, sintáctico y semántico, varios errores, rango de comentario abierto, Unicode, fin de línea, resultados por pestaña, edición, recompilación correcta y descarte de revisiones antiguas.

## Avance: clones sin dependencia LL(1)

Se implementó `ide/clones.py` como detector léxico y `ide/interfaz/clones.py` como trabajador/panel. Compara `.cls` entre archivos de la carpeta de proyecto, con subcarpetas y buffers abiertos. Presenta pares Tipo I/II con ubicaciones, comparación y selección en el editor; invalida snapshots antiguos y admite cancelación. El detector no analiza gramática ni equivalencia semántica.

Las decisiones de umbral, normalización y límites están documentadas en `../README.md`; los ejemplos reproducibles están en `../ejemplos/clones/`. La dependencia del detector externo propuesta inicialmente queda reemplazada por esta implementación. El análisis automático de errores, sugerencias y reparaciones continúa siendo trabajo pendiente.
