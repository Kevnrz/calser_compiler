# Contexto y entrega: parser predictivo LL(1)

Responsable: Saddy. Implementación disponible en `ide/nucleo/parser.py`. Un mismo compañero continuará con los motores de sugerencias y reparaciones.

La documentación de uso vigente es [Parser predictivo: API y entrega a los motores](parser_predictivo.md). Contiene tipos, estados, límites, posiciones y ejemplos ejecutables. Este documento reemplaza la propuesta inicial del parser; los otros motores deben adaptarse a la API implementada.

## Decisiones de Saddy

- 1B: programas completos y contexto en el cursor; recuperación múltiple después.
- 2A: pila explícita y tabla LL(1), reutilizando el núcleo recibido.
- 3A: primer error, sin sincronización ni cambios en el código. 3B queda para después.
- 4A: resultado, diagnósticos y estado; sin árbol/AST. Traza optativa limitada.
- 5B: implementar primero el parser y fijar después su contrato real.

## Qué se implementó

```python
from ide.nucleo.parser import analizar_sintaxis, contexto_en

r = analizar_sintaxis("F main [] : void ; E ; .")
assert r.aceptado
texto = "F main [] : void ; Vm total : "
c = contexto_en(texto, len(texto))
assert c.valido
assert c.esperados == ("bool", "int")
```

Los resultados son dataclasses congeladas. La cima de la pila está a la derecha. Los rangos usan offsets Python desde 0, fin exclusivo; línea/columna desde 1. Los errores léxicos mantienen su modelo original. Consultar la guía de API antes de convertirlos o de aplicar cambios en Qt.

El contexto no detecta automáticamente la palabra parcial: el motor de sugerencias indica `inicio_fragmento` para excluirla del análisis y recibe su prefijo. No hay nombres insertables, variables declaradas ni propuestas de reparación en este módulo.

## Dependencias y límites de la entrega

El parser depende de `lexico.py`, `gramatica.py` y `tabla.py`, todos existentes. Ninguno de los dos motores es una dependencia del parser. Ambos motores sí pueden consultar esta API.

La interfaz de CALSIDE mantiene E3/E4, basada en lexer y ccom. E5 está parcial: parser terminado, motor y UI de sugerencias pendientes. E6 sigue pendiente. No se modificaron la gramática ni la semántica del compilador.

No se creó `modelos.py`: los modelos reales del parser viven en `parser.py`. Evitar duplicarlos. Los tipos de propuestas y ediciones corresponden a los motores del compañero.

## Verificación y siguiente paso

- `make check-parser`: 26 pruebas nuevas; no depende de Qt.
- `make check-ide`: 96 pruebas combinadas correctas.
- Comparación con Bison mediante `--ast-json`: 105 fuentes sin errores léxicos, sin diferencias de aceptación; cinco fuentes con errores léxicos excluidas.

El compañero puede usar directamente los ejemplos de [la API](parser_predictivo.md) para construir sugerencias y reanalizar candidatos de reparación. Después se integrarán los motores mediante adaptadores y widgets, con revisión del documento y Deshacer. No implementar el trabajo del compañero en el parser.
