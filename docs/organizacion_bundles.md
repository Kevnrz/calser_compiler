# Organización de bundles

La rama `Organizacion-Bundles` incorpora una representación interna de las
instrucciones antes de empaquetarlas (paso 1) y un organizador local activado
por defecto (paso 2). La opción `--no-schedule` conserva la emisión original
para comparación. Este documento mantiene las decisiones y verificaciones de
los pasos implementados.

## Paso 1 — representación interna

## Representación de instrucciones

`CGInstruction`, en `include/codegen.h`, guarda:

- Texto de la instrucción y línea del fuente.
- Unidad funcional y slot original.
- Máscara de slots permitidos: una ALU normal admite slots 0 y 1; CRYPTO solo
  slot 1, BRU solo slot 2 y LSU solo slot 3.
- Máscaras de registros leídos y potencialmente escritos, con bits ISA 0–15.
  Las escrituras a `rz` se descartan y no crean dependencias de escritura.
- Efectos de memoria, control, llamada, estado crypto y escritura condicional.
- Distancia mínima para consumir un resultado: productor en B, consumidor
  desde B+2, según ISA §9. Las lecturas pueden venir de cualquiera de los slots.

`src/codegen_instruction.c` describe la sintaxis generada y admite números de
registro y alias. No resuelve etiquetas ni codifica palabras; la validación
final sigue a cargo del ensamblador. Su tabla de formas debe mantenerse junto
con `src/assembler.c` si se amplía la ISA.

`cg_describe_instruction()` devuelve 0 si conoce la instrucción, 1 si no puede
describirla. En este último caso, `known=0`, slots permitidos en cero y efecto
`CG_EFFECT_BARRIER`: el futuro organizador no puede tratarla como independiente.
No se reemplaza ni se elimina su texto; el ensamblador conserva sus diagnósticos.
El texto de esta API se presta, no se copia ni se libera dentro de ella.

Hay casos que requieren más que máscaras de registros:

- `sw`/`sb` leen tanto el dato como la base y escriben memoria.
- `lw`/`lb` leen la base y producen un registro con la misma separación habitual.
- `ifmov` y `feis` pueden conservar el destino; se marca escritura condicional.
- `jas` y `jefR` pueden escribir enlace; las llamadas se distinguen del salto.
- Los saltos con destino numérico se marcan como relativos al PC: cambiar de
  bundle exige resolver o preservar ese desplazamiento, no copiarlo sin ajuste.
- Las instrucciones crypto afectan estado no visible en los registros y se
  marcan como barreras. `auth` además marca la pausa del pipeline; esa pausa
  no cambia la separación de registros establecida por la ISA.

## Flujo anterior al empaquetado

`cg_ir_count()` y `cg_ir_record(i)` exponen el flujo en orden de emisión.
Cada `CGIRRecord` conserva línea del fuente y dirección del bundle actual:

| Tipo | Origen | Uso para el siguiente paso |
|---|---|---|
| `CG_IR_OPERATION` | `cg_op()` | Operación candidata al empaquetado |
| `CG_IR_LABEL` | `cg_label()` | Frontera de entrada/control |
| `CG_IR_FIXED_BUNDLE` | `cg_bundle()` | Cuatro slots ya calendarizados, conservar juntos |
| `CG_IR_GAP` automático | Separación agregada por `cg_op()` | Podrá sustituirse por trabajo útil al analizar dependencias |
| `CG_IR_GAP` explícito | `cg_gap()` | Separación solicitada por el generador/plantilla; conservar inicialmente |

Los registros son de solo lectura. El emisor copia los textos y es dueño de
ellos; los libera con `cg_free()`. Los punteros a registros valen hasta la
siguiente emisión, `cg_init()` o `cg_free()`.

Durante el paso 1, el adaptador conservador construía las líneas `.asm` y las
direcciones como antes. En el paso 2 sigue disponible con `--no-schedule`.
La organización nueva reconstruye las líneas desde el IR, sin cambiar la
imagen de datos ni reagrupar las plantillas manuales.

## Validación

Desde la raíz del repositorio:

