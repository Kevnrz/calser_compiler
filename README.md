<p align="center">
  <img src="assets/branding/calser-logo.png" alt="Logo de Calser" width="240">
</p>

<h1 align="center">Calser Compiler</h1>

<p align="center">
  Compilador de Calser desarrollado en C con Flex y Bison.<br>
  Proyecto del curso de Compiladores · CE1108
</p>

## Estado del proyecto

El ejecutable `ccom` realiza análisis léxico, sintáctico y semántico. Construye
el árbol sintáctico abstracto (AST), la tabla de símbolos con direcciones de
memoria y la tabla de etiquetas. Los errores muestran archivo, línea y columna.

`ccom -s` genera ensamblador y una imagen inicial de datos `.mem`. La
integración CRYPTO se valida compilando y simulando `p09_crypto.cls`. También
hay rutinas aritméticas y de acceso a arreglos con pruebas independientes.
El generador general todavía está en desarrollo; el ensamblado a binario
no está integrado.

Para continuar el generador, consultar el [contexto para Kevin](docs/contexto_kevin_fase4.md).

El AST también se puede explorar como un árbol gráfico local; consultar
[el visor del AST](docs/visor_ast.md).

## Requisitos

- GCC o un compilador de C compatible.
- GNU Make 4.3 o posterior.
- Flex y Bison.
- Python 3.9 o posterior para las pruebas de generación de código y el paquete de VS Code.
- Bash para el script de pruebas del compilador.

Los comandos siguientes se ejecutan desde la raíz del repositorio, en Linux
o un entorno compatible como WSL.

## Compilar y ejecutar

```bash
make
./ccom -v pruebas/p02_factorial.cls
```

Una entrada correcta devuelve código `0`. Los errores de análisis devuelven
`1`; los errores de uso de la línea de comandos devuelven `2`.

```bash
./ccom pruebas/p02_factorial.cls       # análisis; muestra errores si existen
./ccom -t pruebas/p02_factorial.cls    # árbol sintáctico abstracto
./ccom -m pruebas/p02_factorial.cls    # tablas de símbolos y etiquetas
./ccom -t -m -v pruebas/p02_factorial.cls
```

| Opción | Función | Estado |
| --- | --- | --- |
| `-t` | Mostrar el AST en terminal | Disponible |
| `--ast-json <archivo>` | Exportar el AST para el visor gráfico | Disponible |
| `-m` | Mostrar las tablas de símbolos y etiquetas | Disponible |
| `-v` | Mostrar el avance de cada fase | Disponible |
| `-o <archivo>` | Seleccionar el nombre del binario | Se guarda; aún no se escribe el binario |
| `-s` | Emitir ensamblador `.asm` e imagen de datos `.mem` | Disponible |
| `-p` | Mostrar bundles en ensamblador y binario | Pendiente; muestra un aviso |
| `-x` | Ensamblar una entrada `.asm` | Pendiente; devuelve un error |

Consulta los detalles en [Uso y banderas de ccom](docs/ccom_banderas.md).

## Ver el árbol gráfico del programa

Desde la raíz del repositorio, con comandos compatibles con Fish y Bash:

```sh
make
./ccom --ast-json build/p09_crypto.ast.json pruebas/p09_crypto.cls
xdg-open tools/visor_ast.html
```

Si `xdg-open` no está disponible, abrir `tools/visor_ast.html` con un navegador
desde el administrador de archivos. El visor funciona sin servidor, sin
instalar paquetes y sin conexión a internet.

Al abrirlo se muestra un ejemplo de p09. Para visualizar el archivo que acabás
de generar, pulsar **Abrir AST JSON** y seleccionar
`build/p09_crypto.ast.json`. Para otro programa, cambiar la entrada `.cls` y
el nombre del JSON en el comando anterior.

El árbol se dibuja con nodos y ramas, desde la raíz hasta sus hojas. Las
declaraciones y los bloques empiezan plegados; pulsar **Expandir todo** para
mostrarlo completo.

- **Rueda del mouse:** acercar o alejar.
- **Arrastrar el fondo:** desplazarse por el árbol.
- **Clic en un nodo:** ver tipo, texto/valor y ubicación en el código.
- **Doble clic en un nodo:** expandir o plegar su rama.
- **Ajustar árbol:** encuadrar los nodos visibles; **Ir a la raíz:** centrar la raíz.

