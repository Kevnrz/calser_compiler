# Motor de sugerencias de ccom: M3

Implementado el 2026-11-11. Adapta el comportamiento de `origin/feat/reparaciones-sugerencias` a **Flex + Bison/LAC**, sin importar ni fusionar las dataclasses duplicadas de esa entrega. Reutiliza el lexer y las producciones de ccom. No llama al parser LL(1) Python, no ejecuta semántica, no repara errores ni genera artefactos.

## Uso

```sh
./ccom --suggest-json 14 programa.cls
./ccom --suggest-json 14 -v --time programa.cls
```

El cursor es un offset absoluto **en bytes UTF-8**, en una frontera entre caracteres. Para `F main [] : vo`, el cursor 14 ofrece `void` con rango `[12,14)`. En el IDE esta conversión se realiza desde el cursor Unicode del documento, que a su vez fue convertido desde Qt/UTF-16.

Solo admite una fuente y `-v`/`--time`. No se combina con `--check`, `--check-json`, `--tokens-json`, listados, salidas ni ensamblado. Un cursor no numérico/negativo es uso incorrecto (retorno 2); fuera de la fuente o en medio de UTF-8 produce estado error (retorno 1).

## Respuesta JSON v1

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

## Cómo calcula las propuestas

1. `suggestions_run`, en `src/suggestions.c`, lee la fuente acotada e instala un observador de diagnósticos silencioso.
2. Flex recorre la fuente completa y publica sus rangos mediante `LexerObserver` (`include/lexer_service.h`). Se detectan comentarios, fragmento alrededor del cursor y errores anteriores. Los errores del sufijo posterior no bloquean el prefijo consultado.
3. Para palabras/números, el fragmento se toma cuando `inicio < cursor <= fin`; para operadores extensibles `= ! < >`, cuando `inicio <= cursor < fin`. Los delimitadores y operadores ya completos se consideran parte del prefijo. Un carácter ASCII de reservada incompleta, como `V`, puede observarse como fragmento inválido y completarse a `Vi`/`Vm`; no se modifica la regla de Flex ni se tolera ese error al compilar.
4. Se analiza únicamente el prefijo anterior al fragmento con el parser existente. En modo consulta, `syntax_lex` sustituye el EOF temporal por un **TOKEN_CURSOR privado**, que no pertenece a ninguna producción. `yyreport_syntax_error` consulta `yypcontext_expected_tokens` con **LAC**, distingue ese marcador de un error real anterior y recoge los terminales concretos.
5. Así un programa completo también puede ofrecer continuaciones, sin confundir EOF con un error del usuario. No se derivan reglas a partir de mensajes humanos ni se copia una gramática a Python.
6. C excluye identificadores genéricos, números genéricos, EOF y símbolos internos. Filtra por el prefijo escrito y ordena: palabras, delimitadores, operadores; alfabético dentro de cada grupo.
7. Cada propuesta reemplaza el fragmento completo. En `vo|id` reemplaza `void`, evitando duplicar el sufijo. Se omiten reemplazos idénticos al fragmento ya terminado; tras una palabra completa, un espacio permite consultar las continuaciones siguientes.

Los comentarios de línea incluyen el punto antes de LF; los bloques abiertos incluyen EOF. Después de un bloque cerrado sí se puede consultar. No hay sugerencias dentro de comentarios.

Las propuestas son **continuaciones sintácticas**, no garantías semánticas. No se inventan nombres declarados ni literales numéricos. Consultar ámbitos, completar símbolos declarados o añadir plantillas queda como ampliación explícita posterior.

## Memoria, límites y cancelación

Cada solicitud usa un proceso independiente. Límite nativo: **200000 bytes**, **50000 tokens/tramos** y **64 opciones**. Se informa límite sin publicar una lista parcial como completa. El máximo de opciones del contrato sustituye al límite opcional de 24 de la entrega Python; no se añadió una opción CLI de truncamiento arbitrario.

