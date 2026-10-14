# Visor gráfico del AST

El visor dibuja el AST real con raíz arriba, ramas y hojas. Todos los tipos de
nodo se conservan, incluidos `ARGUMENTS`, `TYPE`, `BLOCK` y listas; no es un
árbol simplificado del código. Los hermanos mantienen el orden del AST.

## Abrir la primera demostración

Desde la raíz del repositorio (Fish o Bash):

```sh
make
./ccom --ast-json build/p09_crypto.ast.json pruebas/p09_crypto.cls
xdg-open tools/visor_ast.html
```

También se puede abrir el HTML desde el administrador de archivos con un
navegador. El visor empieza con un AST de p09 incluido como demostración.
Para ver la exportación nueva o cualquier otro programa, pulsar **Abrir AST
JSON** y seleccionar su archivo. El ejemplo incluido no se actualiza
automáticamente al modificar p09; cargar el nuevo JSON para ver los cambios.

- Rueda: acercar y alejar en torno al cursor.
- Arrastrar el fondo: desplazarse por el dibujo.
- Clic en nodo: consultar ID, tipo, texto/valor, línea, columna e hijos.
- Doble clic o botón del panel: plegar/expandir una rama.
- **Expandir todo**: mostrar el árbol completo y ajustarlo a la pantalla.
- **Ajustar árbol**: encuadrar los nodos actualmente visibles.
- **Ir a la raíz**: centrar la raíz con un zoom legible.
- Teclado: Tab para enfocar controles/nodos, Enter para seleccionar y espacio
  para plegar un nodo enfocado.

Las declaraciones y los bloques empiezan plegados para que la primera vista
sea legible. Sus conexiones internas aparecen al expandirlos. Los colores
separan estructura (azul), operaciones (violeta) y hojas (amarillo).

El dibujo usa la distribución clásica: todos los hijos de un padre comparten
fila. Los rectángulos miden 152 × 64 unidades y hay 190 unidades entre el
inicio de un nivel y el siguiente (126 libres entre cajas). Los textos largos
se abrevian en el dibujo; el panel y la descripción del nodo muestran el valor
completo. Estas dimensiones reducen el ancho y dejan más espacio para seguir
las conexiones.

## Exportación para otros programas

La opción `--ast-json archivo` exporta antes de la semántica. Si no se pide
`-s`, `-m` ni `-p`, termina allí: es una operación de inspección sintáctica.
No necesita que el programa tenga generación de ensamblador completa y no
certifica su validez semántica. Puede combinarse con `-t` y `-v`.

Formato versión 1:

```json
{
  "schema": "calser.ast",
  "version": 1,
  "source": "programa.cls",
  "root": {
    "id": 1,
    "kind": "PROGRAM",
    "line": 1,
    "column": 1,
    "children": []
  }
}
```

Cada nodo tiene ID entero único dentro de esa exportación, tipo, ubicación e
hijos ordenados. `text` aparece cuando el nodo tiene texto; `value` aparece en
números y booleanos. Los IDs se asignan en preorden y pueden cambiar al editar
el fuente. Línea o columna 0 significa que no hay ubicación disponible.
No se exportan punteros internos ni símbolos semánticos.

El visor es un HTML autónomo con SVG, CSS y JavaScript incluidos. Los archivos
se leen localmente con el selector; no se envían a servicios externos. Valida
la estructura antes de cargarla y muestra errores sin perder el árbol anterior.
Admite hasta 20 MiB, 20 000 nodos y 256 niveles. No incluye búsqueda ni salto
al código fuente en esta primera versión.

## Validación

```sh
make check-ast
node pruebas/ast/probar_visor.cjs
```

La primera prueba valida el JSON del compilador y errores de uso/escritura.
La segunda requiere Node solo para desarrollo y comprueba la lógica con
elementos SVG simulados: ramas, niveles, ausencia de solapamiento entre nodos,
selección, plegado, zoom y rechazo de datos inválidos. También verifica la
separación vertical, que los hermanos compartan fila y que las cajas no se
superpongan. No sustituye la revisión
visual en un navegador. El uso del visor no necesita Node.
