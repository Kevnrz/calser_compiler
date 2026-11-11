# Reparaciones sintácticas C: M4

Implementado el 2026-11-11. Adapta el motor de `origin/feat/reparaciones-sugerencias` a Flex/Bison-LAC, conservando su alcance conservador: **insertar `;`, `.`, `]` o `)`** en el primer error, validar la copia y ofrecer solo candidatos que lo superan. No se fusionó la implementación Python ni sus dataclasses duplicadas.

No inventa identificadores, operandos ni sentencias como `E;`. No repara semántica. Un programa sintácticamente válido que carece de E puede seguir teniendo un error semántico de ccom; en ese caso este motor no propone insertar E.

## Invocaciones

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

## Motor y memoria

`syntax_service.c` escanea cada copia completa con Flex, observa los rangos de tokens reales y descarta candidaturas si hay errores léxicos. Después llama al parser real con EOF real. El hook `yyreport_syntax_error` consulta LAC y recoge posición del token inesperado, último token válido y los cuatro cierres esperados. No interpreta el texto humano de los diagnósticos.

`repairs.c` enumera únicamente esos cierres, construye una copia por candidato y vuelve a llamar `syntax_analyze`. La copia se acepta como propuesta si el parser acepta todo el programa o supera el token original que causaba el error. En EOF se exige consumir la inserción hasta el EOF posterior. Las propuestas parciales quedan **solo manuales**.

Orden: aceptación completa primero, mayor avance después y catálogo fijo `; . ] )` para desempatar. No se elige automáticamente la primera propuesta de una lista ambigua.

**Posición de inserción:** después del último token válido y antes de sus espacios/comentarios finales. La entrega original insertaba ante el token inesperado de la línea siguiente; aquí un `;` faltante en `Vm x : int` queda como `Vm x : int;`, sin moverlo delante de E ni introducirlo dentro de un comentario. Los rangos de comentarios proceden de Flex; Python no los reconoce de nuevo.

Las precondiciones usan el token válido anterior y su texto exacto; el IDE además exige contexto completo vigente (documento/revisión/texto/cursor/selección) al aplicar. Eso sustituye la huella redundante y las precondiciones narrativas de la entrega, sin convertirlas en rangos vacíos.

Cada análisis reinicia diagnósticos, posiciones, estados Flex, observadores y pila Bison. Los buffers y strings del escaneo se liberan. Los destructores condicionales del parser liberan subárboles/cadenas temporales tanto al aceptar la copia como al abortar; la compilación clásica conserva su propiedad habitual del AST. Las copias candidatas se liberan tras cada intento y no se reutilizan pilas corruptas.

Límites por análisis: **200000 bytes**, **50000 tokens/tramos**, cuatro candidatos. Si cualquier candidato supera límites o falla internamente, se devuelve límite/error sin propuestas parciales. El proceso del IDE tiene 2,5 s y 1 MB por canal, con cancelación que mata/recoge el hijo. El coordinador conserva su plazo general de 3 s, también durante las verificaciones automáticas posteriores.

## Integración CALSIDE

`ide/reparaciones_ccom.py` valida sobre, estados, reglas, inserción, razones y progreso. Convierte bytes UTF-8 a índices Python y comprueba que cada precondición no vacía coincide con la instantánea original. No contiene lexer/parser del lenguaje ni acepta mensajes narrativos como precondiciones.

`ide/interfaz/asistencia.py` carga el proveedor C por defecto y captura el ccom configurado para la tarea. **Alt+Enter**, **clic derecho → Reparaciones de código** o el botón del panel consultan el motor real. Consultar/seleccionar no aplica: se revisa Cambios/Motivo y se pulsa Aplicar reparación. Ctrl+Z deshace una propuesta; no se guarda, compila ni ejecuta automáticamente. Editar/mover cursor/cambiar pestaña invalida la aplicación. `.asm` no usa estas reparaciones.

### Automatización opcional

El catálogo de `politica_asistencia.py` incluye una sola regla revisada: **INSERCION_PUNTO_COMA**. Incorporarla no la habilita. Por defecto `reparaciones_automaticas=False` y la selección de reglas está vacía. El usuario debe habilitar la opción y marcar la regla en **Editar → Preferencias de asistencia**.

C marca candidata automática únicamente una propuesta única de `;`, con aceptación sintáctica completa y antes de una reservada de inicio de sentencia, nunca en EOF. La interfaz exige además confirmar con Enter una línea no vacía, una inserción local al final de esa línea, precondición real en la línea, sin selección/conflicto/solo lectura/compilación/pérdida de foco.

La bandera del motor no basta: la política vuelve a consultar **C mediante --syntax-json** sobre original y copia concreta; exige error sintáctico original y aceptación completa del resultado, sin errores léxicos. Python conserva solamente permisos, contexto, restricciones de interacción y aplicación. Antes de editar, el hilo gráfico vuelve a comprobar todo lo vigente. No guarda ni encadena otra reparación y Deshacer/Rehacer no dispara un nuevo ciclo.

Corchetes, paréntesis, puntos finales, EOF, candidatos parciales/ambiguos y líneas que no satisfacen estas condiciones permanecen manuales. Una aceptación sintáctica no prueba intención ni corrección semántica.

## Pruebas

- `make check-repairs-json`, incluido en `make check`: ejemplos de la entrega, cuatro cierres, EOF parcial, progreso/aceptación, operandos no inventados, Unicode/CRLF, comentarios, límites y opciones/salidas intactas.
- `ide/pruebas/test_reparaciones_ccom.py`: 12 pruebas nuevas del motor/transporte real y la interfaz; precondiciones inválidas, timeout, cancelación con eliminación del snapshot, panel/Deshacer, regla optativa y aplicación automática sin cadena.
- Política/interfaz previas: verificadas con el parser C, sin invocar LL(1) para validar lenguaje. La API Python LL(1) se conserva autorizada; clones sigue pendiente M5.
- Suite completa IDE: 371 pruebas correctas; suite del compilador correcta.
- ASan/LSan: 124 consultas de reparación/sintaxis, incluidas copias reanalizadas, sin errores ni fugas detectadas.


## Revisión de integración M6

La cabecera JSON común se valida en `ide/ccom.py:informe_json`; los campos/estados específicos permanecen en cada adaptador. Los analizadores LL(1) Python se conservan para consulta/pruebas, fuera del backend gráfico. [Decisiones, memoria, comentarios y recorrido real](../ide/docs/revision_m6.md).
