# Contexto de implementación: motor de reparaciones

Responsable: compañero de reparaciones. Corresponde al motor de E6/F12; Saddy implementa el parser predictivo.

## Resultado esperado

Detectar errores sintácticos y proponer ediciones explicadas sobre el texto original. Clasificar de forma conservadora cuáles podrían aplicarse automáticamente. Una reparación sintáctica no demuestra que el programa expresa la intención del usuario ni que es semánticamente correcto.

## Contexto compartido y límites

Proyecto: CALSIDE, IDE nativo de Calser hecho con Python/PySide6. Repositorio de trabajo: `/home/saddygr/Documentos/GitHub/calser_compiler`. Fecha de este reparto: 2026-11-05.

Ya existen el editor, pestañas, guardado, compilación F5, diagnósticos gráficos, clones Tipo I/II y terminales. E3 conecta el lexer Python al editor. E4 ejecuta `ccom --check` sobre una copia temporal del texto para validar sintaxis y semántica sin guardar ni generar artefactos. El parser LL(1) complementará esa ruta; no sustituirá la semántica ni la compilación de ccom.

El núcleo recibido contenía lexer, gramática LL(1), FIRST/FOLLOW y tabla. **Saddy ya implementó el parser predictivo** en `ide/nucleo/parser.py`; sugerencias y reparaciones siguen pendientes. La API real y sus ejemplos están en [parser_predictivo.md](parser_predictivo.md). Los motores siguen pendientes, pero sus tipos y firmas ya están fijados en [contrato_asistencia.md](contrato_asistencia.md). Usar los tipos implementados en `ide/nucleo/asistencia.py`; no duplicarlos en el motor.

Reparto decidido por Saddy:

- Saddy: parser predictivo (completar la ejecución de LL(1)).
- Un mismo compañero: motor de sugerencias.
- Ese mismo compañero: motor de reparaciones.
- Integración posterior en CALSIDE: adaptadores, coordinación de tareas y widgets de E5/E6.

Los motores deben ser Python puro, sin PySide6, acceso a widgets, lectura obligatoria de archivos, procesos ccom ni escritura del documento. Reciben texto en memoria y devuelven datos. Deben funcionar con documentos sin guardar. No implementar en cada motor otro lexer o una gramática paralela.

Lecturas del repositorio (rutas desde su raíz):

- `ide/nucleo/lexico.py`: `analizar(texto) -> (tokens, errores)`; `Token.tipo`, `lexema`, `linea`, `columna`, `columna_fin`, `valor`.
- `ide/nucleo/gramatica.py`: `CALSER`, `Gramatica`, `Produccion`, `FIN = "$"`, `EPSILON = "ε"`. Una producción vacía representa epsilon.
- `ide/nucleo/tabla.py`: `construir_tabla(CALSER)`, `TablaLL1.entrada(A, a)`, `.fila(A)`, `.primero`, `.siguiente`, `.es_ll1`.
- `ide/docs/gramatica_ll1.md` y `ide/docs/gramatica_calser.md`: transformaciones y lenguaje.
- `grammar/parser.y` y `grammar/lexer.l`: referencia del compilador real; investigar y documentar discrepancias, no modificar el lenguaje silenciosamente.
- `ide/docs/plan_ide.md`, `ide/docs/integrar_nucleo.md`: fases y arquitectura.

### Convenciones que deben compartir los tres módulos

Contrato del parser implementado: offsets globales en caracteres de Python, desde 0, con rangos `[inicio, fin)`; cursor entre 0 y `len(texto)`. Inserción: `inicio == fin`. Línea/columna de presentación: desde 1; una tabulación cuenta un carácter. El lexer actual usa fin de columna inclusivo: convertir expresamente. Qt usa UTF-16 y ccom bytes UTF-8; esa conversión pertenece al adaptador del IDE, no al motor.

El IDE captura `(documento, revision, texto, cursor)` y asocia ese contexto con cada tarea; descarta resultados antiguos. Los motores pueden recibir solo texto/cursor, pero toda respuesta aplicada debe corresponder al mismo texto y revisión. Ningún módulo decide que una respuesta antigua sigue siendo válida.

