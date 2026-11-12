# Asistencia de CALSIDE: modelos, aplicación y permisos

Contrato vigente · 2026-11-11. Sugerencias y reparaciones están conectadas a motores C. `ide/nucleo/asistencia.py` mantiene modelos sin Qt ni dependencia del LL(1). El catálogo automático contiene INSERCION_PUNTO_COMA; está desactivado hasta habilitar automatización. El diálogo selecciona la primera regla revisada si todavía no hay ninguna seleccionada. La vista gris con Tab sí viene activada.

Este documento define el contrato Python/UI. Las decisiones del lenguaje y formatos nativos se explican en [Servicios de ccom](../../docs/ccom_servicios_ide.md). [Guía del sistema](guia_sistema_calside.md), [prueba manual](revision_manual.md#asistencia).

<a id="contrato"></a>

## Archivos y responsabilidades

| Archivo | Responsabilidad |
|---|---|
| `ide/nucleo/asistencia.py` | Tipos inmutables y protocolos de llamada. Python puro; no importa Qt. |
| `ide/ediciones.py` | Validación de rangos/precondiciones, transformación sobre copia y conversiones Python/UTF-16. Python puro. |
| `ide/interfaz/ediciones.py` | Capturar el contexto y aplicar cambios sobre un Editor en el hilo gráfico. |
| `ide/interfaz/asistencia.py` | Coordinador serial de ambos motores, tareas, cancelación, límites y validación de respuestas. |
| `ide/interfaz/sugerencias.py` | Vista gris, lista junto al cursor, aceptación y vigencia de propuestas. |
| `ide/interfaz/sugerencia_inline.py` | Dibuja un sufijo sobre el viewport, sin editar el documento ni su historial. |
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

`ResultadoSugerencias(estado, sugerencias=(), explicacion='')` y `ResultadoReparaciones(estado, propuestas=(), explicacion='')` convierten las colecciones a tuplas. Solo `OK` acepta opciones y debe contener al menos una. `BLOQUEADO`, `LIMITE` y `ERROR` requieren explicación. Un constructor inválido genera un error de contrato; el coordinador debe presentarlo como fallo, no como análisis correcto.

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

## Vista gris con Tab

Al escribir o borrar con Backspace/Supr, una pausa (350 ms iniciales, conservando la espera guardada) consulta el motor C. `sugerencias_inline=True` por defecto habilita esta presentación aunque el perfil antiguo tuviera desactivada la lista por pausa. Puede desactivarse en preferencias.

C marca `inline_preferred` para prefijos escritos, tipos y puntuación local útil. Que una opción sea la única continuación gramatical no basta para insinuar cerrar una función (`.`) o iniciar otra (`F`). C sí prioriza `.` después de los tokens explícitos `E;`, únicamente cuando hay un salto de línea hasta el cursor, si LAC lo permite y el punto no está ya a la derecha; `F` permanece en la lista manual. Enter puede solicitar esa continuación al pasar a una línea nueva; si está habilitada la corrección con Enter y no se aplica una reparación, la consulta de continuación se mantiene. Una lista válida de opciones ambiguas sin preferencia no dispara reparaciones como alternativa; así no se propone un punto para cerrar un cuerpo todavía vacío.

Bison publica el nombre de la función durante la consulta, sin otra gramática en Python. Para la declaración de `main` sin corchetes se propone `[]: ` con el cursor después de los dos puntos y el espacio; `void` es su tipo preferido porque el compilador exige ese retorno y ningún parámetro. En funciones normales se conserva `[]` con cursor dentro; Tab o flecha derecha sale y consulta `:`. Después de `:` se pueden recorrer tipos con Alt+[ / Alt+]. La plantilla compacta no se aplica a lazos, condiciones ni llamadas dentro del cuerpo de main, y las variables locales conservan sus tipos.

La zona de salida proviene del cursor interno y la longitud de la propuesta C. Editar, seleccionar, mover el cursor o perder foco invalida ese salto especial. Los campos nativos opcionales `inline_preferred` y `cursor_relative` se validan en el adaptador y llegan como `preferida_inline` y `cursor_relativo`.

`SugerenciaInline` dibuja el sufijo al final de la línea en un widget transparente a los eventos del mouse. La vista previa no añade texto, revisión, cambios sin guardar ni entradas de Deshacer. El controlador conserva el contexto exacto y la propuesta original. No se muestra sobre texto existente ni dentro de una selección; para reemplazos a mitad de línea se usa Ctrl+Espacio.

**Tab** acepta mediante el aplicador común; **Esc** descarta; **Alt+[ / Alt+]** recorre opciones. Enter conserva su función de nueva línea. Editar, mover cursor o selección, cambiar pestaña o perder foco descarta la vista. Redimensionar o desplazar el viewport solo la recoloca si el contexto sigue vigente; se oculta si el cursor queda fuera del área visible. No se aceptan respuestas de otra revisión y el motor bloquea comentarios.

Si no hay una continuación, puede consultarse el motor de reparaciones con origen interno `vista_inline`: solo se presentan inserciones locales de una única edición exactamente en el cursor. Tab aplica la reparación con sus precondiciones; no se interpreta `automatic_safe` como permiso para editar solo. La lista manual y el panel de reparación siguen disponibles.

## UI manual de sugerencias disponible

- **Ctrl+Espacio dentro del editor**, o Editar → Sugerencias de código, captura el contexto y solicita opciones. El atajo no roba el foco a búsqueda, consolas ni terminales.
- La lista se coloca junto al cursor y se limita al área disponible de pantalla. Muestra etiqueta, categoría, detalle de la opción activa y ayudas de teclado.
- Flechas arriba/abajo y RePág/AvPág recorren opciones. **Enter/Tab** o clic en una opción aceptan mediante el aplicador común; no insertan un salto o tabulación adicional.
- **Esc** cierra sin modificar. Escribir otra tecla cierra y deja que la tecla actúe normalmente en el editor; solo vuelve a consultar tras una pausa si esa preferencia está habilitada.
- Mover el cursor/cambiar selección, editar, cambiar pestaña, perder foco o cerrar el documento invalida solicitudes y lista. Los cambios de geometría recolocan la lista manteniendo la opción elegida. Un clic fuera de la lista también la cierra.
- Se conserva el foco en el editor y se comprueba el contexto otra vez antes de aplicar. Aceptar es una operación de Deshacer; no guarda.
- Estados sin opciones, bloqueo, límite, cancelación y error se informan en la barra de estado sin abrir listas vacías ni diálogos por cada consulta.

## UI manual de reparaciones disponible (paso 5)

**Alt+Enter dentro del editor**, Editar → Reparaciones de código o **Buscar reparaciones** en la pestaña Reparaciones consulta el documento actual. El atajo no intercepta terminales ni búsqueda. El panel se adapta a la distribución inferior o derecha y permite ajustar sus divisiones.

La lista muestra las propuestas del motor. **Cambios** presenta cada edición con línea/columna del original, `−` para el texto anterior y `+` para el posterior; **Motivo** muestra regla, razones y validación declaradas por el motor. Seleccionar o consultar no modifica el documento. La etiqueta `automatica_segura` se informa, pero nunca activa una aplicación por su cuenta.

**Aplicar reparación** es la única acción que modifica el editor. Usa el aplicador común: valida de nuevo todo el contexto y las precondiciones, agrupa los cambios en una operación de Deshacer y no guarda ni compila. **Cancelar** o **Escape** descarta la consulta/propuestas sin modificar texto ni historial.

Editar, mover el cursor/cambiar selección, cambiar pestaña o cerrar el documento invalida la propuesta y deshabilita Aplicar. Dar foco al panel, seleccionar su texto o desplazar el editor no invalida una reparación; perder el foco cierra sugerencias, mientras el desplazamiento conserva las que siguen visibles y vigentes. Una respuesta cancelada o antigua no vuelve a habilitar Aplicar.

Límite de vista previa: **12000 caracteres afectados**, sumando los eliminados y los insertados. Dentro de ese límite se muestra el cambio completo, con hasta 60 caracteres de contexto inalterado a cada lado en la misma línea. Las propuestas mayores se explican y quedan deshabilitadas para aplicar desde este panel; no se aprueban cambios ocultos por truncamiento. La metadata explicativa se limita por separado a 4000 caracteres.

El controlador emite `reparacionesRecibidas(RespuestaAsistencia)` y el panel consume los tipos compartidos sin inventar resultados. La ausencia del módulo, el error, el bloqueo y la falta de opciones se muestran explícitamente. Los proveedores controlados solo existen en pruebas.

El panel no publica diagnósticos ni reemplaza Problemas: conserva las prioridades actuales entre compilación manual, ccom automático y lexer. Aplicar ediciones invalida los diagnósticos antiguos mediante el flujo normal del editor y vuelve a programar su análisis. Los diagnósticos visibles del lenguaje proceden de ccom; las propuestas no crean otra lista de errores Python. La ruta léxica y el análisis completo se identifican por su origen y revisión.


<a id="politica"></a>

## Política y preferencias

### Uso

Abrir **Editar → Preferencias de asistencia…**. Aceptar aplica y guarda los ajustes del perfil sin reiniciar; Cancelar no cambia ajustes ni consulta motores.

| Preferencia | Predeterminado | Efecto |
| --- | --- | --- |
| Habilitar asistencia | Sí | Control general de consultas manuales y automáticas. Al desactivar cancela pendientes y cierra sugerencias. |
| Sugerencias en gris al escribir | Sí | Presenta propuestas C y cierres locales sin modificar el texto; Tab acepta. |
| Lista por pausa (sin vista gris) | No | Solicita opciones tras una pausa de escritura; nunca las acepta sola. Ctrl+Espacio sigue disponible cuando el control general está habilitado. |
| Pausa de asistencia | 350 ms | Ajustable entre 300 y 3000 ms. Se reinicia al seguir escribiendo; también se usa después de confirmar una línea. |
| Corregir al confirmar una línea con Enter | No | Requiere catálogo revisado y reglas seleccionadas. Sin catálogo el control está deshabilitado. |
| Reglas permitidas | Ninguna | Permiso por código exacto; el indicador del motor no agrega permisos. |

Estos ajustes son independientes de **Compilar → Análisis automático** (`ccom --check-json`), del lexer y de F5/F6. Se persisten bajo `asistencia` en el estado JSON existente; campos inválidos recuperan valores predeterminados, sin interpretar cadenas como booleanos.

### Disparadores y cancelación

El Editor emite `entradaAsistencia` **después** de procesar una tecla que realmente cambia el texto. Escribir caracteres imprimibles sin Ctrl/Alt y sin reemplazar una selección puede programar sugerencias. Enter sin modificadores, sin repetición, sin selección y al final de una línea no vacía puede programar reparaciones.

Abrir, recargar, pegar, arrastrar texto, Deshacer/Rehacer y las ediciones programáticas no arman el temporizador. Aceptar explícitamente una propuesta sí consulta la siguiente continuación; no la aplica automáticamente. Las reparaciones automáticas con Enter no encadenan consultas de esta manera. Una entrada mediante IME invalida lo anterior; esta versión conserva la asistencia manual para esa entrada, sin activación automática al confirmar la composición.

Se captura el contexto completo al armar la pausa. Modificar texto, mover cursor/selección, cambiar pestaña, cerrar u ocultar el editor, perder foco, presionar Esc, cambiar proveedor, catálogo o preferencias cancela lo que corresponda. Las sugerencias/manuales de reparaciones conservan sus reglas de interacción del paso 5: leer el panel no invalida una reparación manual. Las solicitudes automáticas sí se cancelan al perder foco.

Antes de consultar y antes de aplicar se comprueba que el editor sigue activo, enfocado, editable, sin selección ni conflicto externo conocido, y que no hay compilación o simulación en curso. También se verifica que coincidan documento, revisión, texto, cursor y ancla. Una consulta manual cancela la automatización anterior y toma prioridad. El cierre detiene la pausa y conserva el protocolo cooperativo de cancelación de trabajadores.

Los cambios de distribución al aparecer un diagnóstico no cancelan la asistencia: se espera al layout de Qt y se recoloca la propuesta, sin volver a consultar C ni cambiar el texto.

### Política inicial de reparación automática

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

### Conexión y aprobación del catálogo real

Archivos de referencia:

- `ide/politica_asistencia.py`: validación de preferencias, `ReglaAutomatica`, catálogo `REGLAS_AUTOMATICAS` y coordinación de validación nativa en el trabajador.
- `ide/interfaz/preferencias_asistencia.py`: diálogo nativo.
- `ide/interfaz/sugerencias.py`: pausa, permisos, vigencia y aplicación en el hilo gráfico.
- `ide/interfaz/asistencia.py`: motor y validación del parser en segundo plano.
- `ide/pruebas/test_politica_asistencia.py`: proveedores controlados exclusivamente de prueba.

El catálogo M4 contiene **INSERCION_PUNTO_COMA**, revisada con casos positivos/negativos. La automatización y su selección vienen apagadas por defecto. No se importa como permiso un listado que venga del motor y no hay reglas ficticias activas en la aplicación.

Para añadir o modificar una regla:

1. Entregar el módulo y firma acordados, importando `Reparacion`, `PrecondicionTexto` y `ResultadoReparaciones` desde `ide.nucleo.asistencia`. Los widgets existentes consumen directamente sus resultados.
2. Documentar por código la forma exacta de edición, justificación, validación y casos ambiguos. Usar `automatica_segura=False` cuando no se pueda justificar la automatización.
3. Añadir positivos y negativos con el parser real: comentarios de línea/bloque, código incompleto, delimitadores anidados, Unicode, cursores, errores múltiples y precondiciones. Una regla puede quedar solo manual.
4. Para una regla que satisface esta política y ha sido revisada, añadir `ReglaAutomatica(codigo_real, descripcion, delimitador)` a `REGLAS_AUTOMATICAS`. No cambiar la evaluación para aceptar un código sin sus pruebas.
5. El usuario habilita corrección automática en preferencias. Si no tenía reglas seleccionadas, el diálogo selecciona la primera regla revisada; puede personalizar después el catálogo. Incorporar una regla al código no habilita la corrección en el perfil por sí solo.
6. Verificar en la ventana: Enter al final de la línea, pausa, modificación esperada, Ctrl+Z, ningún guardado y descarte de respuesta antigua. Mantener las consultas manuales.

Para pruebas aisladas existe `ventana.asistencia.establecer_reglas(...)`; invalida tareas anteriores, pero no habilita preferencias. `establecer_motor(...)` sigue permitiendo proveedores controlados; ninguno se activa por defecto.
