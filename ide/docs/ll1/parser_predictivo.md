# Parser predictivo LL(1)

## Reconocedor completo en C

Paso 2 implementado el 2026-11-22 en `feat/ide-ll1-c`. El código está en `src/servicios_ide/ll1_parser.c` y su interfaz pública en `include/servicios_ide/ll1_parser.h`. El paso 2 entregó el reconocedor nativo. Desde el paso 4, las sugerencias y reparaciones del editor ya lo utilizan. Desde el paso 5, el diagnóstico automático utiliza LL(1) para la sintaxis. La compilación y la construcción del AST mantienen Bison.

### Cómo funciona

`ll1_analyze` recibe texto y longitud en bytes. Flex entrega sus tokens reales mediante `lexer_visit_text`, sin llamar a `yyparse`. La visita libera las cadenas de identificadores, también cuando el parser se detiene por un error o por cancelación. Los callbacks solo permanecen instalados durante la consulta.

Las producciones se guardan como datos C en `src/servicios_ide/ll1_grammar.inc`. `tools/generar_datos_ll1.py` permite regenerarlas desde la referencia auditada y comprobarlas con `--check`. Esa herramienta asigna los índices y exporta las producciones. No calcula los conjuntos ni la tabla del reconocedor. El ejecutable usa los datos incluidos y no necesita Python para analizarlos.

El código C calcula PRIMERO y SIGUIENTE por punto fijo y construye la tabla predictiva. Rechaza su uso si encuentra conflictos. La tabla queda interna e inmutable después de prepararse. Cada análisis utiliza una pila propia, expande las producciones en orden inverso y consume los terminales que coinciden con el token de entrada. Solo acepta al consumir el fin de entrada y dejar la pila vacía.

No construye AST ni hace análisis semántico. Un nombre no declarado puede ser sintácticamente válido y se revisará en la fase semántica de ccom. El reconocedor se detiene en el primer obstáculo y no reutiliza la recuperación de Bison. Al detectar un error calcula los terminales esperados desde toda la continuación de la pila, evitando presentar el SIGUIENTE global como si fuera el contexto concreto.

### API y resultados

```c
LL1Result ll1_analyze(const char *text, size_t length, const LL1Limits *limits,
                      LL1Cancel cancel, void *data);
```

Los argumentos `limits`, `cancel` y `data` pueden ser NULL. Un texto NULL con longitud cero representa un archivo vacío. Un texto NULL con longitud positiva se rechaza como argumento inválido. El llamador conserva su texto y recibe el resultado por valor, sin punteros prestados.

`LL1Result` informa estado, rango del token inesperado, línea, columna, cantidad de tokens consumidos, pasos, causa de límite y terminales esperados. Los offsets son bytes UTF-8 en un intervalo que incluye el inicio y excluye el final. Las líneas y columnas parten de uno. La columna sigue a Flex y cuenta bytes, por lo que la interfaz deberá convertirla a las unidades de Qt cuando se conecte el motor.

Los estados son `LL1_ACCEPTED`, `LL1_SYNTAX_ERROR`, `LL1_LEXICAL_ERROR`, `LL1_LIMIT`, `LL1_CANCELLED` y `LL1_INTERNAL_ERROR`. Un límite, una cancelación o un fallo interno nunca cuentan como aceptación. `ll1_terminal_name` y `ll1_expects` permiten consultar los terminales del resultado sin depender de sus índices internos.

Los valores predeterminados permiten 200000 bytes, 50000 tokens consumidos, 1000000 pasos y 4096 símbolos en la pila. Se pueden solicitar límites menores. La cancelación se consulta entre pasos y tokens. Un bloque de espacios o comentarios se termina de escanear antes de volver a consultarla. Se conservan las posiciones del primer error léxico comunicado por Flex, incluido el inicio de un comentario sin cerrar.

Flex y el servicio de diagnósticos siguen usando estado global. Por eso este servicio no se ejecuta simultáneamente con otro análisis dentro del mismo proceso. Cada llamada limpia los buffers y callbacks que instala. La futura conexión puede mantener el modelo actual de una consulta por subproceso.

### Comprobación del paso 2

```sh
make check-ll1-c
```

Si la carpeta `build` contiene dependencias de una estructura anterior del repositorio, se puede compilar en una carpeta temporal sin borrar esos archivos.

