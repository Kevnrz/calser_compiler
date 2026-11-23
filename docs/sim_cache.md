# Simulador con caché

`tools/sim_cache.py` ejecuta el ensamblador de `ccom` con el modelo funcional de
`sim_asm.py` y mide los ciclos del procesador con L1, L2, buffers y memoria
principal. Los registros, la memoria plana, el estado CRYPTO, los avisos de
separación y la traza se obtienen de la misma ejecución funcional. La jerarquía
modela etiquetas y direcciones de línea para reproducir la temporización.

## Uso

Desde la raíz del repositorio:

```bash
python3 tools/sim_cache.py pruebas/ensamblado/simulador/cache_ejemplo.asm
python3 tools/sim_cache.py ejercicios/ejercicio3_cifrado/ejercicio3.asm \
  --data-image ejercicios/ejercicio3_cifrado/ejercicio3.mem \
  --load-data ejercicios/ejercicio3_cifrado/datos.txt \
  --cache 2 --mem-div 4 --metricas-json metricas.json
```

| Opción | Efecto |
|---|---|
| `--cache ideal` | RAM combinacional. Cada acceso queda listo en el ciclo del pedido. |
| `--cache 0` | Adaptador y memoria principal. |
| `--cache 1` | L1, write buffer de 4 líneas y memoria principal. |
| `--cache 2` | L1, write buffer de 4 líneas, L2, eviction buffer de 8 líneas y memoria principal. Es el valor por defecto. |
| `--mem-div 2\|4\|5` | Latencia de memoria de 39, 53 o 60 ciclos respectivamente. El divisor por defecto es 4. |
| `--metricas-json RUTA` | Escribe las métricas en JSON. Los niveles ausentes aparecen como `null` y se omiten del reporte de texto. |
| `--data-image RUTA` | Inicializa la memoria estática a partir de una imagen `.mem`. |
| `--load-data RUTA` y `--data-address DIRECCIÓN` | Carga un archivo binario. La dirección puede ser decimal o hexadecimal y su valor por defecto es `0x1000`. |
| `--max-bundles N` | Limita la ejecución funcional a `N` bundles. El valor por defecto es 100 000. |
| `--traza RUTA` y `--traza-max N` | Escribe la traza funcional en UTF-8. El límite por defecto es 5000 eventos. |
| `--dump-memory RUTA` | Escribe los 64 KiB de memoria plana al final de la ejecución funcional. |

El código de salida es 0 al terminar sin avisos, 1 si hubo avisos de separación
o se alcanzó el límite y 2 si hay un error de argumentos, ensamblador o archivos.

## Instante de medición

El pipeline empieza con dos ciclos de llenado. Un salto tomado introduce una
burbuja y un `auth` introduce 13 ciclos de pausa CRYPTO. La memoria y CRYPTO
pueden pausar al mismo tiempo. La jerarquía avanza durante llenado, burbujas,
pausas y ejecución normal.

La foto se toma antes del flanco que confirmaría el último elemento de EX
mientras el `jef` a sí mismo está en ID y el pipeline puede avanzar. Si el
primer bundle es ese `jef`, la foto queda tras el primer flanco y
`ex_anulado` vale 1. Cuando la ejecución termina por fin del archivo o por
límite, el reporte muestra `sin corte`. Si el límite cae en un salto tomado,
la medición abarca su burbuja. Se comprueba esta identidad en cada corrida:

```text
ciclos = bundles + saltos + pausa_total + 2 - ex_anulado
```

La primera línea muestra el número de bundles que ejecutó el modelo funcional,
incluido el `jef` final. El campo `bundles` de las métricas cuenta los
confirmados en la foto. `sim_asm.py` cuenta el `jef` final y su burbuja con RAM
combinacional. Los tres ejercicios suministrados marcan un ciclo menos en
`ideal` con el corte del procesador.

## Métricas

