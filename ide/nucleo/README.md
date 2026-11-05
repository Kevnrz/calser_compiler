# Núcleo del IDE de Calser

Núcleo en Python para el IDE propio de Calser (decisiones D4 y D5).
Solo usa la biblioteca estándar de Python. Implementa el análisis léxico,
la gramática LL(1) y su tabla predictiva; el parser, las sugerencias y la
corrección están pendientes.

## Núcleo

| Módulo | Qué hace | Teoría |
|---|---|---|
| `lexico.py` | Texto → tokens y errores léxicos. Mismas reglas que `grammar/lexer.l` | Aho §3.5.3 |
| `gramatica.py` | Gramática LL(1) de Calser como datos | Aho §4.2, §4.3.3–4.3.4 |
| `tabla.py` | PRIMERO, SIGUIENTE y tabla predictiva M; detecta conflictos | Aho §4.4.2–4.4.3 (Alg. 4.31) |

Pendientes: `parser` (parser predictivo con pila, Aho §4.4.4), `sugerencias`
(fila M[tope, ·]) y `correccion` (recuperación a nivel de frase, Aho §4.4.5).

### Uso

Desde la raíz del repositorio, en Python:

```python
from ide.nucleo.lexico import analizar
from ide.nucleo.gramatica import CALSER
from ide.nucleo.tabla import construir_tabla

tokens, errores = analizar("Vm x : int = 3;")
print([(t.tipo, t.linea, t.columna) for t in tokens])
print(errores)

tabla = construir_tabla(CALSER)
print(tabla.es_ll1)
print({a: str(p) for a, p in tabla.fila("resto_c").items()})
```

Salida:

```text
[('Vm', 1, 1), ('id', 1, 4), (':', 1, 6), ('int', 1, 8), ('=', 1, 12), ('num', 1, 14), (';', 1, 15)]
[]
True
{'A': 'resto_c -> A tras_a', '.': 'resto_c -> ε'}
```

El tipo de cada token es el nombre del terminal en la gramática (`id`, `num`,
`Vm`, `[`, `==`, …), así que el parser compara el token directamente con M.

### Diferencia con `lexer.l`

Las columnas cuentan caracteres, no bytes. Un carácter no ASCII, como `á`
fuera de un comentario, da un solo error y ocupa una columna; Flex da un
error por byte. Fuera de eso, los tokens, posiciones y errores coinciden con
Flex en los 110 programas `.cls` del repo y en 4 archivos de casos borde
(comparación hecha con un volcador de tokens compilado desde `lexer.l`).

## Pruebas

```bash
make check-ide
```

Desde la raíz, también se puede usar
`python3 -m unittest discover -s ide/pruebas -v`. Las pruebas
comprueban las reglas de `lexer.l`, los ejemplos de Aho (Ej. 4.30, Fig. 4.17
y Ej. 4.33) y los conteos de `../docs/gramatica_ll1.md`.

## Documentos

- [Gramática original](../docs/gramatica_calser.md): gramática original,
  copia fiel de `grammar/parser.y`, verificada con `bison -v`.
- [Gramática LL(1)](../docs/gramatica_ll1.md): transformación a LL(1),
  PRIMERO, SIGUIENTE, verificación y ejemplos paso a paso.
- [Integración del núcleo](../docs/integrar_nucleo.md): API actual y conexión
  con CALSIDE.

Si cambia `grammar/parser.y` (por ejemplo, la declaración de módulo de D1),
hay que actualizar los dos documentos de gramática y `ide/nucleo/gramatica.py`. Después,
`make check-ide` dice si la gramática sigue siendo LL(1).
