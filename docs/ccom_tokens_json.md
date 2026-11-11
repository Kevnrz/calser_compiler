# Servicio léxico Flex: `--tokens-json` (M2)

Implementado el 2026-11-11. Reutiliza `grammar/lexer.l`: no llama al parser ni al semántico y no genera artefactos. Puede resaltar documentos incompletos y tokens posteriores a un error sintáctico. El lenguaje y los modos clásicos de compilación mantienen sus reglas.

```sh
./ccom --tokens-json programa.cls
./ccom --tokens-json -v --time programa.cls
```

Solo se admite una entrada y opciones auxiliares `-v`/`--time`. Se rechazan listados/salidas y combinaciones con `--check`/`--check-json`. Retornos 0 (léxico correcto), 1 (errores, fallo o límite) y 2 (uso incompatible, sin prometer JSON). `--help` conserva su funcionamiento.

## Transporte v1

Comparte el sobre y el contrato de diagnósticos de [--check-json](ccom_check_json.md), cambiando `operation` a `lex` y añadiendo `fatal` y `tokens`:

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

## Implementación y límites

- `YY_USER_ACTION` conserva el contador absoluto de bytes; `LEX_RETURN` publica clasificación antes de devolver el token habitual. En compilación clásica el observador está inactivo.
- `lexer_scan_only` reinicia el estado de Flex, recorre hasta EOF y libera inmediatamente las cadenas de identificadores que de otro modo consumiría Bison.
- `diagnostics_json.c` captura los mismos errores y añade los tramos. Máximo 100000 tramos, 128 diagnósticos, 4096 bytes por mensaje y 65536 bytes de mensajes. Un exceso produce `status: limit`, retorno 1 y explicación; no equivale a un documento válido.
- La entrada de esta operación se limita a 8 millones de bytes. Un archivo ausente/demasiado grande devuelve `status: error`, logs en stderr y retorno 1.
- stdout contiene únicamente el objeto JSON; logs, progreso y duración van en stderr.

## Integración CALSIDE

`Editor` espera 300 ms después de escribir; `CoordinadorAnalisis` conserva una tarea activa y la última solicitud por documento. `TareaLexica` llama al adaptador en un QThread. El adaptador usa `subprocess.Popen` sin shell y un temporal privado UTF-8; los buffers del usuario no se guardan.

El servicio admite 2 millones de caracteres por instantánea en el IDE, 5 s para el proceso, 16 MB de stdout y 1 MB de stderr. Las salidas se recogen en archivos temporales y se vigila su tamaño. Cancelación, edición y cierre solicitan interrupción; se mata y recoge el hijo y se eliminan temporales. No se actualizan widgets desde el trabajador.

`ccom.lexico_json` valida versión, operación, código, fuente, listas, categorías y fronteras de UTF-8 antes de aceptar el informe. `convertir_lexico` transforma los rangos a UTF-16 por línea para Qt; solo divide comentarios multilínea para pintarlos. Los diagnósticos conservan offsets nativos y origen **ccom léxico**. Un error por byte de un emoji conserva su identidad; la selección visual se amplía al carácter completo.

Al recibir, el editor comprueba documento/revisión. Los colores no cambian texto, guardado ni Deshacer. `Compilar → Configurar ccom` vuelve a solicitar léxico con el ejecutable elegido. Sin una versión compatible se informa el fallo: no hay fallback al lexer Python.

El análisis sintáctico/semántico automático sigue usando `--check-json` después de otra pausa de 600 ms y solo si el léxico está libre de errores. F5/F6 mantienen los perfiles M1B. El LL(1), el tokenizador de clones y la validación de reparaciones Python se conservan en sus recorridos pendientes; M2 retira su uso del resaltado y diagnóstico léxico vivo.

## Verificación

`make check-tokens-json`, incluido en `make check`: clasificación, comentarios, Unicode/CRLF, error léxico, EOF fatal, continuación sin parser, límites, opciones y salidas intactas. `make check-ide`: transporte inválido, posiciones Qt, cancelación real, proceso antiguo, edición/pestañas y Deshacer. Pruebas nuevas en `ide/pruebas/test_tokens_ccom.py`; la suite de comprobación usa Flex real al simular fallos únicamente de `--check-json`.

M3 comparte este transporte en `ide/servicio_ccom.py` y añade [sugerencias desde Flex/Bison-LAC](ccom_suggestions_json.md). Los límites léxicos y el contrato de tokens M2 se conservan.


## Revisión de integración M6

La cabecera JSON común se valida en `ide/ccom.py:informe_json`; los campos/estados específicos permanecen en cada adaptador. Los analizadores LL(1) Python se conservan para consulta/pruebas, fuera del backend gráfico. [Decisiones, memoria, comentarios y recorrido real](../ide/docs/revision_m6.md).
