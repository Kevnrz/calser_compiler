# Generación de CRYPTO

El módulo `src/codegen_crypto.c` traduce las siete sentencias `AST_CRYPTO`
con las plantillas de `docs/crypto_para_ccom.md` y la ISA de `docs/isa.md`.
Está conectado al recorrido del AST y al evaluador de expresiones de `ccom`.

```bash
make check-crypto
make check
./build/emitir_crypto demo 128 0 0xFFF0 dynamic > /tmp/crypto-generado.asm
python3 -c 'from pathlib import Path; Path("/tmp/crypto-datos.bin").write_bytes(bytes(range(256)) * 4)'
python3 tools/sim_asm.py /tmp/crypto-generado.asm --load-data /tmp/crypto-datos.bin --dump-memory /tmp/crypto-memoria.bin
python3 -c 'from pathlib import Path; m = Path("/tmp/crypto-memoria.bin").read_bytes(); original = m[0x1000:0x1400]; assert m[0x2000:0x2400] != original; assert m[0x3000:0x3400] == original; assert m[0x4000:0x4008] == original[:8]; print("OK: cifrado, descifrado y rechazo sin sesión")'
```

El emisor auxiliar recibe `demo`, `single` o `inplace`; cantidad de bloques;
llave 0–3; máscara de registros vivos; y opcionalmente `dynamic`. En esta
última modalidad los argumentos salen de variables en memoria, incluido
`origen + 0`, en lugar de literales. Es salida del módulo C con un evaluador
auxiliar, **todavía no salida de un programa `.cls` compilado por `ccom`**.

## Interfaz del generador

```c
CGCryptoContext crypto = {
    .eval = evaluar_argumento_crypto,
    .user = contexto_del_generador,
    .function = simbolo_de_la_funcion,
    .live_mask = mascara_de_temporales_vivos,
    .next_label = 1
};
/* Mantener crypto durante todo el programa; actualizar function/live_mask
 * por sentencia. No reiniciar next_label al cambiar de función. */
int error = cg_crypto(&crypto, nodo_AST_CRYPTO);
```

El adaptador `eval(user, expr, dst, preserve_mask)` debe devolver 0 en éxito,
generar la expresión en el registro ISA `dst` y preservar los registros
marcados en `preserve_mask`. Este módulo evalúa los argumentos en orden:

| Argumento | Registro destino | Argumentos previos que se preservan |
|---|---|---|
| origen | r6 / t2 | ninguno |
| destino | r7 / t3 | r6 |
| bloques | r9 / t5 | r6 y r7 |
| token, palabra de llave, ancla | r10 / t6 | ninguno |

`k` y `w` se extraen de los nodos `AST_NUMBER`; no pasan por el evaluador.
El evaluador debe respetar las reglas normales del pipeline. Además el módulo
pone una separación después de cada evaluación para proteger su frontera.

Antes de evaluar, se guardan **todos** los temporales marcados en `live_mask`.
La máscara usa los números ISA: `1u << 4` representa t0 y `0xFFF0` representa
t0–t11. La dirección para rN es `temp_base + 4*(N-4)`. Al terminar se recuperan
los valores originales, incluso los registros usados para argumentos.

**El evaluador no debe sobrescribir esas casillas guardadas**, tampoco durante
llamadas o spills. Su adaptador debe coordinar esta reserva con su asignador de
registros/memoria; si no puede evaluarlo conservando `preserve_mask` y las
casillas guardadas, debe devolver error. No asumir que sus spills actuales ya
son compatibles. El adaptador de `generate.c` guarda los argumentos previos en casillas libres
del área temporal, evalúa la expresión y los recupera. Rechaza una evaluación
si no hay casillas libres suficientes; nunca reutiliza las casillas de los
temporales vivos guardados por el módulo.

Las etiquetas usan el prefijo privado `.L_crypto_` y un contador por programa.
Se intercambian las mitades al cargar y guardar en descifrado. Un límite de
bloques <= 0 evita acceder a los buffers. Las operaciones solo usan registros
temporales; rl/rx/rg no se alteran por el módulo.

Un fallo del evaluador puede dejar emisión parcial; el generador debe abortar
la compilación y descartar el buffer, no escribir un `.asm` incompleto. El
módulo valida tipo de nodo, cantidad de argumentos, selectores y área de
salvado antes de empezar a emitir.

## Validación

`probar.py` ejecuta 184 casos sobre el código emitido por C. Comprueba el
resultado contra una referencia independiente de Feistel, constantes de
32 bits con signo, las cuatro llaves, límites negativos/cero, argumentos
dinámicos, cifrado en el mismo lugar, temporales vivos, etiquetas únicas y
separación. El emisor también prueba contratos inválidos y errores del
evaluador. La autenticación verifica el token Lamport `x99 = 0xF47BD70B`
contra el ancla inicial `0xFEFF95FF` mediante Ascon-Hash256 truncado a 32 bits.

## Programa compilado por ccom

Comandos compatibles con Fish y Bash, desde la raíz del proyecto:

```bash
make
./ccom -s pruebas/p09_crypto.cls -o /tmp/p09_crypto.bin
python3 -c 'from pathlib import Path; Path("/tmp/calser-datos-crypto.bin").write_bytes(bytes(range(256)) * 4)'
python3 tools/sim_asm.py /tmp/p09_crypto.asm --data-image /tmp/p09_crypto.mem --load-data /tmp/calser-datos-crypto.bin --dump-memory /tmp/p09_crypto_final.bin
python3 -c 'from pathlib import Path; m = Path("/tmp/p09_crypto_final.bin").read_bytes(); original = m[0x1000:0x1400]; assert m[0x2000:0x2400] != original; assert m[0x3000:0x3400] == original; assert m[0x4000:0x4008] == original[:8]; print("OK: cifrado, descifrado y rechazo sin sesión")'
```

Con `-s`, `ccom` escribe el ensamblador `.asm` y la imagen inicial de datos
`.mem` junto al nombre indicado por `-o`; estos comandos no producen un binario
ensamblado. La imagen contiene las variables globales constantes. Los
inicializadores globales dinámicos se ejecutan al entrar a main; los locales
se inicializan al recorrer su declaración. Las variables sin inicializador
se ponen en cero.

`probar_integracion.py`, incluido en `make check-crypto`, comprueba once
escenarios mediante el compilador real: p09 y su ejecución por CLI, argumentos
calculados (incluida multiplicación), cero y negativos, una variable global
dinámica, constantes de 32 bits e inicializadores, y fallos de escritura de
salida. En p09 deben coincidir los bytes descifrados con los originales, no
haber avisos de separación y terminar con `AUTH=0 KVLOCK=1 EXC=1`.

La prueba de p09 satisface el criterio de integración del punto 4. No demuestra
que estén terminadas todas las construcciones del lenguaje: las llamadas en
expresiones y los accesos a arreglos dependen de completar el generador general.
El ejercicio 3 requiere su programa concreto para validar ese recorrido.
