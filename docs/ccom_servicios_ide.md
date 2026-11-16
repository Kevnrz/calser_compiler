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

La cabecera común se valida en `ide/ccom.py:informe_json`; los estados y el contenido específico se validan en cada adaptador. No confundir `syntax` (solo sintaxis) con `check` (también semántica). [Arquitectura del IDE](../ide/docs/guia_sistema_calside.md), [asistencia y permisos](../ide/docs/asistencia.md).


<a id="check"></a>

## Análisis completo y diagnósticos

Implementado el 2026-11-11. La interfaz llama al front-end existente en C y recibe datos estructurados. No hay otra gramática, parser ni semántico para esta operación.

### Invocación y compatibilidad

```sh
./ccom --check-json programa.cls
./ccom --check-json -v --time programa.cls
```

Ejecuta exactamente las fases léxica, sintáctica y semántica de `--check`. No genera ni modifica `.bin`, `.hex`, `.mem`, `.asm` o AST JSON. Solo admite las opciones auxiliares `-v`/`--time`; combinarlo con otra consulta, incluido `--check`, devuelve error de uso (2). Las opciones de salidas, ensamblado, listados, TAC, niveles y optimización/métricas se rechazan antes de abrir la entrada.

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
- F5 conserva el lector textual y F6 el simulador. El filtro léxico del editor usa Flex mediante `--tokens-json`; el LL(1) Python no participa en el recorrido productivo.

