# Integrar el núcleo en CALSIDE

Saddy: en `develop` están `ide/nucleo/` (`lexico.py`, `gramatica.py` y
`tabla.py`), las pruebas `ide/pruebas/test_lexico.py` y `test_tabla.py`, y
`ide/docs/` con la gramática y esta guía. El núcleo usa Python estándar,
sin dependencias. PySide6 está aceptado para la interfaz como excepción a D5.

Para ajustarlo a tu estructura, `ide/__init__.py` y `makefile` quedaron
idénticos a los tuyos; las pruebas importan desde `ide.nucleo` y se ejecutan
con `make check-ide`. Se quitó `ide/pruebas/__init__.py`. El README del núcleo
se movió de `ide/README.md` a `ide/nucleo/README.md` para no chocar con tu
`ide/README.md` de la interfaz.

## Traerlo a tu rama

Desde la raíz, con los cambios de `develop` ya incorporados allí:

```bash
git checkout feat/calside
git merge develop
```

Se espera un merge sin conflictos, según el merge de prueba previo informado
por Luis. El `README.md` de la raíz se mezcla solo; tu `ide/README.md`,
`makefile` e `ide/__init__.py` quedan como los tenés. Esta adaptación no repite
el merge de prueba ni ejecuta git.

## API disponible

Los siguientes ejemplos se ejecutan en Python desde la raíz del repositorio.
`analizar(texto)` devuelve `(tokens, errores)`, dos listas.
Cada `Token` expone `tipo`, `lexema`, `linea`, `columna`, `columna_fin`
(última columna ocupada, inclusiva) y `valor` (nombre para `id`, entero para
`num`, `None` para los demás). Los tipos son nombres de terminales de la
gramática: `id`, `num`, `Vm`, `[`, `==`, etc.

```python
from ide.nucleo.lexico import analizar

tokens, errores = analizar("Vm x : int = 3;")
print([(t.tipo, t.lexema, t.linea, t.columna, t.columna_fin, t.valor)
       for t in tokens])
print(errores)
```

Salida:

```text
[('Vm', 'Vm', 1, 1, 2, None), ('id', 'x', 1, 4, 4, 'x'), (':', ':', 1, 6, 6, None), ('int', 'int', 1, 8, 10, None), ('=', '=', 1, 12, 12, None), ('num', '3', 1, 14, 14, 3), (';', ';', 1, 15, 15, None)]
[]
```

Cada `ErrorLexico` expone `mensaje`, `linea`, `columna`, `largo` y `fatal`.
Un carácter desconocido se descarta y permite continuar; un comentario de
bloque sin cerrar produce un error fatal y corta el análisis:

```python
from ide.nucleo.lexico import analizar

tokens, errores = analizar("x @ #[ sin cerrar")
print([t.tipo for t in tokens])
for e in errores:
    print((e.mensaje, e.linea, e.columna, e.largo, e.fatal))
```

Salida:

```text
['id']
("caracter no reconocido: '@'", 1, 3, 1, False)
('comentario abierto en la linea 1 nunca se cierra', 1, 5, 2, True)
```

`construir_tabla(CALSER)` devuelve una `TablaLL1`: `es_ll1` indica ausencia
de conflictos; `entrada(no_terminal, terminal)` devuelve una producción o
`None`; `fila(no_terminal)` devuelve un diccionario de terminal a producción;
`anulables()` devuelve los no terminales que pueden derivar la cadena vacía.

```python
from ide.nucleo.gramatica import CALSER
from ide.nucleo.tabla import construir_tabla

tabla = construir_tabla(CALSER)
print(tabla.es_ll1)
print(tabla.entrada("tras_a", "["))
print(tabla.entrada("tras_a", "id"))
print({a: str(p) for a, p in tabla.fila("resto_c").items()})
print(len(tabla.anulables()), "resto_c" in tabla.anulables())
```

Salida:

```text
True
tras_a -> [ expr ] ; cuerpo resto_c
None
{'A': 'resto_c -> A tras_a', '.': 'resto_c -> ε'}
21 True
```

## Conexión con tu plan y posiciones

En `ide/docs/plan_ide.md` de tu rama, F09 (resaltado y errores léxicos durante
la edición) ya puede llamar a `analizar(texto)`. Como proponés en el plan,
conviene un adaptador de tu lado, como `ide/nucleo.py` o similar. Usá un nombre
como `ide/adaptador_nucleo.py` para evitar la ambigüedad con el paquete
existente `ide/nucleo/`; allí podés traducir tokens y errores para Qt.

**Pendiente, entrega 2:** parser predictivo y sugerencias, con una API pensada
para que la interfaz la llame directamente. Habilitará F11 (sugerencias).
**Pendiente, entrega 3:** corrección a nivel de frase, para F12 (correcciones).
Estas funciones todavía no están implementadas.

El núcleo cuenta caracteres de Python: línea y columna desde 1, tabulación
igual a una columna. Qt cuenta unidades UTF-16 en sus posiciones de texto;
al pasar a un desplazamiento local desde 0, las cuentas coinciden para
caracteres del BMP. Con caracteres fuera del BMP, como emojis, hay que
convertir: el desplazamiento Python `i` en una línea equivale a
`len(linea[:i].encode("utf-16-le")) // 2` unidades UTF-16. Para un token,
`i = token.columna - 1`; su extremo exclusivo usa `token.columna_fin`.
Comentarios y espacios no generan tokens: el resaltado de comentarios debe
obtener sus rangos aparte. Un comentario sin cerrar da un `ErrorLexico` fatal
y conserva solo los tokens anteriores al comentario.

## Ajustes sugeridos en tu rama

Si te queda cómodo, revertí estos cuatro cambios ajenos al IDE:

- `pruebas/e62_crypto_alineacion.cls`: el cambio de `4098` a `8000`.
- `pruebas/e07_dimension_cero.cls`: las líneas en blanco agregadas al final.
- `ejercicios/ejercicio1_factorial/ejercicio1.asm`.
- `editors/vscode/package.py`.

Es un pedido sin obligación: si no lo hacés, se restauran mediante un PR
aparte después de que entre tu rama. También conviene dejar un solo módulo
de clones: hoy tu rama tiene `ide/clones.py` e `ide/interfaz/clones.py`.

## Verificación del núcleo

Desde la raíz, cualquiera de los dos comandos ejecuta las 20 pruebas:

```bash
make check-ide
python3 -m unittest discover -s ide/pruebas -v
```

Resultado esperado: `Ran 20 tests` y `OK`. En `develop`, `make ide` falla
hasta que entre tu rama; es esperado y no hace falta ejecutarlo aquí.
