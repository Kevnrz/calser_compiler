# Clones léxicos nativos para CALSIDE · M5

Estado: implementado el 2026-11-11. `ccom --clones-json <mínimo> consulta.ccl`.
El argumento de entrada es un paquete de instantáneas CCL1, no un archivo Calser.
Solo admite `-v` y `--time`; las consultas y las opciones de generación son excluyentes.
No ejecuta parser, semántica, generación ni simulador. No guarda documentos ni genera artefactos.

## Responsabilidades y conexiones

1. `ide/interfaz/clones.py` reúne archivos `.cls` permitidos del proyecto y reemplaza su contenido por los buffers abiertos con nombre. Conserva las exclusiones, la comparación visual, el mínimo, la cancelación y la navegación vigilada. Los documentos nuevos con ruta pueden participar sin existir en disco; los documentos sin nombre quedan excluidos.
2. `ide/clones.py` mantiene los modelos `Fragmento`/`ParClon`, empaqueta instantáneas y valida el protocolo. Ya no tiene regex, reservadas, comparación, firmas ni normalización del lenguaje. El helper de inspección `tokenizar` consulta `--tokens-json`; el detector productivo no lo llama.
3. `ide/servicio_ccom.py` crea un directorio privado, escribe el paquete y lanza el ejecutable configurado, con el mismo transporte cancelable de los otros servicios. No utiliza shell. Siempre recoge el hijo y elimina el paquete, incluso al cancelar o fallar.
4. `src/clones.c` lee el paquete acotado, observa los reconocimientos de `grammar/lexer.l` mediante `lexer_scan_text` y compara en C. Las rutas de los documentos solo identifican fuentes: **nunca se abren desde C**. Cada texto analizado coincide con la copia enviada; no hay que guardar antes de analizar.
5. El adaptador convierte extremos UTF-8 a caracteres Python para el panel y el editor. No reconstruye tokens. La ventana descarta resultados si cambian buffers, carpeta, mínimo o ejecutable.

## Equivalencia

- **Tipo I:** misma clase y lexema, ignorando espacios y comentarios. `x = x + 1;` y `x=x+1;` coinciden.
- **Tipo II:** biyección de identificadores dentro del fragmento; variación de literales de la misma clase. `x=x+1;` y `contador=contador+2;` coinciden. `x=x+y;` y `a=b+b;` no coinciden completos.
- Los literales se normalizan por clase (enteros o booleanos), como en el detector anterior; no se exige una biyección entre valores literales. Las apariciones de identificadores sí deben ser consistentes en ambas direcciones.
- Tipos, operadores, delimitadores y reservadas conservan su lexema. Un cambio de `+` a `-` impide un clon que lo atraviese, pero puede haber un fragmento común más corto si supera el mínimo.
- Se comparan fragmentos contiguos entre documentos distintos, sin exigir programas completos. No se demuestra equivalencia semántica ni se detectan clones con sentencias insertadas/reordenadas.
- Se eliminan inclusiones dentro del mismo tipo. Una subparte exacta puede coexistir con un Tipo II más amplio. El orden es determinista: longitud descendente y después tipo, archivos e inicios.
- **Flex es la autoridad:** cadenas, identificadores inválidos, números fuera de rango y comentarios abiertos bloquean la consulta. El antiguo regex aceptaba formas que Calser no admite; no se conserva esa divergencia. No se comparan tokens recuperados como si no hubiese errores.

## Entrada CCL1, versión 1

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

## Respuesta JSON v1

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

## Índice y memoria del motor C

Los lexemas se internan en una tabla acotada; cada token guarda valor, clase, bytes, línea y ocurrencia previa del identificador en su documento. Las ventanas Tipo II usan distancias a ocurrencias previas internas a la ventana, por lo que los nombres concretos no afectan su firma. Las colisiones de hash siempre se verifican token a token con mapas directos/inversos antes de producir resultados.

Se publica el índice de un documento solo después de recorrer sus ventanas, evitando comparaciones dentro del mismo archivo. Cada candidato compatible se extiende a derecha/izquierda con la misma correspondencia. Se registran intervalos cubiertos por diagonal para no repetir extensiones. El filtro final conserva pares máximos por tipo.

El proceso atiende una sola consulta: un contexto estático prestado adapta el callback de Flex; no es una API concurrente dentro del mismo proceso. Reinicia y destruye el scanner para cada documento, libera sus cadenas de identificadores y desconecta observadores. Todos los buffers/tablas se liberan antes de devolver, incluso con error o límite. No construye AST.

## Límites y cancelación

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

## Verificación

- `make check-clones-json`, incluido en `make check`: equivalencias, negativos, Flex, comentarios, Unicode/CRLF, estados, paquete truncado/duplicado/excesivo, límites, exclusión de banderas y preservación de artefactos.
- `ide/pruebas/test_clones.py`: casos anteriores sobre C y recorrido real del panel, buffers sin guardar, comparación, navegación e invalidación.
- `ide/pruebas/test_clones_ccom.py`: contrato estricto, falta/incompatibilidad de ccom, límites y cancelación/plazo reales con recogida del hijo y limpieza del paquete; ausencia de llamadas al lexer/parser Python.
- Suite completa: 379 pruebas del IDE correctas y suite del compilador correcta; la última prueba de interfaz se verificó además de las 378 de la primera ejecución completa.
- 250 casos diferenciales con entradas Flex válidas producen los mismos pares/rangos que el detector previo. La suite nativa pasó ASan/UBSan/LSan. No se preservan las aceptaciones inválidas del antiguo regex.


## Revisión de integración M6

La cabecera JSON común se valida en `ide/ccom.py:informe_json`; los campos/estados específicos permanecen en cada adaptador. Los analizadores LL(1) Python se conservan para consulta/pruebas, fuera del backend gráfico. [Decisiones, memoria, comentarios y recorrido real](../ide/docs/revision_m6.md).
