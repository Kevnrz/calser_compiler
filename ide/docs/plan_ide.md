# Plan de CALSIDE · IDE de Calser

> **Actualización M1 (2026-11-11):** el análisis automático usa `ccom --check-json` y conserva el front-end real en C. Saddy aclaró que el LL(1) y el simulador pueden seguir en Python; no se exige migrarlos. Las demás etapas se describen en el [plan de integración](plan_migracion_logica_c.md).

Guía de arquitectura, conexiones e inventario para revisar/comentar el código: [guia_sistema_calside.md](guia_sistema_calside.md).

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

Actualización M6 (2026-11-11): revisión de dependencias, LL(1) de consulta explícitamente fuera de producción, cabecera JSON común y comentarios de servicios/adaptadores. Recorrido gráfico con varios documentos, C real y bloqueo de imports de analizadores Python; 382 pruebas IDE y suite de ccom verificadas. [Revisión y orden de lectura](revision_m6.md).

Actualización M5 (2026-11-11): clones Tipo I/II con Flex/comparación C, instantáneas múltiples CCL1 y panel existente. Sin tokenizador/normalizador Python. Límites, cancelación y bloqueo explícito de errores léxicos. [Contrato M5](../../docs/ccom_clones_json.md).

Actualización M4 (2026-11-11): reparaciones reales C (`--repair-json`), cuatro cierres validados sobre copias, precondiciones verificables y panel conectado. La política automática usa `--syntax-json` en C. Una regla revisada de punto y coma, desactivada hasta permiso explícito en preferencias. 371 pruebas IDE y suite del compilador correctas. [Contrato M4](../../docs/ccom_repairs_json.md).

Actualización M3 (2026-11-11): sugerencias reales en C con Flex + Bison/LAC, `--suggest-json`, Ctrl+Espacio y pausa opcional. La entrega se adaptó a C sin usar sus modelos duplicados. Reparaciones sigue pendiente M4. [Contrato M3](../../docs/ccom_suggestions_json.md).

Actualización M2 (2026-11-11): resaltado/errores léxicos vivos desde Flex mediante `ccom --tokens-json`, sin recorrido léxico ni búsqueda de comentarios Python en el editor. Se conservan snapshots, cancelación y Deshacer. Ver [contrato léxico](../../docs/ccom_tokens_json.md).

Actualización M1B (2026-11-11): botón Compilar dividido, F5 según modo seleccionado, diálogo de destino/opciones compatibles y preferencias. Cuatro modos reales: análisis, compilación, AST JSON sintáctico y ensamblado .asm; F6 requiere compilación completa con listado y artefactos renovados. Ver [plan de integración](plan_migracion_logica_c.md#resultado-de-m1b-2026-11-11).

Actualización M1 (2026-11-11): `ccom --check-json` implementado y conectado al análisis automático. Diagnósticos tipados y listas/rangos validados, sin regex de mensajes en ese recorrido. 312 pruebas del IDE y suite completa del compilador correctas. F5/F6, lexer en vivo, LL(1) y simulador se conservan. Ver [contrato del servicio](../../docs/ccom_check_json.md).

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
| F19 | P1 | Artefactos y opciones del compilador | Requisito confirmado por el profesor (2026-11-11): selector de modo en Compilar/F5, destino y opciones compatibles. Parcial: la compilación fija actual funciona; falta la selección gráfica. Ver M1B y sección 13 de [migración](plan_migracion_logica_c.md). |
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

El parser expone `analizar_sintaxis` y `contexto_en` en `ide/nucleo/parser.py`; API y ejemplos en [parser_predictivo.md](parser_predictivo.md). Los tipos y firmas de ambos motores están fijados en [contrato_asistencia.md](contrato_asistencia.md), con implementación compartida en `ide/nucleo/asistencia.py`. Los motores siguen pendientes; no deben volver a definir sus tipos.

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
6. [x] **Preferencias y política automática:** diálogo persistente, pausa opcional, desactivación general, permiso por regla y comprobación real del parser en segundo plano. Disparador conservador: Enter al final de una línea y pausa. Catálogo de producción vacío hasta revisar las reglas de la entrega real; ver [política y conexión](politica_asistencia.md).
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

Detalle y prueba manual en [proyecto_archivos.md](proyecto_archivos.md). No se promete un bloqueo exclusivo frente a escritores externos entre la última comprobación y el reemplazo: son operaciones distintas del sistema de archivos. La validación del equipo de defensa permanece en E8/E9. Preferencias de tema y timeout quedan fuera de esta entrega.

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
