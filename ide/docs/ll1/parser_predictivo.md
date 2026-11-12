# Parser predictivo LL(1): API disponible y entrega a los motores

Estado M6 (2026-11-11): API Python de consulta/estudio y pruebas diferenciales, conservada por autorización explícita. Se implementó originalmente en `feat/parser-ll1` el 2026-11-05. **El editor ya utiliza C** para análisis, sugerencias, reparaciones y clones; no hay integración gráfica pendiente de este parser ni fallback al LL(1). [Frontera actual](../guia_sistema_calside.md), [contratos productivos](../asistencia.md).

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