```sh
make check-bundles
make check
```

`pruebas/bundles/probar_ir.c` verifica las formas de la ISA, máscaras,
efectos, slots, distinción de separaciones, propiedad de los textos y reinicio.
`revisar_ir.c` recorre el flujo del generador real; `probar.py` comprueba
factorial, p09, p11 y factorial con `R`, sin instrucciones desconocidas.
`make check` incluye estas pruebas.

También se compararon 26 programas compilables de `pruebas/` antes y después:
`.asm`, `.bin`, `.hex`, `.mem` y tablas idénticos byte por byte. Las pruebas
completas pasan; el flujo se verificó con AddressSanitizer y UndefinedBehaviorSanitizer.

Referencia de rendimiento de p02, todavía vigente al finalizar este paso:
111 bundles estáticos (55 vacíos), 309 bundles ejecutados, 337 ciclos,
resultado `f=120`, cero avisos y ningún bundle con ambas ALU activas.

El apartado siguiente documenta la organización local añadida después de
verificar esa referencia.

## Paso 2 — organizador local (implementado)

Decisiones del 13 de octubre de 2026:

1. Se organizará el flujo IR al terminar la generación y antes de resolver
   las direcciones de etiquetas y ensamblar. Las interfaces textuales existentes
   recibirán los bundles ya organizados.
2. Cada región termina en etiqueta, salto, bundle manual, separación explícita
   o instrucción con efectos que obligan a mantenerla fija. No se moverá trabajo
   entre regiones ni desde el camino posterior a un salto.
3. Las dependencias RAW requieren distancia 2. Para WAR y WAW se conservará
   el orden con al menos un ciclo, de forma conservadora. Todos los accesos a
   memoria se ordenarán entre sí, sin suponer que sus direcciones son distintas.
4. Un salto solo podrá emitirse cuando todas las operaciones anteriores de su
   región ya estén colocadas; podrá compartir bundle con las independientes.
5. Los huecos automáticos podrán sustituirse por trabajo útil. Los huecos
   explícitos y los bundles manuales se preservarán. Al cerrar una región se
   mantendrá una separación de frontera para proteger entradas y plantillas.
6. Se comprobarán escrituras del bundle previo al entrar a una región nueva:
   un consumidor no podrá adelantarse sobre una carga o un bundle fijo.
7. Las regiones largas se dividirán en grupos de hasta 256 operaciones para
   acotar memoria y costo del análisis. El corte conserva una separación segura.
8. Si hay instrucciones desconocidas o desplazamientos numéricos relativos al
   PC, se conservará la emisión original del programa completo: reorganizarlo
   sin resolver esos destinos podría cambiar el flujo.
9. Se ofrecerá `--no-schedule` para comparar la emisión conservadora. Las líneas
   del fuente de cada slot se conservarán también cuando un bundle combine
   operaciones procedentes de diferentes sentencias.

### Cambios realizados

| Archivo | Cambio |
|---|---|
| `include/codegen_schedule.h` | Interfaz del organizador de una región, callback de salida y límite de 256 operaciones |
| `src/codegen_schedule.c` | Grafo de dependencias y selección de operaciones listas por ciclo |
| `src/codegen.c` | `cg_schedule()`, reconstrucción de bundles/direcciones, conservación de plantillas, anotaciones por slot y escritura de listados sin truncar bundles largos |
| `include/codegen.h` | Contrato de finalización y de consulta de direcciones después de organizar |
| `src/main.c` | Activación por defecto antes de resolver etiquetas; opción `--no-schedule`; limpieza ante fallos |
| `tools/sim_asm.py` | Lectura de anotaciones por slot y atribución de la traza a la línea correspondiente |
| `pruebas/bundles/emitir_organizador.c` | Pares de ensambladores escalar/organizado y casos de fronteras |
| `pruebas/bundles/probar_organizador.py` | Ejecución diferencial, verificación de trazas y medición de factorial |
| `makefile` | Compilación del módulo y pruebas integradas en `make check-bundles`/`make check` |

### Cómo selecciona operaciones

