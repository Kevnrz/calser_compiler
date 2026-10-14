# Fase 4 — Saddy: cifrado, temporales, etiquetas y simulador

**Rama:** `s-fase4`, desde `integracion`. **Lee primero** [isa.md](isa.md).
Antes de escribir código, lee `semantic.c` (asignación de áreas),
`symbol_table.h`, `label_table.c/h`, `main.c` y `crypto_para_ccom.md`.

Los puntos 1 a 3 no dependen de Kevin: empieza por ahí. El 4 necesita su
`codegen.h`.

---

## 1. Cambio 12a — 12 casillas de temporales (`semantic.c`)

- Al cerrar el área de cada función, reservar **12 palabras (48 bytes)** al
  final, después de los locales. Guardar su dirección en un campo nuevo del
  símbolo de la función, `temp_base`, y sumarlas a `area_size`.
- `main` también las reserva (puede llamar funciones).
- Mostrar `temp_base` en la impresión de la tabla de símbolos.
- **Efecto esperado:** en `pruebas/resultados.txt` cambian los `area`, `size` y
  las direcciones de las funciones que vienen después. Nada más. Revisa a mano
  una: en `p09` o `prueba_R.cls`, `factorial` pasa de `size=16` a `size=64`.
- Si un programa ya no cabe en 0x0FFF, el error de memoria existente debe
  saltar; ningún programa de `pruebas/` está cerca de ese límite.

## 2. Cambio 12b — tabla de etiquetas (`label_table.c/h`)

- Nombres de función en lugar de contador: `.L_func_<nombre>`.
- Nueva etiqueta por función: `.L_body_<nombre>` (después del prólogo, destino
  de `R`).
- Etiquetas para `&` y `|`: una de salida por nodo (`.L_sc_end_N`).
- Una etiqueta global `.L_arith_error`, la que pide `cg_arith_emit` de Gabriel
  para `/`, `%` y `^`.
- Imprimir el tipo de nodo como texto (`FUNCTION`, `CONDITIONAL`…), no como
  número.
- `resultados.txt` cambia solo en el bloque de la tabla de etiquetas.

## 3. Simulador de `.asm` (`tools/sim_asm.py`)

Para que Kevin compruebe su código esta noche, sin esperar al ensamblado ni al
procesador. Parte de tu `sim_programa_crypto.py`:

- Lee un `.asm` con la sintaxis de `isa.md` §11 (etiquetas, 4 slots por
  bundle, `nopi`).
- Ejecuta con la semántica del pipeline: los 4 slots leen antes de escribir;
  el resultado de un registro **no** es visible en el bundle siguiente (1 de
  separación); el bundle después de un salto tomado se descarta.
- **Avisa** cuando un bundle lee un registro escrito en el bundle anterior
  (violación de separación).
- Termina cuando detecta un salto a sí mismo (fin de `main`) o tras N bundles.
- Imprime al final las palabras de memoria 0x0000–0x00FF no nulas y los
  registros.
- Debe ejecutar el ejemplo de `isa.md` §11 y terminar con `r4 = 4`.

## 4. Cifrado en la Fase 4 (cuando exista `codegen.h` de Kevin)

**Documento base: `crypto_para_ccom.md`** (§1 instrucciones y codificación, §2
calendarización, §4 plantillas verificadas, §6 las 7 operaciones de Calser).

**Traducción de las 5 operaciones simples** (las instrucciones CRYPTO van solo
en el slot 1, una por bundle, y ese bundle no lleva ALU1):

| Calser | Ensamblador |
|---|---|
| `autenticar [ e ] ;` | calcular `e` en `tN` → bundle vacío → `{ nopi \| auth tN \| nopi \| nopi }` |
| `cerrar_sesion [ ] ;` | `{ nopi \| logout \| nopi \| nopi }` |
| `cargar_llave [ k , w , e ] ;` | calcular `e` en `tN` → bundle vacío → `{ nopi \| kv_write tN, k, w \| nopi \| nopi }` |
| `sellar [ ] ;` | `{ nopi \| kv_lock \| nopi \| nopi }` |
| `cambiar_ancla [ e ] ;` | calcular `e` en `tN` → bundle vacío → `{ nopi \| sanc tN \| nopi \| nopi }` |

`k` y `w` salen del `AST_NUMBER` del nodo y van como campo de la instrucción.
`auth` congela el pipeline en hardware: no hace falta nada extra después.

**`cifrar` y `descifrar` con argumentos que no son números fijos.** Las
plantillas de §4 empiezan con `mov t2, ORIGEN`, `mov t3, DESTINO`,
`mov t5, n`, que solo sirven si son constantes. En general (por ejemplo, el
ejercicio 3, donde origen y destino son `direccion [ … ]`):

1. Calcular origen, destino y bloques en registros con la función de
   expresiones de Kevin.
2. Pasarlos a `t2`, `t3` y `t5` (`saddi t2, tX, 0`) y poner `mov t4, 0`.
3. Un bundle vacío.
4. `{ nopi | nopi | bge rz, t5, fin_N | nopi }` y luego el lazo de §4 tal cual.

Si los registros de los argumentos coinciden con `t0`–`t5`, calcula primero en
otros o guarda en `temp_base`, para no pisarlos antes de copiarlos.

- `cg_crypto(ASTNode *node)`: expande cada `AST_CRYPTO` con lo anterior y las
  plantillas de §4, usando `cg_bundle` (son plantillas calendarizadas a mano).
- Argumentos: calcularlos en registros con la función de expresiones de Kevin;
  `k` y `w` son el `AST_NUMBER` del nodo y van como campo de la instrucción.
- Renombrar las etiquetas de cada expansión (`cifra_1`, `fin_cifra_1`…).
- `descifrar` con el intercambio de mitades de C-17.
- Antes de la plantilla, guardar en `temp_base` los temporales vivos, como en
  una llamada.

**El simulador debe implementar CRYPTO** (`feis` con la función F, la bóveda,
`kv_write`, `kv_lock`, `logout`, y `auth` como siempre exitoso salvo con el
token de prueba falso `999`), para poder comprobar el resultado y no solo la
estructura. Para la memoria de 0x1000 en adelante, que se le pueda cargar un
archivo de datos de prueba.

**Listo cuando**, con datos de prueba cargados en 0x1000:

| Programa | Resultado esperado en el simulador |
|---|---|
| `pruebas/p09_crypto.cls` | Sin avisos de separación. 0x3000–0x33FF igual a 0x1000–0x13FF (el descifrado devuelve el original). 0x2000–0x23FF distinto del original. 0x4000–0x4007 igual a 0x1000–0x1007 (el `cifrar` sin sesión no cifra) |
| El ejercicio 3 (`coordinacion_pendientes.md` §3) | 0x3000–0x33FF igual a 0x1000–0x13FF; `n1` = `n2` = 8 |

## 5. Qué reportar

`git diff --stat`, el diff de `resultados.txt` explicado, la salida del
simulador con el ejemplo de `isa.md` §11 y con los `.asm` que te pase Kevin, y
el `.asm` de `p09_crypto`.