No compartir pilas mutables entre llamadas. Límites explícitos de pasos/resultados y estados de cancelación o límite alcanzado, sin presentarlos como análisis correcto. Ambas APIs aceptan `cancelar=None` como argumento opcional por nombre; consultar el callback en bucles y propagarlo al parser. Errores del usuario se devuelven como datos; fallos internos o contratos inválidos no deben ocultarse como «sin errores».

`DiagnosticoSintactico`, `ContextoPredictivo`, `SimboloEntrada` y `ResultadoSintactico` ya existen en `ide/nucleo/parser.py`. Importarlos desde ahí. Los tipos compartidos de asistencia, incluido `EdicionTexto`, están implementados en `ide/nucleo/asistencia.py`; importarlos desde allí. No crear otra definición en los motores ni duplicar los modelos del parser. El aplicador del IDE está listo; su interfaz y sus tareas se detallan en [contrato_asistencia.md](contrato_asistencia.md).

### Entrega y validación común

Entregar código, pruebas reproducibles y documentación de API con ejemplos de entrada/salida, límites conocidos y cambios respecto al contrato. Ejecutar `make check-ide`; referencia histórica del parser: 96 pruebas; preparación actual del IDE, incluidas preferencias/política de asistencia: 294 pruebas combinadas. Añadir pruebas en `ide/pruebas/`. Si se toca ccom o su gramática, ejecutar también las comprobaciones del compilador pertinentes. Coordinar cambios compartidos y comunicar rama/commit para integración; estos documentos no autorizan publicar, fusionar ni enviar mensajes automáticamente.

## Dependencia y API fijada

Crear `ide/nucleo/correccion.py`. Consumir `analizar_sintaxis(texto)` de `ide.nucleo.parser`, ya implementado. Revisar `estado`, `diagnosticos`, `errores_lexicos`, `pila` y `esperados`; los diagnósticos exponen rangos y el símbolo encontrado. Reanalizar cada candidato desde el texto modificado: la API actual no reanuda una pila ni aplica sincronización. No implementar otra gramática ni deducir todas las correcciones mediante regex.

```python
proponer_reparaciones(texto: str, *, cancelar=None) -> ResultadoReparaciones
```

Primera versión centrada en el primer error fiable; ampliar después con un mecanismo explícito para evitar propuestas incompatibles sobre múltiples errores.

`ResultadoReparaciones(estado, propuestas=(), explicacion="")`: usar `EstadoAsistencia` y una tupla de `Reparacion`; ver campos exactos en el contrato compartido. Cada `Reparacion`:

- `codigo` estable, `descripcion` y explicación del error objetivo.
- Una o varias `EdicionTexto(inicio, fin, texto)` sobre **el texto original**, no sobre posiciones desplazadas por ediciones anteriores.
- `automatica_segura: bool`, `razones` como tupla de texto y `precondiciones` como tupla de `PrecondicionTexto(inicio, fin, texto_esperado)`. Por defecto, falso. El indicador no autoriza por sí mismo automatización.
- `validacion`: texto que documenta qué error resolvió el candidato y hasta dónde se reanalizó. `cursor_final`, si se proporciona, es un offset absoluto sobre el texto resultante.
- Contexto que permita al IDE asociarla al diagnóstico y verificar texto/revisión antes de usarla.

Ediciones no superpuestas; fusionar inserciones coincidentes o compartidas con el inicio/fin de otro rango, pues el aplicador las rechaza por ambiguas. Para aplicarlas sobre una copia, ordenar de derecha a izquierda. No ejecutar cambios sobre archivos o widgets.

## Alcance y seguridad de la propuesta