Para cada par de operaciones en orden del IR se construyen las dependencias
necesarias. Una lectura del resultado exige dos posiciones de distancia;
reutilizaciones de registros y accesos a memoria conservan al menos una.
El salto final depende además de todas las operaciones anteriores, aunque
no comparta registros con ellas. No se adelantan instrucciones del camino
posterior al salto: ese camino ya es otra región.

En cada ciclo se recorren las operaciones en orden original y se eligen las
que ya satisfacen todas sus dependencias y tienen un slot libre. Una ALU
prefiere su slot original, pero puede usar la otra ALU. La selección puede
saltar una operación temporalmente bloqueada para colocar otra independiente.
Si ninguna puede ejecutarse, se emite un bundle vacío. Esta primera versión
no usa prioridades de camino crítico ni reorganiza instrucciones entre bloques.

Ejemplo real de la multiplicación de factorial, ahora emitido en el bundle 67:

```asm
{ sll r7, r7, r10 | srl r8, r8, r10 | jef __ccom_arith_0_mul | nopi }
```

Los dos desplazamientos son independientes. El salto no consume sus resultados
y todos los slots se ejecutan antes de transferir el control, según ISA §10.
La siguiente vuelta ve los nuevos valores después de la burbuja del salto.

### Fronteras y salidas

`cg_schedule()` reconstruye el texto al terminar la generación. Se llama antes
de `label_table_resolve()` y del ensamblador; `.asm`, `.bin`, `.hex`, la tabla
impresa y la traza usan las mismas direcciones nuevas. El IR conserva las
operaciones y sus direcciones de emisión originales como referencia; no son
las direcciones finales. Las direcciones finales se consultan con
`cg_label_address()` después de organizar.

El pase es idempotente y, una vez finalizado, el emisor rechaza instrucciones
nuevas hasta `cg_init()`. `cg_free()` libera IR, líneas y anotaciones. Si el
organizador falla, la compilación aborta y no ensambla su salida parcial.

Los bundles manuales se copian con sus slots originales. Al comenzar una región
se consideran las escrituras del bundle físico anterior, incluso si era manual.
Si una lectura todavía no puede verlas, se inserta separación. Lo mismo aplica
al entrar a una operación de estado aislada, como `auth`. El pase no corrige
las dependencias entre dos bundles manuales: siguen siendo responsabilidad de
la plantilla que los emite.

Cuando un bundle mezcla líneas del fuente, el listado conserva una línea
principal y agrega las líneas de cada slot, por ejemplo:

```asm
{ nopi | nopi | jef fin_main | sw t0, 68(rz) } # 0026 linea 18 slots 0=0,1=0,2=18,3=17
```

El simulador atribuye el salto a la línea 18 y el almacenamiento a la 17.
`0` significa sin línea asociada o slot vacío. Los listados anteriores,
sin esa extensión, siguen siendo compatibles. Todas las anotaciones son
comentarios; el ensamblador conserva el recorrido de ida y vuelta con `-x`.
Un `.asm` externo con `-x` se ensambla tal como está: este pase solo organiza
la salida del compilador.

### Pruebas y resultados

Verificación final del paso 2:

- `make check` completo pasando, incluido p11.
- 143 ejecuciones equivalentes en registros, memoria y estado crypto,
  incluidas 128 secuencias aleatorias reproducibles.
- Casos de listados largos sin truncar el texto, RAW, WAR, WAW, memoria con alias,
  escritura condicional, salto tomado
  y no tomado, etiquetas, separación explícita, bundles manuales y regiones
  de más de 256 instrucciones.
- Verificación de que un bundle puede usar los cuatro slots.
- Dos casos de entrada sin separación tras un bundle fijo: el organizador
  protege al consumidor ALU y a una instrucción `auth`.
- Fallback a texto idéntico ante desplazamientos numéricos o instrucciones
  desconocidas, e idempotencia de la finalización.
- Almacenamiento y salto en un bundle mixto: líneas individuales correctas
  en anotaciones y en eventos de la traza.
