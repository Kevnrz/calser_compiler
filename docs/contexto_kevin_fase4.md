# Contexto para Kevin — continuación de Fase 4

Estado revisado el 13 de octubre de 2026 en la rama
`S-fase4-crypto-simulador`, después de integrar `origin/k-fase4/codegen`.
Este documento describe la implementación y los límites comprobados; no
sustituye el [contrato común](../fase4_contrato_comun.md).
Al redactarlo, la última integración de crypto y este documento aún no tenían
commit. La referencia vigente de arquitectura es [isa.md](isa.md).

## Qué ya está listo del trabajo de Saddy

1. Cada función, incluido main, reserva 12 palabras (48 bytes) de temporales
   al final de su área. El símbolo expone `temp_base` y `area_size` incluye la
   reserva.
2. La tabla de etiquetas dispone de `.L_func_<nombre>`, `.L_body_<nombre>`,
   etiquetas de cortocircuito `.L_sc_end_N` y `.L_arith_error`.
   Tener las etiquetas no significa que el generador implemente todas ellas.
3. `tools/sim_asm.py` ejecuta ensamblador VLIW textual y detecta lecturas sin
   separación. Incluye Feistel, bóveda de llaves y autenticación de prueba.
4. `AST_CRYPTO` está conectado al generador y al evaluador real de expresiones.
   `pruebas/p09_crypto.cls` genera `.asm` y `.mem` y se ejecuta sin avisos de
   separación, con cifrado y descifrado verificados byte por byte.

La autenticación del simulador acepta cualquier token salvo `999`, como
sustituto de prueba acordado. No implementa Ascon ni cadenas Lamport reales.
No se debe interpretar esta validación como una prueba del hardware crypto.

## Archivos y puntos de entrada

| Archivo | Qué contiene |
|---|---|
| `src/generate.c` | Recorrido del AST, evaluador de expresiones, adaptador crypto e inicializadores |
| `include/codegen_crypto.h`, `src/codegen_crypto.c` | API y expansión de las siete sentencias crypto |
| `include/codegen.h`, `src/codegen.c` | Bundles, etiquetas e imagen de datos |
| `src/semantic.c` | Reserva de temporales y enlaces AST → símbolos |
| `src/main.c` | Escritura de `.asm` y `.mem` con `-s`, y propagación de errores de escritura |
| `src/codegen_arith.c` | Rutinas aritméticas; se corrigieron argumentos de formato `%d` para registros |
| `tools/sim_asm.py` | Simulación, carga de `.mem`, carga binaria de buffers y volcado final |
| `pruebas/crypto/probar.py` | Pruebas independientes del módulo crypto |
| `pruebas/crypto/probar_integracion.py` | Recorrido real `.cls` → `ccom` → `.asm`/`.mem` → simulador |

`emit_statement()` despacha `AST_CRYPTO` a `emit_crypto_statement()`.
Esta función captura el mapa `temp_used`, configura la función actual y la
máscara de registros vivos, llama `cg_crypto()` y recupera el mapa del
asignador. `generate_code()` inicializa una sola vez el contexto crypto y
su contador de etiquetas por programa.

## Contratos que hay que conservar al ampliar el evaluador

- `cg_op()` emite la operación y un bundle vacío de separación. Quien use
  `cg_bundle()` directamente debe calendarizar sus propias dependencias.
- Variables y casillas temporales usan direcciones absolutas con base `rz`.
  No cambiar los accesos a base `rg` sin resolver el contrato de memoria.
- `cg_crypto()` devuelve **0 en éxito y 1 en error**. La API aritmética tiene
  otra convención: comprobar cada interfaz antes de conectarla.
- `CGCryptoEval(user, expr, dst, preserve_mask)` debe dejar el resultado en
  `dst`, conservar los registros de `preserve_mask` y devolver 0 en éxito.
- `live_mask` usa números ISA: el bit 4 representa `t0/r4`; `0xFFF0` representa
  los 12 temporales. El módulo salva rN en `temp_base + 4*(N-4)` antes de
  evaluar argumentos y lo restaura al terminar.
- El adaptador actual `emit_crypto_argument()` guarda los argumentos previos
  en casillas libres de esa misma área. Excluye las casillas ocupadas por
  `live_mask`, libera el asignador para evaluar y luego recupera los argumentos.
  Esto permite que una multiplicación use los registros de trabajo que necesita.
  Si no quedan casillas, la compilación falla explícitamente.
- Al añadir spills o llamadas dentro de expresiones, coordinar esas reservas:
  no sobrescribir temporales vivos ni argumentos crypto ya guardados.
- `next_label` se inicializa una vez por programa, no por función o sentencia.
  Las expansiones usan etiquetas privadas `.L_crypto_*`.
- Un fallo puede dejar código parcial en memoria. Abortar la generación;
  no escribir ese buffer como un programa válido.

| Argumento crypto | Destino | Registros previos a conservar |
|---|---|---|
| origen | r6 / t2 | ninguno |
| destino | r7 / t3 | r6 |
| bloques | r9 / t5 | r6 y r7 |
| token, palabra de llave o ancla | r10 / t6 | ninguno |

Los selectores `k` y `w` son literales `AST_NUMBER`, no argumentos evaluados
como registros. Esta convención pertenece a la expansión del compilador;
no obliga a todo programa ISA a usar esos registros para sus operandos.

## Inicializadores y archivos de salida

Las globales constantes, incluidas listas de arreglos, se escriben en la
imagen de datos. Las globales dinámicas se inicializan al entrar a main.
Los locales se ponen en cero o en su inicializador al recorrer la declaración.
`semantic.c` enlaza los identificadores de declaraciones, funciones y variable
de retorno con sus símbolos; estos enlaces son necesarios para el generador.