| Métricas | Significado |
|---|---|
| `cache`, `mem_div`, `tmem`, `corte` | Modo, divisor, latencia de memoria y tipo de terminación de la medición. |
| `ciclos`, `bundles`, `instrucciones`, `ipc`, `saltos` | Ciclos, bundles confirmados, slots confirmados distintos de `nopi`, instrucciones por ciclo y saltos tomados registrados. |
| `pausa_crypto`, `pausa_memoria`, `pausa_total`, `ex_anulado` | Ciclos de pausa de cada causa, ciclos con al menos una causa y marca de burbuja en EX al corte. |
| `l1_lecturas`, `l1_escrituras`, `l2_lecturas`, `l2_escrituras` | Pedidos por nivel y tipo. Sus sufijos `_aciertos` y `_fallos` cuentan los pedidos clasificados. L2 clasifica al terminar la búsqueda. |
| `l1_hit_rate`, `l1_miss_rate`, `l2_hit_rate`, `l2_miss_rate` | Aciertos o fallos divididos entre lecturas más escrituras del nivel. La tasa es cero cuando no hay accesos. |
| `l1_pausa_buffer`, `l1_pausa_l2`, `l1_pausa_memoria` | Espera de llenados completados, clasificada por el origen de cada respuesta. La pausa de un llenado pendiente al corte ya figura en `pausa_memoria` del procesador. |
| `l1_pausa_lleno`, `l2_pausa_lleno` | Ciclos en que una víctima sucia espera espacio en el buffer del nivel. |
| `wbuf_*` y `ebuf_*` | `recibidas` cuenta líneas encoladas, `forwarding` cuenta lecturas respondidas desde la cola, `ciclos_lleno` mide cuánto tiempo estuvo llena y `ocupacion_max` es su máximo de líneas simultáneas. |
| `mem_lecturas`, `mem_escrituras`, `mem_transferencia`, `mem_ciclos_totales`, `mem_utilizacion` | Transferencias aceptadas por tipo, ciclos ocupados, ciclos observados y fracción de ciclos ocupados. |
| `amat_medido`, `amat_formula` | En `CACHE=2`, el AMAT medido es 1 más la pausa de memoria por acceso a L1. La estimación es `1 + mL1 × (8 + mL2 × tmem)`, donde `mL1` y `mL2` son las tasas de fallo de los niveles. El AMAT medido parte de 1 si L1 no recibió accesos. |

## Ejemplo con una escritura y una lectura

`cache_ejemplo.asm` inicializa dos registros, deja un bundle de separación,
escribe una palabra en la dirección 64, la lee y termina con un `jef` a sí
mismo. Con `MEM_DIV=4` la foto de los tres modos de referencia tiene 3 bundles,
3 instrucciones, 0 saltos y `EX_anulado=0`.

| CACHE | Ciclos | Pausa de memoria y total | Lecturas de memoria | Escrituras de memoria | Ciclos de transferencia |
|---|---:|---:|---:|---:|---:|
| ideal | 5 | 0 | — | — | — |
| 0 | 111 | 106 | 1 | 1 | 105 |
| 2 | 66 | 61 | 1 | 0 | 53 |

- En `ideal`, la lectura está lista en el ciclo 6. La foto queda después del flanco 5.
- En `CACHE=0`, la memoria acepta la escritura en el flanco 5 y responde en el ciclo 58. Acepta la lectura en el flanco 59 y responde en el ciclo 112. La foto queda después del flanco 111.
- En `CACHE=2`, la escritura falla en una L1 fría. L2 acepta el pedido en el flanco 5 y pide la línea a memoria en el ciclo 13. La línea llega en el ciclo 66 y L1 la instala sucia. La lectura acierta en el ciclo 67, por lo que la foto queda después del flanco 66 con un fallo de escritura en L1 y un fallo de lectura en L2. Ambos buffers registran cero líneas recibidas.

`make check-cache` comprueba los módulos, las cifras de referencia y la
equivalencia funcional en los cuatro modos. El target forma parte de `make check`.