- 26 programas reales comparados con y sin organizador: variables y zona de
  archivos equivalentes, misma imagen inicial, mismo estado crypto y cero
  avisos de separación. No se comparan direcciones de retorno guardadas en
  casillas internas porque las direcciones de código cambian legítimamente.
- AddressSanitizer y UndefinedBehaviorSanitizer sin errores ni fugas al
  organizar y liberar los escenarios, incluidos los aleatorios.

| Medida de `p02_factorial.cls` | Conservador | Organizado |
|---|---:|---:|
| Bundles estáticos | 111 | 81 |
| Bundles vacíos estáticos | 55 | 34 |
| Bundles ejecutados | 309 | 203 |
| Ciclos | 337 | 231 |
| Bundles estáticos con ambas ALU | 0 | 2 |
| Veces que se ejecuta un bundle con ambas ALU | 0 | 14 |
| Resultado de `f` | 120 | 120 |
| Avisos de separación | 0 | 0 |

Son **106 ciclos menos (31,5 %)** para esta entrada, sin modificar el fuente.
No es una garantía de mejora idéntica en todos los programas.

### Reproducir factorial y observar las dos ALU

Comandos compatibles con Fish y Bash, desde la raíz del repositorio:

```sh
make
./ccom -s pruebas/p02_factorial.cls -o build/factorial_paralelo.bin
./ccom --no-schedule -s pruebas/p02_factorial.cls -o build/factorial_escalar.bin
python3 tools/sim_asm.py build/factorial_paralelo.asm --data-image build/factorial_paralelo.mem
python3 tools/sim_asm.py build/factorial_escalar.asm --data-image build/factorial_escalar.mem
less build/factorial_paralelo.asm
make check-bundles
```

Buscar `sll r7` en el ensamblador organizado: comparte bundle con `srl r8`
en la segunda ALU y el salto en BRU. Los artefactos en `build/` son generados
y no necesitan commit.

### Límites que quedan para mejoras posteriores

- La organización es local y conservadora: no cruza etiquetas ni adelanta
  trabajo entre caminos condicionales.
- No realiza análisis de alias: todos los accesos a memoria se mantienen en
  orden, aunque algunas direcciones puedan ser independientes.
- Mantiene una separación al final de cada región y las separaciones explícitas.
  Todavía hay huecos que podrían aprovecharse con análisis entre bloques.
- Conserva sin reorganizar las plantillas manuales de crypto y constantes.
- Con desplazamientos numéricos o instrucciones desconocidas, el programa
  completo conserva la emisión original. Resolver o reubicar esos destinos
  sería otro cambio.
- No cambia la asignación de registros ni agrega spills o desenrollado de bucles.



## Paso 3 — verificación de dependencias (implementado)

Las reglas de RAW/WAR/WAW y memoria se incorporaron en el paso 2 porque eran
necesarias para empaquetar correctamente. Este paso agrega una comprobación
independiente del calendario producido antes de emitir sus bundles.

Decisiones:

- Preparar el calendario completo de cada región en memoria y validarlo antes
  de llamar al callback de salida. Un calendario rechazado no emite esa región.
- Verificar identidad y cobertura de las operaciones: ninguna omitida,
  duplicada o ajena al flujo original.
- Verificar slots compatibles, destinos únicos, separación de resultados y
  orden de reutilización de registros y de todos los accesos a memoria.
- Usar directamente las reglas de la ISA y el orden del IR en el verificador,
  sin consultar el grafo de dependencias construido por el organizador.
- Registrar qué regla, operación, bundle y registros explican cada fallo.
- Añadir pruebas negativas de calendarios deliberadamente incorrectos, además
  de la comparación de ejecuciones válidas y los casos límite del pipeline.
- Mantener las salidas y medidas del paso 2 si su calendario ya era correcto.

### Archivos y funcionamiento del paso 3

| Archivo | Cambio |
|---|---|
| `src/codegen_verify.c` | Verificador independiente de posiciones y efectos |
| `include/codegen_schedule.h` | `cg_verify_schedule()`, categorías de fallo y `CGScheduleIssue` |
| `src/codegen_schedule.c` | Calendario completo, verificación antes de emitir y límite de progreso |
| `pruebas/bundles/probar_dependencias.c` | Calendarios incorrectos y casos válidos de frontera |
| `pruebas/bundles/probar_organizador.py` | Verifica que un rechazo no ejecute callbacks de emisión |
| `makefile` | Módulo y prueba nuevos incluidos en `make check-bundles` |

