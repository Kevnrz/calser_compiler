# CALSIDE: migración de la lógica del lenguaje a C

Fecha inicial: **2026-11-10**. Actualizado: **2026-11-11**. Este documento revisa la conexión inicial de motores y propone reutilizar el front-end C de ccom, conservando las excepciones de lenguaje aclaradas después por Saddy. Registra la indicación del profesor comunicada por Saddy: la aplicación/interfaz puede ser Python, pero la lógica del lenguaje debe estar en C y reutilizar el compilador existente.

Estado: **M1 implementado y verificado (2026-11-11)**: `ccom --check-json` y análisis automático estructurado. No se han fusionado los motores ni retirado componentes del IDE; M1B también implementado y verificado (2026-11-11): modos gráficos y preferencias. Las demás etapas siguen pendientes salvo indicación expresa.

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

M1 concreta la operación `check` con `--check-json`; véase el [protocolo implementado](../../docs/ccom_check_json.md). Las operaciones de tokens, sugerencias, reparaciones y clones siguen siendo diseño pendiente.

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

El parser Bison actual informa el primer fallo sintáctico y no tiene reglas de recuperación que permitan recorrer sistemáticamente el resto del programa. El parser LL(1) Python tampoco lo resuelve: su contrato se limita al primer error.

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
- [x] Exponer diagnósticos desde `src/diagnostics.c` sin inferirlos con regex en Python.
- [x] Reutilizar las fases existentes de parseo/semántica sin generación de código.
- [x] Adaptar `ide/ccom.py` y `interfaz/comprobacion.py` al servicio y sus fallos explícitos.
- [x] Probar fuente sin guardar, rutas con espacios, Unicode, falta de main, varios errores semánticos, cancelación y cero artefactos.

Criterio de cierre: el IDE recibe problemas estructurados del compilador real y no contiene reglas semánticas propias. F5/F6 y la CLI previa siguen funcionando.

Verificado: 312 pruebas del IDE, suite completa `make check`, contrato `make check-analysis-json` y 91 fuentes reales con respuestas compatibles. Protocolo, límites y alcance en [ccom_check_json.md](../../docs/ccom_check_json.md). F5 mantiene el lector textual; M1 cambia el análisis automático. No añade recuperación de errores sintácticos ni sustituye el LL(1).

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

- [ ] Instrumentar el lexer existente para exponer clasificación y rangos, incluidos comentarios para presentación, sin otra tabla de reglas Python.
- [ ] Transportar tokens/diagnósticos léxicos en la respuesta C y convertir posiciones en el adaptador.
- [ ] Reemplazar el recorrido Python de análisis vivo y la dependencia productiva de `ide/nucleo/lexico.py`.
- [ ] Mantener revisión, cancelación, colores, Deshacer y comportamiento sin guardar.

Criterio de cierre: una corrección en `grammar/lexer.l` es la fuente de las reglas usadas por compilación y editor.

### M3 — Sugerencias contextuales en C

- [ ] Trasladar comportamiento y casos de la entrega, no sus dataclasses duplicadas.
- [ ] Obtener fragmento/comentario del lexer C y contexto/esperados del parser existente.
- [ ] Distinguir prefijo válido de programa completo; no considerar cualquier EOF como error previo.
- [ ] Filtrar/rankear opciones en C, sin inventar `id`, `num` o símbolos declarados que no se hayan consultado.
- [ ] Conectar respuesta al panel actual de sugerencias y preferencias de pausa.
- [ ] Migrar pruebas de prefijos, cursor en medio de palabra, comentarios, límites, errores previos y cancelación.

Criterio de cierre: Ctrl+Espacio y pausa funcionan con el motor C real; no requieren la gramática/tabla/parser Python.

### M4 — Reparaciones en C y autorización de reglas

- [ ] Usar diagnósticos y expectativas estructuradas del parser C para construir candidatos.
- [ ] Reanalizar copias con el mismo parser, reiniciando y liberando su estado entre intentos.
- [ ] Trasladar pruebas de `;`, `.`, `]`, `)` y los negativos del compañero.
- [ ] Normalizar precondiciones y validación; no convertir instrucciones descriptivas en una precondición vacía que siempre pase.
- [ ] Resolver la diferencia entre inserción ante el siguiente token e inserción al final de la línea previa.
- [ ] Conectar panel, vista previa y aplicador existentes.
- [ ] Mover a C la validación del lenguaje que hoy hace `politica_asistencia.py`; conservar en Python preferencias, vigencia y aplicación.
- [ ] Habilitar solo reglas revisadas, con permiso del usuario, sin guardar ni encadenar reparaciones y con un Deshacer.

Criterio de cierre: consultas manuales y ruta automática usan el motor C y sus reglas comprobadas; no hay análisis del lenguaje en Python.

### M5 — Clones con el flujo léxico C

- [ ] Reemplazar el tokenizador y comparación Python de `ide/clones.py` por lógica C.
- [ ] Reutilizar tokens reales para Tipo I/II y preservar correspondencia consistente de nombres/literales.
- [ ] Definir envío de varias instantáneas, incluidas las de buffers sin guardar, sin perder límites ni cancelación.
- [ ] Mantener el panel de comparación/navegación en Python.

Criterio de cierre: no existe un segundo tokenizador Calser productivo para clones.

### M6 — Retiro de duplicados, comentarios y comprensión

- [ ] Revisar duplicaciones y retirar solo las sustituidas por recorridos comprobados; el LL(1) está autorizado en Python y no se elimina por obligación de lenguaje. Documentar su papel si se conserva como apoyo.
- [ ] Decidir explícitamente si los archivos anteriores se eliminan o conservan como material histórico fuera de producción; no seguir presentándolos como backend actual.
- [ ] Actualizar pruebas del IDE: casos del lenguaje contra el servicio C; dobles solo para aislar transporte/UI.
- [ ] Comentar cada archivo C y adaptador: propósito, consumidores, memoria, unidades, estado global, errores y cancelación.
- [ ] Actualizar `guia_sistema_calside.md`, contratos, plan y guías de motores.
- [ ] Ejecutar las suites afectadas del compilador y del IDE, y el recorrido manual de varios documentos.

Criterio de cierre: mapa y comentarios describen el sistema real; el análisis principal y los motores reutilizan C, el papel del LL(1) Python permitido queda explícito y cada recorrido tiene pruebas.

### M7 — Iteración futura de recuperación sintáctica

- [ ] Añadir sincronización al parser C para producir varios errores fiables en una misma consulta.
- [ ] Definir semántica sobre entrada recuperada y evitar generación con errores.
- [ ] Probar precisión, cascadas, progreso y límites.

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
| `src/diagnostics.c`, `include/diagnostics.h` | Añadir salida/colección estructurada reutilizable con lista de diagnósticos. | Nada de diagnóstico del lenguaje inferido por regex. |
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
