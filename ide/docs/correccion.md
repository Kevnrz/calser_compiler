# Motor de reparaciones sintácticas

Módulo: `ide.nucleo.correccion`  
Estado: primera versión experimental, enfocada en el primer error fiable.

## API pública

```python
from ide.nucleo.correccion import proponer_reparaciones, aplicar_ediciones

resultado = proponer_reparaciones(texto)
for reparacion in resultado.propuestas:
    if reparacion.automatica_segura:
        copia = aplicar_ediciones(texto, reparacion.ediciones)
```

`proponer_reparaciones(texto: str, *, max_candidatos: int = 4)` no modifica el
texto ni accede a archivos, Qt, ccom o widgets. Usa `analizar_sintaxis` del
parser existente; no define lexer ni gramática propios.

### Modelos

- `EdicionTexto(inicio, fin, texto)`: rango `[inicio, fin)` sobre el texto
  original. Una inserción tiene `inicio == fin`.
- `ValidacionReparacion`: estado y diagnóstico del reanálisis del candidato,
  si el error original se superó y si el programa completo fue aceptado.
- `Reparacion`: código estable, descripción, explicación, diagnóstico objetivo,
  ediciones, clasificación automática, razones, precondiciones, validación,
  SHA-256 y longitud del texto original.
- `ResultadoReparaciones`: estado general, propuestas, primer diagnóstico,
  límites, motivo, SHA-256 y longitud del texto original.

Estados de `ResultadoReparaciones`:

| Estado | Significado |
|---|---|
| `sin_error` | El parser aceptó el programa. |
| `propuestas` | Hay una o más propuestas que superaron el punto del error original. |
| `sin_propuesta` | No se encontró una inserción admitida que supere el error. |
| `bloqueado` | Error léxico, cancelación, límite o estado no reparable. |

`huella_texto` y `longitud_texto` permiten al adaptador comprobar que la
propuesta corresponde al mismo contenido. El IDE todavía debe asociar su
propio identificador de documento y revisión; el motor no conoce esos datos.

## Catálogo inicial

| Código | Disparador | Edición | Clasificación |
|---|---|---|---|
| `INSERCION_PUNTO_COMA` | `;` está entre los terminales esperados | Inserta `;` antes del símbolo encontrado o en EOF | Automática solo bajo las condiciones estrictas descritas abajo; en los demás casos manual |
| `INSERCION_PUNTO_FINAL` | `.` está entre los esperados | Inserta `.` | Siempre manual |
| `INSERCION_CORCHETE_CIERRE` | `]` está entre los esperados | Inserta `]` | Siempre manual |
| `INSERCION_PARENTESIS_CIERRE` | `)` está entre los esperados | Inserta `)` | Siempre manual |

No se insertan aperturas `[`/`(`, no se borran tokens y no se inventan
identificadores, operandos, valores ni declaraciones. Una propuesta solo se
conserva si el reanálisis consume la inserción y supera el símbolo del error
original. El reanálisis puede encontrar otro error posterior; por eso
`error_resuelto` no equivale a `aceptado`.

### Ejemplos

Punto y coma omitido antes de una sentencia inequívoca:

```text
F main [] : void ;
Vm x : int
E ;
.
```

Propuesta: insertar `;` antes de `E`. El candidato completo es aceptado y la
propuesta puede clasificarse como automática según la política inicial.

Expresión incompleta:

```text
F main [] : void ;
x = 3 + ;
E ;
.
```

No se propone insertar un terminador: el parser espera un operando y el motor
no lo inventa.

Función sin punto final:

```text
F main [] : void ;
E ;
```

Se propone insertar `.` en EOF, pero es manual porque EOF puede ser simplemente
un prefijo que el usuario todavía está escribiendo.

## Política de automatización

La primera versión solo permite automatizar una inserción de `;` cuando:

1. el error no está en EOF;
2. el símbolo siguiente es un inicio de sentencia reservado/no ambiguo
   (`F`, `C`, `A`, `L`, `S`, `E`, `R`, `Vm`, `Vi` o una palabra reservada de
   criptografía), nunca un `id` que pudiera continuar una expresión;
3. existe exactamente una propuesta validada;
4. el candidato completo es aceptado por el parser;
5. no hay errores léxicos.

Incluso en ese caso, `automatica_segura` significa que cumple esta política
sintáctica conservadora, no que se conozca la intención del usuario. El IDE
debe decidir el disparador (por ejemplo, después de una pausa), verificar
texto/revisión y permitir desactivar la automatización. Ninguna reparación
prueba corrección semántica.

## Aplicación de ediciones

`aplicar_ediciones` valida rangos y solapamientos y aplica desde la derecha
hacia la izquierda. Dos inserciones en el mismo offset se rechazan: si una
futura regla necesita varias, el generador debe fusionarlas explícitamente en
una sola edición. Las ediciones actuales son una sola inserción y conservan
espacios, comentarios, CRLF, tabulaciones y Unicode fuera del punto editado.

## Bloqueos y límites

- Cualquier error léxico en el texto bloquea propuestas, incluso si parece
  posterior al primer error sintáctico. Esto evita reparar sobre una
  tokenización incompleta o recuperada.
- `limite` y `cancelado` del parser no se interpretan como ausencia de errores.
- `max_candidatos` es un entero positivo; el catálogo tiene como máximo cuatro
  terminales candidatos. Si se trunca la lista se informa `max_candidatos`.
- Cada candidato se analiza una vez sobre una copia completa; no hay reparación
  recursiva ni recuperación por sincronización.
- Solo se aborda el primer diagnóstico sintáctico. No se combinan propuestas
  de errores diferentes.
- El motor no decide si el usuario terminó de escribir.

## Pruebas

```bash
PYTHONPATH=. pytest -q ide/pruebas/test_correccion.py
```

Las pruebas cubren programa correcto, inserciones, expresiones incompletas,
EOF, comentarios, Unicode, CRLF, rangos, solapamientos, idempotencia,
clasificación automática y bloqueo por errores léxicos.

La integración completa debe ejecutar `make check-ide` en el repositorio raíz,
con PySide6, corpus y Makefile disponibles. El ZIP de trabajo recibido solo
contiene el subdirectorio `ide/`, por lo que no incluye el Makefile raíz ni el
corpus externo necesario para esa comprobación.