**Múltiples errores:** el lexer/semántico ya pueden emitir listas y M7 permite varios errores sintácticos independientes mediante recuperación conservadora. Un AST con errores no pasa a semántica ni generación. [Alcance y límites](#recovery).

### Verificación

```sh
make
make check-analysis-json
make check
make check-ide
```


Las pruebas específicas cubren fases reales, varios errores, errores generales, tildes/emoji, CRLF, rutas con espacios/comillas, límites, opciones inválidas, artefactos intactos, respuestas malformadas/versiones incompatibles y navegación. Las pruebas previas de procesos conservan cancelación, timeout, cierre, Guardar como, limpieza de temporales y prioridad de F5.

### Operación léxica complementaria (M2)

El resaltado del editor consulta [--tokens-json](ccom_servicios_ide.md#tokens): utiliza el mismo lexer, sin parser, y añade comentarios/rangos para presentación. Su operación es `lex`; no sustituye ni altera el contrato `operation: check` de este documento.

<a id="tokens"></a>

## Tokens y resaltado

Implementado el 2026-11-11. Reutiliza `grammar/lexer.l`: no llama al parser ni al semántico y no genera artefactos. Puede resaltar documentos incompletos y tokens posteriores a un error sintáctico. El lenguaje y los modos clásicos de compilación mantienen sus reglas.

```sh
./ccom --tokens-json programa.cls
./ccom --tokens-json -v --time programa.cls
```

Solo se admite una entrada y opciones auxiliares `-v`/`--time`. Se rechazan listados/salidas y combinaciones con `--check`/`--check-json`. Retornos 0 (léxico correcto), 1 (errores, fallo o límite) y 2 (uso incompatible, sin prometer JSON). `--help` conserva su funcionamiento.

### Transporte v1

Comparte el sobre y el contrato de diagnósticos de [--check-json](ccom_servicios_ide.md#check), cambiando `operation` a `lex` y añadiendo `fatal` y `tokens`:

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

El análisis sintáctico/semántico automático sigue usando `--check-json` después de otra pausa de 600 ms y solo si el léxico está libre de errores. F5/F6 mantienen los perfiles M1B. El LL(1) Python queda para consulta/pruebas; clones y validación de reparaciones utilizan C. La interfaz no carga otro tokenizador de Calser.

### Verificación

`make check-tokens-json`, incluido en `make check`: clasificación, comentarios, Unicode/CRLF, error léxico, EOF fatal, continuación sin parser, límites, opciones y salidas intactas. `make check-ide`: transporte inválido, posiciones Qt, cancelación real, proceso antiguo, edición/pestañas y Deshacer. Pruebas nuevas en `ide/pruebas/test_tokens_ccom.py`; la suite de comprobación usa Flex real al simular fallos únicamente de `--check-json`.

M3 comparte este transporte en `ide/servicio_ccom.py` y añade [sugerencias desde Flex/Bison-LAC](ccom_servicios_ide.md#suggest). Los límites léxicos y el contrato de tokens M2 se conservan.

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

Las llamadas de Calser usan `nombre[argumentos]`. Si aparece `(` después de un identificador y LAC esperaba `[`, el diagnóstico C señala ese paréntesis y explica `nombre[...]`, en lugar de priorizar el mensaje genérico de `;` faltante. Esto no cambia la sintaxis ni reescribe la llamada.

### Cómo calcula las propuestas

1. `suggestions_run`, en `src/servicios_ide/suggestions.c`, lee la fuente acotada e instala un observador de diagnósticos silencioso.
2. Flex recorre la fuente completa y publica sus rangos mediante `LexerObserver` (`include/analisis/lexer_service.h`). Se detectan comentarios, fragmento alrededor del cursor y errores anteriores. Los errores del sufijo posterior no bloquean el prefijo consultado.
3. Para palabras/números, el fragmento se toma cuando `inicio < cursor <= fin`; para operadores extensibles `= ! < >`, cuando `inicio <= cursor < fin`. Los delimitadores y operadores ya completos se consideran parte del prefijo. Un carácter ASCII de reservada incompleta, como `V`, puede observarse como fragmento inválido y completarse a `Vi`/`Vm`; no se modifica la regla de Flex ni se tolera ese error al compilar.
4. Se analiza únicamente el prefijo anterior al fragmento con el parser existente. En modo consulta, `syntax_lex` sustituye el EOF temporal por un **TOKEN_CURSOR privado**, que no pertenece a ninguna producción. `yyreport_syntax_error` consulta `yypcontext_expected_tokens` con **LAC**, distingue ese marcador de un error real anterior y recoge los terminales concretos.
5. Así un programa completo también puede ofrecer continuaciones, sin confundir EOF con un error del usuario. No se derivan reglas a partir de mensajes humanos ni se copia una gramática a Python.
6. C excluye identificadores genéricos, números genéricos, EOF y símbolos internos. Filtra por el prefijo escrito y ordena: palabras, delimitadores, operadores; alfabético dentro de cada grupo.
7. Cada propuesta reemplaza el fragmento completo. En `vo|id` reemplaza `void`, evitando duplicar el sufijo. Si no hay una completación y Flex reconoció un token completo hasta el cursor, C vuelve a consultar LAC después de ese token. Así `F main` puede ofrecer su continuación sin exigir un espacio y `void` completo puede ofrecer `;`. No se avanza sobre un prefijo inválido.
8. `inline_preferred` (booleano opcional) distingue propuestas útiles para la vista gris: prefijos escritos, tipos y puntuación local. No se priorizan `.` ni `F` sin prefijo por el mero hecho de ser continuaciones únicas. C sí prioriza `.` después de los tokens `E;` cuando existe un salto de línea entre el terminador y el cursor, LAC lo ofrece y no existe ya ese punto a la derecha; `F` continúa sin preferencia automática. Una lista válida sin preferencia tampoco deriva a reparaciones para cerrar el cuerpo. Una línea vacía con muchas instrucciones posibles conserva la lista manual, pero no elige `C` por orden alfabético. La ausencia del campo se interpreta como false.
9. Cuando `[` es la única continuación, C puede devolver el par `[]` con `cursor_relative: 1`: al aceptar, el cursor queda dentro. En la declaración de `main` devuelve `[]: ` con cursor relativo 4, o solo `[]` con cursor relativo 2 si ya hay dos puntos a la derecha. El contexto de función procede de una acción de Bison, y los tokens Flex delimitan el nombre declarado; el atajo no afecta `[` dentro del cuerpo. En el retorno de main se prioriza `void`; los tipos locales y de otras funciones siguen disponibles. Este campo opcional mide bytes UTF-8 del texto propuesto y el adaptador lo convierte a caracteres Python. No se duplica un delimitador que ya está a la derecha del cursor.

Los comentarios de línea incluyen el punto antes de LF; los bloques abiertos incluyen EOF. Después de un bloque cerrado sí se puede consultar. No hay sugerencias dentro de comentarios.

Las propuestas son **continuaciones sintácticas**, no garantías semánticas. No se inventan nombres declarados ni literales numéricos. Consultar ámbitos, completar símbolos declarados o añadir plantillas de instrucciones completas queda como ampliación posterior; el par de corchetes ya se presenta desde C.

### Memoria, límites y cancelación

Cada solicitud usa un proceso independiente. Límite nativo: **200000 bytes**, **50000 tokens/tramos** y **64 opciones**. Se informa límite sin publicar una lista parcial como completa. El máximo de opciones del contrato sustituye al límite opcional de 24 de la entrega Python; no se añadió una opción CLI de truncamiento arbitrario.

Flex libera las cadenas del escaneo completo que normalmente consumiría Bison. El escaneo del prefijo reinicia posiciones/estado y sus buffers. Los destructores liberan cadenas y subárboles descartados. El símbolo inicial distingue transferencia del AST a main de liberación en consultas; el marcador solo actúa en sugerencias. No se reutilizan pilas entre procesos.

`ide/servicio_ccom.py` comparte con M2 el transporte cancelable, sin shell. Captura stdout/stderr en temporales, los vigila y siempre mata/recoge el hijo si se cancela o falla. M3 limita su proceso a **2,5 s**, stdout a **1 MB** y stderr a **1 MB**; el coordinador mantiene su plazo general de 3 s. Las fuentes sin guardar nunca se escriben en la ruta del usuario: se consulta una copia privada eliminada al terminar.

### Conexión al IDE

- `ide/sugerencias_ccom.py`: firma `sugerir(texto, cursor, *, cancelar=None, root=None, configurado='')`, devuelve los tipos de `ide/nucleo/asistencia.py`. Valida versión, operación, fuente, cursor, código, estado, cantidades, rangos, fronteras UTF-8, textos y duplicados. Solo convierte posiciones; no clasifica ni filtra lenguaje.
- `ide/interfaz/asistencia.py`: carga ese adaptador por defecto y captura root/ccom configurado para cada tarea. El proveedor de reparaciones también consulta C; véase la sección de reparaciones. Proveedores explícitos de pruebas siguen disponibles.
- `AsistenciaEditor`: mantiene **Ctrl+Espacio**, menú Editar y **clic derecho → Sugerencias de código**, lista, Enter/Tab/clic, Escape y consulta tras pausa. Las respuestas se descartan al editar, mover cursor/selección o cambiar/cerrar pestaña. Cambiar ccom invalida la consulta/lista anterior.
- La vista gris por pausa está **activada por defecto**. La lista por pausa continúa optativa en **Editar → Preferencias de asistencia**; Ctrl+Espacio siempre conserva la consulta explícita. Mostrar una propuesta no la acepta.
- Aplicar usa el contexto completo capturado y una operación de Deshacer, sin guardar. `.asm` no consulta asistencia de Calser.

### Pruebas

`make check-suggestions-json`, incluido en `make check`, migra los ejemplos de la entrega y cubre Bison/LAC, palabras parciales, medio de palabra, tipos válidos, EOF de programa completo, comentarios, errores anteriores/posteriores, Unicode/CRLF, determinismo, límites y opciones. Se consultan además múltiples posiciones de una fuente real.

`ide/pruebas/test_sugerencias_ccom.py` verifica el motor real y el contrato, rechazo de JSON incompatible, ausencia de consultas al lexer/LL(1) Python, cancelación/límites, Ctrl+Espacio, pausa, aceptación/Deshacer, configuración de ccom y cancelación del hijo por edición. `make check-ide` comprueba también las interfaces y descartes ya existentes.


El menú contextual conserva copiar/cortar/pegar y consulta en el punto pulsado (teclado: cursor actual). La consulta se inicia tras cerrar el menú para que la devolución de foco no la invalide. Respeta asistencia desactivada, solo lectura y entradas .asm. La interfaz también presenta por defecto continuaciones en gris al final de la línea; Tab acepta y Esc descarta. Ctrl+Espacio conserva la lista para otros reemplazos. La vista previa no inserta texto ni genera propuestas propias.

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

Cada análisis reinicia diagnósticos, posiciones, estados Flex, observadores y pila Bison. Los buffers y strings del escaneo se liberan. Los destructores liberan subárboles/cadenas temporales; el símbolo inicial distingue consulta de transferencia del AST a main. La compilación clásica libera los árboles parciales sin ejecutar fases posteriores. Las copias candidatas se liberan tras cada intento y no se reutilizan pilas corruptas.

Límites por análisis: **200000 bytes**, **50000 tokens/tramos**, cuatro candidatos. Si cualquier candidato supera límites o falla internamente, se devuelve límite/error sin propuestas parciales. El proceso del IDE tiene 2,5 s y 1 MB por canal, con cancelación que mata/recoge el hijo. El coordinador conserva su plazo general de 3 s, también durante las verificaciones automáticas posteriores.

### Integración CALSIDE

`ide/reparaciones_ccom.py` valida sobre, estados, reglas, inserción, razones y progreso. Convierte bytes UTF-8 a índices Python y comprueba que cada precondición no vacía coincide con la instantánea original. No contiene lexer/parser del lenguaje ni acepta mensajes narrativos como precondiciones.

`ide/interfaz/asistencia.py` carga el proveedor C por defecto y captura el ccom configurado para la tarea. **Alt+Enter**, **clic derecho → Reparaciones de código** o el botón del panel consultan el motor real. Consultar/seleccionar no aplica: se revisa Cambios/Motivo y se pulsa Aplicar reparación. Ctrl+Z deshace una propuesta; no se guarda, compila ni ejecuta automáticamente. Editar/mover cursor/cambiar pestaña invalida la aplicación. `.asm` no usa estas reparaciones.

#### Automatización opcional

El catálogo de `politica_asistencia.py` incluye una sola regla revisada: **INSERCION_PUNTO_COMA**. Incorporarla no la habilita. Por defecto `reparaciones_automaticas=False` y la selección de reglas está vacía. El usuario debe habilitar la opción en **Editar → Preferencias de asistencia**; el diálogo selecciona la primera regla revisada si no había ninguna seleccionada.

C marca candidata automática únicamente una propuesta única de `;`, con aceptación sintáctica completa, sin errores léxicos y nunca en EOF. También puede aparecer antes de una asignación que comienza con un identificador, sin exigir una reservada. La interfaz exige además confirmar con Enter una línea no vacía, una inserción local al final de esa línea, precondición real en la línea, sin selección/conflicto/solo lectura/compilación/pérdida de foco.

La bandera del motor no basta: la política vuelve a consultar **C mediante --syntax-json** sobre original y copia concreta; exige error sintáctico original y aceptación completa del resultado, sin errores léxicos. Python conserva solamente permisos, contexto, restricciones de interacción y aplicación. Antes de editar, el hilo gráfico vuelve a comprobar todo lo vigente. No guarda ni encadena otra reparación y Deshacer/Rehacer no dispara un nuevo ciclo.

Corchetes, paréntesis, puntos finales, EOF, candidatos parciales/ambiguos y líneas que no satisfacen estas condiciones permanecen manuales. Una aceptación sintáctica no prueba intención ni corrección semántica.

### Pruebas

- `make check-repairs-json`, incluido en `make check`: ejemplos de la entrega, cuatro cierres, EOF parcial, progreso/aceptación, operandos no inventados, Unicode/CRLF, comentarios, límites y opciones/salidas intactas.
- `ide/pruebas/test_reparaciones_ccom.py`: 12 pruebas nuevas del motor/transporte real y la interfaz; precondiciones inválidas, timeout, cancelación con eliminación del snapshot, panel/Deshacer, regla optativa y aplicación automática sin cadena.
- Política/interfaz previas: verificadas con el parser C, sin invocar LL(1) para validar lenguaje. La API Python LL(1) se conserva para consulta/pruebas; clones utiliza Flex y comparación C.

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

<a id="contrato-yyparse"></a>

### Contrato de yyparse(): AST parcial y fases posteriores

`yyparse()` puede devolver 0 tras recuperar errores; eso solo significa que llegó al símbolo inicial. **No significa que el programa sea válido.**

`parser_accept(FILE *input, ASTNode **tree)`, declarada en
`include/analisis/parser_service.h` e implementada al final de `grammar/parser.y`,
reúne la aceptación completa y llama a `parser_finish()`. Entrega el AST, que
puede ser parcial o NULL y que el llamador debe liberar siempre. No abre ni
cierra el flujo ni inicia diagnósticos: esas responsabilidades quedan en el
conductor y en cada auxiliar (Aho §4.8–4.9 y §5.3.1). `phase_parse` y los seis
auxiliares de pruebas la usan; conservan sus mensajes y códigos de salida.
Las consultas del IDE mantienen su modo independiente `lexer_parse_text`.


`parser_syntax_errors()` conserva el contador de errores de la consulta. `src/main.c:phase_parse` exige tanto éxito de Bison como **cero errores sintácticos y léxicos** antes de continuar. Si falla cualquiera:

- no se ejecuta el análisis semántico sobre el AST recuperado;
- no se exporta AST JSON ni se imprime con `-t`;
- no hay generación, organización de bundles ni ensamblado;
- no se crean/truncan artefactos de salida;
- el código de salida es 1.

Esta regla se aplica a **todo consumidor de `yyparse()`**, incluidos los programas
auxiliares de prueba: comprobar solo `yyparse() == 0` permite aceptar un AST
recuperado. La condición que centraliza `parser_accept` es `resultado == 0 &&
parser_syntax_errors() == 0 && lexical_errors == 0`; el contador sintáctico se
consulta mediante la función declarada en `include/analisis/parser_service.h`.

Por ejemplo, este programa tiene una expresión ausente antes del `;`:

```text
F main [] : void ;
    Vm x : int = ;
    E ;
.
```

El parser emite un diagnóstico sintáctico en la línea 2, se sincroniza en `;` y
termina con **`yyparse() == 0`, `parser_syntax_errors() == 1` y
`lexical_errors == 0`**. El consumidor debe rechazarlo aunque Bison haya
terminado normalmente: `ccom` devuelve 1 y no continúa con las fases posteriores.
En la integración C6 se corrigieron los seis auxiliares que aún comprobaban solo
el retorno: `pruebas/representacion/tac_analysis/desde_fuente.c`,
`pruebas/generador_o1/tac_allocate/desde_fuente.c`, `pruebas/generador_o1/tac_lower/desde_fuente.c`,
`pruebas/representacion/tac_build/probar.c`, `pruebas/representacion/tac_build/ejecutar.c` y
`pruebas/ensamblado/bundles/revisar_ir.c`.

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

La evidencia histórica de conteos y sanitizadores está en la [bitácora](../ide/docs/bitacora_ide.md). El [plan](../ide/docs/plan_ide.md) contiene pendientes actuales; esta referencia describe contratos implementados.
