# Motor de sugerencias (E5) — primera versión

## API

```python
from ide.nucleo.sugerencias import proponer_sugerencias
r = proponer_sugerencias('F main [] : vo', len('F main [] : vo'))
for s in r.sugerencias:
    print(s.etiqueta, s.inicio, s.fin, s.texto)
# void 12 14 void
```

`proponer_sugerencias(texto, cursor, *, max_resultados=24, cancelar=None)` devuelve `ResultadoSugerencias` con `estado`, `cursor`, `inicio_fragmento`, `fin_fragmento`, `esperados`, `sugerencias`, `contexto`, `limite_alcanzado`, `motivo`.

Cada `Sugerencia` contiene `codigo`, `etiqueta`, `texto`, `inicio`, `fin`, `categoria`, `explicacion`, `automatica_segura=False`.

Aplicación manual en copia: `nuevo = texto[:s.inicio] + s.texto + texto[s.fin:]`.

## Contrato y límites

- Usa `contexto_en` del parser existente. No implementa otro parser ni gramática.
- Offsets de Python `[inicio, fin)` sobre el texto original. El adaptador Qt convierte a UTF-16.
- El IDE debe comprobar identidad de documento, revisión y texto antes de aplicar resultados.
- No usa ccom, Qt, disco ni widgets. No modifica texto ni ejecuta reparaciones.
- Los terminales `id` y `num` se muestran en `esperados`, pero no se convierten en nombres o valores ficticios.
- Las sugerencias son posibilidades sintácticas para el **prefijo**, no una garantía de aceptación del documento completo ni de corrección semántica.
- Prefijos con errores, comentarios abiertos, cancelación o límites no devuelven sugerencias. `limite_alcanzado` indica truncamiento o límite del parser.
- Si el cursor está al final de un identificador, se trata como fragmento de completado; para obtener sugerencias de continuación tras un identificador completo, introducir un espacio o separador. No hay recuperación de errores en el motor.
- No hay completado de nombres del ámbito, firmas, fragmentos estructurales ni ranking semántico.
- No se autoaplican sugerencias; `automatica_segura` es siempre falso.
- La lista de terminales concretos se deriva de la gramática del parser, filtrando a palabras reservadas, operadores y delimitadores conocidos por el lexer. Cualquier ampliación del lenguaje debe coordinarse con el lexer y la gramática.

## Pruebas

`python -m unittest ide.pruebas.test_sugerencias -v`

Para integración completa, ejecutar `make check-ide` en la raíz real del repositorio.
