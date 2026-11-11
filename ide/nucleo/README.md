# Núcleo Python de CALSIDE: contratos y LL(1) de consulta

Estado M6 (2026-11-11). Este paquete contiene dos grupos distintos, sin Qt:

| Grupo | Módulos | Uso actual |
| --- | --- | --- |
| Productivo | `asistencia.py` | Modelos compartidos entre adaptadores C, coordinador y aplicador. Valida datos, no lenguaje. |
| Consulta/estudio | `lexico.py`, `gramatica.py`, `tabla.py`, `parser.py` | API LL(1), ejemplos de teoría y pruebas diferenciales. La interfaz no los carga para analizar documentos. |

Saddy autorizó conservar el LL(1) en Python. Se mantiene como material de estudio y API explícita; **no hay respaldo a ese parser si ccom falta**. El análisis visible, colores, sugerencias, reparaciones y clones usan Flex/Bison/semántico o comparación en C, según la operación. El simulador Python es otra herramienta externa autorizada.

## Contratos productivos

`ContextoDocumento`, `EdicionTexto`, `PrecondicionTexto`, `Sugerencia`, `Reparacion`, resultados y estados viven en `asistencia.py`. Sus rangos son caracteres Python `[inicio,fin)`. No importa `lexico`, `gramatica`, `tabla` ni `parser`.

[Contrato de asistencia](../docs/asistencia.md), [arquitectura y revisión M6](../docs/guia_sistema_calside.md).

## API LL(1) de consulta

```python
from ide.nucleo.lexico import analizar
from ide.nucleo.gramatica import CALSER
from ide.nucleo.tabla import construir_tabla
from ide.nucleo.parser import analizar_sintaxis, contexto_en

tokens, errores = analizar("Vm x : int = 3;")
assert not errores
assert construir_tabla(CALSER).es_ll1
assert analizar_sintaxis("F main [] : void ; E ; .").aceptado
texto = "F main [] : void ; Vm total : "
assert contexto_en(texto, len(texto)).esperados == ("bool", "int")
```

La gramática LL(1) es una transformación para estudio de la Bison, no una autoridad productiva independiente. FIRST/FOLLOW/tabla se calculan por punto fijo. El parser usa pila explícita, devuelve el primer error o contexto del prefijo y no genera AST ni comprueba semántica. El lexer de consulta cuenta caracteres Python; Flex produce errores/rangos por byte UTF-8. No intercambiar ambas convenciones ni presentar sus resultados como el backend gráfico.

## Pruebas

`test_lexico.py`, `test_tabla.py` y `test_parser.py` conservan pruebas académicas y comparaciones diferenciales. No validan por sí solas una función gráfica. Las suites `test_*_ccom.py` y `test_arquitectura_ccom.py` verifican los recorridos reales C.

```sh
make check-parser
make check-ide
make check-recorrido-ide
```

[Gramática original](../docs/ll1/gramatica_calser.md), [transformación LL(1)](../docs/ll1/gramatica_ll1.md), [API detallada](../docs/ll1/parser_predictivo.md). Si cambia Bison, revisar las comparaciones y actualizar la documentación de estudio cuando corresponda; la aplicación no depende de reconstruir esa tabla Python.