Con `-s`, `ccom` escribe dos archivos junto al nombre dado por `-o`:

- `.asm`: texto de bundles y etiquetas.
- `.mem`: `@0` seguido de palabras de 32 bits en hexadecimal, una por línea,
  para la zona `0x0000–0x0FFF`.

Dar `-o programa.bin` permite elegir el nombre base; este recorrido no genera
un `.bin` ensamblado. La escritura a binario/hex queda fuera de esta integración.
El simulador necesita `--data-image` para cargar las variables de `.mem` y
`--load-data` para inyectar los bytes que el programa va a cifrar.

## Cómo reproducir la validación

Desde la raíz del repositorio, con comandos compatibles con Fish y Bash:

```sh
make
make check
python3 pruebas/simulador/probar.py
./ccom -s pruebas/p09_crypto.cls -o /tmp/p09_crypto.bin
python3 -c 'from pathlib import Path; Path("/tmp/calser-datos-crypto.bin").write_bytes(bytes(range(256)) * 4)'
python3 tools/sim_asm.py /tmp/p09_crypto.asm --data-image /tmp/p09_crypto.mem --load-data /tmp/calser-datos-crypto.bin --dump-memory /tmp/p09_crypto_final.bin
python3 -c 'from pathlib import Path; m = Path("/tmp/p09_crypto_final.bin").read_bytes(); original = m[0x1000:0x1400]; assert m[0x2000:0x2400] != original; assert m[0x3000:0x3400] == original; assert m[0x4000:0x4008] == original[:8]; print("OK: cifrado, descifrado y rechazo sin sesión")'
```

`/tmp` es una carpeta del sistema, no una carpeta del repositorio. Los archivos
son temporales; se regeneran con estos comandos y no hace falta versionarlos.

El resultado esperado de p09 es cero avisos de separación y
`AUTH=0 KVLOCK=1 EXC=1`. `EXC=1` es intencional: el programa prueba una
operación sin sesión, el token rechazado `999` y un cambio de ancla sin sesión.
La operación sin sesión conserva las mitades originales en el destino de
esta plantilla; no produce ciphertext autorizado.

Última ejecución verificada: 1746 casos aritméticos, 1190 de arreglos,
184 del módulo crypto, 7 escenarios de integración y 37 tests del simulador,
todos pasando. Los escenarios de integración cubren p09, argumentos
calculados con multiplicación, cero/negativos, global dinámica, constantes
INT32_MIN/MAX e inicializadores, y error al escribir salidas.
Las pruebas independientes de arreglos no demuestran que `AST_INDEX` esté
conectado al generador general.

## Pendientes visibles para continuar el generador general

Estos puntos salen de revisar el código actual; no todos tienen una prueba
reproductora añadida. El éxito de p09 no demuestra que estén resueltos.

1. **Llamadas:** `emit_expression()` todavía rechaza `AST_CALL`. La ruta de
   llamadas como sentencia usa `emit_function_call()`, pero esa rutina rechaza
   funciones `void`. Revisar argumentos, retorno, preservación de `rl`,
   temporales y prólogo/epílogo con el contrato común; las etiquetas de entrada
   y cuerpo se emiten actualmente consecutivas, sin el prólogo allí requerido.
2. **Memoria de llamadas:** `emit_stack_push/pop()` usa una pila apuntada por
   `rx` que vive en la zona de `ccom`: empieza en la primera dirección libre
   después de la última área (`semantic_data_end()`) y llega hasta `0x0FFF`.
   Ya no toca `0x1000` en adelante, que es la zona de los archivos de
   `load_file`. El semántico exige al menos 256 bytes libres para ella. Sigue
   pendiente decidir si se pasa al salvado en `temp_base` del contrato.
3. **Arreglos:** `AST_INDEX` se rechaza en expresiones y la asignación admite
   solo destinos identificadores. Conectar `codegen_array` y validar lectura,
   escritura, parámetros y referencias desde un `.cls` real.
4. **Ciclo S:** `AST_LOOP_S` todavía informa que no está implementado.
5. **Cortocircuito:** `AST_AND`/`AST_OR` evalúan ambos lados y emiten `and`/`or`.
   Implementar el comportamiento de `&`/`|` con saltos y las etiquetas
   `.L_sc_end_N`; tener esas etiquetas en la tabla no completa el comportamiento.
6. **Presión de registros:** el asignador no tiene spills generales. Las
   rutinas aritméticas reservan lhs, rhs, resultado y ocho registros de trabajo.
   Expresiones más complejas pueden agotar registros; coordinar cualquier
   solución nueva con las reservas del adaptador crypto.
7. **R y ejercicio 3:** existe emisión de `AST_REWIND`, pero no se validó aquí
   con un programa completo del ejercicio 3. Hace falta ese programa concreto
   y pruebas de argumentos simultáneos/funciones antes de darlo por terminado.
8. **Ensamblado:** conectar y verificar las salidas `.bin`/`.hex` con el trabajo
   correspondiente de Gabriel; el simulador actual consume `.asm` textual.

Para detalles de las plantillas y las pruebas, consultar
[crypto_para_ccom.md](crypto_para_ccom.md),
[pruebas/crypto/README.md](../pruebas/crypto/README.md) y
[pruebas/simulador/README.md](../pruebas/simulador/README.md).

## Visor del AST añadido después de la integración crypto

`--ast-json archivo` exporta el árbol sintáctico antes de la semántica. El visor
`tools/visor_ast.html` lo dibuja localmente con SVG, selección, plegado, zoom y
desplazamiento. La exportación sola termina después del parseo; no debe
interpretarse como compilación semántica exitosa. Ver [visor_ast.md](visor_ast.md).