```sh
make BUILD_DIR=/tmp/calside-ll1-build check-ll1-c
```

`pruebas/ll1/probar.c` comprueba el reconocedor nativo, los límites, la cancelación, los offsets con UTF-8 y CRLF, una profundidad real excesiva y 300 consultas repetidas. El auxiliar se enlaza con `yyparse` interceptado. Si LL(1) intenta llamar a Bison, la prueba aborta.

`pruebas/ll1/probar.py` compara la aceptación del auxiliar nativo con `ccom --ast-json`. Se comprobaron 171 archivos y 296 variantes de un programa con arreglos y ciclos. Ambos analizadores coincidieron. También pasaron los casos nativos bajo AddressSanitizer, UndefinedBehaviorSanitizer y detección de fugas. Estos resultados no equivalen a una prueba matemática de equivalencia para todo programa posible.

Los servicios actuales de tokens, sugerencias, reparaciones, diagnósticos y recuperación siguen pasando sus comprobaciones dirigidas. La consulta del contexto de un prefijo se implementó en el paso 3, descrito a continuación. La adaptación de sugerencias y reparaciones se documenta en el paso 4 de este archivo.

## Contexto del cursor en C

Paso 3 implementado el 2026-11-22. La función `ll1_context` reutiliza la máquina del reconocedor completo, pero no exige que el prefijo termine como un programa. Cuando Flex alcanza el cursor, la consulta devuelve los terminales posibles desde toda la pila y conserva la diferencia entre código incompleto y código incorrecto.

```c
LL1Context ll1_context(const char *text, size_t length, size_t cursor, size_t point,
                       const LL1Limits *limits, LL1Cancel cancel, void *data);
```

`length` es el tamaño del documento completo. `cursor` es la posición que interesa consultar. `point` es el inicio del fragmento parcial que se quiere completar. Los tres usan bytes UTF-8 del texto original. Se exige que `point` no supere el cursor y que ambos estén dentro del documento y en fronteras de carácter.

Cuando `point` coincide con el cursor, se consumen todos los tokens del prefijo. Para completar una palabra como `vo`, el llamador sitúa `point` al inicio de esa palabra. Ese fragmento debe ser una palabra, un número o un operador parcial. No puede empezar en medio de un token anterior. El sufijo posterior al cursor no aporta tokens ni errores a la consulta. El límite de tamaño del documento completo sigue siendo aplicable.

Por ejemplo, al consultar al final de `F main [] :` se devuelven `bool`, `int` y `void`. Al consultar `F main [] : vo` con el punto al inicio de `vo`, se obtienen esas mismas alternativas de la gramática. Filtrarlas con el prefijo escrito para ofrecer solo `void` corresponde al motor de sugerencias del paso 4.

`LL1Context.analysis` conserva el resultado del parser. Sus estados adicionales son `LL1_PREFIX_VALID`, `LL1_IN_COMMENT` y `LL1_INVALID_CONTEXT`. Un prefijo válido puede necesitar muchas instrucciones más y no se anuncia como un programa aceptado. `can_end` solo es verdadero si el prefijo válido también puede terminar en ese punto como programa sintácticamente completo. No certifica su semántica.

Dentro de un comentario de línea o de bloque la consulta devuelve `LL1_IN_COMMENT`, el tipo de comentario y su inicio. No devuelve terminales para sugerir. La clasificación procede de los estados y eventos de Flex. Un comentario de bloque abierto en el cursor no se convierte en un error artificial de cierre. En cambio, `ll1_analyze` sigue denunciándolo cuando se analiza como documento completo. Después del cierre de bloque o del salto de línea que termina un comentario de línea, el contexto vuelve a permitir continuaciones.

Los errores léxicos o sintácticos anteriores al punto siguen bloqueando la consulta. Los terminales guardados en un resultado de error describen el primer obstáculo, no el cursor posterior. Los consumidores deben comprobar el estado antes de usarlos. Las posiciones inválidas, los fragmentos mal formados y los cortes de un carácter UTF-8 se rechazan sin propuestas. Los límites y la cancelación se mantienen y no se interpretan como contexto válido.

### Verificación del contexto

`make check-ll1-c` incluye el auxiliar de consulta y `pruebas/ll1/probar_contextos.py`. La referencia Python se usa exclusivamente como oráculo de desarrollo para contrastar estados y conjuntos esperados. No participa en el análisis nativo ni en la interfaz.

