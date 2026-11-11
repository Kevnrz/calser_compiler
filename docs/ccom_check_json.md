# Servicio de diagnósticos de ccom: `--check-json` (M1)

Implementado el 2026-11-11. La interfaz llama al front-end existente en C y recibe datos estructurados. No hay otra gramática, parser ni semántico para esta operación.

## Invocación y compatibilidad

```sh
./ccom --check-json programa.cls
./ccom --check-json -v --time programa.cls
```

Ejecuta exactamente las fases léxica, sintáctica y semántica de `--check`. No genera ni modifica `.bin`, `.hex`, `.mem`, `.asm` o AST JSON. Solo admite las opciones auxiliares `-v`/`--time`; combinarlo con `--check` es redundante y válido. Las opciones de salidas, ensamblado y listados se rechazan antes de abrir la entrada.

- **stdout:** un objeto JSON UTF-8, versión 1, seguido por salto de línea.
- **stderr:** resúmenes, progreso, tiempo y errores del proceso. Los diagnósticos individuales van en el JSON sin duplicar sus señaladores textuales.
- **Retorno 0:** análisis correcto, sin diagnósticos.
- **Retorno 1:** errores del programa, límite de informe o fallo del servicio; `status` distingue esos casos.
- **Retorno 2:** uso incorrecto o versión de ccom que no admite la opción; no se promete JSON en estos casos.

Una ayuda solicitada con `--help` mantiene su funcionamiento normal. La compilación F5 y las invocaciones clásicas conservan sus mensajes de texto; esta opción no cambia el lenguaje ni introduce recuperación sintáctica.

## Esquema v1

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

## Posiciones y Unicode

`range` puede ser `null` cuando no hay una ubicación representable en la fuente cargada, incluidos los errores generales como falta de `main`.

Cuando existe:

- `start` y `end` son offsets globales en bytes UTF-8, base 0, con rango `[start, end)`.
- `line` y `column` comienzan en 1; la columna también cuenta bytes. Una tabulación cuenta un byte.
- Los offsets conservan CR/LF originales; el servicio no calcula rangos sobre otra versión normalizada.
- Un punto al final de una línea puede ser un rango vacío; no se extiende sobre el token siguiente.
- El final no sobrepasa el contenido de esa línea. No se inventa una posición cuando el compilador no puede relacionarla con el fuente cargado.

El lexer actual puede reportar por separado cada byte de un carácter no ASCII inválido para Calser. Python conserva el rango nativo en `Problema.rango_utf8`, y amplía **solo la marca visual** a fronteras de caracteres UTF-8 completos para que Qt no seleccione medio carácter. Esta adaptación no cambia las reglas del lexer ni fusiona diagnósticos.

Las futuras ediciones de reparación requerirán fronteras válidas: este permiso de ampliar una marca visual no autoriza redondear cambios del documento.

## Implementación y ciclo de vida

1. `main.c` reconoce la opción y activa la captura antes de compilar.
2. `diag_init` carga las líneas y sus offsets reales. Lexer, parser y semántico siguen invocando `diag_error`/`diag_verror`.
3. Un observador recibe categoría, posición y argumentos de formato prestados. El serializador copia solo el mensaje necesario; no retiene punteros del AST o de las líneas.
4. Se ejecutan y liberan las mismas fases de análisis de la compilación normal.
5. `diag_json_finish` escribe la respuesta, libera los mensajes capturados y desconecta el observador. Un fallo de escritura cambia el retorno a 1 y puede dejar una respuesta incompleta que el cliente debe rechazar.

Archivos C: `include/diagnostics.h`, `src/diagnostics.c`, `include/diagnostics_json.h`, `src/diagnostics_json.c` y la integración de CLI en `src/main.c`.

El servicio es un proceso por solicitud. Su estado global no se comparte entre documentos ni se presenta como una biblioteca C reentrante.

## Límites y cancelación

La captura conserva hasta **128 diagnósticos**, **4096 bytes por mensaje** y **65536 bytes de mensajes acumulados**. Si se supera cualquiera, se informa `limit`/`truncated`; el compilador conserva su conteo real y puede seguir analizando. El tamaño del informe se mantiene acotado.

En el IDE se mantienen el timeout de 5 segundos y el límite de 1 MB por canal. Cancelar una instantánea detiene su QProcess, limpia temporales y evita publicar resultados antiguos. Si el proceso se interrumpe antes de emitir JSON completo, no se interpreta como éxito.

## Adaptación en CALSIDE

- `ide/ccom.py:diagnosticos_json` valida UTF-8, versión, operación, fuente, estado/retorno, contadores, lista, categorías y rangos. Rechaza claves duplicadas, constantes no JSON, Unicode inválido y datos inconsistentes.
- `Problema` conserva `codigo`, `categoria_explicita` y `rango_utf8`. Los lectores clásicos siguen funcionando con los campos previos.
- `ComprobadorCcom` solicita `--check-json` sobre el buffer temporal, reasocia cada problema al documento original y presenta un log legible. No llama al lector de mensajes humanos como alternativa silenciosa.
- Una versión antigua de ccom, una respuesta incompatible o `status=error/limit` produce «No disponible» con explicación. No se publica una lista vacía como si el programa fuera correcto.
- F5 conserva su invocación y lector textual en M1; F6 conserva la simulación. El lexer Python y el LL(1) siguen presentes en esta etapa. El filtro léxico previo de E3 todavía decide cuándo programar la consulta automática; el modo C puede consultarse directamente para errores léxicos.

**Múltiples errores:** la lista ya transporta varios errores léxicos o semánticos emitidos por el front-end. El parser Bison continúa deteniéndose en el primer fallo sintáctico. La recuperación de varios errores sintácticos es una ampliación posterior, no una capacidad añadida por JSON.

## Verificación

```sh
make
make check-analysis-json
make check
make check-ide
```

M1: **312 pruebas del IDE correctas**, incluidas **18 nuevas** de protocolo y UI. La suite completa del compilador también pasó. Se validaron además las respuestas de las **91 fuentes `.cls` de la carpeta principal de pruebas** con el adaptador del IDE, sin incompatibilidades.

Las pruebas específicas cubren fases reales, varios errores, errores generales, tildes/emoji, CRLF, rutas con espacios/comillas, límites, opciones inválidas, artefactos intactos, respuestas malformadas/versiones incompatibles y navegación. Las pruebas previas de procesos conservan cancelación, timeout, cierre, Guardar como, limpieza de temporales y prioridad de F5.

## Operación léxica complementaria (M2)

El resaltado del editor consulta [--tokens-json](ccom_tokens_json.md): utiliza el mismo lexer, sin parser, y añade comentarios/rangos para presentación. Su operación es `lex`; no sustituye ni altera el contrato `operation: check` de este documento.


## Revisión de integración M6

La cabecera JSON común se valida en `ide/ccom.py:informe_json`; los campos/estados específicos permanecen en cada adaptador. Los analizadores LL(1) Python se conservan para consulta/pruebas, fuera del backend gráfico. [Decisiones, memoria, comentarios y recorrido real](../ide/docs/revision_m6.md).