Cada región se prepara como un arreglo de bundles, con capacidad acotada a
`2 * operaciones + 1`. El organizador guarda allí su propuesta y llama al
verificador. Solo si pasa se ejecutan los callbacks que escriben los bundles
de esa región. El arreglo se libera tanto en éxito como en error.

El verificador reconstruye la posición de cada operación usando su identidad
en el IR y revisa máscaras de lectura/escritura y efectos. No usa la matriz
de aristas ni el arreglo de tiempos del organizador. Puede detectar una
propuesta incorrecta aunque el fallo esté en la construcción del grafo o en
la selección de instrucciones. Ambos módulos comparten los metadatos de
`CGInstruction`: una descripción equivocada debe corregirse en
`codegen_instruction.c` y contrastarse con la ISA y las pruebas.

### Reglas comprobadas

| Regla | Comprobación |
|---|---|
| RAW: escribir y luego leer | Consumidor desde B+2; también se considera el bundle físico previo a la región |
| WAR: leer y luego sobrescribir | No adelantar la escritura ni compartir bundle, según la política conservadora |
| WAW: dos escrituras | Conservar orden y no compartir destino en un bundle |
| Memoria | Conservar orden de todos los accesos, incluidos alias entre `sw`/`sb` y `lw`/`lb` |
| Salto final | No colocarlo antes de trabajo previo; el trabajo independiente puede compartir su bundle |
| Recursos | Slot compatible y destinos únicos |
| Cobertura | Cada operación original exactamente una vez, sin operaciones ajenas |
| Escritura condicional | `ifmov` es productor potencial aunque cierta ejecución conserve el destino |
| `rz` | Sus escrituras se descartan y no crean un resultado que deba esperarse |

WAR es deliberadamente más conservadora que la ISA: el hardware lee antes de
escribir dentro del bundle y permite algunos casos WAR simultáneos. Este paso
mantiene la política del organizador sin cambiar su rendimiento.

`sw` seguido de `lw` a la misma dirección puede quedar en bundles consecutivos:
la memoria no necesita el hueco de los registros (ISA §9.4). Pero si el registro
del dato o la base se produjo recién, sigue exigiéndose RAW de registros.
No se confunden ambas dependencias.

### Diagnósticos y alcance

`CGScheduleIssue` identifica categoría de fallo, operación, operación relacionada
si corresponde, bundle y slot relativos a la región y registros involucrados.
Distingue entrada inválida, operación omitida/duplicada/ajena, slot, destino
duplicado, RAW, WAR, WAW, memoria y control.

Ante un calendario rechazado, el organizador informa motivo y línea del fuente,
devuelve error y no emite esa región. El compilador aborta antes de escribir o
ensamblar su salida parcial. Regiones anteriores pueden existir en el buffer
interno; se liberan al limpiar la compilación.

El chequeo es local: no sustituye el análisis entre caminos de control ni
verifica dependencias internas de plantillas manuales. Estas siguen protegidas
por sus fronteras y comprobadas mediante simulación y pruebas crypto.
`--no-schedule` y el fallback conservador no pasan por este verificador porque
no se reorganiza su IR. No se renombraron registros ni se añadió análisis de
alias o movimiento entre bloques.

### Pruebas y resultados del paso 3

```sh
make check-bundles
make check
```

- 19 calendarios negativos rechazados: RAW en el mismo bundle y el siguiente,
  carga consumida pronto, `ifmov` consumido pronto, escritura pendiente al
  entrar, WAR/WAW reordenados, destino duplicado, memoria invertida, salto
  adelantado, omisión, duplicación, operación ajena, slot y entradas inválidas.
- Calendarios válidos aceptados: B+2, separación de frontera, memoria consecutiva,
  ALU junto a salto independiente, `rz` y región vacía.
