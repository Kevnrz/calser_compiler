# Plan de CALSIDE · IDE de Calser

Versión 2 · actualizado el **5 de noviembre de 2026**.

Documento de trabajo para completar el IDE. Sustituye el orden inicial y los anexos de avances de la versión anterior. Los estados describen código existente: una propuesta o una API recibida no equivale a una función conectada al editor.

## 1. Objetivo, alcance y referencias

Construir una aplicación de escritorio propia para editar Calser, recibir asistencia durante la escritura, compilar con `ccom` y detectar clones entre archivos. Debe poder entregarse como aplicación ejecutable, sin exigir VS Code al usuario final.

Referencias:

- Enunciado `proyecto-ii-grupal-perf-cache.pdf`: página 9, punto 4 (sugerencias, correcciones automáticas, errores gráficos con descripción y clones Tipo I/II entre archivos); página 13, §7.2 (iteraciones); página 18, §9.2 (IDE: 15 % de CE1108).
- [Guía del núcleo](integrar_nucleo.md), [gramática original](gramatica_calser.md), [gramática LL(1)](gramatica_ll1.md) y [API del núcleo](../nucleo/README.md).
- [Uso, instalación y límites actuales](../README.md), [banderas de ccom](../../docs/ccom_banderas.md) y código de `ide/`, `grammar/` y `src/main.c`.
- [Mockups](mockups.html): referencias visuales con escenarios simulados; no constituyen la implementación.
- `plan_ide_interfaz.md` recibido del equipo: antecedente del reparto. Las decisiones posteriores de esta conversación prevalecen cuando cambian biblioteca, arquitectura, responsabilidades o secuencia.

**Separación de objetivos:** completar los requisitos del IDE no equivale a terminar el proyecto completo de Compiladores/Arquitectura. Optimizaciones, IR, cachés y simulador tienen sus propias tareas. Las métricas pueden integrarse después; no son condición para la primera versión completa del IDE.

## 2. Estado real al actualizar este documento

Rama: `feat/calside`. El merge de `origin/develop` se completó en `c8b1997`. En esa revisión pasaron **44 pruebas** mediante `make check-ide`: 24 de CALSIDE y 20 del núcleo. Es una referencia de validación de esta revisión, no un conteo que deba mantenerse fijo.

### Implementado y probado

- Aplicación PySide6/Qt Widgets llamada **CALSIDE**, con tema oscuro, acento ámbar y esquina morada estática.
- Cabecera sobre la barra de menús; menús centrados verticalmente.
- Editor con números de línea, resaltado basado en tokens reales del núcleo, pestañas, búsqueda y operaciones de edición.
- Nuevo, abrir, guardar, guardar como, protección de cambios al cerrar y rutas canónicas para evitar pestañas duplicadas.
- Explorador de `.cls` del primer nivel; ocultar/recuperar con botón o Ctrl+B.
- Panel de resultados movible abajo/derecha y terminales PTY independientes en Linux, con entrada por líneas.
- Compilación real asíncrona con `ccom`, salida, cancelación y límite actual de 30 segundos.
- Análisis automático con `ccom --check` sobre copias temporales, pausa de 600 ms después del lexer y límite de 5 segundos; sin guardar ni generar binarios.
- Errores léxicos/sintácticos/semánticos de `ccom` al compilar: subrayado, punto en margen, descripción, navegación y selección del fragmento.
- Diagnósticos por documento y descarte de resultados antiguos al editar o cambiar de ruta.
- Clones Tipo I/II independientes de LL(1), en segundo plano, con comparación, navegación, buffers sin guardar y ejemplos reproducibles.

### Núcleo incorporado y conexión actual

- `ide.nucleo.lexico.analizar(texto)`: conectado al resaltado y a errores léxicos en vivo (E3). Análisis por pausa, documento y revisión; funciona sin guardar.
- Gramática Calser transformada a LL(1), PRIMERO/SIGUIENTE y tabla predictiva.
- Pruebas y documentación de la API.

### Todavía pendiente

- Parser predictivo, sugerencias y reparaciones del compañero; conexión gráfica cuando estén disponibles.
- Detección de cambios externos y protección al sobrescribir archivos modificados fuera del IDE.
- Preferencias persistentes, árbol recursivo del explorador y entrega empaquetada.
- Animación de papel y controles de ventana en la esquina, como mejora posterior.