Flex libera las cadenas del escaneo completo que normalmente consumiría Bison. El escaneo del prefijo reinicia posiciones/estado y sus buffers. Destructores condicionales del parser liberan cadenas y subárboles pendientes al finalizar deliberadamente en el marcador del cursor. La compilación normal conserva su propiedad habitual del AST; el marcador y estos destructores solo actúan en modo consulta. No se reutilizan pilas entre procesos.

`ide/servicio_ccom.py` comparte con M2 el transporte cancelable, sin shell. Captura stdout/stderr en temporales, los vigila y siempre mata/recoge el hijo si se cancela o falla. M3 limita su proceso a **2,5 s**, stdout a **1 MB** y stderr a **1 MB**; el coordinador mantiene su plazo general de 3 s. Las fuentes sin guardar nunca se escriben en la ruta del usuario: se consulta una copia privada eliminada al terminar.

## Conexión al IDE

- `ide/sugerencias_ccom.py`: firma `sugerir(texto, cursor, *, cancelar=None, root=None, configurado='')`, devuelve los tipos de `ide/nucleo/asistencia.py`. Valida versión, operación, fuente, cursor, código, estado, cantidades, rangos, fronteras UTF-8, textos y duplicados. Solo convierte posiciones; no clasifica ni filtra lenguaje.
- `ide/interfaz/asistencia.py`: carga ese adaptador por defecto y captura root/ccom configurado para cada tarea. Reparaciones conserva su ruta pendiente para M4. Proveedores explícitos de pruebas siguen disponibles.
- `AsistenciaEditor`: mantiene **Ctrl+Espacio**, menú Editar y **clic derecho → Sugerencias de código**, lista, Enter/Tab/clic, Escape y consulta tras pausa. Las respuestas se descartan al editar, mover cursor/selección o cambiar/cerrar pestaña. Cambiar ccom invalida la consulta/lista anterior.
- Pausa permanece **desactivada por defecto**: habilitar **Editar → Preferencias de asistencia → sugerencias por pausa**. Mostrar una lista no aplica su primera opción.
- Aplicar usa el contexto completo capturado y una operación de Deshacer, sin guardar. `.asm` no consulta asistencia de Calser.

## Pruebas y pendientes

`make check-suggestions-json`, incluido en `make check`, migra los ejemplos de la entrega y cubre Bison/LAC, palabras parciales, medio de palabra, tipos válidos, EOF de programa completo, comentarios, errores anteriores/posteriores, Unicode/CRLF, determinismo, límites y opciones. Se consultan además múltiples posiciones de una fuente real.

`ide/pruebas/test_sugerencias_ccom.py` verifica el motor real y el contrato, rechazo de JSON incompatible, ausencia de consultas al lexer/LL(1) Python, cancelación/límites, Ctrl+Espacio, pausa, aceptación/Deshacer, configuración de ccom y cancelación del hijo por edición. `make check-ide` comprueba también las interfaces y descartes ya existentes.

Verificación de memoria adicional: compilación temporal con AddressSanitizer/LeakSanitizer, 65 prefijos consultados sin errores ni fugas detectadas. El parser LL(1), el simulador y el tokenizador de clones siguen disponibles en sus recorridos actuales. **M4 sigue pendiente:** motor de reparaciones, validación y autorización de reglas automáticas.

El menú contextual conserva copiar/cortar/pegar y consulta en el punto pulsado (teclado: cursor actual). La consulta se inicia tras cerrar el menú para que la devolución de foco no la invalide. Respeta asistencia desactivada, solo lectura y entradas .asm. El texto de continuación en gris se mantiene como mejora de interfaz pendiente; M3 actual muestra una lista y no inserta automáticamente puntuación adicional.


## Revisión de integración M6

La cabecera JSON común se valida en `ide/ccom.py:informe_json`; los campos/estados específicos permanecen en cada adaptador. Los analizadores LL(1) Python se conservan para consulta/pruebas, fuera del backend gráfico. [Decisiones, memoria, comentarios y recorrido real](../ide/docs/revision_m6.md).
