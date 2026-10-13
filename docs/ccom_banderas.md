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
| `-t` | Imprime el árbol sintáctico abstracto (AST) | stdout | ✅ Disponible |
| `-m` | Imprime la tabla de símbolos con las direcciones de memoria y la tabla de etiquetas | stdout | ✅ Disponible |
| `-v` | Muestra el avance de cada fase (`ok` o `con errores`) | stderr | ✅ Disponible |
| `-o <archivo>` | Nombre del binario de salida. Sin `-o`: el nombre del fuente con `.bin` (`prog.cls` → `prog.bin`) | — | Se guarda; todavía no se escribe ningún archivo |
| `-s` | Genera además el archivo de ensamblador (`.asm`) | — | Avisa que todavía no está disponible |
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
uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] <archivo> [-o <salida>]
```

## Pruebas

`pruebas/correr_pruebas.sh` llama a `ccom -m`, así que `pruebas/resultados.txt`
guarda los errores, las tablas y el código de salida de cada programa.