**Estado actual:** el editor usa el lexer del núcleo. La gramática y la tabla están disponibles como datos; no ejecutan por sí solas un parser predictivo. Sugerencias y reparaciones continúan pendientes.

## 3. Decisiones vigentes

| ID | Decisión | Estado / consecuencia |
|---|---|---|
| D00 | Nombre **CALSIDE**, el IDE de Calser | Aceptado y aplicado |
| D01 | Aplicación propia con Python y PySide6/Qt Widgets | Aceptado e implementado; reemplaza Tkinter |
| D02 | Dependencias externas permitidas | PySide6 aceptado; no añadir otras sin necesidad |
| D03 | Reutilizar recursos útiles del plugin | Patrones de resaltado e icono existentes; conservar avisos aplicables |
| D04 | `ccom` es el compilador y se invoca directamente | Implementado; LL(1) no es intermediario obligatorio |
| D05 | Núcleo para asistencia durante la edición | Lexer conectado al editor; parser/sugerencias/reparaciones pendientes |
| D06 | `QPlainTextEdit` para código | Implementado |
| D07 | `QProcess` para ccom, sin bloquear la UI | Implementado |
| D08 | Distribución clásica adaptable | Implementada; explorador plegable y resultados abajo/derecha |
| D09 | Ámbar + esquina morada de papel | Esquina estática implementada; animación pospuesta |
| D10 | Detector de clones independiente del parser | Implementado; motor e interfaz separados correctamente |
| D11 | Mantener controles nativos de ventana por ahora | Implementado; revisar movilidad y redimensionamiento antes de sustituirlos |

## 4. Responsabilidades y dependencias

| Componente | Responsable según el reparto actual | Dependencia pendiente |
|---|---|---|
| Interfaz, archivos, paneles, terminales y adaptadores | Saddy / trabajo de CALSIDE | Ninguna para continuar con lexer y protección de archivos |
| Integración con `ccom` | Saddy / CALSIDE, coordinado con responsable del compilador | Modo --check implementado; coordinar futuras extensiones del compilador |
| Léxico, gramática y tabla LL(1) | Compañero / C1.1a | Entrega 1 incorporada |
| Parser predictivo y sugerencias | Compañero / próxima entrega | API y casos de prueba reales |
| Reparaciones | Compañero / entrega posterior | Contrato de ediciones y casos seguros |
| Detector y panel de clones | Implementados en CALSIDE por solicitud de Saddy | Mantenimiento y validación final |

No reimplementar el parser del compañero en los widgets. Las funciones que no dependan de esa entrega pueden avanzar en paralelo lógico: `ccom`, archivos, clones, pruebas y empaquetado. Esto no requiere crear agentes ni chats adicionales.

## 5. Funcionalidades y criterios de aceptación

Estados: **implementado** = existe en el prototipo y tiene verificación; **parcial** = falta una parte del criterio; **pendiente** = requiere implementación/integración. P0: base; P1: cierre del IDE; P2: mejoras posteriores. Una función implementada todavía debe pasar el guion final en otra máquina.