- Prueba adicional de latencia incompatible: error con diagnóstico y ningún
  callback de emisión ejecutado.
- `make check` completo pasa, incluidas las 143 ejecuciones diferenciales,
  128 secuencias aleatorias y 26 programas reales del paso 2.
- Los 26 programas organizados antes/después del paso 3 conservan `.asm`,
  `.bin`, `.hex`, `.mem` y tablas idénticos byte por byte.
- AddressSanitizer y UndefinedBehaviorSanitizer pasan para las verificaciones
  negativas y para la organización y liberación de los escenarios completos.

Factorial conserva **80 bundles estáticos, 231 ciclos, resultado 120 y cero
avisos**, con ambas ALU activas 14 veces durante la ejecución. El paso 3 añade
verificación durante la compilación; no cambia los bundles válidos del paso 2.

## Paso 5 — direcciones y archivos finales (verificado)

La finalización ya se conectó en el paso 2 porque era necesaria para ejecutar
el código agrupado. En este paso se comprueba específicamente que ningún
consumidor use las direcciones conservadoras del IR en lugar de las finales.

Decisiones:

- Mantener el orden actual: generar IR → organizar bundles → resolver etiquetas
  finales → escribir listado → ensamblar y escribir `.bin`/`.hex`/`.mem`.
- Contrastar cada dirección anotada con el conteo real de bundles del `.asm`.
- Decodificar directamente los campos BRU del `.bin` y comprobar los destinos
  relativos contra las etiquetas del `.asm`, incluidos saltos hacia atrás.
- Comprobar registros de enlace y fuente de `jas`/`jefR`, además del destino.
- Contrastar encabezado, tamaño, orden de palabras/slots y datos del `.bin`
  con `.hex` y `.mem`; comprobar también la salida `-p`.
- Reensamblar con `-x` y comparar instrucciones, sin exigir que el `.asm`
  contenga la imagen de datos: esa imagen se entrega aparte en `.mem`.
- Probar las 26 entradas compilables en modo organizado y conservador, y
  demostrar que las etiquetas de factorial cambian después del empaquetado.

### Cambios y resultados del paso 5

No fue necesario cambiar la resolución de etiquetas ni la codificación: ya
consumían el calendario final desde el paso 2. Se precisó el comentario de
`src/main.c` para dejar explícito el orden y evitar que un cambio futuro use
las direcciones conservadoras del IR por error.

Se agregó `pruebas/bundles/probar_salidas.py` y el objetivo `make check-salidas`,
incluido también en `make check`. Comprueba cada programa compilable en modo
organizado y con `--no-schedule`:

1. Los índices anotados del `.asm` son consecutivos y corresponden al conteo
   real de bundles. Etiquetas de entrada y tabla final del listado coinciden
   con ese conteo; las etiquetas resueltas de `-m` tienen la misma dirección.
2. El encabezado `.bin` tiene el número final de bundles, punto de entrada 0,
   tamaño correcto de datos y dirección inicial de datos 0. El tamaño total
   es `16 + 16 * bundles + bytes_de_datos`.
3. Se leen las palabras del `.bin` en little endian y orden slot 0..3. La
   representación `.hex` es exactamente slot 3..0, y la salida `-p` coincide.
4. Los bytes finales de datos coinciden con las palabras de `.mem` en little
   endian. Las etiquetas de código no cambian las direcciones de estas variables.
5. Se extrae el opcode BRU y su desplazamiento con signo: 18 bits para saltos
   condicionales y 22 para `jef`/`jas`. `PC + desplazamiento` coincide con la
   dirección final de la etiqueta. También se comprueban registros de comparación
   y enlace. Para `jefR` se verifican fuente, destino y bits reservados; su
   destino dinámico se cubre mediante las pruebas de ejecución de llamadas.
6. Reensamblar el `.asm` con `-x` produce las mismas palabras de instrucciones
   en `.bin` y el mismo `.hex`. El `.bin` reensamblado no incorpora los datos
   de las variables: el `.asm` textual solo entrega instrucciones y comentarios,
   mientras la imagen de datos se conserva en el `.mem` del compilador.

