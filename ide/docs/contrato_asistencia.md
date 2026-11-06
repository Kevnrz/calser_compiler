# Contrato de asistencia de CALSIDE · versión 1

Estado: tipos compartidos, aplicador, coordinador y UI manual de sugerencias implementados. Los motores, la UI de reparaciones y la activación automática todavía están pendientes. Este documento fija el contrato del lado de CALSIDE; compartirlo junto con el código para que las entregas de los motores lo utilicen.

## Archivos y responsabilidades

| Archivo | Responsabilidad |
|---|---|
| `ide/nucleo/asistencia.py` | Tipos inmutables y protocolos de llamada. Python puro; no importa Qt. |
| `ide/ediciones.py` | Validación de rangos/precondiciones, transformación sobre copia y conversiones Python/UTF-16. Python puro. |
| `ide/interfaz/ediciones.py` | Capturar el contexto y aplicar cambios sobre un Editor en el hilo gráfico. |
| `ide/interfaz/asistencia.py` | Coordinador serial de ambos motores, tareas, cancelación, límites y validación de respuestas. |
| `ide/interfaz/sugerencias.py` | Lista junto al cursor y control manual de sugerencias en el editor. |
| `ide/nucleo/sugerencias.py` | Motor pendiente: producir sugerencias basadas en `contexto_en`. |
| `ide/nucleo/correccion.py` | Motor pendiente: producir reparaciones basadas en `analizar_sintaxis`. |

Importar los tipos compartidos; **no volver a definir** `EdicionTexto`, `Sugerencia`, `Reparacion` ni los resultados dentro de los motores. El parser conserva sus propios tipos en `ide/nucleo/parser.py`. No se cambió el parser ni se crearon motores provisionales.

Los motores reciben texto en memoria y devuelven datos. No acceden a widgets, guardan archivos ni invocan ccom. Una reparación no equivale a validación semántica. La UI futura será responsable de disparadores, permisos de automatización y presentación.

## Funciones que deben entregar los motores

```python
from typing import Callable
from ide.nucleo.asistencia import ResultadoSugerencias, ResultadoReparaciones

# ide/nucleo/sugerencias.py
sugerir(texto: str, cursor: int, *,
        cancelar: Callable[[], bool] | None = None) -> ResultadoSugerencias

# ide/nucleo/correccion.py
proponer_reparaciones(texto: str, *,
        cancelar: Callable[[], bool] | None = None) -> ResultadoReparaciones
```

Son firmas del contrato, no funciones implementadas. Los protocolos `MotorSugerencias` y `MotorReparaciones` ya las describen. `cancelar=None` significa que no se proporcionó cancelación; cuando existe, consultar la función en bucles y pasarla al parser. Devolver `CANCELADO` si se interrumpe. Acotar pasos/candidatos/opciones y devolver `LIMITE` cuando no se pueda completar una respuesta fiable. No ocultar fallos internos como una lista vacía.

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
- `automatica_segura` es falso por defecto. Si es verdadero, el modelo exige razones y validación no vacías. Eso **no prueba seguridad ni autoriza aplicación automática**: faltan catálogo acordado, contraejemplos, disparador y política de la UI en E6.

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

Los rechazos incluyen `.explicacion`; no abren diálogos por su cuenta. Las señales normales del Editor actualizan la pestaña, invalidan diagnósticos anteriores y programan de nuevo el análisis. La UI futura decidirá qué explicación presentar y cuándo cerrar la lista.

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
- **Esc** cierra sin modificar. Escribir otra tecla cierra y deja que la tecla actúe normalmente en el editor; no se solicita otra lista automáticamente en esta etapa.
- Mover el cursor/cambiar selección, editar, desplazar el editor, cambiar pestaña, perder foco o cerrar el documento invalida solicitudes y lista. Un clic fuera de la lista también la cierra.
- Se conserva el foco en el editor y se comprueba el contexto otra vez antes de aplicar. Aceptar es una operación de Deshacer; no guarda.
- Estados sin opciones, bloqueo, límite, cancelación y error se informan en la barra de estado sin abrir listas vacías ni diálogos por cada consulta.

El controlador también admite `ventana.asistencia.solicitar('reparaciones')` y emite `reparacionesRecibidas(RespuestaAsistencia)` para el paso 5. **No aplica reparaciones automáticamente ni crea todavía su interfaz.** Tampoco publica nuevos diagnósticos del parser frente a ccom: esa prioridad sigue pendiente de integración real.

## Entrega e integración pendientes

1. [ ] Los compañeros entregan sus funciones, catálogo y pruebas con el parser real usando los tipos compartidos. Toda discrepancia con este contrato se documenta.
2. [x] Coordinador en segundo plano con contexto, serialización, cancelación y descarte de solicitudes anteriores.
3. [x] UI manual de sugerencias con pruebas controladas y aceptación mediante el aplicador.
4. [ ] UI de reparaciones y aplicación explícita de las propuestas (paso 5).
5. [ ] Preferencias de asistencia, activación por pausa y catálogo/disparadores de corrección automática (paso 6).
6. [ ] Conectar entregas reales, definir prioridad frente a ccom y validar casos reales antes de declarar E5/E6 terminadas.

La automatización sigue deshabilitada hasta disponer de catálogo y disparadores comprobados. El aplicador no decide que una reparación es segura.

Verificación: pasos 1/2, 23 pruebas nuevas y 227 combinadas en su entrega; pasos 3/4, **26 pruebas nuevas y 253 combinadas correctas** con `make check-ide`. Se cubren proveedores ausentes/rotos, contratos inválidos, hilos seriales, sustitución de pendientes, cancelación, timeout, cierre aplazado, foco, teclado/ratón, Unicode, contexto obsoleto, geometría y ninguna aplicación implícita de reparaciones.