| ID | Prioridad | Función | Estado actual y criterio para cerrarla |
|---|---|---|---|
| F01 | P0 | Abrir carpeta/proyecto | Parcial: primer nivel funciona. Completar árbol de subcarpetas, exclusiones y errores de acceso sin bloquear. |
| F02 | P0 | Nuevo/abrir/guardar/guardar como | Implementado: UTF-8, rutas canónicas, identidad actualizada y sin pestañas duplicadas. Protección de cambios externos se sigue en F17. |
| F03 | P0 | Pestañas y cambios pendientes | Implementado: texto/cursor por documento y guardar/descartar/cancelar al cerrar. |
| F04 | P0 | Edición y búsqueda | Implementado: deshacer/rehacer, portapapeles, búsqueda y margen sincronizado. |
| F05 | P0 | Compilar documento activo | Implementado: guarda antes de compilar, cancelar guardado cancela la operación, UI utilizable durante proceso. |
| F06 | P0 | Resolver/configurar ccom | Implementado durante la sesión. Persistir ruta y comprobar configuración inválida en F18. |
| F07 | P0 | Salida y estado del proceso | Implementado: stdout/stderr, código, fallo de inicio, cancelación y timeout. Tiempo límite configurable pendiente en F18. |
| F08 | P0 | Errores gráficos y navegables | Implementado al compilar: origen, categoría, archivo, rango, subrayado, descripción y navegación. Errores sin posición quedan en Salida. |
| F09 | P1 | Resaltado con lexer del núcleo | Implementado: tokens reales, comentarios coloreados aparte y rangos UTF-16. El formato conserva revisión, estado de guardado y Deshacer. |
| F10 | P1 | Análisis mientras se escribe | Implementado: lexer en vivo (300 ms) y ccom --check después de 600 ms adicionales. Snapshots por revisión, cancelación, temporales y resultados independientes de F5. Parser predictivo pendiente como motor complementario. |
| F11 | P1 | Sugerencias | Pendiente: Ctrl+Espacio, teclado, aceptar/cancelar y reemplazo exacto del rango. Activación automática cuando el contexto sea adecuado. |
| F12 | P1 | Reparaciones propuestas | Pendiente: descripción y edición precisa para terminadores/delimitadores Calser; cada reparación se deshace en una operación. |
| F13 | P1 | Corrección automática | Pendiente: catálogo de casos inequívocos, disparador definido y pruebas positivas/negativas. Un botón manual no cubre por sí solo este requisito. |
| F14 | P1 | Clones Tipo I entre archivos | Implementado: ignora formato/comentarios y permite comparar/navegar entre ubicaciones. |
| F15 | P1 | Clones Tipo II entre archivos | Implementado: renombramiento consistente y literales del mismo tipo; preserva operadores/reservadas; umbral y límites documentados. |
| F16 | P1 | Recuperación ante fallos | Parcial: lectura/escritura/procesos y cancelación cubiertos; completar fallos del análisis automático y estados de archivo borrado. |
| F17 | P1 | Cambios externos | Pendiente: detectar cambio/borrado, ofrecer recargar o conservar y evitar sobrescritura silenciosa. |
| F18 | P2 | Preferencias | Pendiente: ruta de ccom, fuente, tema y distribución persistentes. Mantener teclado y contraste legibles. |
| F19 | P2 | Artefactos y opciones del compilador | Parcial: genera `.asm` y salidas; falta abrirlas desde la UI y exponer únicamente banderas existentes. |
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
  archivos.py                   lectura y guardado atómico
  ccom.py                       ejecutable, diagnósticos y categorías
  clones.py                     detector léxico independiente de Qt
  interfaz/
    ventana.py                  ventana, acciones, procesos y coordinación
    editor.py                   editor, margen, resaltado y marcas
    analisis.py                 cola serial y trabajador léxico
    comprobacion.py             ccom --check, QProcess y temporales
    clones.py                   trabajador y panel de clones
    terminal.py                 terminal de comandos
    terminal_pty.py             proceso auxiliar Linux
  nucleo/
    lexico.py                   tokens y errores léxicos
    gramatica.py                gramática como datos
    tabla.py                    PRIMERO/SIGUIENTE y tabla LL(1)
  ejemplos/clones/
  pruebas/
  docs/
