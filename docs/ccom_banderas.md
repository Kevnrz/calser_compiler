# `ccom` — uso y banderas

```
ccom [opciones] <archivo> [-o <salida>]
```

Las opciones pueden ir antes o después del archivo. Sin opciones, `ccom`
compila en silencio: si no hay errores no imprime nada y sale con código 0.
Los errores siempre salen por stderr, con cualquier bandera.

## Banderas

| Bandera | Qué hace | Dónde sale | Estado |
|---|---|---|---|
| `--ast-json <archivo>` | Exporta el AST sintáctico a JSON para el visor gráfico | archivo JSON | ✅ Disponible |
| `-t` | Imprime el árbol sintáctico abstracto (AST) | stdout | ✅ Disponible |
| `-m` | Imprime la tabla de símbolos con las direcciones de memoria y la tabla de etiquetas | stdout | ✅ Disponible |
| `-v` | Muestra el avance de cada fase (`ok` o `con errores`) | stderr | ✅ Disponible |
| `-o <archivo>` | Nombre del binario de salida. Sin `-o`: el nombre del fuente con `.bin` (`prog.cls` → `prog.bin`) | — | Define el nombre base de las salidas; el binario aún no está integrado |
| `-s` | Escribe ensamblador (`.asm`) e imagen de datos (`.mem`) | archivos | ✅ Disponible |
| `-p` | Muestra los bundles en ensamblador y en binario | — | Avisa que todavía no está disponible |
| `-x` | Toma un `.asm` y genera el binario, sin compilar Calser | — | Error: todavía no está disponible |

Se pueden combinar: `ccom -t -m prog.cls` imprime el AST y después las tablas.

## Ejemplos

```bash
./ccom prog.cls              # compila; solo muestra errores
./ccom -t prog.cls           # árbol
./ccom -m prog.cls           # tabla de símbolos y de etiquetas
./ccom -v prog.cls           # avance por fase
./ccom -t -m -v prog.cls     # todo lo anterior
./ccom prog.cls -o prog.bin  # nombre del binario (aún no se genera)
```

Salida de `-v` en un programa correcto (stderr):

```
ccom: fase 1-2: analisis lexico y sintactico ... ok
ccom: fase 3: analisis semantico ... ok
ccom: fase 3: tabla de etiquetas ... ok
```

Con un error semántico, primero salen los errores y el resumen, y la fase que
falló termina en `... con errores`; las siguientes no corren:

```
ccom: fase 1-2: analisis lexico y sintactico ... ok
prog.cls:2:32: error semantico: el argumento 'k' de 'cifrar' debe estar entre 0 y 3
     2 |     cifrar [ 4096 , 8192 , 1 , 4 ] ;
       |                                ^
Analisis semantico fallido: 1 error(es).
ccom: fase 3: analisis semantico ... con errores
```

Con `-t` y errores semánticos, el AST se imprime igual, porque se construye
antes del análisis semántico. Con errores léxicos o sintácticos no hay AST.

## Códigos de salida

| Código | Significado |
|---|---|
| 0 | Compiló sin errores |
| 1 | Errores léxicos, sintácticos o semánticos, o `-x` (aún no disponible) |
| 2 | Error de uso: bandera desconocida, `-o` sin nombre, ningún archivo o más de uno |

Línea de uso que se imprime en los errores de uso:

```
uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] [--ast-json <json>] <archivo> [-o <salida>]
```

## Pruebas

`pruebas/correr_pruebas.sh` llama a `ccom -m`, así que `pruebas/resultados.txt`
guarda los errores, las tablas y el código de salida de cada programa.

## AST gráfico local

```sh
./ccom --ast-json build/p09_crypto.ast.json pruebas/p09_crypto.cls
```

Abrir `tools/visor_ast.html` en un navegador y seleccionar el JSON con
**Abrir AST JSON**. También incluye un ejemplo de p09 para verlo inmediatamente.
No requiere servidor, paquetes ni conexión. Ver [visor_ast.md](visor_ast.md).

`--ast-json` solo, o junto a `-t`/`-v`, termina después del análisis sintáctico:
no comprueba la semántica ni genera ensamblador. Su código 0 significa que el
AST se pudo construir y exportar, no que el programa completo sea válido.
Permite explorar construcciones cuyo generador todavía está pendiente.
Con `-s`, `-m` o `-p`, exporta el AST y continúa el recorrido normal; puede
conservar un JSON válido aunque las fases siguientes fallen. Si falla el
análisis léxico o sintáctico, no escribe un JSON nuevo. Un archivo de una
exportación anterior puede seguir existiendo; revisar siempre el código de salida.
