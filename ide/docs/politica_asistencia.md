# Preferencias y política de asistencia — paso 6

> **Actualización M1 (2026-11-11):** el análisis automático usa `ccom --check-json` y conserva el front-end real en C. Saddy aclaró que el LL(1) y el simulador pueden seguir en Python; no se exige migrarlos. Las demás etapas se describen en el [plan de integración](plan_migracion_logica_c.md).

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

- `ide/politica_asistencia.py`: validación de preferencias, `ReglaAutomatica`, catálogo `REGLAS_AUTOMATICAS` y evaluación pura.
- `ide/interfaz/preferencias_asistencia.py`: diálogo nativo.
- `ide/interfaz/sugerencias.py`: pausa, permisos, vigencia y aplicación en el hilo gráfico.
- `ide/interfaz/asistencia.py`: motor y validación del parser en segundo plano.
- `ide/pruebas/test_politica_asistencia.py`: proveedores controlados exclusivamente de prueba.

El catálogo de producción está **vacío**. No se importa como permiso un listado que venga del motor y no hay reglas ficticias activas en la aplicación.

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