1. Acordar catálogo real de `;`, `.`, `[]` y `()` según `gramatica.py`; describir para cada caso disparador, precondiciones, edición, ambigüedades y ejemplos negativos.
2. Basarse en terminal esperado, token encontrado, pila/contexto y posiciones. Un delimitador faltante puede deberse a una expresión incompleta: no insertar por simple conteo de signos.
3. Separar recuperación del parser y reparación del texto. Saltar tokens para seguir analizando no autoriza eliminarlos del documento.
4. Explorar un conjunto acotado de candidatos y reanalizar copias. No ocultar un error nuevo anterior ni considerar «avance» por haber descartado arbitrariamente código. Si se usa coste/ranking, documentarlo y dar resultados deterministas.
5. Que un candidato sea el único probado o que pase sintaxis **no basta** para marcarlo seguro. Ante alternativas razonables, quedar como propuesta manual. No renombrar identificadores ni inventar operandos, valores o declaraciones para satisfacer la gramática.
6. Proteger comentarios y sus contenidos, incluyendo comentarios incompletos. No cambiar un comentario a código accidentalmente. Tener cautela con errores léxicos: devolver bloqueo o una política explícita, no operar sobre una tokenización inválida sin avisar.
7. El motor no conoce si el usuario terminó de escribir. Informar precondiciones y restricciones de automatización; el IDE decidirá el disparador tras una pausa/contexto acordados. Un prefijo incompleto al EOF no autoriza cerrar automáticamente toda la función.
8. Limitar candidatos, pasos y número de ediciones. No producir reparaciones recursivas ilimitadas. Mostrar cuándo se alcanzó un límite.
9. Conservar espacios y comentarios fuera de rangos editados. Evitar formatear todo el documento para corregir un signo.

## Pruebas de aceptación

- Casos positivos y negativos por cada regla del catálogo, basados en programas de la gramática real.
- `;` omitido frente a expresión que todavía requiere un operando; propuesta distinta o bloqueo justificado.
- Delimitador de cierre omitido frente a apertura ambigua; no cambiar límites de una construcción silenciosamente.
- Programa correcto: ninguna reparación innecesaria.
- Comentarios con delimitadores, Unicode antes de la edición, CRLF, tabulaciones y EOF.
- Varios errores: propuestas consistentes con el alcance declarado; sin rangos superpuestos.
- Aplicar las ediciones sobre una copia produce el resultado documentado; todos los offsets corresponden al original.
- Reanálisis, límite de búsqueda, determinismo e idempotencia de los casos corregidos.
- Una matriz que documente qué propuestas son manuales y cuáles se consideran automáticas, con contraejemplos que deben rechazarse.

## Integración posterior en CALSIDE

El IDE mostrará explicación y propuesta, validará documento/revisión/texto, aplicará cada reparación como una única operación de Deshacer y permitirá desactivar automatización. El motor no debe guardar automáticamente ni invocar ccom. E6 requiere demostrar alguna corrección automática conservadora además de propuestas manuales; si ningún caso cumple condiciones suficientes, comunicarlo sin etiquetar casos ambiguos como seguros.

## Entrega esperada

API, catálogo de reglas, pruebas, ejemplos antes/después y motivos de seguridad. Coordinar con Saddy el diagnóstico/contexto necesario y con sugerencias únicamente las estructuras compartidas: sugerir cómo continuar y reparar un error son operaciones distintas.

## Conexión preparada en CALSIDE (pasos 3/4)

El coordinador ya invoca la API real mediante importación diferida de los módulos y firmas de este documento. Devuelve errores de contrato explícitos y valida rangos antes de publicar opciones. Respetar `cancelar` y los tipos compartidos permite conectar la entrega sin reescribir los widgets. Las UI manuales, las preferencias, la pausa y la política automática están listas. Falta integrar el motor real y revisar su catálogo. Ver [política y aprobación de reglas](politica_asistencia.md). El motor solo entrega los tipos compartidos; no necesita implementar widgets. Ver [contrato_asistencia.md](contrato_asistencia.md) para límites, formatos, estados y verificación actual.

El panel manual se abre con Alt+Enter, permite revisar Cambios/Motivo y exige Aplicar incluso para `automatica_segura`. Valida el contexto y agrupa cada propuesta en un Deshacer. Por revisión humana, la vista previa admite hasta 12000 caracteres afectados (eliminados + insertados); las propuestas mayores no pueden aplicarse desde el panel. No guarda ni compila.

Paso 6: la ruta automática es independiente de Alt+Enter. Exige permiso por código revisado, una propuesta, una inserción local de delimitador al final de la línea confirmada con Enter, precondiciones y aceptación completa por el parser en el trabajador. El catálogo de producción está vacío. Las reglas no compatibles pueden seguir siendo manuales; entregar la API acordada no exige modificar los widgets. La aprobación y activación del catálogo se detalla en `politica_asistencia.md`.