Resultados:

- **26 programas × 2 modos** con salidas y direcciones coherentes.
- **720 instrucciones BRU** codificadas correctamente, entre ambas modalidades.
- Reubicación demostrada: `.L_func_factorial` pasa de **38 a 28**, y el código
  de la llamada usa el desplazamiento calculado hacia 28. Factorial pasa de
  110 a 80 bundles sin cambiar su resultado.
- `make check` completo pasa, incluida esta prueba y las de dependencias,
  trazas, etiquetas, ensamblado y ejecución existentes.

Para repetir solo este paso:

```sh
make check-salidas
```

Las direcciones de `CGIRRecord.bundle_address` siguen siendo las de la emisión
original. Para la dirección final se usa `cg_label_address()` después de
`cg_schedule()`; listado, tabla y ensamblador ya utilizan esa secuencia.

## Paso 6 — demostración y aceptación final (completado)

La prueba principal sigue siendo `pruebas/p02_factorial.cls`, sin modificarlo.
Se generará un reporte reproducible que compare emisión conservadora y
organizada y conserve los ensambladores, las trazas y métricas de ambas.

Criterios de aceptación:

- Ambos modos obtienen `5! = 120` y terminan sin avisos de separación.
- El modo organizado ejecuta realmente bundles con ambas ALU, no solo los
  contiene en una rama que nunca se visita.
- Se reducen los ciclos, bundles estáticos y bundles vacíos de esta prueba.
- Se verifican también entradas 0, 1, 2, 10, 12 y negativa con una referencia
  independiente; 12! todavía cabe en entero de 32 bits con signo.
- El reporte incluye los bundles paralelos concretos y cuántas veces se
  ejecutó cada uno, además de los comandos para reproducir la comparación.
- La aceptación específica de factorial complementa, sin reemplazarlas, las
  pruebas de llamadas, arreglos, cortocircuito, crypto, dependencias y formatos.

### Implementación y artefactos del paso 6

Se agregó `tools/validar_bundles.py` y el objetivo `make validar-bundles`, que
forma parte de `make check`. Usa el compilador y el simulador reales, conserva
p02 sin modificarlo y escribe en `build/validacion_bundles/`:

- `reporte.md`: comparación y evidencia de los bundles paralelos.
- `reporte.json`: métricas y resultados legibles por otro programa.
- `factorial_escalar.*` y `factorial_paralelo.*`: `.asm`, `.bin`, `.hex`, `.mem`,
  salida de compilación, reporte de ejecución y traza de cada modalidad.

Las variantes de entrada se crean y prueban en una carpeta temporal; no se
modifica `pruebas/p02_factorial.cls`. Su referencia de resultado usa
`math.factorial()` para entradas no negativas y verifica el retorno 0 de p02
para -1. El script termina con código 1 si falla un criterio y elimina sus
reportes anteriores antes de empezar para evitar mostrar una aceptación vieja.

El contador instrumenta las visitas reales del simulador a cada bundle.
En la versión aceptada:

```asm
# Bundle 54, ejecutado 4 veces:
{ mov r10, 1 | mov r6, 0 | nopi | lw t0, 4(rz) }

# Bundle 67, ejecutado 10 veces:
{ sll r7, r7, r10 | srl r8, r8, r10 | jef __ccom_arith_0_mul | nopi }
```

Son **14 ejecuciones de bundles con ambas ALU**, además de compartir trabajo
con LSU o BRU. Los números de bundle pueden cambiar con futuras mejoras; la
aceptación no exige esos índices concretos, sino paralelismo ejecutado y
reducción de ciclos, bundles estáticos y bundles vacíos.

### Resultado de aceptación

Comparación completa del mismo programa, factorial de 5:

| Medición | Antes: escalar | Ahora: organizado |
|---|---:|---:|
| Bundles generados | 111 | 81 |
| Bundles vacíos | 55 | 34 |
| Bundles con varias instrucciones | 0 | 7 |
| Bundles usando ambas ALU | 0 | 2 |
| Ocupación de slots | 12,61 % | 17,28 % |
| Ciclos ejecutados | 337 | 231 |
| Resultado | `5! = 120` | `5! = 120` |
| Avisos de separación | 0 | 0 |

