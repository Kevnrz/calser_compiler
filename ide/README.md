# IDE de Calser

IDE propio para Calser, escrito en Python (decisiones D4 y D5). No es la
extensión de VS Code de `editors/vscode/`: esa solo resalta la sintaxis.

El IDE tiene dos partes:

- **Núcleo** (`nucleo/`): análisis léxico y sintáctico, sugerencias y
  corrección. Solo usa la biblioteca estándar de Python y no depende de la
  interfaz.
- **Interfaz**: todavía no existe y se define aparte. Solo llama funciones
  del núcleo.

## Núcleo

| Módulo | Qué hace | Teoría |
|---|---|---|
| `nucleo/lexico.py` | Texto → tokens y errores léxicos. Mismas reglas que `grammar/lexer.l` | Aho §3.5.3 |
| `nucleo/gramatica.py` | Gramática LL(1) de Calser como datos | Aho §4.2, §4.3.3–4.3.4 |
| `nucleo/tabla.py` | PRIMERO, SIGUIENTE y tabla predictiva M; detecta conflictos | Aho §4.4.2–4.4.3 (Alg. 4.31) |

Pendientes: `parser` (parser predictivo con pila, Aho §4.4.4), `sugerencias`
(fila M[tope, ·]) y `correccion` (recuperación a nivel de frase, Aho §4.4.5).

### Uso

Desde la carpeta `ide/`:

```python
from nucleo.lexico import analizar
from nucleo.gramatica import CALSER
from nucleo.tabla import construir_tabla

tokens, errores = analizar("Vm x : int = 3;")
# tokens: Vm(1,1) id(1,4) :(1,6) int(1,8) =(1,12) num(1,14) ;(1,15)

tabla = construir_tabla(CALSER)
tabla.es_ll1                       # True
{a: str(p) for a, p in tabla.fila("resto_c").items()}
# {'A': 'resto_c -> A tras_a', '.': 'resto_c -> ε'}
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

Equivale a `python3 -m unittest discover -s ide/pruebas -t ide`. Las pruebas
comprueban las reglas de `lexer.l`, los ejemplos de Aho (Ej. 4.30, Fig. 4.17
y Ej. 4.33) y los conteos de `docs/gramatica_ll1.md`.

## Documentos

- [docs/gramatica_calser.md](docs/gramatica_calser.md): gramática original,
  copia fiel de `grammar/parser.y`, verificada con `bison -v`.
- [docs/gramatica_ll1.md](docs/gramatica_ll1.md): transformación a LL(1),
  PRIMERO, SIGUIENTE, verificación y ejemplos paso a paso.

Si cambia `grammar/parser.y` (por ejemplo, la declaración de módulo de D1),
hay que actualizar los dos documentos y `nucleo/gramatica.py`. Después,
`make check-ide` dice si la gramática sigue siendo LL(1).