```

Componentes de E3 ya creados: `adaptador_nucleo.py`, `diagnosticos.py` e `interfaz/analisis.py`. El modelo común explicita origen, unidad de columna e identidad del documento; el coordinador procesa snapshots sin tocar widgets desde el trabajador. La cola reemplaza solicitudes pendientes del mismo documento y rechaza respuestas de revisiones antiguas.

No refactorizar todo el IDE solo para igualar un árbol de carpetas. Mantener `ide/clones.py` e `ide/interfaz/clones.py`: uno es motor, otro presentación. No hace falta LSP para completar el alcance.

## 7. Contratos para las integraciones pendientes

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

### 7.3 Sugerencias y reparaciones: contrato todavía por acordar

- Sugerencias: texto + revisión + cursor de entrada; lista con etiqueta, contenido insertable, rango a reemplazar y contexto.
- Reparaciones: descripción + revisión + una o varias ediciones no superpuestas, cada una con inicio/fin y texto nuevo.
- El motor debe indicar cuándo una reparación es segura para aplicación automática. La interfaz no deduce correcciones estructurales por regex.
- Acordar cómo responde sobre texto incompleto, comentarios, fin de archivo, varios errores y cancelación. Confirmar comportamiento de concurrencia; serializar solicitudes hasta verificarlo.
- Ajustar el adaptador a la API real que entregue el compañero; los nombres anteriores son conceptos, no funciones ya existentes.

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

Implementación: adaptador puro, comentarios de presentación, timer de 300 ms y cola serial con snapshots por revisión; errores con origen `núcleo`. Desduplicación por rango frente a ccom. Límite explícito de 2 millones de caracteres. Verificación E3: 56 pruebas combinadas, incluidos cierre durante análisis, resultados antiguos, recuperación ante fallos, Unicode y preservación de guardado/Deshacer.

### E4 · Análisis automático con ccom — completado, independiente de LL(1)

1. Coordinar y comprobar el modo de solo análisis del compilador (§7.4), con pruebas que demuestren ausencia de salidas binarias.
2. Conectar copias temporales del buffer a un proceso separado, con pausa, cancelación y revisión.
3. Remapear rutas y publicar errores sintácticos/semánticos sobre el documento original.
4. Deduplicar/priorizar frente a resultados del lexer y conservar la compilación manual como acción diferenciada.
5. Probar que no cambia el archivo en disco y que temporales/procesos se limpian incluso al fallar.

**Criterio cumplido:** una variable no declarada y una sentencia incompleta se señalan sin pulsar F5; corregirlas retira el mensaje y el archivo original continúa sin guardar. Usa copias temporales eliminadas tras éxito, cancelación o fallo, y orígenes separados. F5 tiene prioridad y reanuda otros documentos.

Verificación E4: 70 pruebas combinadas de CALSIDE/núcleo; `make check-analysis` y `make check` del compilador correctos. Se probaron original/artefactos intactos, errores sintácticos/semánticos, edición/cierre/Guardar como durante el proceso, timeouts, ejecutable antiguo, fallos de inicio, salida excesiva y prioridades entre pestañas. La prioridad es compilación manual sobre ccom automático y, para errores léxicos equivalentes, ccom sobre núcleo. Los mensajes de sintaxis conservan la explicación y posición de Bison; mejorar su contexto sigue pendiente.

### E5 · Parser predictivo y sugerencias — depende de entrega 2

1. Recibir rama/PR, API real y pruebas del parser/sugerencias; revisar diferencias con Bison.
2. Incorporar la entrega sin reemplazar la integración existente de ccom.
3. Conectar sugerencias inicialmente a Ctrl+Espacio.
4. Implementar lista, selección con flechas, aceptar con Enter/Tab, Escape y reemplazo por rango.
5. Añadir activación al escribir con contexto y pausa apropiados.
6. Evitar sugerencias en comentarios y conservar selección/cursor cuando el resultado sea antiguo.

**Terminado cuando:** se puede escribir una declaración o construcción Calser apoyándose en opciones válidas; aceptar reemplaza solo el fragmento previsto; cancelar no cambia el texto. Si el parser aporta diagnósticos en vivo, definir su prioridad frente a ccom antes de mostrarlos.

### E6 · Reparaciones y corrección automática — depende de entrega 3

1. Acordar el contrato de ediciones y un catálogo de casos por `;`, `.`, `[]` y `()` conforme a la gramática real.
2. Mostrar propuestas con explicación y aplicar primero por acción explícita para validar sus rangos.
3. Agrupar cada reparación como una sola operación de deshacer.
4. Habilitar automatización únicamente para los casos inequívocos acordados y con disparador definido (no interrumpir una construcción que el usuario sigue escribiendo).
5. No aplicar respuestas antiguas, no tocar comentarios por error ni guardar automáticamente el documento.
6. Probar positivos y negativos por categoría; permitir deshacer y desactivar la automatización.

**Terminado cuando:** se demuestran correcciones realmente automáticas, no solo un botón manual, con deshacer y sin cambios incorrectos en casos ambiguos. Si un caso no es seguro, queda como propuesta explicada.

### E7 · Protección de archivos y experiencia de proyecto — independiente del núcleo

1. Registrar el estado conocido del disco al abrir/guardar y detectar cambios, borrado o reemplazo externo.
2. Ofrecer recargar/conservar; comprobar nuevamente antes de sobrescribir para no perder ediciones ajenas.
3. Completar árbol de subcarpetas con exclusiones explícitas y sin exploración bloqueante.
4. Persistir ruta de ccom, fuente, paneles y preferencias que se hayan acordado; recuperarse de configuración inválida.
5. Revisar teclado, foco, tamaño mínimo, escalado y cierre con terminales/análisis activos.
6. Verificar que F01–F17 y F21–F24 satisfacen sus criterios, no solo el caso feliz.

**Terminado cuando:** editar un archivo desde fuera del IDE no provoca pérdida silenciosa; las preferencias útiles sobreviven al reinicio y el recorrido funciona con teclado y en el equipo de defensa.

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

- Esquina morada que se dobla y revela minimizar/maximizar/restaurar/cerrar; clic y teclado, animación breve y opción de movimiento reducido. Conservar comportamiento de ventana antes de sustituir controles nativos.
- Tema claro y refinamientos visuales, iconos de archivos y preferencias adicionales.
- Abrir artefactos del compilador y exponer opciones nuevas cuando estén realmente implementadas.
- Métricas por optimización cuando exista contrato de datos; integración con enlazado/IR cuando sus tareas estén listas.
- Emulación completa de terminal para aplicaciones curses, redimensionamiento PTY y otras plataformas.

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
| API de entregas 2/3 | Recibir y verificar parser, sugerencias y reparaciones; mantener un adaptador |
| Correcciones seguras | Catálogo, disparadores, deshacer y casos que deben seguir siendo propuestas |
| Diagnósticos solapados | Lexer/ccom manual/automático ya tienen prioridad. Revisar de nuevo al incorporar el parser predictivo |
| Mejoras del proyecto 1 | Inventariar observaciones y aclarar qué mejoras exige §7.2 con el equipo/profesor antes de cerrar |
| Paquete ejecutable | Herramienta, recursos, dependencias del sistema y distribución de ccom |
| Plazo | El plan recibido proponía 6/11, 12/11 y 17/11 como hitos; reestimar con entregas y trabajo real. No son compromisos nuevos de este documento |

Riesgos a vigilar: divergencias entre gramáticas, resultados antiguos, diferencias UTF-8/Unicode/UTF-16, reparación de construcciones aún incompletas y pérdida por cambios externos. Mantener corpus compartido, revisión por documento y separación entre motor y presentación. La ausencia temporal del parser no bloquea el resto del IDE.

## 12. Lista de cierre del IDE

- [x] Base de aplicación propia, editor, archivos y paneles.
- [x] Compilación manual con ccom y diagnósticos gráficos.
- [x] Clones I/II reales entre archivos.
- [x] Terminales básicas y distribución adaptable.
- [x] Núcleo entrega 1 incorporado al repositorio y suite combinada correcta.
- [x] E3: lexer conectado al resaltado y a errores en vivo.
- [x] E4: análisis automático de ccom sin guardar ni generar artefactos.
- [ ] E5: parser/sugerencias recibidos y conectados.
- [ ] E6: reparaciones y corrección automática verificables.
- [ ] E7: cambios externos, proyecto y experiencia estabilizados.
- [ ] E8: paquete ejecutable probado en otra máquina.
- [ ] E9: corpus, documentación, diagramas y guion final completados.
- [ ] Aclaradas e incorporadas las mejoras pendientes del proyecto 1.

La animación de la esquina y las mejoras de E10 se mantienen separadas de esta lista de cierre funcional.

## 13. Mantenimiento del plan

Al terminar una etapa: actualizar estado y criterio de aceptación, registrar evidencia relevante y anotar la siguiente tarea disponible. Si cambia la arquitectura, reescribir la sección afectada; no acumular anexos que contradigan tareas anteriores. Conservar IDs F01–F26 para seguir requisitos y revisión.

Próxima etapa del orden principal: **E5, incorporar parser predictivo y sugerencias cuando llegue la entrega 2 del compañero**. Mientras se recibe, E7 (cambios externos y experiencia de proyecto) puede avanzar de forma independiente.