Los conteos de bundles de esta tabla son estáticos: cuentan las posiciones
del ensamblador generado. Los 2 bundles con ambas ALU se ejecutan 14 veces
por las vueltas del ciclo. Los ciclos son una medición de la ejecución.

La reducción es de 30 bundles generados y 106 ciclos (31,45 % menos ciclos).
La ocupación solo aumenta 4,67 puntos porcentuales y sigue siendo baja: aún
hay muchos slots vacíos. Esta primera versión demuestra paralelismo correcto,
pero todavía deja margen para mejorar la calendarización y las separaciones
conservadoras descritas anteriormente.

| Criterio | Resultado |
|---|---|
| Factorial de 5 | 120 en ambos modos |
| Ciclos | 337 → 231, reducción de 31,45 % |
| Bundles estáticos | 111 → 81 |
| Bundles vacíos | 55 → 34 |
| Bundles ejecutados | 309 → 203 |
| Ocupación estática de slots | 12,61 % → 17,28 % |
| Dos ALU ejecutadas | 0 → 14 veces |
| Avisos de separación | 0 en ambos modos |
| Entradas adicionales | -1, 0, 1, 2, 10 y 12 correctas en ambos modos |
| Verificación completa | `make check` pasa |

Esta medición usa el modelo de la ISA, no ejecución en RTL. La mejora es la
observada para p02 con entrada 5; programas con menos operaciones independientes
pueden mejorar menos. Se conservan las limitaciones locales y conservadoras
documentadas en los pasos anteriores.

### Cómo verlo y repetirlo

Desde la raíz del repositorio (Fish o Bash):

```sh
make validar-bundles
less build/validacion_bundles/reporte.md
less build/validacion_bundles/factorial_paralelo.asm
less build/validacion_bundles/factorial_paralelo.traza.txt
```

Para ejecutar el mismo ensamblador de nuevo:

```sh
python3 tools/sim_asm.py build/validacion_bundles/factorial_paralelo.asm --data-image build/validacion_bundles/factorial_paralelo.mem
python3 tools/sim_asm.py build/validacion_bundles/factorial_escalar.asm --data-image build/validacion_bundles/factorial_escalar.mem
```

Los reportes y artefactos en `build/` son generados y están ignorados por Git;
lo que se versiona es el script, las pruebas y esta documentación. La aceptación
es reproducible con `make validar-bundles` y también se ejecuta en `make check`.


## Corrección del bundle vacío terminal — 14 de octubre

El último bundle vacío provenía del salto a sí mismo de `.L_arith_error`:
`cg_op()` agregaba su separación automática y el organizador conservaba una
separación de frontera. No había consumidor ni continuación después de ese
salto terminal, por lo que ese bundle era innecesario.

`src/generate.c` ahora emite el manejador como bundle completo con `cg_bundle()`,
igual que el final de main. No se eliminaron separaciones internas ni se cambió
el orden del resto del programa. `probar_salidas.py` verifica en ambos modos
que el último bundle sea el salto terminal y no un bundle vacío.

`make check` completo pasó y regeneró los reportes. Las mediciones anteriores
en este documento describen la versión previa a esta corrección. La comparación
actual del mismo factorial de 5 es:

| Medición | Escalar actual | Organizado actual |
|---|---:|---:|
| Bundles generados | 110 | 80 |
| Bundles vacíos | 54 | 33 |
| Bundles con varias instrucciones | 0 | 7 |
| Bundles con ambas ALU | 0 | 2 |
| Ocupación de slots | 12,73 % | 17,50 % |
| Ciclos | 337 | 231 |
| Resultado | 120 | 120 |
| Avisos | 0 | 0 |

Los ciclos no cambian: ese último bundle no se ejecutaba. La mejora consiste
en no incluir una palabra de paquete completa (16 bytes) inútil al final del
código. Las direcciones de las etiquetas existentes tampoco cambian.
