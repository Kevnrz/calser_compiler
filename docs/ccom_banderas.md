# `ccom` — uso y banderas

```
ccom [opciones] <archivo> [-o <salida>]
```

Las opciones pueden ir antes o después del archivo. Sin opciones, `ccom`
compila en silencio: si no hay errores no imprime nada, escribe los archivos
de salida y sale con código 0. Los errores siempre salen por stderr, con
cualquier bandera.

## Banderas

| Bandera | Qué hace | Dónde sale | Estado |
|---|---|---|---|
| `--ast-json <archivo>` | Exporta el AST sintáctico a JSON para el visor gráfico | archivo JSON | ✅ Disponible |
| `-t` | Imprime el árbol sintáctico abstracto (AST) | stdout | ✅ Disponible |
| `-m` | Imprime la tabla de símbolos con las direcciones de memoria y la tabla de etiquetas | stdout | ✅ Disponible |
| `-v` | Muestra el avance de cada fase (`ok` o `con errores`), incluido el ensamblado | stderr | ✅ Disponible |
| `-o <archivo>` | Nombre del binario de salida. Sin `-o`: el nombre del fuente con `.bin` (`prog.cls` → `prog.bin`). Es también el nombre base de las demás salidas | archivos | ✅ Disponible |
| `-s` | Escribe además el ensamblador (`.asm`) | archivo | ✅ Disponible |
| `-p` | Muestra cada bundle en ensamblador y en binario | stdout | ✅ Disponible |
| `-x` | Toma un `.asm` y genera el binario, sin compilar Calser | archivos | ✅ Disponible |

Se pueden combinar: `ccom -t -m prog.cls` imprime el AST y después las tablas.

## Ejemplos

```bash
./ccom prog.cls              # compila; solo muestra errores
./ccom -t prog.cls           # árbol
./ccom -m prog.cls           # tabla de símbolos y de etiquetas
./ccom -v prog.cls           # avance por fase
./ccom -t -m -v prog.cls     # todo lo anterior
./ccom prog.cls -o prog.bin  # nombre del binario y base de .hex y .mem
./ccom -s prog.cls           # además escribe prog.asm
./ccom -p prog.cls           # bundles en ensamblador y en binario
./ccom -x prog.asm           # ensambla un .asm, sin compilar Calser
```

## Archivos de salida

Cuando no hay errores, `ccom` escribe siempre tres archivos con el nombre base
de `-o` (o el del fuente): `prog.cls` produce `prog.bin`, `prog.hex` y
`prog.mem`. Con cualquier error no se escribe ninguno de los tres.

| Archivo | Contenido |
|---|---|
| `.bin` | Encabezado de 4 palabras de 32 bits *little endian*: cantidad de bundles, punto de entrada (0), tamaño de datos en bytes y dirección de datos (0). Luego cada bundle en 16 bytes (slot 0 primero, cada palabra *little endian*) y al final los datos |
| `.hex` | Un bundle por línea: `slot3_slot2_slot1_slot0`, cada slot en 8 dígitos hexadecimales en mayúscula |
| `.mem` | `@0` y luego una palabra de datos por línea, en hexadecimal |
| `.asm` | Solo con `-s`: el ensamblador generado, en la sintaxis de `isa.md` §11 |

**`-x archivo.asm`** ensambla el archivo sin compilar Calser y escribe los
mismos tres archivos. No hay imagen de datos: el tamaño de datos es 0 y el
`.mem` queda solo con `@0`. Con `-x`, la bandera `-s` se ignora (el `.asm` ya
es la entrada) y sale `ccom: aviso: -s se ignora con -x`; `-t` y `-m` no
tienen efecto.

**`-p`** imprime una línea por bundle: su índice en 4 dígitos, el bundle en
ensamblador y el bundle como en el `.hex`:

```
0000  { mov r4, 0 | mov r5, 3 | nopi | nopi }  00000000_00000000_49400003_49000000
0001  { nopi | nopi | nopi | nopi }  00000000_00000000_00000000_00000000
0002  { saddi r4, r4, 1 | nopi | blt r4, r5, lazo | nopi }  00000000_0D500000_00000000_41100001
```

Los errores de ensamblado (los de `isa.md` §11.4) salen por stderr con la
línea del `.asm`, y `ccom` termina con código 1:

```
ccom: prog.asm:3: error de ensamblado: etiqueta no definida: 'lazo'
```

Para el código que genera el compilador, el nombre es `<generado>` y la línea
es la del `.asm` que escribe `-s`.

Salida de `-v` en un programa correcto (stderr):

```
ccom: fase 1-2: analisis lexico y sintactico ... ok
ccom: fase 3: analisis semantico ... ok
ccom: fase 3: tabla de etiquetas ... ok
ccom: fase 4: generacion de codigo ... ok
ccom: ensamblado ... ok
```

Con `-x` solo sale la línea del ensamblado.

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
| 1 | Errores léxicos, sintácticos, semánticos, de generación o de ensamblado, o un archivo de salida que no se pudo escribir |
| 2 | Error de uso: bandera desconocida, `-o` sin nombre, ningún archivo o más de uno |

Línea de uso que se imprime en los errores de uso:

```
uso: ccom [-v] [-t] [-m] [-s] [-p] [-x] [--ast-json <json>] <archivo> [-o <salida>]
```

## Pruebas

`pruebas/correr_pruebas.sh` llama a `ccom -m`, así que `pruebas/resultados.txt`
guarda los errores, las tablas y el código de salida de cada programa. Pasa
`-o` a una carpeta temporal, para no dejar archivos de salida en `pruebas/`.

`make check-asm` corre `pruebas/ensamblador/probar.py`: los vectores de
`isa.md` §6.5, el ejemplo de §11.1, el formato del `.bin`, los errores de
§11.4 y el ensamblado del `.asm` que genera el compilador.

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