`--ast-json` usado solo exporta el árbol después del análisis sintáctico y
termina allí: permite inspeccionar programas aunque su generación de código
esté pendiente, pero no certifica que pasen el análisis semántico. Para
exportar y continuar la compilación, combinarlo con `-s`.

Más detalles y formato del JSON en [Visor del AST](docs/visor_ast.md).

## Un programa Calser

Los programas usan la extensión `.cls`. Este ejemplo declara una variable,
realiza una suma y termina la función principal:

```calser
F main [] : void ;
    Vm resultado : int = 2 + 3 ;
    E ;
.
```

La sintaxis se define en [grammar/parser.y](grammar/parser.y) y los tokens en
[grammar/lexer.l](grammar/lexer.l). En `pruebas/` hay ejemplos válidos y casos
que ejercitan errores del compilador.

## Estructura

```text
calser_compiler/
├── src/                 # Implementación en C
├── include/             # Cabeceras de los módulos
├── grammar/             # lexer.l (Flex) y parser.y (Bison)
├── pruebas/             # Programas Calser y pruebas de generación de código
│   ├── aritmetica/
│   └── arreglos/
├── docs/                # Documentación del compilador
├── assets/branding/     # Logo del lenguaje e icono de los programas
├── editors/vscode/      # Extensión para Calser, Lex/Flex y Yacc/Bison
├── build/               # Archivos generados y objetos; ignorados por Git
├── makefile
└── README.md
```

| Módulo | Responsabilidad |
| --- | --- |
| `main` | Opciones de ejecución y coordinación de las fases |
| `ast` | Construcción, impresión y liberación del AST |
| `semantic` | Análisis semántico |
| `symbol_table` | Símbolos, ámbitos y direcciones |
| `label_table` | Tabla de etiquetas |
| `diagnostics` | Reporte de errores |
| `codegen_arith` / `codegen_array` | Rutinas de generación de código con pruebas independientes |

Las cabeceras están en `include/` y las implementaciones en `src/`. Flex y
Bison generan sus archivos en `build/generated/`; los objetos y dependencias
se guardan en `build/obj/`. El ejecutable se mantiene en `./ccom`.

## Pruebas

Para ejecutar los casos del compilador y guardar errores, tablas y códigos
de salida en un reporte:

```bash
make
bash pruebas/correr_pruebas.sh
```

El reporte se escribe en `pruebas/resultados.txt`. Este script recopila
resultados para su revisión; no compara automáticamente salidas esperadas.

Para verificar las rutinas de generación de código:

```bash
make check-arith
make check-arrays
make check-crypto
# O ejecutar todas:
make check
```

Estas pruebas usan un modelo de software de la arquitectura objetivo. Las
pruebas crypto incluyen el recorrido `.cls` → `ccom` → `.asm`/`.mem` →
simulador; no representan una simulación RTL.

Para limpiar los productos de compilación:

```bash
make clean
```

## Soporte en VS Code

La extensión incluida registra tres lenguajes:

| Extensión | Lenguaje |
| --- | --- |
| `.cls` | Calser |
| `.l` | Lex/Flex |
| `.y` | Yacc/Bison |

Incluye resaltado de sintaxis, comentarios, cierre de parejas y el tema de
iconos **Calser (Seti)**, que muestra el hexágono ámbar en los programas `.cls`.
Los colores del código los determina el tema de colores de cada integrante.

Para generar e instalar el paquete local:

```bash
python3 editors/vscode/package.py
code --install-extension editors/vscode/calser-language-0.3.0.vsix --force
```

Después ejecuta **Developer: Reload Window** y selecciona **Preferences:
File Icon Theme → Calser (Seti)**. También puedes instalar el `.vsix` con
**Extensions: Install from VSIX…**.

Si `.cls` se reconoce como LaTeX, crea o ajusta `.vscode/settings.json`:

```json
{
  "files.associations": { "*.cls": "calser" },
  "workbench.iconTheme": "calser-seti"
}
```

Hacer `git pull` comparte el código de la extensión; cada integrante debe
instalarla en su VS Code. `.vscode/` y los paquetes `.vsix` están ignorados
por Git. Más detalles en [editors/vscode/README.md](editors/vscode/README.md).

## Identidad visual

- [Logo del lenguaje](assets/branding/calser-logo.png).
- [Icono de los programas](assets/branding/calser-file-icon.png).

La extensión conserva una copia del icono para que pueda instalarse de forma
independiente del repositorio. Los avisos de los recursos Seti están en
[ThirdPartyNotices.txt](editors/vscode/ThirdPartyNotices.txt).
