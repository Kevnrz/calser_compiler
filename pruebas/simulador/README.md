# Simulador de ensamblador

Desde la raíz del repositorio:

```bash
python3 tools/sim_asm.py programa.asm
python3 tools/sim_asm.py programa.asm --max-bundles 10000
python3 pruebas/simulador/probar.py
python3 tools/sim_asm.py pruebas/simulador/ejemplo_r4.asm
```

El ejemplo propio calcula 2 + 2: termina con `r4 = 4`, memoria `0x0000 = 4`
y cero avisos de separación. No es el ejemplo de `isa.md` §11: la copia
actual de `docs/isa.md` termina en §10.

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
según el formato R de §6.1. El literal que aparece en §4.3 debe cargarse en un
registro antes de usar esas instrucciones; el simulador no agrega bundles ni
expande pseudoinstrucciones.

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
- CRYPTO se valida y cuenta, pero no cambia registros, memoria o privilegios.
  Se conservan las dependencias de `feis` para comprobar la calendarización.
  Esta modalidad no permite comprobar resultados de cifrado o autenticación.
- Termina por salto tomado a sí mismo, fin del archivo o límite de bundles
  ejecutados. Drena las escrituras pendientes antes de mostrar el resultado.
- Muestra los 16 registros y las palabras no nulas entre `0x0000` y `0x00FF`.
  Los avisos incluyen bundle, línea, slot y registro.

No carga automáticamente la imagen de datos del compilador ni archivos `.mem`.
Los programas que requieran datos iniciales deben escribirlos desde su `.asm`
o inicializar `Simulator.memory` cuando se usa el módulo desde Python.

Códigos de salida: `0` para ejecución sin avisos; `1` si hay avisos de
separación o se alcanza el límite; `2` para errores de entrada o ejecución.