Se contrastaron 423 posiciones de cursor, incluidas declaraciones, tipos parciales, llamadas, índices de matrices, ramas, ciclos, comentarios y UTF-8/CRLF. Los resultados nativos coincidieron con la referencia LL(1). También pasaron los casos de posiciones inválidas, sufijos con errores ignorados, cancelación y límites. El auxiliar sigue abortando si se llama a `yyparse`.

Se conservaron las comparaciones de programas completos en 171 archivos y 296 variantes. Las comprobaciones nativas con 300 análisis completos y 200 consultas repetidas de contexto pasaron bajo ASan, UBSan y detección de fugas. Los servicios actuales de ccom también pasaron sus regresiones dirigidas. El paso 3 mantuvo los consumidores anteriores. El paso 4 adapta sugerencias y reparaciones a esta API sin cambiar el contrato JSON productivo.

## Sugerencias y reparaciones con LL(1) en C

Paso 4 implementado el 2026-11-22. `suggestions.c` usa `ll1_context` para obtener los terminales posibles antes de la palabra parcial. Flex sigue delimitando el reemplazo completo, incluso cuando el cursor está en medio de una palabra. El motor C filtra por el prefijo escrito y mantiene las preferencias de vista gris, las plantillas de corchetes y el tratamiento especial de `main`. El IDE presenta las propuestas y aplica la seleccionada con Deshacer.

`syntax_service.c` usa `ll1_analyze` para analizar documentos completos. Conserva un recorrido previo de Flex para detectar errores léxicos y recoger posiciones. `repairs.c` toma los cierres esperados del primer error y prueba cada inserción en una copia completa. Una propuesta informa si la copia fue aceptada o si solamente avanzó más allá del obstáculo original. La automatización sigue limitada a un único punto y coma seguro, con aceptación completa y fuera de EOF.

Las rutas `--suggest-json`, `--repair-json` y `--syntax-json` ya usan LL(1). Mantienen los campos, offsets UTF-8 y estados que esperan los adaptadores Python. No se modificó el código de los widgets ni se trasladaron reglas del lenguaje a Python. Los hooks antiguos se conservaron inactivos durante ese paso y se retiraron al cerrar el paso 6.

`--check-json` y la compilación conservan Bison. El paso 5 conecta el diagnóstico automático mediante una operación específica y mantiene la semántica existente, como se explica a continuación. LL(1) se detiene en el primer error y no ofrece recuperación ni semántica propia.

### Verificación del paso 4

`pruebas/ll1/probar_asistencia.py` ejecuta 30 consultas de los servicios reales con `yyparse` interceptado. Cualquier llamada a Bison aborta el auxiliar. Contrasta además el JSON con el ejecutable productivo y comprueba que las fuentes y sus artefactos no cambian. Incluye palabras parciales, tipos, corchetes de `main`, punto tras `E;`, errores anteriores y posteriores, comentarios, Unicode, cuatro cierres y reparaciones parciales.

La comparación independiente con Bison ahora usa `--ast-json`, porque `--syntax-json` ya pasó a LL(1). Se conserva la comparación de 171 archivos y 296 variantes. La referencia Python sigue reservada a las pruebas de contexto. Los contratos detallados están en [los servicios del compilador](../../../docs/ccom_servicios_ide.md).

## Diagnóstico automático del editor

Paso 5 implementado el 2026-11-22. `ComprobadorCcom` ejecuta `--check-ll1-json` sobre su copia temporal. La operación devuelve `check_ll1` con el mismo sobre v1 de diagnósticos y las mismas categorías y coordenadas UTF-8. El adaptador exige esa operación y no vuelve a consultar Bison ni un parser Python si falla el servicio. Las pausas, cancelación, límites de transporte y descartes por documento y revisión se conservan.

En C, `syntax_check_ll1` lee una fuente acotada y llama a `ll1_analyze`. Si hay un error sintáctico publica el primer obstáculo y sus terminales esperados, sin llamar a `yyparse`. Un error léxico se presenta mediante los mensajes y posiciones reales de Flex. Un límite produce un informe incompleto y el editor lo muestra como fallo de análisis, nunca como documento válido. Los rangos explícitos conservan EOF después de un salto de línea.

