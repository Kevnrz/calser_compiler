# Simulador de ensamblador

Desde la raíz del repositorio:

```bash
python3 tools/sim_asm.py programa.asm
python3 tools/sim_asm.py programa.asm --max-bundles 10000
python3 pruebas/simulador/probar.py
python3 tools/sim_asm.py pruebas/simulador/ejemplo_r4.asm
```

El ejemplo propio calcula 2 + 2: termina con `r4 = 4`, memoria `0x0000 = 4`
y cero avisos de separación. Es un ejemplo propio. El ejemplo oficial de
§11.1 está en `ejemplo_isa.asm`: también termina con r4 = 4. La referencia
actualizada de este repositorio se encuentra en `docs/isa.md`;
se utiliza la versión actualizada de la ISA.

## Entrada

Una etiqueta identifica el siguiente bundle; puede aparecer sola o junto a él.
Cada bundle tiene cuatro slots explícitos:

```asm
inicio: { mov t0, 2 | nopi | nopi | nopi }
        { nopi | nopi | nopi | nopi }
        { add t0, t0, t0 | nopi | nopi | nopi }
```

Se aceptan comentarios `#`, `;` y `//`, enteros decimales y hexadecimales,
registros `r0`–`r15` y los alias del contrato `rz`, `rl`, `rx`, `rg`, `t0`–`t11`.
Los saltos aceptan etiquetas o desplazamientos con signo respecto al bundle
actual. `jas` y `jefR` guardan `PC+1`, según la ISA.

Los desplazamientos `sll`, `srl` y `sra` reciben su cantidad en un registro,
según el formato R de §6.1 y la corrección C-02. El simulador no agrega
bundles ni expande pseudoinstrucciones.

## Ejecución y salida

- Registros en cero y memoria little endian de 64 KiB, inicialmente en cero.
- Los cuatro slots leen el estado anterior al bundle. Las escrituras de
  registros quedan disponibles después de un bundle de separación.
- Una lectura demasiado temprana avisa y utiliza el valor anterior; no se
  introduce una pausa automática para corregir el programa.
- El salto tomado completa su bundle, descarta el siguiente prefetched e
  introduce un ciclo vacío antes de ejecutar el destino.
- `sw` seguido de `lw` no requiere separación por memoria. `lw` sí tiene la
  latencia de las demás escrituras de registros. Para palabras se ignoran
  los dos bits bajos de dirección, como indica §5.4.
- Dos destinos iguales dentro de un bundle se rechazan, excepto escrituras
  a `rz`, que se descartan.
- CRYPTO implementa `feis`, una bóveda de 4 llaves de 4 palabras,
  `kv_write`, `kv_lock`, `logout`, `sanc` y autenticación de prueba.
  Sin autorización, las instrucciones protegidas no tienen efecto y activan
  `EXC`; `logout` siempre puede ejecutarse. El sello permanece hasta reset.
  Una escritura rechazada por el sello también activa `EXC`.
- `auth` acepta cualquier token salvo `999`, según `docs/fase4_saddy.md`.
  El éxito activa `AUTH`, actualiza el ancla y limpia `EXC`; el fallo quita
  autorización y activa `EXC`. Se cuentan 14 ciclos por `auth` sin agregar
  bundles; las dependencias entre registros conservan la separación habitual.
  **Esta autenticación es un sustituto de prueba: no calcula Ascon ni valida
  cadenas reales de Lamport.** `sanc` actualiza el ancla, pero la regla de
  aceptación del sustituto sigue siendo «token distinto de 999».
- `AUTH`, `KVLOCK` y `EXC` se muestran en el reporte del simulador. Ninguna
  instrucción del programa puede leerlos ni extraer palabras de la bóveda.
- `jefR` usa los 22 bits bajos del registro destino (C-06).
- Un salto condicional a su propio bundle continúa ejecutándose, como en el
  ejemplo oficial. Solo el `jef` a sí mismo se reconoce como final de main.
- Termina por `jef` a sí mismo, fin del archivo o límite de bundles
  ejecutados. Drena las escrituras pendientes antes de mostrar el resultado.
- Muestra los 16 registros y las palabras no nulas entre `0x0000` y `0x00FF`.
  Los avisos incluyen bundle, línea, slot y registro.

`--data-image programa.mem` carga la imagen de datos generada por `ccom -s`
en `0x0000–0x0FFF`. El formato empieza con `@0` y contiene hasta 1024 palabras
de ocho dígitos hexadecimales, una por línea. Las palabras se guardan en little
endian; el resto de la zona permanece en cero. Esta imagen se carga antes de
los bytes de `--load-data`. Desde Python se usa
`Simulator.load_data_image(texto)`.
`--load-data archivo.bin` inyecta bytes sin modificar su orden, por defecto en
`0x1000`. `--data-address` permite elegir otra dirección de la zona de archivos
`0x1000–0xFFFF`; se rechaza una carga que no quepa. Desde Python se puede usar
`Simulator.load_data(bytes, direccion)` o inicializar `Simulator.memory` para
la imagen de datos del compilador. La bóveda empieza en cero y está separada
de esta memoria.

`--dump-memory archivo.bin` guarda los 64 KiB finales, incluidos los rangos
crypto que no aparecen en el reporte de palabras bajas. No permite sobrescribir
el `.asm`, la imagen `.mem` ni el archivo de datos de entrada.

Códigos de salida: `0` para ejecución sin avisos; `1` si hay avisos de
separación o se alcanza el límite; `2` para errores de entrada o ejecución.


## Demostración de cifrado y descifrado

`ejemplo_crypto.asm` reproduce las plantillas del contrato, carga una llave,
sella la bóveda, cifra 128 bloques, descifra, cierra sesión e intenta cifrar
sin autorización. Finalmente prueba el token falso `999`. Es un programa
manual de prueba; todavía no es salida generada por `ccom`.

Comandos compatibles con Fish y Bash, desde la raíz del proyecto:

```bash
python3 -c 'from pathlib import Path; Path("/tmp/calser-datos-crypto.bin").write_bytes(bytes(range(256)) * 4)'
python3 tools/sim_asm.py pruebas/simulador/ejemplo_crypto.asm --load-data /tmp/calser-datos-crypto.bin --dump-memory /tmp/calser-memoria-crypto.bin
python3 -c 'from pathlib import Path; m = Path("/tmp/calser-memoria-crypto.bin").read_bytes(); original = m[0x1000:0x1400]; assert m[0x2000:0x2400] != original; assert m[0x3000:0x3400] == original; assert m[0x4000:0x4008] == original[:8]; print("OK: cifrado distinto, descifrado correcto y operación sin sesión sin cifrar")'
```

Debe terminar sin avisos de separación, con `AUTH=0 KVLOCK=1 EXC=1`.
La suite comprueba la función de ronda con una referencia independiente, las
16 palabras de la bóveda, privilegios, el sello persistente, entradas de
-1, 0, 1, 3, 8, 128 y 512 bloques, las cuatro llaves, cifrado en el mismo lugar,
y carga/volcado de archivos. Una excepción crypto no detiene el programa ni
cambia por sí sola el código de salida: el caso sin sesión es una prueba
intencional y el reporte expone `EXC` para comprobarlo.

Para ejecutar el programa `.cls` compilado y cargar sus variables iniciales,
consulta [la prueba de integración crypto](../crypto/README.md#programa-compilado-por-ccom).
