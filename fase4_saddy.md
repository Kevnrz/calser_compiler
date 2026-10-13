# Fase 4 — Saddy: cifrado, temporales, etiquetas y simulador

**Rama:** `s-fase4`, desde `integracion`. **Lee primero** `fase4_contrato_comun.md`.
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

- `cg_crypto(ASTNode *node)`: expande cada `AST_CRYPTO` con las plantillas de
  `crypto_para_ccom.md` §4, usando `cg_bundle` (son plantillas calendarizadas a
  mano).
- Argumentos: calcularlos en registros con la función de expresiones de Kevin;
  `k` y `w` son el `AST_NUMBER` del nodo y van como campo de la instrucción.
- Renombrar las etiquetas de cada expansión (`cifra_1`, `fin_cifra_1`…).
- `descifrar` con el intercambio de mitades de C-17.
- Antes de la plantilla, guardar en `temp_base` los temporales vivos, como en
  una llamada.

**Listo cuando:** `pruebas/p09_crypto.cls` con `-s` genera su `.asm` y tu
simulador lo ejecuta sin avisos de separación (las instrucciones CRYPTO puede
tratarlas como «sin efecto» si el simulador no implementa la bóveda).

## 5. Qué reportar

`git diff --stat`, el diff de `resultados.txt` explicado, la salida del
simulador con el ejemplo de `isa.md` §11 y con los `.asm` que te pase Kevin, y
el `.asm` de `p09_crypto`.