Si LL(1) acepta el documento completo, el conductor reutiliza el recorrido de análisis de ccom. Bison construye el AST y el semántico existente revisa nombres, tipos y `main`. Esta construcción sigue siendo necesaria porque LL(1) reconoce sintaxis y no genera árboles. No se añadió otro semántico ni se modificó el compilador para que genere código desde la pila predictiva. La copia privada del editor permanece fija durante ambas etapas.

El análisis automático muestra un error sintáctico por consulta. Al corregirlo se analiza otra revisión y puede aparecer el siguiente. Los servicios de sugerencias y reparaciones ya usan LL(1) desde el paso 4. La compilación F5 y `--check-json` conservan Bison y su recuperación de múltiples errores. Los comentarios válidos sin código continúan filtrándose en la interfaz a partir de los tokens de Flex.

`make check-ll1-ide` comprueba 15 documentos, diagnósticos semánticos, rangos UTF-8 y EOF, límites, incompatibilidad de opciones y conservación de archivos. Los errores sintácticos y léxicos se contrastan con el auxiliar que aborta si se llama a Bison. La validación automatizada final se registra en el paso 6. La revisión visual humana se mantiene en la guía manual.

## Cierre de la integración

Paso 6 completado el 2026-11-22. Se retiraron de Bison los hooks de sugerencias y reparaciones, el marcador privado de cursor y el estado que detenía las consultas antiguas. También se eliminó `lexer_parse_text`, que ya no tenía consumidores. El destructor del símbolo inicial conserva la transferencia del AST al conductor y la recuperación sigue liberando los subárboles descartados. Las acciones de construcción de funciones se ajustaron al retirar la acción intermedia obsoleta.

La gramática válida extraída de Bison conserva 101 producciones y su transformación LL(1) mantiene 118 producciones, 60 no terminales, 47 terminales y 336 celdas sin conflictos. El auxiliar nativo sigue abortando ante una llamada a `yyparse`. No hay fallback productivo a los analizadores Python.

| Recorrido | Responsabilidad de C | Responsabilidad de Python |
| --- | --- | --- |
| Colores y errores léxicos | Flex y `--tokens-json` | Convertir posiciones y dibujar |
| Diagnóstico automático | LL(1) reconoce sintaxis con `--check-ll1-json`. Si acepta, Bison construye el AST y el semántico existente revisa el programa | Consultar una copia vigente y mostrar problemas |
| Sugerencias | `ll1_context` y el motor C eligen continuaciones, reemplazos y preferencias | Dibujar la lista y el gris, aplicar con Deshacer |
| Reparaciones | LL(1) aporta cierres y valida cada copia con `--repair-json` y `--syntax-json` | Presentar cambios y controlar permisos de aplicación |
| Compilar y ejecutar | Bison, semántico, optimizador y generadores conservan la compilación | Manejar procesos y presentar la salida del simulador externo |
| Clones | Flex y comparación Tipo I/II en C | Preparar copias y presentar los pares |

La evidencia automatizada incluye equivalencia sintáctica en 171 fuentes y 296 variantes, 423 posiciones de contexto y los servicios reales bajo sanitizadores. Las regresiones del compilador incluyen AST, programas, matrices y equivalencia O0/O1 en 67 programas, con entradas externas de cero y patrón. Pasaron 158 pruebas dirigidas de gramática e integración del IDE. La integración gráfica se ejecuta sin pantalla mediante Qt y cubre documentos, cancelación, diagnóstico progresivo, sugerencias, reparaciones y ejecución. El recorrido aislado prohíbe importar los analizadores Python.

Para repetir la comprobación nativa desde la raíz del repositorio se puede usar una carpeta de objetos nueva y conservar la caché anterior.

```sh
make -j2 BUILD_DIR=/tmp/calside-ll1-verificacion check-ll1-c check-ll1-ide
```

Estas comprobaciones no prueban equivalencia para todos los programas posibles ni sustituyen la revisión humana de legibilidad y manejo del escritorio. La lista breve para esa revisión está en [revisión manual](../revision_manual.md#revision-ll1). El empaquetado continúa como trabajo independiente del IDE.

## API Python de referencia


Estado M6 (2026-11-11): API Python de consulta/estudio y pruebas diferenciales, conservada por autorización explícita. Se implementó originalmente en `feat/parser-ll1` el 2026-11-05. **El editor ya utiliza C** para análisis, sugerencias, reparaciones y clones; no hay integración gráfica pendiente de este parser ni fallback al LL(1) Python. [Frontera actual](../guia_sistema_calside.md), [contratos productivos](../asistencia.md).

## Decisiones cerradas

- **1B:** analizar programas completos y consultar el contexto hasta el cursor.
- **2A:** pila explícita y tabla LL(1) existente.
- **3A:** detenerse en el primer error; recuperación por sincronización (3B) queda para después.
- **4A:** resultados, diagnósticos y estado predictivo; sin árbol ni AST. Traza opcional limitada.
- **5B:** implementar el parser primero y documentar su API real para que los motores se adapten.

Este módulo es Python puro. Reutiliza `lexico.py`, `gramatica.py` y `tabla.py`; no cambia la gramática ni ccom. No usa Qt, no guarda archivos y no aplica reparaciones. Permanece fuera de los widgets: el análisis visible y los motores M1–M5 reutilizan ccom. Los ejemplos siguientes describen exclusivamente esta API de consulta.

## Uso inmediato

Desde Python, con la raíz del repositorio en el directorio de trabajo:

```python
from ide.nucleo.parser import analizar_sintaxis, contexto_en

texto = "F main [] : void ; Vm total : int = 3 ; E ; ."
r = analizar_sintaxis(texto)
assert r.aceptado
assert r.estado == "aceptado"

texto = "F main [] : void ; Vm total : "
c = contexto_en(texto, len(texto))
assert c.valido
assert c.esperados == ("bool", "int")
assert not c.puede_terminar
```

Un prefijo puede ser válido aunque el programa completo todavía no lo sea:

```python
texto = "F main [] : void ; Vm total : int = "
assert not analizar_sintaxis(texto).aceptado
c = contexto_en(texto, len(texto))
assert c.valido
assert c.esperados == ("!", "(", "-", "[", "falso", "id", "num", "verdadero")
```

`[` es válido aquí porque la declaración admite un inicializador de arreglo. En una expresión ordinaria no se agrega esa alternativa. Los motores no deben reemplazar estos resultados por listas fijas de palabras.

## API pública

```python
analizar_sintaxis(texto: str, *, traza=False, cancelar=None) -> ResultadoSintactico
contexto_en(texto: str, cursor: int, inicio_fragmento: int | None = None,
            *, traza=False, cancelar=None) -> ContextoPredictivo

ParserPredictivo(gramatica=CALSER, *, max_caracteres=2_000_000,
                 max_pasos=1_000_000, max_pila=4096, max_traza=500)
```

La clase ofrece los mismos métodos. Sirve para reutilizar una configuración, probar gramáticas pequeñas o cambiar límites. Construye la tabla una vez y rechaza conflictos LL(1) con `ValueError`. La gramática debe conservarse sin cambios después de construir el parser. Las funciones de módulo reutilizan una instancia para Calser; las pilas y los resultados son locales a cada llamada.

`cancelar`, si se usa, es una función sin argumentos que devuelve un booleano. Se consulta antes del análisis y entre pasos. No interrumpe en mitad del lexer o del recorrido de comentarios: el tamaño del texto está limitado, pero no es una cancelación instantánea. Las excepciones de programación no se convierten en aceptación ni en una lista vacía.

### ResultadoSintactico

Dataclass congelada con estos campos:

| Campo | Significado |
|---|---|
| `estado` | Estado de la tabla de estados inferior. |
| `pila` | Tupla de símbolos restantes; **cima a la derecha**, `$` al fondo. |
| `esperados` | Tupla ordenada de terminales de FIRST de la continuación completa. Nunca incluye epsilon. |
| `diagnosticos` | Tupla vacía o un `DiagnosticoSintactico`; no hay recuperación. |
| `errores_lexicos` | Tupla de los `ErrorLexico` originales del lexer para el texto consultado. |
| `tokens_consumidos` | Tokens reales consumidos; no cuenta EOF virtual. |
| `offset_alcanzado` | Fin del último token consumido o posición del error; al aceptar, fin del texto. |
| `pasos` | Acciones ejecutadas por la máquina, incluida aceptación o error sintáctico. |
| `traza` | Tupla de `PasoParser`, si se pidió; vacía por defecto. |
| `traza_truncada` | Indica que hubo más acciones que el límite de traza; no significa fallo del análisis. |
| `motivo` | Explicación de bloqueo/cancelación o nombre del límite alcanzado. |
| `aceptado` | Propiedad verdadera únicamente si `estado == "aceptado"`. |

| Estado | Interpretación |
|---|---|
| `aceptado` | Programa completo sintácticamente aceptado y sin errores léxicos. |
| `prefijo_valido` | Todos los tokens del prefijo fueron consumidos; aún puede faltar código. Solo contexto. |
| `error_sintactico` | Primer token incompatible o EOF prematuro; conserva pila y diagnóstico. |
| `error_lexico` | No se continúa a través del primer error léxico. |
| `comentario` | Cursor dentro de comentario con prefijo anterior válido; no ofrece terminales. Solo contexto. |
| `limite` | Se alcanzó tamaño, pasos o profundidad. No es aceptación. |
| `cancelado` | El solicitante pidió cancelación. No es aceptación. |

El lexer se ejecuta sobre todo el texto consultado antes del recorrido sintáctico. Por ello `errores_lexicos` puede contener un error posterior al primer error sintáctico. La máquina se detiene en el primer obstáculo que encuentra en orden de entrada; no salta un error léxico para aceptar los tokens restantes. `esperados` en un resultado bloqueado describe la pila detenida, **no** constituye una sugerencia fiable para un cursor posterior.

### Diagnósticos, símbolos y traza

`DiagnosticoSintactico` contiene `codigo` (`SINTAXIS_TOKEN` o `SINTAXIS_EOF`), `mensaje`, `encontrado`, `esperados` y `pila`. Expone propiedades `inicio`, `fin`, `linea` y `columna` del símbolo encontrado.

`SimboloEntrada` contiene `tipo`, `lexema`, `inicio`, `fin`, `linea`, `columna`. EOF tiene tipo `$`, lexema vacío y rango `[len(texto), len(texto))`. No inventar un carácter después del archivo.

`PasoParser` contiene `numero`, `accion` (`expandir`, `consumir`, `aceptar`, `error`), `pila` antes de esa acción, `indice_token`, `encontrado` y `produccion` (solo para expansión). No es un árbol.

Ejemplo que puede usar reparaciones:

```python
texto = "F main [] : void ; vs\nE ; ."
r = analizar_sintaxis(texto)
d = r.diagnosticos[0]
assert d.codigo == "SINTAXIS_TOKEN"
assert d.encontrado.lexema == "E"
assert (d.inicio, d.fin, d.linea, d.columna) == (22, 23, 2, 1)
assert d.esperados == ("=", "[")
assert d.pila == ("$", "funciones_p", ".", "cuerpo", "sent_id_p")
```

Que el parser espere `=` o `[` no demuestra qué quería escribir el usuario. El motor de reparaciones decide si hay una propuesta justificable; no debe insertar uno de ellos automáticamente por ser esperado.

## Consulta del cursor y fragmentos parciales

`contexto_en` analiza únicamente `texto[:punto]`. El sufijo a la derecha no participa: errores posteriores al cursor no bloquean esta consulta.

- Si no se pasa `inicio_fragmento`, `punto = cursor`. El lexer trata el prefijo como tokens completos. **No hay detección automática de la palabra a completar.**
- Si se pasa, `punto = inicio_fragmento`. Se excluye ese fragmento y se devuelven los terminales válidos antes de él. `prefijo` conserva `texto[inicio_fragmento:cursor]` para filtrar sugerencias.
- `inicio_fragmento` debe estar entre 0 y el cursor y no partir un token anterior. El fragmento no vacío debe ser palabra ASCII, número o fragmento de operador compuesto por `= ! < >`, de hasta dos signos. No es un rango arbitrario de código.
- Cursor y offsets deben ser enteros válidos (no booleanos); contratos inválidos generan `ValueError`/`TypeError`, no un diagnóstico del lenguaje.

```python
texto = "F main [] : vo"
inicio = texto.index("vo")
c = contexto_en(texto, len(texto), inicio)
assert c.valido
assert c.prefijo == "vo"
assert c.esperados == ("bool", "int", "void")
assert contexto_en(texto, len(texto)).estado == "error_sintactico"
```

**El motor de sugerencias debe elegir el comienzo y el fin del reemplazo.** Por ejemplo, en `Vm to|tal : int ;`, pasar el comienzo de `total` permite obtener `id` como esperado, pero si se reemplaza la palabra hay que incluir también `tal` para no duplicarlo. El parser no devuelve texto insertable ni un catálogo de nombres declarados.

### ContextoPredictivo

Campos: `cursor`, `punto`, `inicio_fragmento`, `prefijo`, `tipo_comentario` (`linea`, `bloque` o `None`), `resultado`. Este último es un `ResultadoSintactico` con métricas, traza y estado.

Atajos: `estado`, `pila`, `esperados`, `diagnosticos`, `errores_lexicos`, `valido`, `puede_terminar`. `valido` solo es verdadero para `prefijo_valido`; `puede_terminar` además exige `$` en esperados. Un programa completo puede permitir también otra función: después de una función Calser, los esperados incluyen `$` y `F`.

Si el cursor está dentro de `#…` o `#[…]#`, se analiza solo hasta el inicio de ese comentario y se devuelve `estado="comentario"`, sin sugerencias ni el error fatal artificial de cortar un bloque en el cursor. En ese caso `punto` e `inicio_fragmento` señalan el comienzo del comentario, y `prefijo` es vacío. Si ya había un error antes, se conserva el estado de error y se informa también el tipo de comentario. Justo después de cerrar un bloque o de un salto de línea que termina un comentario de línea se vuelve a contexto de código. Comentarios de bloque no se anidan, igual que en el lexer.

## Posiciones y responsabilidades de integración

Offsets en caracteres Python desde 0, rangos `[inicio, fin)`. Líneas y columnas desde 1; una tabulación cuenta un carácter; `\r` cuenta columna y `\n` inicia línea, igual que en el lexer. Los `ErrorLexico` conservan la API original (`linea`, `columna`, `largo`, `fatal`), sin offsets globales. Convertirlos mediante los comienzos de línea si se necesitan rangos.

Qt usa UTF-16 y ccom bytes UTF-8: los adaptadores de CALSIDE convierten las posiciones. No usar estos offsets directamente como posiciones Qt con texto Unicode.

CALSIDE captura documento, revisión, texto y cursor y descarta respuestas antiguas, pero sus reparaciones productivas reanalizan copias con `--syntax-json` en C. Esta API LL(1) puede reanalizar una copia explícitamente para estudio; no conoce widgets ni identidad de documento y no ofrece reanudación desde una pila ni recuperación por sincronización.

No se ha creado `modelos.py`: las estructuras del parser viven en `parser.py`. Importarlas desde ahí sin duplicarlas. Los modelos de sugerencias y de ediciones pertenecen al compañero; si se extraen tipos comunes después, conservar importaciones compatibles.

## Pruebas y alcance comprobado

```bash
make check-parser   # Solo parser; no necesita PySide6.
make check-ide      # Suite completa de IDE y núcleo.
```

La prueba diferencial usa `ccom --ast-json` dentro de un temporal: esa ruta valida sintaxis sin exigir semántica. `ccom --check` también ejecuta semántica y no sirve comparar solo su código de salida para este fin. Si no existe el ejecutable ccom, se omite explícitamente esa prueba; construirlo con `make` para ejecutarla.

Resultado verificado: 26 pruebas nuevas del parser y 96 pruebas combinadas aprobadas. Corpus diferencial: 110 archivos `.cls` del repositorio, 105 sin errores léxicos comparados con Bison sin discrepancias; los otros cinco no forman parte de la comparación sintáctica. Incluye programas con errores semánticos que deben seguir siendo sintácticamente válidos.

Se cubren prefijos antes/después de tokens reales, FIRST de toda la pila frente a FOLLOW global, fragmentos, comentarios, EOF, Unicode/CRLF/tabulaciones, límites, cancelación, trazas y reutilización concurrente.

Limitaciones deliberadas: primer error, sin AST, sin recuperación, sin semántica, sin identificadores declarados, sin aplicación de reparaciones y sin integración visual nueva. Los límites son por llamada: 2 millones de caracteres, 1 millón de acciones, 4096 símbolos en pila y 500 entradas de traza. `max_pila` mínimo 2; los otros límites son enteros positivos. El lenguaje reconocido no cambia por aumentar estos límites.
