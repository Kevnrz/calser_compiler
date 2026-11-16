# Diseño integral de `ccom` y plan del refactor (Proyecto II, CE1108, Grupo 6)

Documento del "refactor" (tarea C6, fases 3 a 5). Lo lleva el chat del refactor. Fuente de
verdad: el enunciado del Proyecto II. Terminología: Aho et al. y las notas del curso; lo que
sale de otra fuente se indica.

## 0. Estado

| Parte | Contenido | Estado |
|---|---|---|
| 1 | Requisitos del enunciado | Escrita |
| 2 | Mapa de componentes | Escrita |
| 3 | Flujos y datos entre componentes | Escrita |
| 4 | Decisiones tomadas, incluido lo que Calser permite (4.5) y las decisiones del 15/11 (4.6) | Escrita |
| 5 | Diseño contra el código actual | Escrita |
| 6 | Efecto de la integración del IDE sobre el léxico y el parser | Escrita |
| 7 | Contrato de integración de las optimizaciones | Corregida contra el TAC real (15/11) |
| 8 | Conexión con el IDE y las métricas | Escrita |
| 9 | Plan de implementación del refactor (pasos P0 a P15) y su cierre | P0–P12, P14 y P15 integrados en `develop`; P13 pospuesto |
| 10 | Lo que se queda como está | Escrita |
| 11 | Diseño técnico de las optimizaciones (estructuras, pseudocódigo, casos de prueba) | Corregida contra el TAC real (15/11) |
| 12 | Enlazado de archivos (`P`) | Escrita |
| 13 | IDE y medición (archivo `.tac`, nivel y pasadas en F5, medición, gráficas) | Escrita |
| 14 | Las 15 pruebas obligatorias | Corregida contra el TAC real (15/11) |
| 15 | Avisos para el cerebro | Escrita |
| A | Anexo: decisiones vigentes del camino optimizante (absorbe `diseno_camino_optimizante.md`) | Escrito |

**Estado (15/11).** El refactor está integrado en `develop` (`0195d1e`, PR #25). Los pasos se
revisaron por partes en los PR #18 a #24 hacia `refactor-c6`; P15 entró por el PR #24
(`3d3993a`, merge `66c7d54`). P13 (cascadas) queda pospuesto para la etapa del IDE y de los
errores (9.14).

El 15/11 el diseño de las optimizaciones (partes 4.4, 7, 11 y 14) se revisó contra el TAC real
que genera el constructor y contra lo que Calser permite y rechaza (4.5), con una revisión
del agente sobre 75 programas y el corpus. Se corrigieron esperados, supuestos y cinco
puntos de algoritmo (4.6). Siguen las optimizaciones, cada una en su rama y su PR hacia
`develop`, con este diseño como especificación.

Para retomar en otro chat: leer este apartado, 4.4 a 4.6, 9.0 y la sección de la parte 7 y 11
de la optimización que toque.

**Base del análisis.** Código de `Kevnrz/calser_compiler`, rama `develop`. El chat no puede
compilar ni correr suites (no hay Flex): las afirmaciones sobre comportamiento salen de leer
el código y de los reportes del agente, que sí ejecuta; cada paso las confirma con las
protecciones.

## 1. Qué tiene que hacer el sistema

Requisitos de CE1108 tomados del enunciado. Cada componente de la parte 2 responde al menos a
uno.

| ID | Requisito | Enunciado |
|---|---|---|
| R1 | Léxico y parser que acepten la sintaxis de Calser | §4.1 |
| R2 | Enlazado entre archivos | §4.1 (decisión D1: `P "modulo" ;`) |
| R3 | Corregir los aspectos del Proyecto I, sintácticos y semánticos | §4.1 |
| R4 | Tablas implementadas de manera eficiente | §4.1 |
| R5 | Representación intermedia de tres direcciones, útil para optimizar, con bandera para generarla | §4.1 |
| R6 | Identificación de bloques básicos | §7.2 iteración 2 |
| R7 | Loop unrolling con factor configurable y heurística; parcial y total | §4.2, §6 |
| R8 | Eliminación de código muerto usando liveness | §4.2, §6 |
| R9 | Reordenamiento seguro dentro de bloques básicos (RAW, WAR, WAW y control) | §4.2, §6 |
| R10 | Spilling de registros | §4.2, §6 |
| R11 | Renombramiento (dependencias WAW y WAR) | §4.3, §6 |
| R12 | Calendarización estática y empaquetado en bundles: slots, unidad funcional, dependencias, NOPs; sin forwarding del hardware | §4.2, §3.1.1 |
| R13 | Binario o ensamblador ejecutable en el procesador VLIW | §4.2 |
| R14 | Bandera para ver la calendarización de cada paquete | §4.2 |
| R15 | Simulación por ciclos | §4.3 |
| R16 | IDE: sugerencias de código al escribir | §4.4 |
| R17 | IDE: correcciones automáticas (terminación de línea, cierres de bloque, corchetes, paréntesis) | §4.4 |
| R18 | IDE: errores marcados gráficamente y con buena descripción | §4.4 |
| R19 | IDE: clones Tipo I y Tipo II, entre archivos | §4.4 |
| R20 | 15 pruebas obligatorias, 3 por optimización, cada una observable de forma aislada | §6 |
| R21 | Métricas por prueba, en forma gráfica: instrucciones antes y después, bytes, ciclos del simulador, tiempo de compilación, eliminadas, reordenadas, renombradas | §6 |
| R22 | Compilar los benchmarks con distintos niveles de optimización y medir en el hardware | §7.3 |
| R23 | Para cada optimización: regla, precondiciones, transformación y ejemplos | §6 |
| R24 | El ISA no cambia; 4096 bundles como máximo en la ROM | §3.1.2, §3.2.1 |

## 2. Mapa de componentes

Cuatro bloques: el compilador `ccom` (A), el IDE (B), ejecución y medición (C) y verificación
(D). Esta parte describe **qué debe existir**; la comparación con el código está en la parte 5.

### A. Compilador `ccom`

#### A1. Conductor

| | |
|---|---|
| Responsabilidad | Leer la línea de comandos, decidir el modo y ejecutar las fases en orden; liberar todo al final |
| Modos | Compilar (niveles `-O0`, `-O1`, `-O2`, o pasadas sueltas con `--opt=`); ensamblar un `.asm` (`-x`); consultas del IDE (exclusivas) |
| Entrega | Código de salida: 0 bien, 1 error de compilación o de archivos, 2 mal uso |
| Reglas | Con errores de análisis no se genera ni se optimiza. Las consultas del IDE no admiten banderas de compilación |
| Requisitos | R5, R7, R14, R22 |

#### A2. Análisis (*front end*)

| Componente | Responsabilidad | Recibe | Entrega | Requisitos |
|---|---|---|---|---|
| A2.1 Enlazado de módulos | Resolver `P "modulo" ;` y reunir los archivos en un solo programa; cada archivo una vez | Archivo principal | Un único AST; errores de módulo no encontrado y de `P` fuera del inicio | R2 |
| A2.2 Analizador léxico | Convertir el texto en tokens con posición | Texto | Tokens; errores léxicos | R1 |
| A2.3 Analizador sintáctico | Construir el AST; recuperarse de un error para reportar varios | Tokens | AST **aceptado** o rechazo. Aceptado: sin errores léxicos ni sintácticos | R1, R3 |
| A2.4 Analizador semántico | Ámbitos, tipos, constantes, firmas, llamadas sin recursión; anotar cada identificador con su símbolo | AST aceptado | AST anotado y tabla de símbolos; errores semánticos | R3, R4 |
| A2.5 Asignación de memoria de datos | Dirección de globales y un área estática por función (retorno, parámetros, locales, casillas de preservación, extensión de derrame) | Tabla de símbolos | Direcciones y áreas | R10, R13 |
| A2.6 Diagnósticos | Presentar cada error con archivo, línea, columna y descripción, en texto o en datos para el IDE | Errores de todas las fases | Mensajes | R3, R18 |

#### A3. Representación intermedia de tres direcciones

| Componente | Responsabilidad | Entrega | Requisitos |
|---|---|---|---|
| A3.1 Forma | Instrucciones de tres direcciones, bloques básicos y grafo de flujo. Tres etapas sobre la misma estructura: alto nivel, ISA virtual (temporales ilimitados) e ISA física (registros reales). Edición, validación e impresión | La estructura que usan A3 a A5 | R5, R6 |
| A3.2 Construcción | Traducir el AST a alto nivel, formar los bloques básicos y marcar los lazos `S` y `L` | Representación de alto nivel | R5, R6 |
| A3.3 Análisis de flujo de datos | Variables vivas (liveness) con el estado observable como raíz; lazos naturales y profundidad; intervalos de vida | Conjuntos de vivos, lazos, intervalos | R7, R8, R10 |

**Operaciones complejas.** `*`, `/`, `%`, `^` y las siete de CRYPTO no existen en el ISA. En la
representación son una sola instrucción en todas las etapas y se expanden al emitir, con las
rutinas de máquina (A5.4). No se desglosan dentro de la representación: hacerlo obligaría a
escribir la división, la potencia y el cifrado una segunda vez, mientras `-O0` sigue
necesitando las rutinas del Proyecto I byte a byte. Costo aceptado: las optimizaciones las ven
como una caja cerrada (no se reordenan por dentro, piden 8 auxiliares, el destino no puede
coincidir con una fuente).

**Frontera virtual/física.** Todo lo anterior a la asignación de registros (A5.3) trabaja con
temporales virtuales; todo lo posterior, con los 16 registros reales. `-O0` entra directo al
lado físico. Dos piezas solo se consideran "la misma" si están del mismo lado.

#### A4. Optimizador

| Componente | Etapa | Responsabilidad | Requisitos |
|---|---|---|---|
| A4.1 Gestor de pasadas | — | Ejecutar las pasadas activas en orden fijo, validar la representación después de cada una, medir su tiempo | R7–R11, R21 |
| A4.2 Unrolling | Alto nivel | Replicar el cuerpo de un lazo `S` según un factor (dado o por heurística); total si N es constante y pequeño; lazo de residuo si no | R7 |
| A4.3 Renombramiento | Alto nivel | Dentro de un bloque, dar un temporal nuevo a las redefiniciones de una variable local escalar (quita WAW y WAR) | R11 |
| A4.4 DCE | Alto nivel | Quitar asignaciones cuyo resultado no está vivo y bloques inalcanzables, sin tocar lo que tiene efectos | R8 |
| A4.5 Reordenamiento | ISA virtual | Cambiar el orden dentro de un bloque para exponer paralelismo, respetando dependencias | R9 |

Orden fijo: unrolling → renombramiento → DCE → (bajada) → reordenamiento → (asignación con
spilling). `-O1` es el camino sin pasadas, la línea base del "antes"; `-O2` las activa todas.

#### A5. Generación de código (*back end*)

| Componente | Responsabilidad | Recibe | Entrega | Requisitos |
|---|---|---|---|---|
| A5.1 Generador directo (`-O0`) | Traducir el AST anotado a instrucciones con registros reales. Es el generador del Proyecto I y la referencia de corrección | AST anotado | Lista de instrucciones | R13 |
| A5.2 Selección de instrucciones (bajada) | Pasar de alto nivel a ISA virtual | Alto nivel | ISA virtual | R13 |
| A5.3 Asignación de registros y spilling | Pasar de temporales a registros reales. `-O1`: *linear scan* (fuera de Aho), sin derrame. Con `--opt=spill` o `-O2`: coloreo de Chaitin con derrame a memoria (7.6) | ISA virtual, intervalos, profundidad de lazos | ISA física; plan de preservación; contadores de derrame | R10 |
| A5.4 Emisión y rutinas de máquina | Escribir las instrucciones finales. Las rutinas de máquina son las secuencias fijas que usan los dos caminos: aritmética por software, arreglos, CRYPTO, pila, preservación de registros, arranque y fin del programa | ISA física (o llamadas de A5.1) | Lista de instrucciones | R13 |
| A5.5 Calendarización y empaquetado | Asignar cada instrucción a un slot según su unidad funcional y sus dependencias (RAW a distancia 2; WAR, WAW y memoria a distancia 1); rellenar con NOPs; verificar | Lista de instrucciones | Bundles | R12, R14 |
| A5.6 Ensamblado y salidas | Resolver etiquetas en dos pasadas, codificar, comprobar el límite de la ROM y escribir `.asm`, `.bin`, `.hex`, `.mem` | Bundles en texto | Archivos | R13, R24 |
| A5.7 Modelo del ISA | Fuente única de lo que define la máquina: instrucciones, unidad y slot, efectos, registros y su papel, rango de inmediatos, descomposición de constantes, tamaño de la ROM | — | Consultas para A3, A5 y C1 | R24 |

#### A6. Métricas de compilación

Reúne y escribe un JSON por compilación: instrucciones antes y después de cada pasada,
emitidas, bundles, NOPs, bytes, demanda y presión de registros, tiempo total y por pasada, y
los contadores de cada optimización. Requisito R21.

#### A7. Servicios para el IDE

Motores en C dentro de `ccom` (decisión D9). Cada uno es una consulta: recibe una instantánea
del texto y responde en JSON por la salida estándar. No generan código.

| Componente | Bandera | Responsabilidad | Requisitos |
|---|---|---|---|
| A7.1 Comprobación | `--check`, `--check-json` | Analizar el programa y devolver todos sus errores con rango y descripción | R18 |
| A7.2 Tokens | `--tokens-json` | Clasificar cada token y comentario para el resaltado | R18 |
| A7.3 Verificación sintáctica | `--syntax-json` | Decir si un texto es sintácticamente aceptado | R17 |
| A7.4 Sugerencias | `--suggest-json` | Proponer lo que puede seguir en el cursor, según lo que el parser espera | R16 |
| A7.5 Correcciones | `--repair-json` | Proponer inserciones de cierre (`;`, `.`, `]`, `)`) y validar cada una volviendo a analizar | R17 |
| A7.6 Clones | `--clones-json` | Detectar fragmentos iguales (Tipo I) o iguales salvo identificadores y literales (Tipo II) entre varios archivos | R19 |
| A7.7 Protocolo | — | Cabecera común, escapado de cadenas, límites, estados y códigos de salida | R16–R19 |

### B. IDE (CALSIDE, Python con PySide6)

El IDE no analiza Calser: todo lo que sabe del lenguaje lo pregunta a `ccom` (A7).

| Componente | Responsabilidad | Usa | Requisitos |
|---|---|---|---|
| B1 Editor | Editar; resaltado; subrayar los errores en su posición | A7.1, A7.2 | R18 |
| B2 Panel de problemas | Listar los errores con su descripción y llevar al lugar | A7.1 | R18 |
| B3 Sugerencias | Mostrarlas mientras se escribe (lista y texto en línea) y aplicarlas | A7.4 | R16 |
| B4 Correcciones | Mostrar las propuestas, aplicarlas con deshacer y, si la política lo permite, aplicarlas solas | A7.5, A7.3 | R17 |
| B5 Clones | Reunir los archivos del proyecto, pedir la detección y mostrar los pares | A7.6 | R19 |
| B6 Compilar (F5) | Lanzar `ccom` con un perfil de banderas, incluido nivel y pasadas; mostrar salida y errores | A1 | R22 |
| B7 Ejecutar (F6) | Correr el programa compilado en el simulador y mostrar resultado y traza | C1 | R15 |
| B8 Métricas | Mostrar en gráficas las métricas de cada prueba | A6, C2 | R21 |
| B9 Adaptadores | Lanzar `ccom` como proceso, validar cada respuesta contra el protocolo, convertir posiciones | A7.7 | R16–R19 |
| B10 Proyecto | Explorador, archivos, búsqueda, terminal, preferencias | — | — |
| B11 Material LL(1) | Gramática LL(1), PRIMERO, SIGUIENTE y tabla; estudio y documentación | — | Documentación |

### C. Ejecución y medición

| Componente | Responsabilidad | Requisitos |
|---|---|---|
| C1 Simulador por ciclos | Ejecutar el ensamblador generado con el modelo VLIW de calendarización estática: registros, memoria, CRYPTO, avisos de separación RAW, traza y ciclos | R15, R21 |
| C2 Medición y gráficas | Por prueba y optimización: compilar antes y después, tomar el JSON de A6 y los ciclos de C1, tabular y graficar | R21 |
| C3 Procesador de arqui (externo) | Ejecutar el binario en el RTL con caché y contadores | R22 |

### D. Verificación

| Componente | Responsabilidad | Requisitos |
|---|---|---|
| D1 Regresión | Suites que fijan el comportamiento de cada componente; `-O0` byte a byte | R3 |
| D2 Verificación de pasadas | Alto nivel: ejecutar la representación antes y después y comparar el estado observable. ISA: comparar en C1 contra `-O0`. Validación de forma tras cada pasada | R7–R11 |
| D3 Pruebas obligatorias | Los 15 programas, 3 por optimización | R20 |
| D4 Benchmarks de integración | Programas para medir en C3 con cada nivel | R22 |

## 3. Flujos y datos entre componentes

### 3.1 Compilación

```
fuentes ──A2.1──► texto ──A2.2──► tokens ──A2.3──► AST aceptado ──A2.4, A2.5──► AST anotado
                                                                                  │
            ┌────────────────────────── -O0 ──────────────────────────────────────┤
            │                                                                     │ -O1 / -O2
            ▼                                                                     ▼
     A5.1 generador directo                                   A3.2 construcción ► alto nivel
            │                                                 A4.2 ► A4.3 ► A4.4  (pasadas)
            │                                                 A5.2 bajada ► ISA virtual
            │                                                 A4.5 reordenamiento
            │                                                 A5.3 asignación y spilling ► ISA física
            │                                                                     │
            └──────────────► A5.4 emisión y rutinas de máquina ◄──────────────────┘
                                          │  lista de instrucciones
                                          ▼
                              A5.5 calendarización ► bundles
                                          ▼
                              A5.6 ensamblado ► .asm .bin .hex .mem
```

### 3.2 Consulta del IDE

```
B1…B5 ──► B9 adaptador ──► ccom (A1 en modo consulta) ──► A7.x ──► A2.2, A2.3, A2.4
                 ▲                                           │
                 └──────────── JSON del protocolo A7.7 ◄─────┘
```

### 3.3 Medición de una optimización

```
prueba ──► ccom -O1 ───────────────► JSON (A6) + .asm ──► C1 ──► ciclos ─┐
prueba ──► ccom -O1 --opt=pasada ──► JSON (A6) + .asm ──► C1 ──► ciclos ─┴► C2 ► datos ► gráficas (B8)
```

### 3.4 Datos que cruzan entre componentes

| Dato | Lo produce | Lo consume |
|---|---|---|
| Tokens con posición | A2.2 | A2.3, A7.2, A7.4, A7.6 |
| AST aceptado | A2.3 | A2.4 |
| AST anotado y tabla de símbolos con áreas | A2.4, A2.5 | A3.2, A5.1, A5.4 |
| Representación de alto nivel | A3.2 | A4.2, A4.3, A4.4, A5.2, D2 |
| Representación ISA virtual | A5.2 | A4.5, A5.3 |
| Representación ISA física y plan de preservación | A5.3 | A5.4 |
| Lista de instrucciones | A5.1, A5.4 | A5.5 |
| Bundles | A5.5 | A5.6 |
| Diagnósticos | Todas las fases | A2.6, A7.1 |
| JSON de consulta | A7 | B9 |
| JSON de métricas | A6 | C2, B8 |
| Estado final, traza y ciclos | C1 | B7, C2, D2 |

## 4. Decisiones tomadas

### 4.1 Criterios

| Tema | Decisión |
|---|---|
| Naturaleza | Diseño funcional de un proyecto académico. Se unifica donde quita un riesgo real de que los dos caminos se contradigan, donde una optimización lo necesita para engancharse, o donde corrige un defecto. No se agrega estructura que no haga falta: complica y es menos defendible |
| Punto de partida | El código existente se mantiene salvo razón concreta. `-O0` y `-O1` funcionan y no se pueden romper |
| Frontera virtual/física | Solo se unifican piezas que trabajan del mismo lado de la asignación de registros |
| Renombres | Solo lo que un paso toca, más el código nuevo. Los nombres muy usados (`TAC_P1_*`, `TEMP_FIRST`) se conservan y pasan a apuntar a la definición única |
| Contrato con el IDE | Puede cambiar el contenido de una respuesta si el IDE lo absorbe sin tocar su código. El IDE en sí queda fuera de esta tarea |
| Organización | Misma estructura de carpetas en `src/`, `include/` y `pruebas/`, una por componente (P1) |
| Protecciones de cada paso | (1) `-O0` byte a byte: mismos `.bin`, `.hex`, `.mem`, `.asm` y salida estándar; (2) `make check-o1-equivalence`: los 39 programas dan en `-O1` el mismo estado observable que en `-O0`; (3) `make check` y `make ejercicios`; (4) respuestas de las consultas del IDE sin cambios |

### 4.2 Decisiones de diseño

| Tema | Decisión | Razón |
|---|---|---|
| `rx` | Sigue siendo el puntero de pila | Convención del Proyecto I; cambiarla rompe `-O0` |
| `rg` | Registro auxiliar del derrame en `-O1`/`-O2`: se carga, se usa y se suelta; nunca queda vivo entre instrucciones, así que no necesita casilla de preservación. Con dos fuentes derramadas en la misma instrucción, el segundo auxiliar es el registro del destino (la instrucción lo va a sobrescribir) o, si no hay, un registro prestado con preservación | Le da un uso real. El hardware lo trata como un registro cualquiera |
| `sra`, `ifmov`, `xori`, `lb`, `sb` | No se emiten. Calser no tiene tipo de un byte (`lb`, `sb`) ni operador de desplazamiento (`sra`); `!` ya cuesta una instrucción con `seq` (`xori`); no hay optimización pedida que use copia condicional (`ifmov`). `ccom -x` las sigue aceptando y codificando | Ningún uso mejora el código generado |
| Límite de 4096 bundles | Una comprobación, en el ensamblado, por donde pasan `-O0`, `-O1`/`-O2` y `-x`. Se rechaza la imagen de la ROM (`.bin`, `.hex`, `.mem`); el `.asm` puede quedar escrito | Es un límite de la ROM del procesador, no del lenguaje. Corrige un defecto del Proyecto I ya registrado |
| `--no-schedule` | Se mantiene como bandera de verificación, no de uso | Es la referencia para probar el organizador (`tools/validar_bundles.py`) y da el dato del "antes" (110 → 80 bundles en el ejercicio 1) |
| Rango del inmediato de 18 bits | Una función en el modelo del ISA; hoy está en 12 puntos de 8 archivos | Es una restricción del ISA |
| Funciones compartidas del generador `-O0` | Pasan a un módulo de rutinas de máquina, para que la emisión de `-O1` no dependa del generador del otro camino | Es donde el spilling buscará cargar y guardar |
| Operaciones complejas | Una tabla en la representación: cada operación de alto nivel con su forma pendiente de expansión; las propiedades de comportamiento se escriben una vez y la forma pendiente las hereda. La elección de la rutina del Proyecto I queda en la emisión, indexada por nombre y no por posición | Hoy la lista de las 11 está en 5 lugares y 4 puntos dependen del orden de los enumerados. Las 11 filas coinciden campo por campo en lo que describe comportamiento |
| Interruptor del derrame | `spill` sigue en la tabla de pasadas (da el nombre para `--opt` y para el JSON) como política de la asignación: el gestor no ejecuta nada por ella. La asignación recibe el interruptor en una estructura propia y devuelve tiempo y contadores en su resultado | `tac_metrics.h` incluye a `tac_allocate.h`: la asignación no puede depender del gestor ni de las métricas sin crear un ciclo |
| Escapado de cadenas JSON | Una pieza neutral con el escapador estricto (el que hoy usa el IDE, sin cambiarle un byte). La usan `--ast-json`, `--metrics` y las consultas. Los tres JSON siguen siendo distintos | Hoy hay dos, y el de `ast.c` deja pasar bytes que no son UTF-8 válido |
| Verificación de programas con CRYPTO | El intérprete de alto nivel los rechaza; se verifican con el comparador contra `-O0` en el simulador | Enseñarle CRYPTO al intérprete sería escribir el cifrado otra vez |

### 4.3 Lo que se decidió no cambiar

Ver la parte 10.

### 4.4 Alineación con el curso (15/11)

Revisión de las optimizaciones contra las presentaciones del curso (*Optimización de Código*,
*Spilling de Registros*, *Optimización para el Paralelismo y la Localidad*).

| Tema | Curso | Diseño | Resultado |
|---|---|---|---|
| Validez | La salida observable (valores, errores, no terminación) no cambia | Regla 11.1 | Alineado |
| Bloques y grafo de flujo | Líderes: la primera instrucción, los destinos de salto y la instrucción que sigue a un salto; arista cuando el control puede pasar de un bloque a otro | Los bloques se forman al construir el TAC; las caídas (la instrucción que sigue a un salto no tomado) son aristas del grafo | Alineado |
| TAC de estructuras de control | `while`: `if` hacia el cuerpo y `goto` a la salida | La condición se invierte y el cuerpo se alcanza por caída (Aho §6.6.5, evitar saltos redundantes): una instrucción menos por lazo | Equivalente; alineado con el libro |
| TAC de variables | `E → id { E.place = id.place }`: la variable se usa directo | P15 (9.18) | Alineado |
| DCE | Local (resultado no usado, sin efectos) y global con variables vivas, repitiendo hasta punto fijo | 7.2, 11.2. Además se quitan bloques inalcanzables (Aho §8.7.2), que no están en la presentación | Alineado, más un complemento del libro |
| Renombramiento | Nombre nuevo por temporal para liberar el bloque | 7.3, 11.3: los temporales ya nacen únicos; se renombran las variables locales | Alineado |
| Reordenamiento | Intercambio de instrucciones independientes; RAW, WAR, WAW | 7.5, 11.5 | Alineado |
| Unrolling | No está en las presentaciones; Aho §10.5.1 y notas de arqui §5.2.2 | 7.4, 11.4 | Alineado con el libro |
| Spilling | Coloreo de grafos de Chaitin; víctima por costo/grado con 10^profundidad | El spilling (`--opt=spill`, `-O2`) asigna y derrama con Chaitin (7.6, 11.6). `-O1` conserva su asignación actual, sin derrame | Alineado |

**Decisión 1 (P15).** El TAC usa la variable directo, como el curso. Solo se copia a un temporal
si queda una llamada a función por evaluar en la misma construcción, o en `R`, para conservar
la asignación simultánea de parámetros antes de sobrescribirlos. Integrada (9.18).

**Decisión 2 (spilling).** La asignación de registros de `-O1` (camino por el TAC sin ninguna
optimización) se queda como está: *linear scan*, y error si no alcanzan los 12 registros.
Asignar registros no es una optimización: el TAC usa temporales ilimitados y el procesador
tiene 12, así que es parte de traducir. El spilling sí es una optimización (§4.2 del
enunciado): con `--opt=spill` o `-O2`, la asignación la hace el coloreo de Chaitin, que asigna
y derrama. Al *linear scan* no se le agrega derrame. Detalle en 7.6 y 11.6.

**Fuera del alcance** (lo enseña el curso, no lo pide el enunciado): subexpresiones comunes,
plegado y propagación de constantes, expresiones disponibles, definiciones alcanzables, código
invariante, reducción de fuerza, intercambio de ciclos, *tiling* y la prueba del GCD.

**Para la defensa.** Los bloques básicos se forman al construir el TAC, no con el algoritmo de
líderes de la presentación; el resultado cumple la misma definición. El LL(1) (PRIMERO,
SIGUIENTE, tabla M y analizador predictivo) ya existe en `ide/nucleo/` y `ide/docs/ll1/`; su
revisión queda para la etapa del IDE.

### 4.5 Lo que Calser permite y lo que llega al TAC

Las optimizaciones trabajan sobre el TAC, así que todo esperado y todo supuesto se calcula
sobre el TAC real, no sobre el fuente. Lo que el análisis rechaza nunca llega al TAC: no se
diseña ni se prueba como programa fuente.

**Lo que Calser rechaza** (no llega al TAC)

| Regla | Dónde |
|---|---|
| Sentencia inmediatamente después de `E` o `R` en el mismo bloque | `semantic.c:575` |
| Función sin ningún `E` (o con `R` y sin `E`) | `semantic.c:1199-1203` |
| Recursión: solo existe `R`, que reinicia la función actual; cualquier otro ciclo de llamadas se rechaza | `semantic_expr.c:468`, `:531` |
| Invocar `main`; `main` que no sea `F main [] : void` | `semantic.c` |
| `S` con paso cero (el paso es siempre un literal) | `semantic.c:711` |
| `S` con inicio y fin constantes que nunca se ejecuta | `semantic.c:722` |
| Asignar al control de un `S` o a cualquier `Vi` | e38 |
| Índice constante fuera de rango | `semantic_expr.c:356` |
| División entre cero o exponente negativo con constantes | `semantic_expr.c` |
| Expresión constante que desborda `int` | `semantic_expr.c` |
| Usar como valor una función `void`; usar CRYPTO como valor | `semantic_expr.c:898`, e65 |

"Constante" en el semántico incluye expresiones (`2+2`). El TAC no pliega: `S [ i , 0 , 2+2 , 1 ]`
llega con el fin como temporal, no como el entero 4.

**Lo que Calser permite y sí llega al TAC**

| Comportamiento | Consecuencia |
|---|---|
| Un `C` con `E` en todas sus ramas seguido de otra sentencia (la regla solo mira la sentencia inmediatamente anterior) | La unión queda inalcanzable, con todo lo que siga. Ocurre en 6 programas del corpus (`factorial_r`, los cuatro `r_arreglos`, `p07_retorno_recursion`) |
| La variable de retorno arranca en 0 al entrar a la función (no al reentrar con `R`) | `r = 0` implícito en toda función con retorno (`tac_build.c:82-84`) |
| Los locales se reinicializan en cada entrada, también con `R`; sin inicializador, a 0 | `x = 0` implícito por cada local escalar sin inicializador (`tac_build_init.c`); un local no conserva valor entre llamadas |
| Grafo de llamadas sin ciclos; `R` vuelve a la entrada de la misma función | Las áreas estáticas por función son seguras |
| `S`: control inmutable, paso literal distinto de cero, fin capturado una vez en `$limite_Sk` | El número de vueltas es constante solo si inicio y fin llegan como enteros a la marca |
| `E` dentro del cuerpo de un `S` | Lazos con salida anticipada (`e_en_s`, la búsqueda de `ejercicio2`) |
| Índices dinámicos | Sin comprobación en ejecución (`claude/mejoras_posibles.md`) |
| CRYPTO solo como sentencia | Siempre es efecto, nunca una definición muerta |

**Forma del TAC que generan las estructuras de control.** En `S`, `L` y `C` la cabecera o la
condición entran al cuerpo (o a la rama verdadera) por caída, sin salto con etiqueta. El
constructor no genera ciclos formados solo por saltos con etiqueta. Bloques de unión vacíos no
aparecen en el corpus, pero la representación los admite.

**Regla para los esperados.** Cada PR de optimización guarda en `pruebas/optimizador/<pasada>/`
el listado HIGH (o ISA virtual, para reordenamiento y spilling) de cada programa de prueba,
junto con su esperado, para que el número se pueda comprobar instrucción por instrucción.

### 4.6 Decisiones del 15/11 sobre los algoritmos

Tomadas tras la revisión contra el TAC real.

| # | Tema | Decisión | Alternativas descartadas |
|---|---|---|---|
| 1 | Borrar bloques inalcanzables (DCE, unrolling total) | Usar `tac_block_remove` en un ciclo: reconstruir aristas, borrar cualquier bloque del conjunto que ya se pueda borrar y repetir hasta que no quede ninguno; lo que no se pueda borrar se deja, sin error | Función nueva que borre un conjunto (cubre casos que Calser no genera); borrar en orden fijo (falla con ciclos) |
| 2 | Unrolling de un `S` con salida anticipada | No se desenrolla si algún bloque del lazo, salvo la cabecera, tiene un sucesor fuera del lazo | Ampliar la región clonada o convertir caídas en `goto`: más riesgo, poco beneficio en búsquedas |
| 3 | Renombramiento en `main` | No renombrar una definición de una raíz observable de `main` si entre ella y la siguiente hay una instrucción con `may_trap` o `uses_globals` (en HIGH: `CALL`, `DIV`, `MOD`, `POW`) | Guardar el origen de cada lectura en la liveness; no renombrar nada en `main` |
| 4 | Distancias del reordenamiento | La distancia entre dos instrucciones es la mayor de todas las reglas que se cumplen; RAW nunca baja de 2 | Ninguna razonable |
| 5 | Asignación con spilling | Chaitin con `--opt=spill` y `-O2`; `-O1` conserva el *linear scan* sin derrame | Solo Chaitin también en `-O1` (cambiaría la línea base); *linear scan* con derrame |

## 5. Diseño contra el código actual

Estado: **E** existe, **P** existe parcialmente, **F** falta. La última columna remite al paso
del refactor (parte 9) o a la tarea del plan.

| Componente | Código actual | Estado | Qué le falta o qué se integra |
|---|---|---|---|
| A1 Conductor | `src/main.c` (1529 líneas) | E | La secuencia de `-O1` vive dentro de `main.c` (`phase_o1`) y nadie más puede usarla → P9. Límite de ROM → P5 |
| A2.1 Enlazado | — | F | Tarea C2.2. Diseño en la parte 12 |
| A2.2 Léxico | `grammar/lexer.l` | E | Se queda |
| A2.3 Sintáctico | `grammar/parser.y`, `src/ast.c` | E | La aceptación del AST se repite en 7 lugares → P6. Cascadas → P13 (pospuesto, 9.14) |
| A2.4 Semántico | `semantic.c`, `semantic_expr.c`, `semantic_scope.c`, `semantic_calls.c`, `semantic_crypto.c`, `symbol_table.c`, `ast_constant.c` | E | Se queda |
| A2.5 Memoria de datos | `data_area.c` | E | Se queda. La extensión de derrame ya existe y está probada |
| A2.6 Diagnósticos | `diagnostics.c` | E | Se queda. Con el enlazado necesitará el archivo de cada error |
| A3.1 Forma | `tac.c`, `tac_isa.c`, `tac_pseudo.c`, `tac_cfg.c`, `tac_edit.c`, `tac_validate.c`, `tac_print.c`, `tac_loop_marks.c` | P | Tabla de operaciones complejas repetida → P7. No se puede clonar un bloque ni fijar un orden nuevo dentro de un bloque → P11 |
| A3.2 Construcción | `tac_build.c`, `tac_build_expr.c`, `tac_build_flow.c`, `tac_build_init.c`, `tac_build_stmt.c` | E | Se queda |
| A3.3 Análisis | `tac_live.c`, `tac_locations.c`, `tac_loops.c`, `tac_intervals.c` | P | Las raíces observables solo se calculan en una prueba (`pruebas/tac_analysis/desde_fuente.c`) → P10. La pertenencia a un conjunto de vivos busca la posición del valor recorriendo todas las ubicaciones (costo cúbico en la asignación) → P14 |
| A4.1 Gestor | `tac_passes.c` | P | Las cinco pasadas son una misma función vacía en este archivo; `spill` se ejecuta como pasada antes de asignar → P8 |
| A4.2 a A4.5 Pasadas | — | F | Tareas C3.1 a C3.3. Sus archivos y su registro los deja P8. Contrato en la parte 7, diseño técnico en la parte 11 |
| A5.1 Generador `-O0` | `generate.c`, `generate_expr.c`, `generate_stmt.c`, `generate_init.c`, `generate_support.c`, `label_table.c` | E | Solo cambian llamadas (P2, P3) |
| A5.2 Bajada | `tac_lower.c`, `tac_lower_control.c`, `tac_lower_value.c` | E | Solo cambian llamadas (P2, P7) |
| A5.3 Asignación | `tac_allocate.c`, `tac_allocate_sites.c` | P | Sin derrame: `no_register_available` solo da error. No recibe interruptor ni profundidad de lazos → P12. El derrame es la tarea C3.4, con Chaitin en un archivo propio (7.6); el *linear scan* de `-O1` queda sin derrame |
| A5.4 Emisión y rutinas | `tac_emit.c`, `tac_emit_pseudo.c`, `codegen_arith.c`, `codegen_array.c`, `codegen_crypto.c`, más cuatro funciones en `generate_support.c` y `generate_expr.c` | P | Preservación en `temp_base`, fin de programa, arranque de pila y registros fijos de CRYPTO repetidos entre caminos; la emisión de `-O1` depende del generador `-O0` → P3 |
| A5.5 Calendarización | `codegen.c`, `codegen_schedule.c`, `codegen_instruction.c`, `codegen_verify.c` | E | Se queda |
| A5.6 Ensamblado | `assembler.c`, `write_outputs` en `main.c` | P | El límite de ROM solo se comprueba en `-O1` → P5 |
| A5.7 Modelo del ISA | `isa_table.c`, `isa_constant.c` | P | Rango del inmediato, nombres y papeles de registros repetidos fuera → P2 |
| A6 Métricas | `tac_metrics.c` | P | Contadores escritos como ceros fijos; `"nivel":"O1"` fijo → P9 |
| A7 Servicios del IDE | `diagnostics_json.c`, `syntax_service.c`, `suggestions.c`, `repairs.c`, `clones.c` | E | Solo el escapado de cadenas → P4 |
| B1–B5, B7, B9–B11 | `ide/` | E | Fuera del refactor |
| B6 Compilar | `ide/compilacion.py`, `ide/interfaz/opciones_compilacion.py` | P | No puede pedir nivel, pasadas, factor ni métricas. Diseño en la parte 13 |
| B6 Tres direcciones | — | F | El IDE no genera ni muestra el `.tac`. Diseño en 13.1 |
| B8 Métricas | — | F | Tarea C4.2. Diseño en la parte 13 |
| C1 Simulador | `tools/sim_asm.py` | E | Se queda |
| C2 Medición | — | F | Tarea C4.2. Diseño en la parte 13 |
| D1 Regresión | 39 suites de `make check` | E | Se queda |
| D2 Verificación de pasadas | `pruebas/tac_build/interpretar.c` y `ejecutar.c`; `pruebas/tac_emit/comparar.py` | P | Ninguno puede activar pasadas → P10 |
| D3 Pruebas obligatorias | — | F | Tarea C4.1. Diseño en la parte 14 |

**Hallazgo sobre la duplicación.** Un detector de bloques repetidos sobre todo `src/` casi no
encuentra código copiado: solo la función `out` (en `codegen_arith.c` y `codegen_array.c`) y
las filas de las tablas. La duplicación real es de conocimiento: la misma regla o la misma
lista escrita en varias tablas y constantes. Por eso no se ve buscando texto repetido.

## 6. Efecto de la integración del IDE sobre el léxico y el parser

Comparación de `grammar/lexer.l` y `grammar/parser.y` entre `2268981` (antes) y `6ad9fb4`
(ahora).

### 6.1 Qué cambió en el comportamiento que ya funcionaba

| # | Antes | Ahora | Efecto |
|---|---|---|---|
| 1 | El parser se detenía en el primer error sintáctico | Tres reglas de recuperación (`globales error ;`, `F error .`, `cuerpo error`) y una función propia de sincronización; reporta hasta 32 errores | Un programa con errores da varios diagnósticos |
| 2 | `yyparse()` distinto de 0 significaba error | Puede devolver 0 con errores | Hay que revisar tres condiciones: retorno 0, `parser_syntax_errors() == 0` y `lexical_errors == 0` |
| 3 | Con error no había árbol | Puede haber un árbol parcial, con funciones omitidas | `main.c` lo libera y no lo pasa al semántico ni a `--ast-json` |
| 4 | La salida de error quedaba vacía al compilar bien | Imprime "Compilación correcta" y los archivos generados | 4 suites se ajustaron |
| 5 | Mensaje genérico al escribir `f(...)` | "las funciones usan nombre[...], no nombre(...)" | Cambia el texto de ese diagnóstico |
| 6 | El parser dejaba sin liberar lo que tenía en la pila al fallar | `%destructor` para nodos y cadenas | Las "fugas del parser" que el plan da como pendientes deberían estar resueltas; falta confirmarlo ejecutando (P0) |

**Lo que no cambió:** los tokens, las reglas para programas correctos y el árbol que producen.
Para un programa válido el resultado es el mismo.

**Comprobado con Bison:** la gramática actual no tiene conflictos; la anterior tampoco. Pasó de
242 a 248 estados.

### 6.2 La causa de las cascadas de e71, e72 y e74

Los tres casos son un error en la **cabecera** de un bloque:

```
F main [] : void ;
    S [ i , 0 , 3 ] ;      ← error correcto: 'S' requiere exactamente 4 partes
    .                      ← este punto debía cerrar el S
    E ;                    ← error falso
.                          ← error falso
```

La recuperación (`cuerpo error`) descarta la cabecera hasta el `;` y además olvida que se abrió
un bloque. Entonces el `.` del `S` cierra `main`, y el `E ;` y el `.` siguientes quedan fuera
de toda función. La corrección está en P13.

### 6.3 Acoplamiento observado, que se deja

- `parser.y` incluye `suggestions.h` y `syntax_service.h`, y pregunta si está en modo consulta
  en reglas, destructores y en cada fin de archivo.
- `lexer.l` llama a `diag_json_token` en cada token, también al compilar normal, y además
  avisa a un observador: dos vías para lo mismo.

Es al revés del principio "el código nuevo se adapta a la base". Se deja como está: funciona,
el costo en ejecución es una llamada por token, y desacoplarlo es rediseñar los motores del
IDE, que está fuera de esta tarea. Queda anotado para quien toque el parser.

### 6.4 Datos para el enlazado `P` (tarea C2.2)

- Todo el estado del parser se reinicia en cada llamada a `yyparse()` (`%initial-action`).
  Se puede analizar cada módulo por separado con la función de aceptación de P6 y unir las
  listas de globales y funciones en un solo AST. No hace falta la pila de entradas de Flex.
- Las posiciones del léxico (`column`, `lex_offset`) solo se reinician en las funciones de
  consulta, no en el camino normal: hay que reiniciarlas por archivo.
- `diagnostics.c` guarda un solo nombre de archivo y sus líneas: con varios módulos, cada
  error debe llevar el archivo que lo produjo (lo exige D1).
- El token de cadena que necesita `P` no existe en el léxico.

## 7. Contrato de integración de las optimizaciones

El refactor deja este contrato cumplido por la infraestructura. Quien implementa una
optimización escribe solo su lógica, dentro de su archivo.

### 7.1 Contrato común de una pasada

**Dónde vive.** Un archivo por pasada, con una sola función pública. El refactor (P8) los crea
con la función vacía y ya registrada. Cuatro personas trabajan en paralelo sin tocar el
archivo de otra.

| Pasada | Archivo | Función | Etapa | Tarea |
|---|---|---|---|---|
| Unrolling | `src/optimizador/tac_pass_unroll.c` | `tac_pass_unroll` | Alto nivel | C3.1 |
| Renombramiento | `src/optimizador/tac_pass_rename.c` | `tac_pass_rename` | Alto nivel | C3.3a |
| DCE | `src/optimizador/tac_pass_dce.c` | `tac_pass_dce` | Alto nivel | C3.2 |
| Reordenamiento | `src/optimizador/tac_pass_reorder.c` | `tac_pass_reorder` | ISA virtual | C3.3b |

El spilling no es una pasada del gestor: vive dentro de la asignación (7.6).

**Firma** (la de `TacPassRun`, que ya existe):

```c
int tac_pass_dce(TacPassContext *context, char *detail, size_t capacity);
```

Devuelve 1 si terminó bien; 0 con la causa en `detail`.

**Qué recibe en el contexto**

| Campo | Uso |
|---|---|
| `program` | La representación en la etapa de la pasada; la pasada la modifica |
| `scopes` | Ámbitos del semántico, para saber qué variable es de qué función |
| `source_name` | Nombre del fuente, para mensajes |
| `unroll_factor` | 0: heurística; 2 a 16: factor pedido con `--unroll=N` |
| `metrics->counters` | **Nuevo (P9).** Donde la pasada anota lo que hizo |

```c
typedef struct { int line; unsigned factor; } TacUnrolledLoop;
typedef struct {
    TacUnrolledLoop *unrolled;   /* un elemento por lazo desenrollado; propio */
    size_t unrolled_count;
    size_t renamed;              /* definiciones que pasaron a un temporal nuevo */
    size_t removed;              /* instrucciones de alto nivel eliminadas */
    size_t removed_blocks;       /* bloques inalcanzables eliminados */
    size_t reordered;            /* instrucciones cuya posición en su bloque cambió */
    size_t spilled;              /* intervalos derramados */
    size_t spill_loads;          /* cargas de derrame insertadas */
    size_t spill_stores;         /* guardados de derrame insertados */
} TacPassCounters;
```

**Qué hace el gestor por la pasada** (ya existe en `tac_pass_execute`, se conserva): la
ejecuta en el orden fijo solo si está activa; valida la forma con `tac_validate` al terminar;
mide su tiempo; si falla, corta con "error interno en pasada X" y el detalle.

**Obligaciones de toda pasada**

| Regla | Razón |
|---|---|
| No cambia el estado observable: variables globales, variables de `main`, memoria externa y estado CRYPTO | Es lo que comprueban las pruebas y el procesador |
| Después de editar, libera y recalcula liveness y lazos antes de volver a usarlos | Sus resultados prestan punteros a instrucciones y bloques |
| Si cambia el grafo, llama a `tac_rebuild_edges` | El gestor valida la forma después |
| No imprime; los errores van en `detail` | La salida la controla el conductor |
| Para saber qué lee, escribe o altera una instrucción usa `tac_instruction_info`, `tac_definitions`, `tac_uses` y `tac_locations_visit`; no decide por código de operación | Las propiedades están en un solo lugar |
| Trabaja función por función | Las áreas y los temporales son por función |

### 7.2 DCE (A4.4, tarea C3.2)

**Usa lo que existe:** `tac_observable_roots`, `tac_live_analyze`, `tac_live_instruction`,
`tac_live_contains`, `tac_instruction_info`, `tac_definitions`, `tac_remove`,
`tac_loops_analyze`, `tac_loops_reachable`, `tac_block_remove`, `tac_rebuild_edges`. No le
falta infraestructura.

Diseño interno: 11.2. Regla ante un fallo: 11.1.

**Algoritmo**

1. Calcular las raíces observables: variables escalares globales y de `main`.
2. Por función: calcular liveness. Las raíces cuentan al salir de `main`; `RETURN` y `CALL`
   leen todas las globales en cualquier función (`uses_globals`), y en `main` las
   instrucciones que pueden fallar y las llamadas leen además las raíces (11.1).
3. Recorrer las instrucciones. Se elimina una instrucción si se cumple todo:
   - define un valor y ese valor no está vivo después de ella;
   - sus propiedades no la protegen: no es `nonremovable`, no escribe memoria, no toca el
     estado CRYPTO, no es barrera, no es un salto ni un retorno;
   - si puede fallar (`/`, `%`, `^`): el divisor es un entero distinto de cero, o el
     exponente es un entero mayor o igual que cero. Sin esa prueba no se elimina.
4. Si se eliminó algo, recalcular liveness y repetir desde 3.
5. Quitar los bloques inalcanzables desde la entrada (decisión 4.6.1).
6. Anotar `removed` (instrucciones del barrido) y `removed_blocks` (bloques quitados). Las
   instrucciones de un bloque quitado no se suman a `removed`.

**Qué elimina en los programas reales.** Sobre todo instrucciones que el constructor agrega
solo: `r = 0` cuando la función asigna `r` en todos los caminos antes de retornar, `x = 0` de un
local sin inicializador que se asigna antes de leerse (fuera de `main`, o en `main` sin una
llamada o división en medio), y copias de resultados a locales que nunca se leen. Las
uniones inalcanzables después de un `C` que retorna en todas sus ramas se quitan como
bloques.

**Ejemplo, sobre el TAC real** (programa `d1` de 14.3)

```
tocar:     t0 = g + k ; g = t0 ; return
calcular:  r = 0 ; t0 = a * 2 ; x = t0 ; t1 = a + 1 ; y = t1 ;
           param a, 0 ; call tocar, 1 ; r = y ; return r
main:      v = 0 ; param 3, 0 ; t0 = call calcular, 1 ; v = t0 ; return
```

- `tocar`: `g = t0` se conserva porque `return` lee las globales.
- `calcular`: primera vuelta, `r = 0` (lo sobrescribe `r = y` sin lecturas en medio) y
  `x = t0` (`x` nunca se lee); segunda vuelta, `t0 = a * 2`. `removed = 3`.
- `main`: `v = 0` se conserva porque la llamada lee las raíces de `main`.

**Comprobación C3.2.** `src/optimizador/tac_pass_dce.c` implementa este recorrido.
`make check-dce` comprueba la tabla de 11.2 y la idempotencia. Las capturas HIGH de
`pruebas/optimizador/dce/*.antes.tac` y `*.despues.tac` fijan los programas de 14.3;
`make check-o1` contrasta esas capturas, los contadores del JSON y el estado observable
con el simulador. `check-pasadas` compara globales y variables de `main`, sin exigir
igual número de instrucciones ejecutadas ni conservar locales de otras funciones.

### 7.3 Renombramiento local (A4.3, tarea C3.3a)

**Usa lo que existe:** `tac_variable_is_rename_candidate` (solo locales `Vm` escalares de la
función; excluye globales, arreglos, parámetros, variable de retorno, control de `S` y su
límite), `tac_definitions`, `tac_temp_new`, `tac_replace_reads`, `tac_instruction_info`. No le
falta infraestructura.

Diseño interno: 11.3.

**Algoritmo**, por bloque básico:

1. Recorrer el bloque de arriba abajo.
2. Para cada variable candidata `v` con más de una definición en el bloque (contando las
   implícitas, como `x = 0`): cada definición **menos la última** pasa a escribir un temporal
   nuevo `t`, salvo que la regla de seguridad lo impida.
3. Las lecturas de `v` entre esa definición y la siguiente pasan a leer `t`.
4. La última definición del bloque sigue escribiendo `v`: la variable queda con su valor
   correcto al salir del bloque, sin agregar copias.
5. Anotar `renamed`: definiciones que pasaron a un temporal.

**Regla de seguridad (decisión 4.6.3).** En `main`, una definición de una raíz observable no se
renombra si entre ella y la siguiente definición de la misma variable hay una instrucción con
`may_trap` o `uses_globals` (en HIGH: `CALL`, `DIV`, `MOD`, `POW`; `RETURN` siempre cierra el
bloque). Esas instrucciones leen el estado observable (11.1): si el programa se detiene ahí, la
variable tiene que estar escrita en memoria. Fuera de `main` esas lecturas implícitas solo
cubren globales, que no son candidatas, así que la regla no bloquea nada.

No cruza bloques (forma local; la global sería SSA, fuera del alcance).

**Ejemplo, sobre el TAC real**

```
Vm x : int = a + b ;          t0 = a + b      x = t0        (def 1)
c = x * 2 ;                   t1 = x * 2      c = t1
x = a - b ;                   t2 = a - b      x = t2        (def 2)
```

La definición 1 (`x = t0`) pasa a escribir un temporal nuevo `t9`, y la lectura de la MUL lee
`t9`:

```
antes:    t0 = a + b ; x  = t0 ; t1 = x  * 2 ; c = t1 ; t2 = a - b ; x = t2
después:  t0 = a + b ; t9 = t0 ; t1 = t9 * 2 ; c = t1 ; t2 = a - b ; x = t2
```

Desaparecen la WAW entre las dos
escrituras de `x` y la WAR entre la lectura y la segunda escritura, y una escritura de `x` a
memoria. `renamed = 1`. Si la declaración no tuviera inicializador (`Vm x : int ;` y después
`x = a + b`), `x = 0` sería una definición más y `renamed = 2`.

### 7.4 Unrolling (A4.2, tarea C3.1)

**Usa lo que existe:** las marcas `TacLoopMark` (cabecera, cuerpo, retorno, control, límite,
inicio, fin y paso, con el contrato B de P15: `initial` es INTEGER, TEMP o NONE; `bound` es
INTEGER, TEMP o exactamente `tac_variable($limite_Sk)`), `tac_loop_mark_state`,
`tac_loops_analyze`, `tac_loops_contains`, `tac_block_insert_after`, `tac_blocks_clone`,
`tac_block_remove`, `tac_rebuild_edges`.

**Qué lazos.** Solo `S` cuya marca está en estado "con arista de retroceso". Se omiten:
- los `L` (no se conoce su número de vueltas);
- un `S` que contiene la cabecera de otra marca, sea `S` o `L` (solo se desenrollan los
  internos);
- un `S` con salida anticipada: algún bloque de su lazo natural, salvo la cabecera, tiene un
  sucesor fuera del lazo (un `E` dentro del cuerpo, o una caída hacia un `RETURN`). Clonarlo
  perdería esa caída (decisión 4.6.2).

El semántico prohíbe asignar el control, así que solo lo cambia el propio lazo.

**Constante.** Un `S` tiene número de vueltas constante solo si `initial` y `bound` de la marca
son INTEGER. Un fin escrito como expresión constante en el fuente (`2+2`) llega como TEMP y
cuenta como no constante: el TAC no pliega.

**Heurística** (factor cuando no se da `--unroll=N`)

| Caso | Acción |
|---|---|
| `initial` y `bound` INTEGER y N ≤ 8 vueltas | Desenrollado total: N copias del cuerpo, sin la prueba del `S` ni salto de retroceso |
| Cualquier otro | Factor 4, con el lazo original como residuo |
| El crecimiento estimado supera el presupuesto | No se desenrolla ese lazo |

Con `--unroll=N` (2 a 16) se usa ese factor en lugar de 4. El presupuesto
(`UNROLL_BUDGET`) es una cota heurística del crecimiento en instrucciones HIGH: no cuenta la
guarda ni la expansión de las operaciones complejas. La comprobación definitiva de la ROM es
la del ensamblado.

**Transformación parcial, con factor k y paso positivo**

```
antes                              después
H:  si i >= lim → salida           G:  t = i + (k-1)·paso
B:  cuerpo                             si t >= lim → H        (quedan menos de k vueltas)
L:  i = i + paso ; ir a H          U:  cuerpo ; i = i + paso  ┐
                                       …                      ├ k copias
                                       cuerpo ; i = i + paso  ┘
                                       ir a G
                                   H:  si i >= lim → salida   (residuo: el lazo original)
                                   B:  cuerpo
                                   L:  i = i + paso ; ir a H
```

La entrada al lazo pasa a apuntar a `G`. Con paso negativo las comparaciones se invierten. En
el TAC real, el cuerpo y el retorno suelen ser el mismo bloque, y un `C` dentro del cuerpo
agrega bloques que se alcanzan por caída; la clonación conserva el orden relativo de los
bloques del conjunto, y con eso sus caídas.

**Transformación total.** Las N copias reemplazan la prueba y el retroceso. Se conservan el
`goto` de entrada (redirigido a la primera copia) y los saltos entre copias. Después de
redirigir y reconstruir aristas, la cabecera y el cuerpo originales quedan inalcanzables y se
quitan con el ciclo de borrado de la decisión 4.6.1.

**Ejemplo, total, sobre el TAC real** (`S [ i , 0 , 4 , 1 ] ; suma = suma + i ; .`):

```
antes                                          después
B1: i = 0 ; $limite_S0 = 4 ; goto B2           B1: i = 0 ; $limite_S0 = 4 ; goto C1
B2: if i >= $limite_S0 goto B4                 C1: t1 = suma + i ; suma = t1 ; i = i + 1 ; goto C2
B3: t0 = suma + i ; suma = t0 ; i = i + 1      C2: …                                  ; goto C3
    goto B2                                    C3: …                                  ; goto C4
B4: return                                     C4: …                                  ; goto B4
                                               B4: return
```

Se anota `{linea del S, factor 4}`. Ese par no distingue un desenrollado total de uno parcial ni
prueba equivalencia: el PR acompaña cada prueba con su listado transformado.

**Ejemplo, parcial.** Con fin en una variable que vale 10 y factor 4: `G` deja pasar mientras
`i + 3 < 10`; se ejecutan dos rondas de 4 (i = 0..7) y el residuo hace las 2 vueltas restantes.

### 7.5 Reordenamiento (A4.5, tarea C3.3b)

**Usa lo que existe:** `tac_locations_visit` (ubicaciones leídas y escritas, incluidas las
implícitas), `tac_instruction_info` (barrera, puede fallar, memoria), la unidad funcional de
cada instrucción ISA, `tac_block_set_order`. El grafo de dependencias es lógica propia de la
pasada.

**No puede reutilizar el organizador.** `cg_schedule_block` trabaja con máscaras de los 16
registros reales; aquí los valores son temporales virtuales. Comparte las reglas de distancia,
que salen del ISA.

**Cómo llega el ISA virtual** (medido en la revisión):
- Las variables escalares, globales y locales, son memoria con base `rz` y desplazamiento
  constante: `lw v4, 4(rz)`, `sw v0, 20(rz)`. Cada lectura de una variable es su propia carga.
- Un elemento de arreglo se calcula (índice × 4 más la base) y se accede con base TEMP:
  dirección desconocida.
- `PARAM` ya no existe como instrucción: se materializa como carga o `mov` y `sw` a la casilla
  del parámetro. `CALL` es `jas`, una barrera dentro del bloque.
- `*`, `/`, `%`, `^` y las CRYPTO son PSEUDO indivisibles (no tienen `isa.instruction`): ocupan
  su ciclo solas; CRYPTO es además barrera.
- Los bloques de los programas reales tienen entre 1 y 118 instrucciones.

**Algoritmo**, por bloque con 3 a 256 instrucciones (fuera de ese rango no se toca y cuenta 0;
calendarización por lista, Aho §10.3–10.4):

1. Construir el grafo de dependencias. Hay arista de `a` a `b` (con `a` antes que `b`) cuando se
   cumple alguna de estas reglas; **la distancia es la mayor de las que se cumplen**
   (decisión 4.6.4):

   | Caso | Distancia |
   |---|---|
   | `b` lee lo que `a` escribe (RAW) | 2 |
   | `b` escribe lo que `a` lee (WAR) | 1 |
   | `a` y `b` escriben lo mismo (WAW) | 1 |
   | Uno escribe memoria y el otro la lee o la escribe, salvo que las dos direcciones sean `rz` más constantes distintas | 1 |
   | `a` o `b` es barrera (`jas`, CRYPTO) | 1, con todas |
   | Uno puede fallar (`/`, `%`, `^`) y el otro es un store, una llamada o CRYPTO | 1 |
   | `b` es el salto final del bloque | 1, con todas |

2. Prioridad de cada instrucción: camino más largo hasta el final del bloque.
3. Simular bundles: por ciclo caben 2 de ALU, 1 de salto y 1 de memoria; una PSEUDO ocupa el
   ciclo sola. Se toma, entre las listas, la de mayor prioridad; en empate, la que estaba antes.
4. El orden nuevo es por ciclo simulado y, dentro del ciclo, por posición original.
5. Aplicarlo con `tac_block_set_order`. `reordered` suma, en todo el programa, las posiciones
   que cambiaron en cada bloque.

**Por qué va antes de asignar.** Después de asignar, dos valores sin relación pueden compartir
registro y aparecer como dependientes (WAR y WAW falsas). Con temporales virtuales esas
dependencias no existen.

**Ejemplo (prueba unitaria sobre ISA virtual armado a mano**, no la bajada de un fuente):

```
1: v1 = lw a(rz)            dependencias: 1→2 (RAW, 2), 3→4 (RAW, 2); memoria: solo lecturas
2: v2 = add v1, v1
3: v3 = lw b(rz)            simulación: ciclo 0: 1 · ciclo 1: 3 · ciclo 2: 2 · ciclo 3: 4
4: v4 = add v3, v3          orden nuevo: 1 3 2 4      reordered = 2
```

En un programa fuente el bloque real incluye las inicializaciones, las cargas de cada lectura,
los stores y el retorno: el contador del programa es mayor (14.4).

### 7.6 Spilling (tarea C3.4)

**Qué es cada cosa (decisión 4.6.5).**
- *Asignación de registros:* poner cada temporal virtual en uno de `t0`–`t11`. La hace siempre
  el camino `-O1`, porque el TAC usa temporales ilimitados. En `-O1` la hace el *linear scan*
  actual (`allocate_function` en `tac_allocate.c`), sin cambios: si no alcanzan los 12
  registros, error "sin registros temporales disponibles".
- *Spilling:* la optimización que permite compilar cuando no alcanzan los registros. Se activa
  con `--opt=spill` o `-O2`, y entonces la asignación la hace el coloreo de Chaitin, como en el
  curso.

**Dónde.** Archivo nuevo `src/generador_o1/tac_allocate_color.c`. `tac_allocate_registers`
elige la estrategia con el interruptor que ya existe desde P12:

```c
typedef struct { int spill; } TacAllocationOptions;   /* NULL o spill = 0: linear scan, sin derrame */
                                                      /* spill = 1: Chaitin con derrame            */
```

Las dos estrategias entregan lo mismo: el mapa temporal→registro de cada función y, con
Chaitin, la lista de derramados. Todo lo demás es común: intervalos, plan por instrucción
(`tac_allocate_sites`), preservación, operaciones complejas y emisión.

**Algoritmo de Chaitin** (presentación *Spilling de Registros*; Aho §8.8.4), por función:

1. Grafo de interferencia: un nodo por temporal; dos temporales interfieren si sus intervalos
   de vida se solapan (los intervalos ya incluyen que las fuentes de una operación compleja
   siguen vivas en la posición de escritura). k = 12 colores (`t0`–`t11`).
2. Simplificar: mientras haya un nodo con grado < k, sacarlo y apilarlo. Si no hay, elegir
   víctima: la de menor costo/grado, con costo = Σ 10^profundidad sobre cada definición y uso
   del temporal y grado = vecinos en el grafo actual; en empate, el de intervalo más largo. Se
   saca y se apila como posible derramado.
3. Seleccionar: desapilar y dar a cada nodo un color que no tenga ningún vecino. Entre los
   colores libres se elige el que se asignó hace más tiempo (rotación), para no reusar
   enseguida un registro y no crear dependencias falsas que sumen bundles. Si un posible
   derramado no tiene color libre, queda derramado; si lo tiene, no se derrama (coloreo
   optimista de Briggs, refinamiento de Chaitin fuera de la presentación).
4. Casillas: reservar `4 × derramados` bytes con `data_area_reserve_spill`; la casilla del
   derramado `k` es `spill_base + 4·k`.

No hace falta repetir el algoritmo después de derramar, como en el curso: el código de
derrame usa `rg`, registro reservado que no entra al grafo y cumple el papel de los
"temporales frescos".

**Plan por instrucción.** Las cargas y guardados no son instrucciones nuevas: se anotan en el
plan de la instrucción (`TacEmissionSite`, campos `loads` y `stores` de P12) y la emisión
escribe, en orden: guardar preservados, cargas de derrame, la instrucción, guardados de
derrame, restaurar preservados.

| Situación | Auxiliar |
|---|---|
| ISA normal con un operando derramado | `rg` |
| ISA normal con dos fuentes derramadas y destino con registro | `rg` y el registro del destino |
| ISA normal con dos fuentes derramadas y sin destino con registro | `rg` y un registro prestado de `t0`–`t11` que la instrucción no use; si está vivo, se guarda y restaura en su casilla de `temp_base` |
| ISA normal con destino derramado | El registro de la primera fuente, o `rg`; después `sw` a la casilla |
| Operación compleja (exige operandos y destino en `t0`–`t11` y 8 auxiliares distintos) | Registros prestados para fuentes y destino derramados, elegidos junto con sus 8 auxiliares; nunca `rg` |
| CRYPTO | El valor se carga directo en su registro fijo |

Cuentan: `spilled` (temporales derramados), `spill_loads` (una carga por fuente derramada
distinta en cada instrucción) y `spill_stores` (un guardado por destino derramado). No
incluyen preservaciones ni auxiliares de expansión.

**Ejemplo.** En `s1` (14.5), `-O1` falla con 14 temporales vivos a la vez. Con `--opt=spill`,
Chaitin derrama los de menor costo/grado (resultados de los primeros productos, que esperan
vivos hasta el final de la expresión): cada uno se guarda al definirse y se carga al usarse.
Los derramados exactos los fija el listado del PR de C3.4.

**Efecto esperado.** En un programa que cabe en 12 registros (todo el corpus y `ejercicios/`),
`--opt=spill` no derrama nada: mismas instrucciones, con otra asignación de registros; los
bundles pueden variar levemente. En un programa que no cabe, `-O1` no compila y
`--opt=spill` sí. En `-O2` permite que compile un lazo desenrollado que suba la presión.

### 7.7 Verificación de las pasadas (D2)

| Pasadas | Cómo | Herramienta |
|---|---|---|
| Unrolling, renombramiento, DCE | Ejecutar la representación de alto nivel sin la pasada y con ella; el estado observable debe ser idéntico | `interpretar_tac` con `--opt=` (P10) |
| Reordenamiento, spilling | Compilar con la pasada activa, ejecutar en el simulador y comparar el estado observable contra `-O0` | `pruebas/generador_o1/tac_emit/comparar.py` con `--opt=` (P10) |
| Todas | Forma válida después de la pasada | `tac_validate`, lo hace el gestor |
| Programas con CRYPTO | El intérprete los rechaza; se verifican solo con el comparador | — |

## 8. Conexión con el IDE y las métricas

### 8.1 Cadena completa

```
IDE (F5) ──► ccom  nivel, --opt=, --unroll=, --metrics m.json ──► m.json ──────────────┐
                                                        └──► .asm ──► simulador ──► ciclos ─┴► datos por prueba ──► gráficas
```

### 8.2 Estado de cada eslabón

| Eslabón | Estado | Qué falta |
|---|---|---|
| Banderas de `ccom` | Existen: `-O1`, `--opt=`, `--unroll=`, `--metrics`. `-O2` reservado hasta C3.5 | Nada del lado de `ccom` |
| JSON de métricas | Existe, con todas sus claves | Contadores reales y nivel real (P9) |
| F5 del IDE | `ide/compilacion.py` define las banderas en una tabla de siete opciones de sí o no (`-s`, `-t`, `-m`, `-p`, `-v`, `--time`, `--no-schedule`) | No puede pedir nivel, lista de pasadas, factor ni archivo de métricas. Afecta a `ide/compilacion.py` y a `ide/interfaz/opciones_compilacion.py`. Las tres nuevas llevan valor, y la tabla actual solo admite casillas |
| Ciclos | `tools/sim_asm.py` los cuenta (`Simulator.cycles`) y los imprime en una línea de texto | `ccom` no los conoce: su JSON deja `ciclos_sim` en `null`. Los agrega quien mide, usando la clase `Simulator` como ya hace `comparar.py` |
| Antes y después | No existe | Dos compilaciones por prueba y pasada (`-O1` y `-O1 --opt=pasada`). Hace falta una herramienta que las lance, junte los dos JSON y los ciclos, y deje un archivo de datos por prueba |
| Gráficas | No existen | El enunciado permite que vayan en el IDE; leerían ese archivo de datos |

### 8.3 Contrato del JSON de métricas después de P9

Mismas claves y mismo orden que hoy. Cambian tres valores:

| Clave | Hoy | Después |
|---|---|---|
| `nivel` | `"O1"` fijo | `"O1"` o `"O2"` |
| `contadores` | Todos en cero, como texto fijo | Los valores de `TacPassCounters` |
| `contadores.desenrollados` | `[]` | Lista de `{"linea": L, "factor": k}` |

`ciclos_sim` sigue en `null`: lo llena la herramienta de medición en su propio archivo.

### 8.4 Qué pide cada métrica del enunciado (§6) y de dónde sale

| Métrica | Fuente |
|---|---|
| Conteo de instrucciones antes y después | `emitidas` de dos JSON: `-O1` y `-O1 --opt=pasada` |
| Tamaño de código en bytes | `bytes` (16 × bundles) |
| Ciclos estimados por el simulador | `Simulator.cycles` sobre el `.asm` de cada compilación |
| Tiempo de compilación | `tiempo_ms.total` y `tiempo_ms.pasadas` |
| Eliminadas, reordenadas, renombradas | `contadores` |

Del lado de `ccom`, el contrato con el IDE y con la medición queda completo con P9. Lo
pendiente está en el IDE (B6, B8) y en la herramienta de medición (C2), tarea C4.2.

## 9. Plan de implementación del refactor

Dieciséis pasos, P0 a P15. Cada uno es una rama y un PR hacia la rama de integración
`refactor-c6`, y lo ejecuta el agente (que solo programa, prueba y documenta). En todos corren
las cuatro protecciones de 4.1. `develop` conserva el código funcional hasta que el refactor
completo pasa la validación final (9.17); entonces entra con un solo PR de `refactor-c6`.

### 9.0 Orden y dependencias

| Paso | Qué deja | Depende de | Lo necesita |
|---|---|---|---|
| P0 | Línea base confirmada | — | Todos |
| P1 | Carpetas en `src/`, `include/` y `pruebas/` | P0 | Todos (rutas definitivas) |
| P2 | Modelo del ISA como fuente única | P1 | P3, P5, P12 |
| P3 | Rutinas de máquina | P2 | Spilling |
| P4 | Escritura de cadenas JSON | P1 | P9 |
| P5 | Límite de ROM en el ensamblado | P2 | Unrolling (tope) |
| P6 | Aceptación única del parser | P1 | P10, enlazado `P` |
| P7 | Tabla de operaciones complejas | P1 | DCE, reordenamiento |
| P8 | Archivos de las pasadas y gestor | P1 | Las cuatro pasadas |
| P9 | Métricas con contadores y nivel | P4, P8 y los campos de derrame de `TacAllocationMetrics` (P12) | Las cinco optimizaciones |
| P10 | Raíces observables y verificación con pasadas | P6, P8 | Las cinco optimizaciones |
| P11 | Clonar bloques y fijar orden | P1 | Unrolling, reordenamiento |
| P12 | Asignación preparada para el derrame | P2, P3, P9 | Spilling |
| P13 | Cascadas (pospuesto) | P6 | — |
| P14 | Búsqueda directa en la liveness | P10, P12 | Todas: tiempo de compilación de `-O1` |
| P15 | Lectura de variables al estilo del curso (integrado; validación en 9.18) | P14 | DCE y renombramiento (TAC más limpio); métricas de alto nivel |

Las decisiones del 15/11 (4.6) no agregan pasos de refactor: usan la infraestructura existente
(`tac_block_remove`, `tac_blocks_clone`, el interruptor de P12) y un archivo nuevo para Chaitin.

Qué necesita cada optimización para empezar:

| Optimización | Pasos |
|---|---|
| DCE | P7, P8, P9, P10 |
| Renombramiento | P8, P9, P10 |
| Unrolling | P5, P8, P9, P10, P11 |
| Reordenamiento | P7, P8, P9, P10, P11 |
| Spilling | P2, P3, P9, P10, P12 |

P2, P4, P6, P7, P8 y P11 no se pisan entre sí: tocan archivos distintos. P13 es el de mayor
riesgo y el único que no desbloquea nada: se pospuso para después de las optimizaciones.

**Dependencia corregida entre P9 y P12.** `tac_metrics_collect` (P9) copia los contadores de
derrame del resultado de la asignación, y esos campos de `TacAllocationMetrics` los define
P12. Como P9 a P12 se hicieron en un mismo PR (#22), las declaraciones de esos campos se
adelantaron al implementar P9; el resto de P12 se hizo en su orden.

**Cuidado común a todos los pasos.** Varios objetivos de prueba del `makefile` enlazan a mano
un subconjunto de fuentes (por ejemplo `probar_constantes_isa` solo `isa_constant.c`, o
`emitir_crypto` seis archivos). Cuando un paso hace que un módulo llame a otro que antes no
necesitaba, hay que agregar ese fuente a esos objetivos. Cada paso lo indica.

### 9.1 P0. Línea base

**Objetivo.** Confirmar, antes de tocar nada, que `develop` (`6ad9fb4`) pasa todo, y dejar la
referencia para comparar byte a byte.

**Qué hace el agente**

1. `make clean`, `make`, `make check`, `make ejercicios`. Reporta el resultado de cada suite.
2. Guarda fuera del repo una copia del ejecutable como referencia (por decisión del plan, el
   oráculo no entra al repo).
3. Con esa referencia genera, para cada programa de `pruebas/p*.cls` y
   `pruebas/programas/*.cls`: `.bin`, `.hex`, `.mem`, `.asm` y la salida estándar en `-O0`; y
   la respuesta de cada consulta del IDE sobre el corpus que usó la fase 1.
4. Reporta cuántos bundles da cada programa del corpus con `--no-schedule` (dato para P5).
5. Comprueba las fugas del parser con un programa con error de sintaxis (por ejemplo
   `pruebas/e16_falta_punto_coma.cls`) usando la herramienta de memoria disponible. Reporta si
   quedan fugas.

**Terminado cuando:** hay reporte con todo en verde, o con la lista exacta de lo que falla hoy.

### 9.2 P1. Organización en carpetas

**Objetivo.** La misma estructura de carpetas en `src/`, `include/` y `pruebas/`, una carpeta
por componente del mapa. Va justo después de la línea base y antes de cualquier otro paso,
para que los archivos nuevos de los pasos siguientes nazcan en su lugar.

**Regla del paso.** Solo se mueven archivos (`git mv`) y se ajustan rutas. No cambia el
contenido de ningún `.c`, `.h`, `.l` ni `.y`. `grammar/` se queda donde está.

**Estructura** (igual en `src/`, `include/` y `pruebas/`)

| Carpeta | Componente del mapa |
|---|---|
| `comun/` | A2.6 diagnósticos y utilidades |
| `isa/` | A5.7 modelo del ISA |
| `analisis/` | A2 análisis |
| `representacion/` | A3 representación intermedia y sus análisis |
| `optimizador/` | A4 gestor y pasadas; A6 métricas |
| `generador_o0/` | A5.1 generador directo |
| `generador_o1/` | A5.2 bajada, A5.3 asignación, emisión |
| `rutinas/` | A5.4 rutinas de máquina |
| `ensamblado/` | A5.5 calendarización y A5.6 ensamblador |
| `servicios_ide/` | A7 servicios para el IDE |

**`src/`** (`main.c` queda en la raíz de `src/`)

| Carpeta | Archivos |
|---|---|
| `comun/` | `diagnostics.c` |
| `isa/` | `isa_table.c`, `isa_constant.c` |
| `analisis/` | `ast.c`, `ast_constant.c`, `semantic.c`, `semantic_expr.c`, `semantic_scope.c`, `semantic_calls.c`, `semantic_crypto.c`, `symbol_table.c`, `data_area.c` |
| `representacion/` | `tac.c`, `tac_isa.c`, `tac_pseudo.c`, `tac_cfg.c`, `tac_edit.c`, `tac_validate.c`, `tac_print.c`, `tac_loop_marks.c`, `tac_build.c`, `tac_build_expr.c`, `tac_build_flow.c`, `tac_build_init.c`, `tac_build_stmt.c`, `tac_live.c`, `tac_locations.c`, `tac_loops.c`, `tac_intervals.c`; internas: `tac_internal.h`, `tac_build_internal.h`, `tac_live_internal.h`, `tac_analysis_internal.h` |
| `optimizador/` | `tac_passes.c`, `tac_metrics.c` |
| `generador_o0/` | `generate.c`, `generate_expr.c`, `generate_stmt.c`, `generate_init.c`, `generate_support.c`, `label_table.c` |
| `generador_o1/` | `tac_lower.c`, `tac_lower_control.c`, `tac_lower_value.c`, `tac_allocate.c`, `tac_allocate_sites.c`, `tac_emit.c`, `tac_emit_pseudo.c`; internas: `tac_lower_internal.h`, `tac_allocate_internal.h`, `tac_emit_internal.h` |
| `rutinas/` | `codegen_arith.c`, `codegen_array.c`, `codegen_crypto.c` |
| `ensamblado/` | `codegen.c`, `codegen_instruction.c`, `codegen_schedule.c`, `codegen_verify.c`, `assembler.c` |
| `servicios_ide/` | `diagnostics_json.c`, `syntax_service.c`, `suggestions.c`, `repairs.c`, `clones.c` |

Son los 58 fuentes actuales. Los archivos que crean los pasos siguientes van a:
`comun/json_text.c`, `rutinas/codegen_common.c`, `optimizador/tac_pass_*.c`.

**`include/`**

| Carpeta | Cabeceras |
|---|---|
| `comun/` | `diagnostics.h` |
| `isa/` | `isa_table.h`, `isa_constant.h` |
| `analisis/` | `ast.h`, `ast_constant.h`, `semantic.h`, `semantic_internal.h`, `semantic_scope.h`, `semantic_calls.h`, `symbol_table.h`, `data_area.h`, `lexer_service.h`, `parser_service.h` |
| `representacion/` | `tac.h`, `tac_build.h`, `tac_live.h`, `tac_loops.h`, `tac_intervals.h` |
| `optimizador/` | `tac_passes.h`, `tac_metrics.h` |
| `generador_o0/` | `generate.h`, `generate_emit.h`, `generate_internal.h`, `label_table.h` |
| `generador_o1/` | `tac_lower.h`, `tac_allocate.h`, `tac_emit.h` |
| `rutinas/` | `codegen_arith.h`, `codegen_array.h`, `codegen_crypto.h` |
| `ensamblado/` | `codegen.h`, `codegen_schedule.h`, `assembler.h` |
| `servicios_ide/` | `diagnostics_json.h`, `syntax_service.h`, `suggestions.h`, `repairs.h`, `clones.h` |

Son las 38 cabeceras actuales.

**`pruebas/`.** Los programas `p*.cls` y `e*.cls`, `resultados.txt` y `correr_pruebas.sh` se
quedan en la raíz de `pruebas/`: los usan más de 20 archivos (herramientas, IDE,
documentación). Las 35 carpetas de suites pasan a:

| Carpeta | Suites |
|---|---|
| `conductor/` | `ayuda`, `exito`, `tiempo`, `ejecutor` |
| `isa/` | `constantes_isa` |
| `analisis/` | `check` (la que hoy se llama `analisis`), `ast`, `constantes`, `recuperacion` |
| `representacion/` | `tac`, `tac_build`, `tac_live`, `tac_loops`, `tac_analysis`, `tac_intervals` |
| `optimizador/` | `tac_passes` |
| `generador_o0/` | `programas`, `matrices`, `etiquetas`, `diagnosticos_generacion` |
| `generador_o1/` | `tac_lower`, `tac_allocate`, `tac_emit` |
| `rutinas/` | `aritmetica`, `arreglos`, `crypto` |
| `ensamblado/` | `bundles`, `ensamblador`, `simulador` |
| `servicios_ide/` | `analisis_json`, `tokens_json`, `sugerencias_json`, `reparaciones_json`, `clones_json`, `integracion_ide` |

`pruebas/` tiene una carpeta más que `src/` (`conductor/`, las suites de la línea de comandos)
y no tiene `comun/`, porque los diagnósticos se prueban a través de las demás.

**Ajustes de rutas** (medidos sobre `6ad9fb4`)

| Dónde | Qué | Cuánto |
|---|---|---|
| `makefile` | `vpath %.c` con las carpetas de `src/`, para que `SOURCES` siga listando nombres sin ruta (son únicos) | 1 línea |
| `makefile` | `CPPFLAGS` con `-I` de cada carpeta de `include/` y de cada carpeta de `src/` que tiene cabeceras internas. Así **ningún `#include` cambia** | 1 línea |
| `makefile` | Rutas escritas a mano en los objetivos de prueba | 60 de `src/`, 38 de `include/`, 65 de `pruebas/` |
| Pruebas en Python | Las que calculan la raíz del repo por profundidad (`…parents[2]`) pasan a `parents[3]`. Las de `tools/` no cambian: `tools/` no se mueve | 30 archivos bajo `pruebas/`, más `matrices/probar.py`, que usa `parents[1]` |
| Pruebas en C | `#include "../tac_analysis/fixture.h"`: sigue igual entre suites de `representacion/`; cambia en las que quedan en `generador_o1/` | Las que incluyen esa cabecera desde otra carpeta |
| `tools/probar.py` | Rutas de su lista de suites | 16 |
| Documentación | `docs/pruebas.md`, `docs/ccom_servicios_ide.md`, `docs/representacion_intermedia.md`, `README.md`, `ide/docs/guia_sistema_calside.md`, `ide/docs/bitacora_ide.md` | Rutas de `src/`, `include/` y `pruebas/` |

**No se toca:** `grammar/`, `tools/` (salvo `probar.py`), `ide/` (salvo sus dos documentos),
`ejercicios/`, `editors/`. El IDE busca `ccom` en la raíz del repo y los programas en
`pruebas/`: nada de eso se mueve.

**Terminado cuando:** `make clean && make && make check && make ejercicios` pasan; `-O0` byte
a byte igual a la referencia de P0; no queda ningún `.c` ni `.h` suelto en `src/` (salvo
`main.c`) ni en `include/`.

**Riesgo.** Es amplio (unas 200 rutas) pero mecánico: cualquier ruta olvidada rompe la
compilación o una suite, no produce un resultado distinto. El inventario del proyecto
(`inventario_*.md`) queda con rutas viejas; no se actualiza.

### 9.3 P2. Modelo del ISA como fuente única

Desde este paso, las rutas de pruebas y de archivos nuevos son las posteriores a P1. Los
archivos existentes se nombran sin carpeta: sus nombres son únicos.

**Objetivo.** Rango del inmediato, nombres de registros, papel de cada registro y tamaño de la
ROM definidos una sola vez.

**Interfaz nueva en `include/isa_table.h`**

```c
#define ISA_IMMEDIATE_BITS 18
#define ISA_OFFSET_BITS    18
#define ISA_TARGET_BITS    22
#define ISA_ROM_BUNDLES    4096

/* 1 si value cabe en un inmediato con signo de 18 bits (-131072..131071).
 * En la cabecera, para que un módulo la use sin tener que enlazar isa_table.c. */
static inline int isa_immediate_fits(int64_t value) {
    return value >= -(INT64_C(1) << (ISA_IMMEDIATE_BITS - 1)) &&
           value <   (INT64_C(1) << (ISA_IMMEDIATE_BITS - 1));
}

/* "rz","rl","rx","rg","t0".."t11". Fuera de 0..15 devuelve "rz" (como gen_reg_name hoy). */
const char *isa_register_name(int reg);

/* Convención de registros de ccom (el ISA solo fija rz). */
enum { REG_ZERO = 0, REG_LINK = 1, REG_STACK = 2, REG_SPILL = 3,
       REG_TEMP_FIRST = 4, REG_TEMP_LAST = 15, REG_TEMP_COUNT = 12 };
#define REG_CALL_CLOBBERS UINT16_C(0xfff0)
```

**Cambios**

| Archivo | Cambio |
|---|---|
| `isa_table.c` | Implementa `isa_register_name`. Una sola tabla de alias, compartida con `isa_parse_register` |
| `assembler.c` | Borra `IMM_BITS`, `OFFSET_BITS`, `TARGET_BITS` y usa los nuevos. `get_immediate` llama a `isa_immediate_fits`. El texto del error no cambia. `fits_signed` se queda para el destino de 22 bits |
| `codegen_instruction.c` | `immediate()` usa `isa_immediate_fits` |
| `isa_constant.c` | La comparación de rango pasa a `!isa_immediate_fits(value)` |
| `tac_isa.c` | Las dos comparaciones (desplazamiento e inmediato) usan la función |
| `tac_lower_value.c`, `tac_lower_internal.h` | Se elimina `tl_immediate`; sus 4 usos llaman a `isa_immediate_fits` |
| `generate_expr.c` (2), `generate_support.c` (2), `generate_stmt.c` (2) | Cada `> 131071` pasa a `!isa_immediate_fits(...)`; el paso del `S` usa `isa_immediate_fits(loop->step)`. Cada punto conserva su comprobación `< 0` |
| `generate_support.c`, `generate_emit.h` | Se elimina `gen_reg_name`; sus 55 usos (en `generate_*.c`, `tac_emit.c`, `tac_emit_pseudo.c`) pasan a `isa_register_name` |
| `tac_print.c` | `print_physical` usa `isa_register_name` |
| `tac.h` | `TAC_P1_RZ`…`TAC_P1_TEMP_LAST` y `TAC_P1_CALL_CLOBBERS` se definen a partir de `REG_*`. Sus nombres no cambian |
| `generate_internal.h` | `TEMP_FIRST` y `TEMP_COUNT` se definen a partir de `REG_*`. Sus nombres no cambian |
| `tac_emit.c` | El 4096 pasa a `ISA_ROM_BUNDLES`; el mensaje imprime el mismo texto |

**No se toca:** `tools/sim_asm.py` (repetición intencional) ni los resultados esperados de las
suites.

**`makefile`.** `isa_constant.c` se enlaza solo en `probar_constantes_isa`: por eso la función
va en la cabecera. `isa_register_name` vive en `isa_table.c`, que ya está en todos los
objetivos que la necesitan (`TAC_SOURCES` lo incluye).

**Suites que lo cubren:** `check-asm`, `check-constantes-isa`, `check-semantica-isa`,
`check-tac`, `check-tac-lower`, `check-bundles`.

**Terminado cuando:** `131071` y `131072` solo aparecen en `isa_table.c`, en el texto del error
del ensamblador y en comentarios; no queda tabla de alias fuera de `isa_table.c`.

**Documentación:** cabecera de `isa_table.h`; sección de registros de
`docs/representacion_intermedia.md`, con `rg` como registro del derrame.

### 9.4 P3. Rutinas de máquina

**Objetivo.** Que las secuencias fijas que usan los dos caminos vivan en un módulo propio, y
que la emisión de `-O1` deje de depender del generador `-O0`.

**Archivos nuevos:** `src/rutinas/codegen_common.c`, `include/rutinas/codegen_common.h`.

```c
void cg_stack_push(int reg);            /* sw reg, 0(rx) ; saddi rx, rx, 4      (antes gen_stack_push) */
void cg_stack_pop(int reg);             /* saddi rx, rx, -4 ; lw reg, 0(rx)     (antes gen_stack_pop)  */
int  cg_emit_arith_op(void *user, const CGArithOp *operation);   /* antes gen_arith_emit_op */

/* Guarda (restore = 0) o restaura (restore = 1) los registros de mask (bits 4..15) en su
 * casilla temp_base + 4·(r−4), en orden ascendente. Al guardar emite antes una separación.
 * Con mask = 0 no emite nada. Devuelve 0 si el área de la función no es válida. */
int  cg_preserve(const FrameLayout *frame, uint16_t mask, int restore);

void cg_program_end(const char *label); /* etiqueta y bundle fijo con jef a sí misma */
void cg_stack_init(int stack_base);      /* mov rx, stack_base; el llamador pasa data_area_end() */
```

En `include/codegen_crypto.h`, los registros fijos de las plantillas:

```c
enum { CG_CRYPTO_REG_SOURCE = 6, CG_CRYPTO_REG_DEST = 7,
       CG_CRYPTO_REG_COUNT = 9,  CG_CRYPTO_REG_ARGUMENT = 10 };
```

**Cambios**

| Archivo | Cambio |
|---|---|
| `generate_support.c` | Salen `gen_stack_push` y `gen_stack_pop` |
| `generate_expr.c` | Sale `gen_arith_emit_op`; el registro del emisor usa `cg_emit_arith_op` |
| `generate_emit.h` | Se elimina (sus cuatro declaraciones ya no existen). `generate_internal.h` y `tac_emit_internal.h` incluyen `codegen_common.h` |
| `codegen_crypto.c` | `save_live` y `restore_live` se eliminan; llaman a `cg_preserve`. `prepare_arguments` usa las constantes de registros fijos |
| `tac_emit_pseudo.c` | `preserve` arma la máscara desde su plan y llama a `cg_preserve`. `crypto` usa las constantes de registros fijos |
| `tac_emit.c` | `emit_terminal` se elimina; usa `cg_program_end`. El arranque de la pila usa `cg_stack_init`. El aviso de pila usa `STACK_MIN_BYTES` en vez del 256 escrito a mano |
| `generate_stmt.c`, `generate.c`, `generate_internal.h` | `gen_end_of_main` se elimina; su llamada y el bloque del manejador de error aritmético de `generate.c` usan `cg_program_end`. El arranque de la pila usa `cg_stack_init` |
| `makefile` | `codegen_common` entra en `SOURCES` y en el objetivo `emitir_crypto`, que enlaza `codegen_crypto.c` a mano. `cg_stack_init` recibe la dirección para que el módulo no dependa de `data_area.c` |

**Detalle que cuida el byte a byte.** `-O0` escribe los registros preservados como `r4` y
`-O1` como `t0`. `cg_preserve` usa el formato de `-O0`. En `-O1` cambia el texto de esas líneas
del `.asm` (no el binario): hay que actualizar dos cadenas esperadas en
`pruebas/generador_o1/tac_emit/probar.c` (`"sw t0, 64(rz)"` y `"lw t0, 64(rz)"`, y las de `t7`).

**No se toca:** el guardado de argumentos de CRYPTO en `generate_expr.c`
(`emit_crypto_argument`), que usa otra política (casillas libres, no la propia).

**Suites que lo cubren:** `check-crypto`, `check-arith`, `check-arrays`, `check-tac-emit`,
`check-o1-equivalence`, `make ejercicios` (el ejercicio 3 usa CRYPTO).

**Terminado cuando:** ningún archivo `tac_*` llama a una función `gen_*`; `temp_base + 4 *`
aparece solo en `codegen_common.c` y en `emit_crypto_argument`.

### 9.5 P4. Escritura de cadenas JSON

**Archivos nuevos:** `src/comun/json_text.c`, `include/comun/json_text.h`.

```c
void json_write_bytes(FILE *out, const char *text, size_t length);
void json_write_string(FILE *out, const char *text);
```

Su contenido es el código actual de `diag_json_write_bytes`, `diag_json_write_string` y
`utf8_sequence` de `diagnostics_json.c`, **movido sin modificar**.

| Archivo | Cambio |
|---|---|
| `diagnostics_json.c`, `diagnostics_json.h` | Pierden esas funciones; sus llamadas usan las nuevas |
| `ast.c`, `ast.h` | Se elimina `json_string`; 3 llamadas cambian de nombre |
| `tac_metrics.c` | 3 llamadas cambian de nombre; incluye `json_text.h` |
| `clones.c`, `repairs.c`, `suggestions.c`, `syntax_service.c` | Solo el nombre de la función en sus llamadas |
| `makefile` | `json_text` entra en `SOURCES` y en cada objetivo que enlaza `ast.c` a mano (`probar_constantes`, `probar_tac_lower`, `probar_tac_allocate` y los demás que lo listen) |

**Efecto.** Consultas del IDE: idénticas. `--ast-json` y `--metrics`: idénticos con texto
válido; con bytes que no son UTF-8 válido pasan de JSON inválido a JSON válido.

**Suites que lo cubren:** `check-ast`, `check-o1` (ruta con comillas, barra y salto de línea),
y las cinco de los servicios del IDE.

### 9.6 P5. Límite de ROM en el ensamblado

**Cambios**

| Archivo | Cambio |
|---|---|
| `assembler.c`, `assembler.h` | Recibe la función de comprobación, con el mismo contrato y el mismo texto que hoy: `int asm_check_rom(size_t bundle_count, const char *source_name, char *error, size_t capacity);` |
| `tac_emit.c`, `tac_emit.h` | Se elimina `tac_emit_check_rom` y su llamada |
| `main.c` | `assemble_and_write` llama a `asm_check_rom` justo después de ensamblar bien y antes de imprimir bundles o escribir archivos. Usa el nombre del archivo de entrada (en `-O0` no el `"<generado>"` interno) |
| `pruebas/generador_o1/tac_emit/probar.c` | Sus aserciones de 4096 y 4097 llaman a `asm_check_rom` |
| `pruebas/ensamblado/ensamblador/probar.py` | Caso nuevo: `-x` con un `.asm` de 4097 bundles da código 1 y menciona 4096 |
| `pruebas/optimizador/tac_passes/probar_cli.py` | Caso nuevo: el mismo programa grande en `-O0 --no-schedule`, si P0 mostró que pasa de 4096 |

**Cambio visible.** En `-O1`, con `-s`, el `.asm` de un programa que no cabe queda escrito
(como ya pasa en `-O0` ante cualquier fallo del ensamblado). No se escriben `.bin`, `.hex`,
`.mem` ni métricas.

### 9.7 P6. Aceptación única del parser

**Interfaz nueva en `include/parser_service.h`** (implementada al final de `parser.y`):

```c
/* Analiza input completo. Deja en *tree el árbol, que puede ser parcial o NULL: el llamador
 * lo libera siempre. Devuelve 1 solo si el árbol es aceptado: yyparse() == 0, ningún error
 * sintáctico y ningún error léxico. Llama a parser_finish. No abre ni cierra el archivo ni
 * inicia los diagnósticos. */
int parser_accept(FILE *input, ASTNode **tree);
```

| Archivo | Cambio |
|---|---|
| `main.c` | `phase_parse` llama a `parser_accept`. Conserva sus mensajes (usa `lexical_errors` y `parser_recovery_limited()` para elegirlos) |
| `pruebas/ensamblado/bundles/revisar_ir.c`, `pruebas/generador_o1/tac_allocate/desde_fuente.c`, `pruebas/representacion/tac_analysis/desde_fuente.c`, `pruebas/representacion/tac_build/ejecutar.c`, `pruebas/representacion/tac_build/probar.c`, `pruebas/generador_o1/tac_lower/desde_fuente.c` | Llaman a `parser_accept` en vez de repetir las tres condiciones |
| `docs/ccom_servicios_ide.md` | La sección del contrato de `yyparse` nombra la función |

Las consultas del IDE no la usan: tienen su propio modo (`lexer_parse_text`).

### 9.8 P7. Tabla de operaciones complejas

**Objetivo.** Las 11 operaciones (`*`, `/`, `%`, `^` y las 7 CRYPTO) descritas una vez.

**Interfaz nueva en `include/tac.h`** (implementada en `tac_pseudo.c`):

```c
TacPseudoKind tac_pseudo_for(TacOpcode high);     /* TAC_PSEUDO_NONE si no es compleja */
TacOpcode     tac_pseudo_high(TacPseudoKind kind);/* TAC_OP_COUNT para EXIT */
int           tac_pseudo_is_arithmetic(TacPseudoKind kind);   /* pide 8 auxiliares */
```

**Tabla** (privada de `tac_pseudo.c`), una fila por operación: operación de alto nivel, forma
pendiente, mnemónico, cantidad de auxiliares, registros que destruye.

| Archivo | Cambio |
|---|---|
| `tac_pseudo.c` | Las 11 filas completas se reemplazan por la tabla. `tac_pseudo_info` copia las propiedades de la operación de alto nivel (`tac_op_info`) y sobrescribe mnemónico, formato, auxiliares y registros destruidos. `EXIT` conserva su fila propia |
| `tac_lower_value.c` | Se elimina la tabla de parejas; usa `tac_pseudo_for`. La pregunta "¿es aritmética?" usa `tac_pseudo_is_arithmetic` |
| `tac_emit_pseudo.c` | Los dos arreglos que eligen la rutina se indexan por nombre de operación (`[TAC_PSEUDO_MUL] = CG_ARITH_MUL`, …), no por resta de enumerados. El rango `MUL..POW` usa `tac_pseudo_is_arithmetic` |

**Por qué es seguro.** Las 11 filas de `tac.c` y `tac_pseudo.c` coinciden en todo lo que
describe comportamiento (lecturas, escrituras, selectores, barrera, no eliminable, estado
CRYPTO, memoria, puede fallar, crea bloques). Solo difieren en mnemónico, formato, auxiliares y
registros destruidos.

**Suites que lo cubren:** `check-tac` (comprueba cada forma pendiente), `check-tac-lower` (tiene
su propia lista de parejas, que sirve de control independiente), `check-tac-emit`,
`check-o1-equivalence`. No toca el camino `-O0`.

### 9.9 P8. Archivos de las pasadas y gestor

**Archivos nuevos:** `src/optimizador/tac_pass_unroll.c`, `src/optimizador/tac_pass_rename.c`, `src/optimizador/tac_pass_dce.c`,
`src/optimizador/tac_pass_reorder.c`. Cada uno con su función (7.1), que por ahora devuelve 1 sin
transformar, y un comentario de cabecera con su contrato de la parte 7.

| Archivo | Cambio |
|---|---|
| `tac_passes.h` | Declara las cuatro funciones |
| `tac_passes.c` | La tabla apunta a ellas. La fila `spill` queda con `run = NULL`: el gestor no la ejecuta. Se elimina `empty_pass` |
| `main.c` (`phase_o1`) | Ya no llama al gestor en el punto de asignación. Después de asignar bien, si `spill` está activo: lo marca como ejecutado, anota su tiempo (cero hasta P12) y lo reporta para `-v` |
| `pruebas/optimizador/tac_passes/probar.c` | El gestor ejecuta 4 pasadas (antes 5): dos aserciones cambian |
| `makefile` | Los cuatro archivos entran en `SOURCES` |

**Lo visible no cambia:** con `--opt=spill`, el JSON sigue listando `spill` en `pasadas` y en
`tiempo_ms.pasadas`, y `-v` sigue mostrando su línea (ahora después de la asignación).

### 9.10 P9. Métricas con contadores y nivel

| Archivo | Cambio |
|---|---|
| `tac_metrics.h` | `TacPassCounters` (7.1) y `TacMetrics` gana `counters` y `level`. Función `tac_metrics_free` para la lista de lazos |
| `tac_metrics.c` | `tac_metrics_write` imprime `nivel` y `contadores` desde la estructura; `desenrollados` como lista de `{"linea","factor"}`. `tac_metrics_collect` copia los contadores de derrame del resultado de la asignación |
| `main.c` | Fija el nivel; libera las métricas |

Las pasadas anotan en `context->metrics->counters`.

**Lo visible no cambia** mientras las pasadas estén vacías: `nivel` sale `"O1"` y los
contadores en cero, que es lo que fija `pruebas/optimizador/tac_passes/probar_cli.py`.

### 9.11 P10. Raíces observables y verificación con pasadas

**Interfaz nueva en `include/tac_live.h`** (implementada en `tac_locations.c`):

```c
/* Variables escalares globales y de main: el estado observable al terminar el programa.
 * Devuelve un vector propio (liberar con free) y su cantidad en *count. */
TacOperand *tac_observable_roots(const Scope *first_scope, size_t *count);
```

Es la función `observable_roots` de `pruebas/representacion/tac_analysis/desde_fuente.c`, movida; esa prueba
pasa a llamarla.

**Regla del estado observable ante un fallo (11.1).** En `tac_live.c`, para alto nivel: una
instrucción con `may_trap` o `uses_globals` lee además todas las variables globales y, si está
en `main`, las raíces observables. Caso nuevo en `pruebas/representacion/tac_live/probar.c`:
en `main`, una asignación seguida de una división y de otra asignación a la misma variable
queda viva.

| Archivo | Cambio |
|---|---|
| `pruebas/representacion/tac_build/ejecutar.c` | Acepta `--opt=lista` y `--unroll=N`. Después de construir, ejecuta las pasadas de alto nivel pedidas (`tac_passes_run`) y luego interpreta. La salida no cambia de formato |
| `pruebas/optimizador/tac_passes/probar_equivalencia.py` (nuevo) | Por cada programa del corpus sin CRYPTO y cada pasada de alto nivel: ejecuta `interpretar_tac` sin la pasada y con ella; el resultado debe ser idéntico |
| `pruebas/generador_o1/tac_emit/comparar.py` | Acepta `--opt=`, que agrega a la compilación que compara contra `-O0` |
| `makefile` | Objetivos `check-pasadas` (alto nivel) y `check-pasadas-isa` (`reorder` y `spill` con el comparador), dentro de `check` |

Con las pasadas vacías las dos suites pasan sin más; empiezan a comprobar algo real cuando
cada optimización se implementa.

### 9.12 P11. Clonar bloques y fijar orden

**Interfaz nueva en `include/tac.h`** (implementada en `tac_edit.c`):

```c
/* Fija el orden de las instrucciones de un bloque. order contiene las mismas count
 * instrucciones del bloque, cada una una vez. Transaccional: devuelve 0 con diagnóstico, sin
 * cambiar nada, si no es una permutación exacta o si el terminador no queda al final. */
int tac_block_set_order(TacBlock *block, TacInstruction *const *order, size_t count,
                        char *error, size_t capacity);

typedef struct { const TacBlock *original; TacBlock *copy; } TacBlockCopy;

/* Clona count bloques de function (alto nivel) y los inserta, en el mismo orden relativo,
 * después de insert_after. Llena copies con cada pareja original y copia. Transaccional. */
int tac_blocks_clone(TacFunction *function, const TacBlock *const *blocks, size_t count,
                     TacBlock *insert_after, TacBlockCopy *copies,
                     char *error, size_t capacity);
```

**Consultas que pasan a ser públicas.** `tac_locations_visit`, `TacLocationVisit` y
`tac_location_equal` se declaran hoy en `tac_live_internal.h`. Pasan a `tac_live.h`: el
reordenamiento las necesita desde otra carpeta.

**Reglas de la clonación**

| Elemento | En la copia |
|---|---|
| Instrucciones | Iguales, con su línea y columna de origen |
| Operando etiqueta que apunta a un bloque del conjunto | Apunta a la copia de ese bloque |
| Operando etiqueta que apunta fuera del conjunto | No cambia (salidas del lazo) |
| Temporal definido dentro del conjunto y usado solo dentro | Temporal nuevo, el mismo en toda la copia |
| Cualquier otro temporal o variable | No cambia |
| Aristas | No se crean: quien clona llama a `tac_rebuild_edges` |
| Marcas de lazos | No se copian |

**Pruebas nuevas** en `pruebas/representacion/tac/probar.c`: permutación válida; permutaciones rechazadas
(instrucción repetida, instrucción ajena, terminador fuera del final); clonación de dos
bloques con un salto interno, un salto externo y un temporal local, validada después de
reconstruir aristas.

### 9.13 P12. Asignación preparada para el derrame

| Archivo | Cambio |
|---|---|
| `tac_allocate.h`, `tac_allocate.c` | `tac_allocate_registers` recibe `const TacAllocationOptions *options` (7.6). `no_register_available` recibe las opciones; su cuerpo sigue dando el error de hoy (el derrame lo escribe C3.4). `TacAllocationMetrics` gana `spilled`, `spill_loads`, `spill_stores`, `spill_ms` |
| `tac_intervals.h`, `tac_intervals.c` | `TacInterval` gana `loop_depth`: la mayor profundidad de lazo de los bloques donde aparece el temporal (con `tac_loops_analyze` y `tac_loops_depth`) |
| `tac_allocate.h` | `TacEmissionSite` gana `loads` y `stores` (11.6), vacíos mientras no haya derrame |
| `tac_emit.c`, `tac_emit_pseudo.c` | La emisión consulta el plan de toda instrucción y escribe en orden: preservar, cargas, instrucción, guardados, restaurar. En CRYPTO, los argumentos con carga no entran en la permutación paralela. Con planes sin cargas ni guardados, la salida es idéntica a la de hoy |
| `pruebas/generador_o1/tac_emit/probar.c` | Caso nuevo con un plan armado a mano: una instrucción ISA con una carga en `rg` y un guardado; se comprueba el orden del texto emitido |
| `main.c` | Pasa el interruptor según `--opt=spill`; toma de la asignación el tiempo del derrame |
| `pruebas/generador_o1/tac_allocate/probar.c`, `pruebas/generador_o1/tac_allocate/desde_fuente.c`, `pruebas/generador_o1/tac_emit/probar.c` | Sus 8 llamadas pasan `NULL` (sin derrame) |
| `pruebas/representacion/tac_intervals/probar.c` | Caso nuevo: un temporal dentro de un lazo tiene profundidad 1; fuera, 0 |

`rg` no requiere cambios de código: la asignación ya solo reparte `t0`–`t11`, y la validación
ya acepta los 16 registros en ISA física. Queda documentado como reservado.

**Aviso para C3.4.** `data_area_reserve_spill` recibe `Symbol *` y la función de la
representación guarda `const Symbol *`: hay que resolver esa diferencia al implementar.

### 9.14 P13. Cascadas de e71, e72 y e74

**Estado: pospuesto** (decisión del 14/11) para después de las optimizaciones, en una
revisión aparte del IDE y de los errores. Solo cambia los diagnósticos: ninguna optimización
depende de él. Al retomarlo:

- Las cascadas no existen en `main` (entrega del Proyecto I): ese parser no tiene reglas de
  recuperación y se detiene en el primer error sintáctico; el semántico sí reporta varios
  errores. La recuperación sintáctica, y con ella las cascadas, llegó con la integración del
  IDE (PR #17).
- El IDE tiene su propio parser LL(1) en Python, y `ComparacionBison`
  (`ide/pruebas/test_parser.py`) lo compara con el de Bison sobre un corpus: hay que revisar
  si P13 afecta esa prueba.
- Las 154 consultas de referencia (`refactor_ref/consultas/`) darán menos diagnósticos en los
  ejemplos con error en la cabecera de un bloque: diferencia esperada, una por una.

**Objetivo.** Un error en la cabecera de un bloque no debe hacer que se pierda el bloque.

**Cambio en `grammar/parser.y`:** reglas de error en la cabecera de cada bloque, que
sincronizan en el `;` y siguen analizando el cuerpo y su cierre:

```
ciclo_s      : TOKEN_S error TOKEN_SEMICOLON cuerpo TOKEN_DOT
ciclo_l      : TOKEN_L error TOKEN_SEMICOLON cuerpo TOKEN_DOT
condicional  : TOKEN_C error TOKEN_SEMICOLON cuerpo alts residual TOKEN_DOT
alts         : alts TOKEN_A error TOKEN_SEMICOLON cuerpo
```

El nodo resultante es nulo (el árbol parcial nunca se usa) y el cuerpo analizado se libera.

**Comprobado:** en una copia temporal de la gramática, las cuatro reglas juntas no generan
ningún conflicto en Bison 3.8.2 (la gramática pasa de 248 a 265 estados). No se pudo ejecutar
el parser resultante.

**Lo delicado.** El parser lleva un registro propio de bloques abiertos (`syntax_blocks`) y un
estado de recuperación (`syntax_recovering`, ancla). La regla nueva tiene que dejar ese
registro con el bloque abierto y salir del estado de recuperación, como hace `syntax_recover`
para las reglas actuales.

**Terminado cuando**

1. e71, e72 y e74 dan exactamente un diagnóstico cada uno, el correcto.
2. El resto de e01–e79 da los mismos diagnósticos que en P0.
3. Bison sigue sin conflictos.
4. `check-recovery` y las suites de los servicios del IDE pasan.

**Efecto en el IDE.** `--check-json` devuelve menos diagnósticos en esos casos; el formato no
cambia y ningún módulo del IDE necesita cambios. Se elimina la sección "Cascadas de
recuperación" de `docs/ccom_banderas.md`.

**Riesgo.** Es el paso más delicado y no desbloquea ninguna optimización. Si el tiempo aprieta,
es el que se puede dejar para después de las optimizaciones sin costo.

### 9.15 P14. Búsqueda directa en la liveness

**Problema.** `location_index` (`tac_live.c`) recorría todo el universo de ubicaciones en cada
consulta, y `touch_set` (`tac_intervals.c`) y los dos recorridos de vivos de
`tac_allocate_sites.c` preguntaban por cada ubicación. El costo de la asignación crecía con el
cubo del tamaño de la función: con -O1, 800 asignaciones tardaban 33 s y 1600, 200 s. Era un
error de este diseño: la parte 10 la había incluido entre las búsquedas que se quedan.

**Cambio** (PR #23)

| Archivo | Cambio |
|---|---|
| `tac_live_internal.h` | `TacLiveAnalysis` guarda una tabla de posiciones por número de temporal (tamaño `next_temp`) y otra de 16 para los registros físicos; `SIZE_MAX` donde no hay ubicación |
| `tac_live.c` | `location_index` resuelve `TAC_TEMP` y `TAC_PHYS` con esas tablas; `TAC_VARIABLE` conserva la búsqueda lineal (solo en alto nivel, pocas). `collect_location` anota la posición. El orden de las posiciones no cambia |
| `tac_live.h`, `tac_live.c` | `int tac_live_contains_at(const TacLiveAnalysis *a, const TacLiveSet *set, size_t index)` responde por posición (Aho §9.2.5) |
| `tac_intervals.c`, `tac_allocate_sites.c` | Consultan por la posición que ya recorren |
| `pruebas/representacion/tac_live/probar.c` | Caso nuevo: `contains_at` y `contains` coinciden en las tres etapas |

**Resultado.** Conjuntos de vivos idénticos bit a bit; 39/39 `.asm` de -O1 idénticos. Con -O1,
800 asignaciones pasan de 32,7 s a 0,52 s, y 1600 de 200 s a 1,5 s (este último sigue rechazado
por exceder la ROM, como debe).

### 9.16 Cierre del refactor: PRs integrados

| PR | Pasos | Commit |
|---|---|---|
| #18 | Diseño (`docs/diseno_ccom.md`) | `ec4b24c` |
| #19 | P1 | `3e51a56` |
| #20 | P2, P3, P5 | `955b59f` |
| #21 | P4, P6 | `38113d4` |
| #22 | P7 a P12 | `de8f1a9` |
| #23 | P14 | `11d5556` (merge `29f0665`) |
| #24 | P15 | `3d3993a` (merge `66c7d54`) |
| #25 | Refactor completo, de `refactor-c6` a `develop` | `0195d1e` |

P0 no tiene PR: es la línea base, guardada fuera del repo en `refactor_ref/` (`ccom_ref`, `o0/`,
`consultas/`, `manifest.json`).

P15 entró por el PR #24, con las comprobaciones completas de 9.18. Antes del PR #25 se repitió
sobre `refactor-c6` la validación de AddressSanitizer (92/92 limpios) y de `check-ide` (30
fallos y 1 error conocidos, ninguno nuevo).

### 9.17 Validación final de `refactor-c6` (`29f0665`)

| Comprobación | Resultado |
|---|---|
| `make` | Sin warnings |
| `make check` | Completo |
| `make ejercicios` | Código 0 |
| -O0 contra la referencia | 84/84 imágenes idénticas; solo diferencias de ruta (16 `.asm`, 28 stderr) |
| Consultas del IDE | 154 invocaciones, 462/462 idénticas |
| Equivalencia de -O1 | 39/39 |
| AddressSanitizer y UBSan | 92/92 limpios (39 con -O1, 28 con -O0, 22 `--check-json`, e71/e72/e74) |
| Tiempo de -O1, 800 asignaciones | 0,59 s y 0,61 s (una muestra) |
| `make check-ide` | 427 métodos con el mismo resultado por método que `feat/calside` (`6ebeffb`): 31 fallos y 1 error, todos preexistentes. Los 29 de la fase 1 más 2 de la terminal integrada que dependen del entorno. Requiere `XDG_DATA_HOME` aislado: sin él, las pruebas de papelera real se cuelgan en los dos árboles |

### 9.18 P15. Lectura de variables al estilo del curso

**Construcción de valores.** Un `AST_IDENTIFIER` escalar devuelve `tac_variable(símbolo)`
sin instrucción: `E → id { E.place = id.place }`, Aho §6.4. Para `x = g + h ;` se
construye `t1 = g + h; x = t1`, sin las copias de lectura de g/h.

Un operando VARIABLE solo se copia a TEMP en dos casos:

1. Queda una llamada a función por evaluar en la misma construcción. Se detectan
   `AST_CALL`/`AST_INDEX` cuyo símbolo es función, a cualquier profundidad. La copia
   ocurre antes de esa evaluación posterior, para conservar el valor leído.
2. `R` reasigna parámetros simultáneamente: captura sus argumentos antes de escribirlos,
   incluso sin llamadas. Por ejemplo, `R f[b,a,...]` debe intercambiar los valores anteriores.

Los literales y TEMP no requieren otra captura. La regla protege binarios, argumentos,
fila antes de columna, inicio antes de un fin de S con llamadas, valor derecho antes de
índices del destino y los argumentos de cifrar/descifrar. C/A/L consumen sus condiciones
antes del cuerpo; el corto circuito conserva su temporal de unión; cada inicializador se
consume antes del siguiente. RETURN consume directamente el símbolo de retorno.

**Captura y marcas de S.** Inicio y fin se resuelven en el ámbito exterior; el control es
un símbolo nuevo Vi. El constructor evalúa ambos, escribe el control y guarda el fin en
`$limite_Sk` antes del cuerpo. El cuerpo puede cambiar la variable exterior del fin, pero la
prueba sigue leyendo la casilla privada. No hace falta una copia a TEMP solo para la marca.

- `initial` conserva INTEGER o TEMP existente; si inicio es VARIABLE, queda NONE
  (inicio dinámico no conservado en metadatos; no significa cero).
- `bound` conserva INTEGER o TEMP existente; si fin es VARIABLE, referencia exactamente
  `tac_variable(limit)`, nunca la variable exterior.
- `limit` tiene exactamente una definición explícita en la función, contada con
  `tac_definitions` por identidad de Symbol. Su bloque está fuera del lazo natural de la
  marca y domina su cabecera. Hay diagnósticos distintos para operandos inválidos, ausencia
  o multiplicidad de definiciones, definición interior y falta de dominancia.

La unicidad es estructural, no una garantía de ausencia de alias de memoria. Este contrato
no agrega comprobación de índices dinámicos ni cambia la semántica de acceso a arreglos.
La bajada, la validación HIGH, el intérprete, la vivacidad y las métricas admiten VARIABLE.
El intérprete conserva su rechazo contractual de CRYPTO; esos controles usan el comparador O0.

#### Efecto en bundles

Menos instrucciones HIGH no garantiza menos bundles. Al eliminar la copia de lectura, cada
PARAM VARIABLE carga al materializarse, justo antes de guardar su casilla de parámetro. El
organizador conserva el orden entre accesos de memoria; no adelanta la siguiente carga sobre
el store anterior. Con consumidor desde B+2 aparecen huecos carga→store. Es el mismo patrón
de transferencia carga/store que usa O0 al desapilar cada argumento; no se afirma identidad
de instrucciones ni de evaluación interna entre los dos generadores.

La línea base acepta ese aumento: la bajada de PARAM no se cambia en P15. Recuperarlo queda
a cargo del reordenamiento de §11.5 cuando se implemente; la pasada actual es un stub. El
análisis de `build/p15_analisis/` reprodujo ejercicio3 667→705 y p04_memoria 143→148 bundles.
En p04 también se carga la columna de matriz después del producto de la fila, retrasando la
suma y la siguiente multiplicación. Ambos programas conservaron el estado observable contra O0.

**Criterio de aceptación de esperados ISA.** La revisión de los doce listados permite
renumeración biyectiva de temporales/registros virtuales por función; eliminación de copias
de VARIABLE; y mover su `lw` hacia el consumidor dentro del mismo bloque, conservando
dirección y base. Después de quitar las copias y apartar esas cargas, el resto debe mantener
operaciones, orden, etiquetas, bloques y saltos. No se permite ningún otro cambio.

Once esperados cambian: cinco solo renumeran, cinco mueven una carga cada uno además de
renumerar, y corto_circuito elimina 22 copias además de renumerar. Las cargas de esas copias
escriben directamente el temporal de unión; la biyección se verifica entre las identidades
que sobreviven. constantes_grandes permanece idéntico. No se modificaron el auxiliar, la
suite ni el comparador para aceptar resultados. Auditoría: `build/p15/final/auditoria_isa.json`.

**Resultados finales (15/11; P15 integrado por el PR #24).**

| Comprobación | Resultado de esta corrida |
|---|---|
| `make clean`, `make`, `make check` | Códigos 0; compilación sin warnings; check completo, incluidos los doce esperados ISA |
| `make ejercicios` | Código 0; cuatro textos regenerados conservados bajo build/ y originales restituidos con SHA-256 |
| -O0 contra manifest de P0 | 84/84 imágenes BIN/HEX/MEM idénticas; solo 16 ASM y 28 stderr por rutas, enumerados en el reporte |
| Consultas nativas del IDE | 154/154; 462 stdout/stderr/códigos idénticos |
| `make check-o1-equivalence` independiente | 39/39, con imágenes externas cero y patrón |
| Versión previa reconstruida fuera del repo | 113 fuentes/headers/gramática/makefile coinciden con hashes pre-P15; sus 39 ASM y 117 imágenes coinciden con build/p15/antes/ |
| Simulación antes/después | 78 pares (39 fuentes × 2 imágenes); sin avisos, terminación y estado observable iguales |
| HIGH total, 39 fuentes | 2029 → 1673 (−356) |
| Bundles totales, 39 fuentes | 5872 → 5898 (+26) |
| Ciclos totales del simulador | 85143 → 85487 (+344); mismo resultado con cero y patrón; suma del corpus, no de una aplicación única |
| Tiempo -O1, recto de 800 asignaciones | 506,575 → 614,063 ms; pared 0.510 → 0.619 s, una muestra por versión |
| Tiempo -O1, S con 800 asignaciones | 512,867 → 493,062 ms; pared 0.517 → 0.497 s, una muestra por versión |

Los tiempos son mediciones nuevas de ambos ejecutables reconstruidos en este entorno con
los mismos fuentes de P14; no constituyen un perfil ni una garantía de aceleración.

Tabla resumida de todas las subidas (HIGH/bundles/ciclos antes → después):

| Fuente | HIGH | Bundles | Ciclos | Causa |
|---|---:|---:|---:|---|
| `ejercicios/ejercicio2_matriz/ejercicio2.cls` | 56 → 51 | 171 → 172 | 724 → 735 | Columna 2D al uso: +2 en comparación, −1 al guardar posición |
| `ejercicios/ejercicio3_cifrado/ejercicio3.cls` | 191 → 133 | 667 → 705 | 16087 → 16285 | PARAM, captura S y operandos al volver de productos |
| `pruebas/generador_o0/programas/s_anidados.cls` | 21 → 18 | 57 → 58 | 485 → 497 | Carga de x después de i*j |
| `pruebas/p04_memoria.cls` | 56 → 46 | 143 → 148 | 591 → 604 | PARAM de bandera e índices 2D |
| `pruebas/p06_tipos.cls` | 74 → 66 | 203 → 202 | 1685 → 1686 | Captura de tope en S: +1 ciclo; dos bundles eliminados en rama no ejecutada |
| `pruebas/p11_ejercicio3.cls` | 191 → 133 | 667 → 705 | 16087 → 16285 | Mismo ASM ejecutable que ejercicio3 |
| `pruebas/p12_ejercicio2.cls` | 56 → 51 | 171 → 172 | 724 → 735 | Mismo ASM ejecutable que ejercicio2 |
| `pruebas/rutinas/arreglos/matriz3x4.cls` | 68 → 57 | 208 → 210 | 1444 → 1456 | Columna 2D al uso, +2 por comparación |

Las ocho subidas tienen causa identificada en carga al uso de VARIABLE/PARAM. El reporte
incluye fragmentos de ASM y las contribuciones dinámicas por bloque; no hay subidas sin
explicar. El caso p06 muestra por qué menos bundles estáticos no implica menos ciclos:
la copia lógica eliminada está en una rama no ejecutada y la captura S sí se ejecuta.

Reporte A–G, tabla completa de 39 programas, diferencias de referencia y registros:
`build/p15/final/reporte.txt`. Datos de medición: `tabla.json`, `tiempos.json`,
`comprobaciones.json`; ASM completos antes/después y bloques de las subidas bajo esa carpeta.

## 10. Lo que se queda como está

| Qué | Dónde | Por qué se queda |
|---|---|---|
| Los dos caminos de generación, con selección de instrucciones distinta (`>=` son dos instrucciones en `-O0` y una en `-O1`) | `generate_*.c` y `tac_lower_*.c` | Decisión: `-O0` es el generador del Proyecto I, intacto |
| El simulador con su propia tabla de instrucciones | `tools/sim_asm.py` | Cumple otro papel: es el modelo contra el que se verifica |
| El material LL(1) en Python | `ide/nucleo/` | Estudio y documentación; no participa en el análisis |
| Tabla de símbolos con lista por ámbito | `symbol_table.c` | Ámbitos pequeños, una búsqueda por identificador, resultado anotado en el AST. Una tabla de dispersión sería sobreingeniería. Reversible: cabe en ese archivo. Preguntar al profesor qué espera por "eficiente" |
| Tabla de etiquetas, etiquetas del ensamblador y búsqueda de direcciones en el listado | `label_table.c`, `assembler.c`, `codegen.c` | Mismo criterio |
| Búsquedas "instrucción → dato" de cada análisis de `-O1` | `tac_live.c`, `tac_intervals.c`, `tac_allocate.c`, `tac_emit.c` | Son de análisis distintos, cada una sobre sus propios datos; solo comparten la forma. **No incluye** la búsqueda de la posición de un valor en los conjuntos de vivos: esa fila la incluía por error y se corrigió en P14 (9.15) |
| Cabecera del protocolo del IDE escrita en 5 servicios | `diagnostics_json.c`, `syntax_service.c`, `suggestions.c`, `repairs.c`, `clones.c` | Una línea fija por archivo; el IDE y las suites la validan en cada respuesta |
| Acoplamiento del parser y el léxico con los servicios del IDE | `parser.y`, `lexer.l` | Funciona; desacoplarlo es rediseñar los motores del IDE |
| Lectura del fuente con límite, repetida | `suggestions.c`, `syntax_service.c` | Es del IDE, fuera de esta tarea |
| Lista de suites en dos lugares | `makefile`, `tools/probar.py` | No afecta a ningún camino |
| La función `out`, igual en dos archivos | `codegen_arith.c`, `codegen_array.c` | Diez líneas privadas; cada módulo se prueba enlazado solo, y compartirla los ata entre sí |
| La función `memory`, igual en dos archivos | `codegen_schedule.c`, `codegen_verify.c` | El verificador es independiente del organizador a propósito |
| Guardado de argumentos de CRYPTO en `-O0` | `generate_expr.c` | Usa otra política que la preservación común |
| `--no-schedule` | `main.c` | Bandera de verificación |

## 11. Diseño técnico de las optimizaciones

Para cada una: interfaz, estructuras con todos sus campos, pseudocódigo, errores y casos de
prueba con entrada y resultado esperado. La parte 7 da el contrato; aquí está cómo se
construye. Notación del pseudocódigo: `←` asigna; los nombres en `código` son funciones y
campos reales del repo.

### 11.1 Regla común: el estado observable ante un fallo

Si una división entre cero o un exponente negativo detienen el programa, lo que queda en
memoria en ese momento tiene que ser lo mismo que sin optimizar. Por eso **toda instrucción
que puede fallar, y toda llamada (porque la función llamada puede fallar), se considera una
lectura del estado observable**: de todas las variables globales, y además de las variables
escalares de `main` cuando la instrucción está en `main`.

Se implementa una sola vez, en la liveness de alto nivel (`tac_live.c`, paso P10):

```
usos(i) ← usos explícitos de i
si etapa = alto nivel y (info(i).may_trap o info(i).uses_globals):
    usos(i) ← usos(i) ∪ variables globales escalares
    si la función es main:
        usos(i) ← usos(i) ∪ raíces observables
```

DCE y renombramiento solo consultan la liveness. No afecta a `-O1` sin pasadas: la asignación
usa la liveness de ISA, donde las variables no son ubicaciones.

Ejemplo, en `main`: `x = 1 ;  y = a / b ;  x = 2 ;`. Sin la regla, `x = 1` parece muerta; si
`b` vale 0 el programa se detiene con `x` sin actualizar. Con la regla, la división lee `x` y
`x = 1` se conserva.

### 11.2 DCE (`src/optimizador/tac_pass_dce.c`)

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_dce(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel válido | 1; programa sin instrucciones muertas y sin los bloques inalcanzables que se puedan quitar; `counters.removed` y `removed_blocks` actualizados | 0 con texto en `detail` si falla un análisis |

**Funciones privadas**

```c
static int    trap_is_impossible(const TacInstruction *instruction);
static int    is_removable(const TacInstruction *instruction, const TacOpInfo *info,
                           const TacLiveAnalysis *live);
static size_t sweep(TacFunction *function, const TacOperand *roots, size_t root_count,
                    char *detail, size_t capacity, int *failed);
static size_t remove_unreachable(TacFunction *function, char *detail, size_t capacity,
                                 int *failed);
```

**Pseudocódigo**

```
tac_pass_dce(contexto):
    raices ← tac_observable_roots(contexto.scopes)
    para cada función f del programa:
        repetir:
            n ← sweep(f, raices);  si falló: liberar raices; devolver 0
            contadores.removed ← contadores.removed + n
        mientras n > 0
        m ← remove_unreachable(f);  si falló: liberar raices; devolver 0
        contadores.removed_blocks ← contadores.removed_blocks + m
    liberar raices
    devolver 1

sweep(f, raices):
    vivos ← tac_live_analyze(f, raices);  si NULL: falló
    n ← 0
    para cada bloque b de f:
        i ← b.first
        mientras i ≠ NULL:
            siguiente ← i.next                 // tac_remove libera i
            info ← tac_instruction_info(i)
            si is_removable(i, info, vivos):
                tac_remove(b, i);  n ← n + 1
            i ← siguiente
    tac_live_free(vivos)
    devolver n

is_removable(i, info, vivos):
    si info.nonremovable o info.memory_write o info.crypto_state o info.barrier: falso
    si info.flow ≠ TAC_FLOW_NEXT: falso                       // saltos y retornos
    si info.may_trap y no trap_is_impossible(i): falso
    definiciones ← tac_definitions(i)
    si no hay definiciones: falso
    vista ← tac_live_instruction(vivos, i)
    para cada d en definiciones:
        si tac_live_contains(vivos, vista.after, d): falso
    verdadero

trap_is_impossible(i):
    si i.op es TAC_DIV o TAC_MOD: i.src2 es entero y i.src2 ≠ 0
    si i.op es TAC_POW:           i.src2 es entero y i.src2 ≥ 0
    si no: falso

remove_unreachable(f):                    // decisión 4.6.1
    m ← 0
    repetir:
        tac_rebuild_edges(f)
        lazos ← tac_loops_analyze(f);  si NULL: falló
        lista ← bloques b de f, en orden de lista, con tac_loops_reachable(lazos, b) = 0
        tac_loops_free(lazos)
        quitado ← falso
        para cada b en lista, mientras no quitado:
            si tac_block_remove(f, b) = 1: quitado ← verdadero;  m ← m + 1
        si no quitado: devolver m         // vacía, o lo que queda no se puede quitar: se deja
```

`tac_block_remove` exige el grafo válido y rechaza un bloque al que otro le salte con etiqueta
o que tenga una marca. Las marcas ya están descartadas cuando corre DCE: el gestor las
descarta si unroll no está activo, y unroll las descarta al empezar. Como el constructor no
genera ciclos formados solo por saltos con etiqueta (4.5), en cada vuelta hay al menos un bloque
del conjunto que se puede quitar; si alguna vez no lo hubiera, quedan sin quitar y no es error.

Dentro de un barrido la liveness queda desactualizada al borrar, pero solo en el sentido seguro:
puede dar por vivo algo que ya no lo está. El barrido siguiente lo recalcula.

**Casos de prueba** (los de programa fuente se calculan sobre su HIGH real; los marcados
"unitaria" usan HIGH armado a mano)

| Caso | Entrada | Esperado |
|---|---|---|
| Local muerta | Función `void` con `Vm x : int = a * 2 ;` y `x` nunca leída | Se eliminan `t0 = a * 2` y `x = t0`; `removed = 2` |
| Local muerta con retorno | Lo mismo en una función `int r` que asigna `r` antes de retornar | Se eliminan además `r = 0`; `removed = 3` |
| Llamada entre medias | Lo anterior con una llamada que modifica una global | `PARAM`, `CALL` y el efecto global se conservan |
| División entre constante | Local muerta con `a / 4` | Se eliminan la división y su copia; `removed = 2` |
| División entre variable | Local muerta con `a / b` | Se conserva la división (puede fallar); se elimina la copia `y = t`; `removed = 1` |
| `MOD` y `POW` seguros | Locales muertas con `a % 3` y `a ^ 2` | Se eliminan; 2 instrucciones cada una |
| Constante insegura (unitaria) | HIGH con divisor entero 0 o exponente negativo | Se conserva (en fuente, `a / 0` y `a ^ -1` son error semántico) |
| Observable | Local con inicializador, sin redefinir, en `main` | No se elimina nada |
| Fallo en `main` | `x = 1 ; y = a / b ; x = 2 ;` en `main`, con inicializadores | `x = 1` se conserva |
| Unión inalcanzable | `C` con `E` en las dos ramas seguido de `E` | `removed_blocks = 1` |
| Efectos | Escritura a arreglo y CRYPTO | Se conservan, junto con el cálculo de la dirección que usa el store |

**Evidencia C3.2.** `pruebas/optimizador/dce/probar.c` construye HIGH desde fuentes
para los diez casos fuente de la tabla; los divisores enteros cero (`DIV` y `MOD`) y
el exponente negativo se arman directamente como HIGH. También comprueba que un ciclo
inalcanzable de etiquetas se conserva sin error, y que dos bloques referenciados en
orden inverso se retiran en vueltas sucesivas. Cada caso vuelve a ejecutar la pasada
para exigir cero cambios adicionales. La eliminación de un bloque con `RETURN` suma
solo `removed_blocks`, como confirma d2 (14.3).

### 11.3 Renombramiento (`src/optimizador/tac_pass_rename.c`)

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_rename(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel válido | 1; redefiniciones locales renombradas; `counters.renamed` actualizado | 0 si falla un reemplazo |

**Estructura**

```c
typedef struct {
    const Symbol *variable;   /* candidata definida más de una vez en el bloque */
    size_t total;             /* cuántas definiciones tiene en el bloque, incluidas las implícitas */
    size_t seen;              /* cuántas se han recorrido */
    TacOperand current;       /* temporal que la reemplaza ahora; tac_none() si ninguno */
} RenameEntry;
```

**Pseudocódigo**

```
tac_pass_rename(contexto):
    raices ← tac_observable_roots(contexto.scopes)
    para cada función f:
        es_main ← f es main
        para cada bloque b de f:
            contadores.renamed ← contadores.renamed + rename_block(contexto, f, b, es_main, raices)
    liberar raices
    devolver 1

rename_block(contexto, f, b, es_main, raices):
    tabla ← collect(contexto, f, b)          // candidatas con total ≥ 2
    si tabla vacía: devolver 0
    n ← 0
    para cada instrucción i de b, en orden:
        para cada entrada e con e.current definido:
            tac_replace_reads(b, i, tac_variable(e.variable), e.current)
        para cada definición d de i que sea la variable de una entrada e:
            e.seen ← e.seen + 1
            si e.seen < e.total y can_rename(i, e.variable, es_main, raices):
                e.current ← tac_temp_new(f)
                i.dest ← e.current
                n ← n + 1
            si no:
                e.current ← tac_none()
    devolver n

collect(contexto, f, b):
    para cada instrucción i de b, para cada definición d de tipo variable:
        si tac_variable_is_rename_candidate(f, d.symbol, contexto.scopes):
            sumar 1 al total de esa variable
    devolver las entradas con total ≥ 2

can_rename(definicion, variable, es_main, raices):        // decisión 4.6.3
    si no es_main o variable no está en raices: verdadero
    para cada instrucción j después de definicion, hasta la siguiente definición de variable:
        info ← tac_instruction_info(j)
        si info.may_trap o info.uses_globals: falso
    verdadero
```

**Casos de prueba** (sobre el HIGH real)

| Caso | Entrada | Esperado |
|---|---|---|
| WAW y WAR | `Vm x : int = a + b ; c = x * 2 ; x = a - b ;` con `x` local | La primera `x` pasa a un temporal y su lectura también; `renamed = 1` |
| Declaración sin inicializador | `Vm x : int ;` y después `x = a + b ; r = x * 2 ; x = a - b ; …` | `x = 0` es una definición más: `renamed = 2` |
| Tres definiciones | `x` inicializada y redefinida dos veces en un bloque | `renamed = 2` |
| Una definición | `x` con inicializador y sin redefinir | `renamed = 0` |
| Parámetro, global, retorno | Redefiniciones de cada uno | `renamed = 0` (no son candidatas) |
| Entre bloques | `x` inicializada en un bloque y redefinida dentro de un `C` | `renamed = 0` |
| Fallo en `main` | `Vm x : int = 1 ; Vm y : int = a / b ; x = 2 ;` en `main` | `renamed = 0`: la división está entre las dos definiciones |
| División que lee la variable | En `main`: `x = 1 ; y = x / b ; x = 2` | `renamed = 0`: la división puede fallar y `x` debe estar escrita |

### 11.4 Unrolling (`src/optimizador/tac_pass_unroll.c`)

**Forma de un lazo `S` en la representación** (la que construye `tac_build_flow.c`, medida en
la revisión):

```
previo:  control = inicio ; limite = fin ; goto prueba
prueba:  if control >= limite goto salida        (paso > 0; con paso < 0 es <=)
cuerpo:  …                                       (se entra por caída)
retorno: control = control + paso ; goto prueba   (muchas veces es el mismo bloque que el cuerpo)
salida:
```

Un `C` dentro del cuerpo agrega bloques alcanzados por caída y una unión con el incremento. La
marca da `header` (prueba), `body`, `latch` (retorno), `control`, `limit`, `initial`, `bound` y
`step`. El destino de un salto es su operando `dest`.

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_unroll(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel con marcas; `context->unroll_factor` (0 o 2..16) | 1; lazos desenrollados; un elemento en `counters.unrolled` por lazo | 0 si falla un análisis, una clonación o la reconstrucción de aristas |

**Estructura**

```c
typedef struct {
    TacLoopMark mark;           /* copia: las marcas se descartan antes de editar */
    const TacBlock **blocks;    /* bloques del lazo natural sin la cabecera, en orden de lista */
    size_t block_count;
    size_t size;                /* instrucciones de esos bloques */
    TacBlock *previous;         /* único bloque de fuera del lazo que entra a la cabecera */
    TacBlock *exit;             /* destino de la prueba de la cabecera */
    int line;                   /* línea del S */
} LoopPlan;

enum { UNROLL_FULL_LIMIT = 8, UNROLL_DEFAULT_FACTOR = 4 };
#define UNROLL_BUDGET (ISA_ROM_BUNDLES / 4)   /* cota heurística de crecimiento HIGH */
```

**Pseudocódigo**

```
tac_pass_unroll(contexto):
    presupuesto ← UNROLL_BUDGET
    para cada función f:
        planes ← collect_plans(f)                    // usa las marcas y tac_loops_analyze
        tac_loop_marks_discard(f)                    // desde aquí se puede editar
        para cada plan p:
            N ← trip_count(p)                        // −1 si no es constante
            si N ≥ 0 y N ≤ UNROLL_FULL_LIMIT:        copias ← N;  total ← verdadero
            si no: copias ← (contexto.unroll_factor ≠ 0 ? contexto.unroll_factor
                                                        : UNROLL_DEFAULT_FACTOR);  total ← falso
            crecimiento ← p.size × copias            // el parcial conserva además el original
            si total: crecimiento ← p.size × máx(0, copias − 1)
            si crecimiento > presupuesto: continuar  // este lazo no se desenrolla
            presupuesto ← presupuesto − crecimiento
            si total: unroll_total(f, p, N)   si no: unroll_partial(f, p, copias)
            agregar {p.line, copias} a contadores.unrolled
        tac_rebuild_edges(f)
    devolver 1

collect_plans(f):
    lazos ← tac_loops_analyze(f)
    para cada marca m de f con kind = TAC_LOOP_S
                          y tac_loop_mark_state(lazos, m) = TAC_LOOP_MARK_WITH_BACKEDGE:
        bloques ← bloques b de f, en orden de lista, con tac_loops_contains(lazo de m, b) y b ≠ m.header
        si algún bloque de bloques es header de otra marca (S o L): saltar m     // solo internos
        si algún bloque de bloques tiene un sucesor fuera del lazo: saltar m     // salida anticipada (4.6.2)
        exteriores ← predecesores de m.header que no están en el lazo
        si exteriores no tiene exactamente un bloque: saltar m
        previous ← ese bloque
        exit ← destino (dest) de la última instrucción de m.header
        guardar plan
    tac_loops_free(lazos)

trip_count(p):
    // Solo INTEGER en la marca: NONE, TEMP y VARIABLE($limite_Sk) son no constantes.
    si p.mark.initial o p.mark.bound no son enteros: devolver −1
    a ← initial;  b ← bound;  s ← step
    si s > 0: devolver máx(0, techo((b − a) / s))
    si no:    devolver máx(0, techo((a − b) / (−s)))

unroll_partial(f, p, k):
    G ← tac_block_insert_after(f, p.previous)
    t ← tac_temp_new(f)
    tac_append(G, TAC_ADD, t, variable(control), entero((k − 1) × step))
    tac_append(G, step > 0 ? TAC_IF_GE : TAC_IF_LE, etiqueta(header), t, variable(limit))
    anterior ← G
    para j de 1 a k:
        copias[j] ← tac_blocks_clone(f, p.blocks, p.block_count, anterior)
        anterior ← último bloque de copias[j]
    para j de 1 a k:
        salto ← última instrucción del clon de latch en copias[j]       // era goto header
        salto.dest ← (j < k) ? etiqueta(primer bloque de copias[j+1]) : etiqueta(G)
    última instrucción de p.previous .dest ← etiqueta(G)              // era goto header

unroll_total(f, p, N):
    anterior ← p.previous
    para j de 1 a N: copias[j] ← tac_blocks_clone(…, anterior);  anterior ← último de copias[j]
    para j de 1 a N:
        salto de copias[j] .dest ← (j < N) ? etiqueta(primero de copias[j+1]) : etiqueta(p.exit)
    última instrucción de p.previous .dest ← (N > 0) ? etiqueta(primero de copias[1])
                                                     : etiqueta(p.exit)
    tac_rebuild_edges(f)
    quitar {header} ∪ p.blocks con el ciclo de remove_unreachable de 11.2   // decisión 4.6.1
```

`trip_count` nunca da 0 para un `S` que llega al TAC con inicio y fin enteros: el semántico
rechaza un `S` constante que no se ejecuta. La guarda asume que `control + (k−1)·paso` no
desborda 32 bits; es una precondición documentada.

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Total | `S [ i , 0 , 4 , 1 ]` | 4 copias, sin la prueba del `S` ni salto de retroceso; `{línea, 4}` |
| Paso negativo | `S [ i , 10 , 0 , -2 ]` | 5 vueltas, total |
| Parcial exacto | Fin en una variable que vale 8 | `bound` = `$limite`; guarda, 4 copias; el residuo no ejecuta vueltas |
| Parcial con residuo | Fin en una variable que vale 6 | Una ronda de 4 y 2 vueltas de residuo |
| Menos que el factor | Fin en una variable que vale 3 | La guarda salta directo al residuo |
| Inicio variable | `S [ i , n , 10 , 1 ]` | `initial` = NONE: parcial, aunque `n` tenga valor inicial conocido |
| Fin como expresión constante | `S [ i , 0 , 2 + 2 , 1 ]` | `bound` es TEMP: parcial, no total |
| Inicio y fin dinámicos iguales | Inicio y fin en variables que valen lo mismo | Parcial; el lazo ejecuta 0 vueltas y el resultado no cambia |
| Factor pedido | `--unroll=8` con 100 vueltas | 12 rondas de 8 y residuo de 4 |
| Anidados | Un `S` dentro de otro, o un `L` dentro de un `S` | Solo se desenrolla el `S` interno; el externo se omite |
| `C` dentro del cuerpo | Cuerpo con un `C` sin `E` | Se desenrolla; cada copia conserva sus caídas |
| `E` dentro del cuerpo | Búsqueda con `E` dentro del `S` | No se desenrolla (salida anticipada) |
| Sin retroceso | Cuerpo que siempre retorna | Marca sin retroceso: no se desenrolla |
| Sin presupuesto | Cuerpo de cientos de instrucciones | No se desenrolla; lista vacía |

### 11.5 Reordenamiento (`src/optimizador/tac_pass_reorder.c`)

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_reorder(TacPassContext *context, char *detail, size_t capacity)` | Programa en ISA virtual válido | 1; cada bloque con un orden que respeta sus dependencias; `counters.reordered` actualizado | 0 si `tac_block_set_order` rechaza un orden (error interno) |

**Estructura**

```c
enum { REORDER_MAX_BLOCK = 256, REORDER_MAX_LOCATIONS = 8 };

typedef struct {
    TacInstruction *instruction;
    TacOpInfo info;
    TacOperand reads[REORDER_MAX_LOCATIONS];    /* ubicaciones leídas, incluidas implícitas */
    TacOperand writes[REORDER_MAX_LOCATIONS];   /* ubicaciones escritas */
    size_t read_count, write_count;
    int accesses_memory, writes_memory;
    int memory_known;                 /* 1 si la dirección es rz + constante */
    int64_t memory_address;
    IsaUnit unit;                     /* ALU, salto, memoria; las PSEUDO no tienen isa.instruction */
    int alone;                        /* PSEUDO (operación compleja o CRYPTO): ocupa su ciclo sola */
    size_t priority;                  /* camino más largo hasta el final del bloque */
    size_t ready_at;                  /* primer ciclo permitido */
    size_t cycle;                     /* ciclo asignado */
    size_t pending;                   /* predecesores aún sin colocar */
    int placed;
} ReorderNode;
```

**Pseudocódigo**

```
reorder_block(f, b):
    n ← cantidad de instrucciones de b
    si n < 3 o n > REORDER_MAX_BLOCK: devolver 0
    nodos[0..n) ← una por instrucción; llenar con tac_instruction_info y tac_locations_visit;
                  unidad desde isa.instruction si es ISA; alone = 1 si es PSEUDO
    D ← matriz n × n en ceros
    para a de 0 a n−1, para c de a+1 a n−1:
        D[a][c] ← dependency(nodos[a], nodos[c])
    para a de n−1 a 0:
        nodos[a].priority ← máx sobre c con D[a][c] > 0 de (D[a][c] + nodos[c].priority), o 0
        nodos[a].pending  ← cantidad de c' < a con D[c'][a] > 0
    ciclo ← 0;  colocados ← 0
    mientras colocados < n:
        libres ← {ALU: 2, salto: 1, memoria: 1};  ciclo_ocupado_solo ← falso
        repetir:
            candidato ← nodo sin colocar con pending = 0 y ready_at ≤ ciclo
                         y unidad libre (y ciclo vacío si es alone),
                         de mayor priority; empate: menor posición original
            si no hay candidato o ciclo_ocupado_solo: salir
            candidato.cycle ← ciclo;  candidato.placed ← 1;  colocados ← colocados + 1
            descontar su unidad de libres;  si candidato.alone: ciclo_ocupado_solo ← verdadero
            para cada c con D[candidato][c] > 0:
                nodos[c].pending ← nodos[c].pending − 1
                nodos[c].ready_at ← máx(nodos[c].ready_at, ciclo + D[candidato][c])
        ciclo ← ciclo + 1
    orden ← nodos ordenados por (cycle, posición original)
    cambiadas ← cantidad de posiciones donde orden difiere del original
    si cambiadas > 0: tac_block_set_order(b, orden, n)
    devolver cambiadas

dependency(a, c):                    // a está antes que c; distancia mayor (decisión 4.6.4)
    d ← 0
    si alguna ubicación de a.writes está en c.reads:  d ← máx(d, 2)  // RAW
    si alguna de a.reads está en c.writes:            d ← máx(d, 1)  // WAR
    si alguna de a.writes está en c.writes:           d ← máx(d, 1)  // WAW
    si a y c acceden memoria y al menos uno escribe:
        si no (a.memory_known y c.memory_known y a.memory_address ≠ c.memory_address):
            d ← máx(d, 1)
    si c es el terminador del bloque:                 d ← máx(d, 1)
    si a.info.barrier o c.info.barrier:               d ← máx(d, 1)
    si (a.info.may_trap y (c.writes_memory o c.info.barrier))
       o (c.info.may_trap y (a.writes_memory o a.info.barrier)): d ← máx(d, 1)
    devolver d
```

Las ubicaciones se comparan con `tac_location_equal`. `memory_known` vale 1 cuando la
instrucción es una carga o un guardado cuya base es el registro físico `rz`; la dirección es su
desplazamiento (las variables escalares, globales y locales, siempre lo cumplen).

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Dos cadenas (unitaria) | ISA armado: `v1 = lw a ; v2 = add v1,v1 ; v3 = lw b ; v4 = add v3,v3` | Orden 1, 3, 2, 4; `reordered = 2` |
| Una cadena (unitaria) | ISA armado con una sola cadena dependiente | Sin cambios; `reordered = 0` |
| Memoria igual | `sw` y después `lw` en la misma dirección `rz` + constante | Nunca se invierten |
| Memoria distinta | `sw` y `lw` en direcciones `rz` + constante distintas | Pueden cambiar de orden |
| Dirección calculada | Guardado en un elemento de arreglo (base TEMP) y una carga escalar | Conservan su orden |
| Barrera | Instrucciones a los dos lados de un `jas` | Ninguna cruza la llamada |
| Puede fallar | Una división y un guardado | Conservan su orden relativo |
| RAW con el salto final | Comparación que define un TEMP y el salto que lo lee | Distancia 2, no 1 |
| Salto final | Bloque que termina en salto condicional | El salto queda último |

En los programas fuente el contador es de todo el programa (incluye, por ejemplo, la
preparación de parámetros en `main`); su valor exacto se fija con el listado ISA de cada
prueba en el PR.

### 11.6 Spilling (`src/generador_o1/tac_allocate_color.c`, `tac_allocate_sites.c` y la emisión)

**Interfaz** (la deja el refactor, P12; la estrategia la elige el interruptor)

```c
typedef struct { int spill; } TacAllocationOptions;   /* NULL o spill = 0: linear scan sin derrame */

TacAllocationResult *tac_allocate_registers(TacProgram *program, const char *source_name,
    const TacAllocationOptions *options, char *error, size_t capacity);
```

| Situación | Resultado |
|---|---|
| Sin interruptor (`-O1`) | *Linear scan* actual, igual que hoy; si no alcanzan los registros, NULL y "sin registros temporales disponibles" |
| Con interruptor, alcanzan los registros | Chaitin colorea sin derramar |
| Con interruptor, no alcanzan | Chaitin colorea y derrama; planes con cargas y guardados; contadores de derrame |
| Con interruptor, no queda espacio de datos para las casillas | NULL y error de memoria de datos |

**Estructuras**

```c
/* Grafo de interferencia, interno de tac_allocate_color.c */
typedef struct {
    size_t count;             /* temporales de la función */
    unsigned char *adjacent;  /* matriz count × count */
    size_t *degree;           /* grado actual (baja al simplificar) */
    double *cost;             /* Σ 10^profundidad de cada definición y uso */
} ColorGraph;

/* TacFunctionAllocation (interna) gana: */
int   *spill_slot;            /* por temporal: índice de casilla, o -1 si tiene registro */
size_t spill_count;

/* TacEmissionSite (tac_allocate.h, P12): */
typedef struct { unsigned physical_register; int address; } TacSpillMove;
TacSpillMove loads[3];    size_t load_count;     /* después de preservar, antes de la instrucción */
TacSpillMove stores[1];   size_t store_count;    /* después de la instrucción, antes de restaurar */

/* TacAllocationMetrics (P12): */
size_t spilled, spill_loads, spill_stores;  double spill_ms;
```

**Pseudocódigo**

```
allocate_color(f, intervalos):
    G ← grafo con un nodo por temporal; arista si sus intervalos se solapan
    costo(v) ← Σ sobre cada definición y uso de v de 10^profundidad del bloque
    pila ← vacía
    mientras G tenga nodos:
        si hay un nodo con grado < 12: sacar el de menor número, apilarlo (no posible derramado)
        si no: v ← nodo de menor costo(v)/grado(v); empate: intervalo más largo; empate: mayor número
               sacar v, apilarlo como posible derramado
    ultimo_uso[0..11] ← 0;  reloj ← 0
    mientras pila no vacía:
        v ← desapilar
        libres ← colores 0..11 no usados por vecinos ya coloreados de v
        si libres vacío: f.spill_slot[v] ← f.spill_count;  f.spill_count ← f.spill_count + 1
                         // un posible derramado que sí encuentra color no se derrama
                         // (coloreo optimista de Briggs, refinamiento de Chaitin)
        si no: c ← color de libres con menor ultimo_uso (rotación); registro(v) ← t_c
               reloj ← reloj + 1;  ultimo_uso[c] ← reloj
    si f.spill_count > 0: data_area_reserve_spill(símbolo de f, 4 × f.spill_count)
    dirección(casilla k) ← frame.spill_base + 4 × k

// en tac_allocate_sites, por cada instrucción con algún operando derramado
plan_spill_site(f, vivos, i, sitio):
    fuentes ← operandos leídos de i que están derramados, sin repetir
    destino ← operando escrito de i si está derramado
    usados ← registros de los operandos no derramados de i
    disponibles ← según el tipo de i:
        ISA normal:  [rg]  +  [registro del destino, si tiene y no es fuente]  +  prestados
        compleja aritmética:  prestados (nunca rg)
        CRYPTO:  el registro fijo de cada argumento
    para cada fuente s:
        r ← siguiente de disponibles
        agregar {r, dirección(casilla de s)} a sitio.loads
        reemplazar s por r en i
    si destino:
        si i es ISA normal y hay fuentes: r ← registro de la primera fuente
        si no: r ← siguiente de disponibles
        agregar {r, dirección(casilla de destino)} a sitio.stores
        reemplazar destino por r en i
    para cada prestado p que tenga un valor vivo después de i: agregar p a los preservados
    si i es compleja aritmética: elegir sus 8 auxiliares excluyendo usados y prestados

prestados: los registros t0..t11 no usados por i, primero los que no tienen valor vivo,
           en orden ascendente

// en la emisión, para toda instrucción con sitio
cg_preserve(frame, máscara de preservados, guardar)
para cada carga:   cg_op(memoria, "lw <reg>, <dirección>(rz)")
emitir la instrucción (ISA, llamada o expansión)
para cada guardado: cg_op(memoria, "sw <reg>, <dirección>(rz)")
cg_preserve(frame, máscara de preservados, restaurar)
```

En CRYPTO, los argumentos que tienen carga no entran en la permutación paralela: primero se
permutan los que vienen de registros, después se emiten las cargas directo al registro fijo.
Caben siempre: una operación compleja usa como máximo 3 operandos y 8 auxiliares, 11 de los 12
temporales.

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Sin presión | Programas del corpus con `--opt=spill` | `derramados = 0`; mismo estado observable que `-O0` |
| Presión en una expresión | `s1` de 14.5 | `-O1`: error; `--opt=spill`: compila, mismo valor que el cálculo a mano, `derramados ≥ 1` |
| Una fuente derramada | ISA normal con una fuente derramada | `lw rg, …` antes; la instrucción lee `rg` |
| Destino derramado | Definición de un derramado en ISA normal | `sw` a la casilla después |
| Operación compleja | `s3` de 14.5 (fuentes de MUL derramadas) | Operandos en registros prestados; resultado correcto |
| Dentro de un lazo | `s2` de 14.5 | Compila; la víctima sigue costo/grado, con costo ×10 dentro del lazo |
| Dos fuentes, sin destino, CRYPTO | ISA armado que fuerce esas víctimas (unitaria) | Auxiliares según la tabla de 7.6; en fuente no se garantiza que ocurran |

### 11.7 Composición de `-O2` (tarea C3.5)

`parse_options` en `main.c` deja de rechazar `-O2`: equivale a
`--opt=unroll,rename,dce,reorder,spill` y fija el nivel 2. Como incluye `spill`, la asignación de
`-O2` es Chaitin. `pruebas/optimizador/tac_passes/probar_cli.py` hoy espera código 2 para
`-O2`; esa aserción cambia en C3.5.

Los niveles responden a §7.3 del enunciado (benchmarks "con diferentes niveles de
optimización"): `-O0` (Proyecto I), `-O1` (TAC sin optimizar, la línea base) y `-O2` (todas). La
medición aislada de §6 usa `-O1` contra `-O1 --opt=X`.

## 12. Enlazado de archivos (`P`, tarea C2.2)

### 12.1 Sintaxis y semántica (decisión D1)

```
P "matrices" ;
P "utilidades" ;

Vm total : int = 0 ;
F main [] : void ;
    E ;
.
```

- `P "modulo" ;` solo al inicio del archivo, una línea por módulo.
- Se busca `modulo.cls` en la carpeta del archivo que importa.
- Todo se reúne en un solo programa: un análisis semántico, una generación, una salida.
- Cada archivo se incluye una vez; repetidos y ciclos se ignoran sin error.
- Nombres repetidos entre archivos dan el error "ya declarado" del semántico.

### 12.2 Léxico y gramática

| Cambio | Detalle |
|---|---|
| Token `TOKEN_P` | La palabra `P`. No rompe ningún programa: los identificadores empiezan en minúscula. Tipo de token para el IDE: `reservada` |
| Token `TOKEN_STRING` | Patrón `\"[^\"\n]*\"`. Su valor es el texto sin las comillas. Tipo de token para el IDE: `cadena` (nuevo) |
| Cadena sin cerrar | Error léxico "cadena sin cerrar", con posición |
| Gramática | `programa : importaciones globales funciones` ; `importaciones : %empty | importaciones TOKEN_P TOKEN_STRING TOKEN_SEMICOLON` |
| AST | Tipo de nodo `AST_IMPORT`, con el nombre en `text` y su posición. La lista de importaciones es el **último** hijo del programa: globales y funciones siguen en las posiciones 0 y 1, que usan el semántico y los generadores |
| `P` fuera del inicio | Caso propio en `yyreport_syntax_error`, cuando el token inesperado es `TOKEN_P`: "'P' solo puede ir al inicio del archivo" |
| `--ast-json` | Las importaciones aparecen como un hijo más del programa |

### 12.3 Cargador de módulos

Archivos nuevos: `src/analisis/modules.c`, `include/analisis/modules.h`.

```c
/* Analiza main_path y, en profundidad, todos sus módulos. Devuelve un solo AST aceptado, o
 * NULL si algún archivo no se aceptó o no se encontró (el error ya quedó reportado).
 * module_dir: carpeta donde buscar los módulos del archivo principal; NULL = su carpeta.
 * El árbol devuelto se libera con ast_free; después, modules_free. */
ASTNode *modules_load(const char *main_path, const char *module_dir);

int         modules_count(void);            /* archivos cargados */
const char *modules_file_name(int file);    /* ruta del archivo número file; 0 = principal */
void        modules_free(void);
```

**Estructura interna**

```c
typedef struct {
    char *path;          /* como se mostrará en los errores */
    char *canonical;     /* realpath, para reconocer repetidos y ciclos */
    char *directory;     /* carpeta donde se buscan sus propios módulos */
} ModuleFile;
static ModuleFile *files;   static int file_count;
```

**Pseudocódigo**

```
modules_load(principal, carpeta):
    aceptado ← verdadero
    árbol ← load_one(principal, carpeta o carpeta de principal, &aceptado)
    si no aceptado: ast_free(árbol); devolver NULL
    devolver árbol

load_one(ruta, carpeta, aceptado):
    número ← registrar archivo (ruta, realpath, carpeta)
    diag_open(ruta);  ast_set_current_file(número)
    abrir ruta;  lexer_begin(archivo)
    si no parser_accept(archivo, &árbol): aceptado ← falso
    cerrar archivo
    si árbol = NULL: devolver NULL
    para cada importación imp del árbol, en orden:
        ruta_m ← carpeta + "/" + imp.text + ".cls"
        si realpath(ruta_m) ya está registrado: continuar          // repetido o ciclo
        si no existe:
            diag_select(número)
            error en la posición de imp: "modulo no encontrado: '<nombre>' (se busco <ruta_m>)"
            aceptado ← falso;  continuar
        módulo ← load_one(ruta_m, carpeta de ruta_m, aceptado)
        si módulo ≠ NULL: merge(árbol, módulo)
    devolver árbol

merge(destino, módulo):
    // los globales y las funciones del módulo quedan ANTES que los propios
    anteponer los hijos de módulo.globales a destino.globales
    anteponer los hijos de módulo.funciones a destino.funciones
    liberar el cascarón de módulo (programa y listas vacías)
```

"Anteponer conservando el orden de los `P`": los módulos se procesan en orden y cada uno se
inserta después de los ya insertados y antes de las declaraciones propias.

**Ejemplo.** `main.cls` importa `a` y `b`; `a` importa `b`.

| Paso | Acción | Archivos |
|---|---|---|
| 1 | Analiza `main.cls` | 0: main |
| 2 | `P "a"`: analiza `a.cls` | 0: main, 1: a |
| 3 | Dentro de `a`, `P "b"`: analiza `b.cls` y lo une a `a` | 0: main, 1: a, 2: b |
| 4 | `a` (ya con `b` delante) se une a `main` | |
| 5 | En `main`, `P "b"`: ya registrado, se salta | |
| Resultado | Orden de declaraciones: `b`, `a`, `main` | |

### 12.4 Cada error con su archivo

| Cambio | Detalle |
|---|---|
| `ASTNode` | Campo nuevo `int file`. `ast_new` lo toma de la variable que fija `void ast_set_current_file(int file)` |
| `diagnostics.c` | Tabla de archivos (nombre, líneas, posiciones) en vez de uno solo. `int diag_open(const char *path)` carga el archivo y devuelve su número; `void diag_select(int file)` elige el que usan los mensajes siguientes. `diag_init(path)` se conserva: equivale a vaciar la tabla, abrir y seleccionar |
| Léxico | `void lexer_begin(FILE *input)`: `yyrestart(input)`, línea y columna a 1, posición en bytes a 0, contadores de error sin tocar (son del programa completo) |
| Semántico | `semantic_error(node, …)` llama a `diag_select(node->file)` antes de reportar |
| Generador `-O0` | `gen_report_error` usa `modules_file_name(node->file)` |
| Camino `-O1` | Sus errores usan el archivo de la función: `modules_file_name` del nodo de declaración del símbolo |
| `main.c` | `phase_parse` llama a `modules_load` en lugar de `parser_accept` |

Con un solo archivo el número es 0 y todos los mensajes salen idénticos a hoy.

### 12.5 Consultas del IDE

El IDE analiza una copia temporal del documento, en una carpeta temporal
(`ide/servicio_ccom.py`, `ide/interfaz/comprobacion.py`): ahí los módulos no están.

| Consulta | Comportamiento |
|---|---|
| Opción nueva `--module-dir <carpeta>` | Válida en compilación normal, `--check` y `--check-json`. Con las demás consultas: código 2 |
| `--check`, `--check-json` | Siguen los `P`, buscando en `--module-dir` si se dio |
| Error dentro de un módulo, en `--check-json` | Un diagnóstico en el rango del `P` del documento analizado: "error en el modulo '<x>': <mensaje> (<x>.cls:L:C)". Categoría y severidad, las del error original. El protocolo no cambia: un solo `source` |
| Error dentro de un módulo, en texto | Con el archivo, línea y columna del módulo |
| `--tokens-json` | `P` como `reservada`; la cadena como `cadena` |
| `--syntax-json`, `--suggest-json`, `--repair-json` | Solo el texto dado; no abren módulos |
| `--clones-json` | Sin cambios |

**Toca la frontera con el IDE**

| Punto | Módulos del IDE | Cambio |
|---|---|---|
| Carpeta de módulos | `ide/interfaz/comprobacion.py` | Agregar `--module-dir <carpeta del documento>` a la llamada de `--check-json` |
| Tipo de token `cadena` | `ide/ccom.py`, `ide/interfaz/editor.py` | Agregarlo a la lista de tipos válidos y darle formato de color |
| Pruebas | `ide/pruebas/test_protocolo_ccom.py`, `ide/pruebas/test_tokens_ccom.py` | Casos con `P` |

### 12.6 Casos de prueba

Carpeta `pruebas/analisis/modulos/`.

| Caso | Archivos | Esperado |
|---|---|---|
| Dos módulos | `main` importa `a` y `b`; usa una función de cada uno | Compila; mismo resultado que el programa escrito en un solo archivo |
| Repetido | `main` importa `a` dos veces | Compila; `a` se incluye una vez |
| Ciclo | `a` importa `b` y `b` importa `a` | Compila; cada uno una vez |
| Diamante | `main` importa `a` y `b`; los dos importan `c` | Compila; `c` una vez |
| No encontrado | `P "nada" ;` | Código 1; error en la línea del `P`, con el archivo que lo contiene |
| `P` tarde | `P` después de una función | Código 1; "'P' solo puede ir al inicio del archivo" |
| Nombre repetido | La misma función en `main` y en `a` | Código 1; "ya declarado" |
| Error en módulo | `a.cls` con un error semántico | Código 1; el mensaje empieza con `a.cls:L:C` |
| Un archivo | Cualquier programa del corpus | Salidas idénticas a las de antes |
| Consulta | `--check-json --module-dir` sobre una copia de `main` | Encuentra los módulos; sin `--module-dir`, error de módulo no encontrado |

## 13. IDE y medición (tareas C1.2 y C4.2)

### 13.1 Archivo de tres direcciones (`.tac`) en el IDE

Hoy el IDE solo reconoce `.cls` (explorador, búsqueda, icono) y trata `.asm` como entrada de
ensamblado. El `.tac` que escribe `ccom -i` no se puede generar ni ver desde el IDE.

**Qué contiene un `.tac`.** La representación de alto nivel tal como queda después de las
pasadas de alto nivel activas. Formato (el de `tac_print`):

```
initial_data bytes=0

function suma_fila (entry B0)
B0: successors [B1]
  s = 0
  goto B1
B2: successors [B4, B3]
  if j >= $limite_S0 goto B4
B3: successors [B2]
  t3 = t1 * 3
  t7 = t6[t5]
```

**Cambios**

| Módulo | Cambio |
|---|---|
| `ide/compilacion.py` | Opción nueva en `OPCIONES`: `'tac': ('Generar tres direcciones (.tac)', '-i', {'compilar'})`. Cuando está activa, `preparar` agrega `destino.with_suffix('.tac')` a las salidas previstas |
| `ide/interfaz/opciones_compilacion.py` | La casilla aparece sola: el diálogo recorre `OPCIONES` |
| `ide/interfaz/explorador.py` | El filtro de nombres y las dos comprobaciones de extensión pasan de `cls` a un conjunto: `cls`, `tac`. Los `.tac` se muestran, no se crean desde el explorador |
| `ide/interfaz/iconos.py` | Icono para `.tac` (el genérico de documento) |
| `ide/interfaz/editor.py` | Un documento `.tac` se abre en **solo lectura**. No se le pide análisis ni tokens a `ccom`, no tiene sugerencias ni reparaciones (igual que hoy con `.asm` en `reparaciones.py`) |
| `ide/interfaz/resaltado_tac.py` (nuevo) | Resaltado propio, por expresiones regulares, de la sintaxis fija del listado |
| `ide/interfaz/ventana.py` | Acción "Ver tres direcciones" (menú Compilar): abre en una pestaña el `.tac` de la última compilación. Se habilita cuando la compilación terminó bien con la opción activa. Si la pestaña ya está abierta, recarga su contenido |
| `ide/busqueda.py`, `ide/interfaz/clones.py` | Sin cambios: siguen solo con `.cls` |

**Resaltado de `.tac`** (`resaltado_tac.py`, una clase `ResaltadorTac(QSyntaxHighlighter)`)

| Elemento | Patrón | Formato |
|---|---|---|
| Cabecera de función | `^function \w+` | Como palabra reservada |
| Etiqueta de bloque | `\bB\d+\b` | Como etiqueta |
| `successors`, `entry`, `initial_data` | Palabras fijas | Atenuado |
| Control | `\b(if|goto|return|call|param|address)\b` | Como palabra reservada |
| Temporal | `\bt\d+\b` | Como identificador secundario |
| Variable oculta | `\$\w+` | Atenuado |
| Número | `-?\b\d+\b` | Como número |

Usa los mismos colores que el editor ya define para Calser. No es análisis del lenguaje: es
un listado generado, de formato fijo. Por eso no pasa por `ccom`.

**Casos de prueba** (`ide/pruebas/test_compilacion.py`, `test_menu_editor.py`)

| Caso | Esperado |
|---|---|
| Perfil con `tac` | Los argumentos incluyen `-i` y las salidas incluyen el `.tac` |
| `tac` en modo distinto de `compilar` | La opción se descarta |
| Abrir un `.tac` | Pestaña de solo lectura; ninguna consulta a `ccom` |
| "Ver tres direcciones" sin compilar | Acción deshabilitada |
| Explorador | Muestra `.cls` y `.tac`; no muestra `.bin`, `.hex`, `.mem` |

### 13.2 Elegir nivel y pasadas en F5

**`ide/compilacion.py`.** El perfil gana cuatro campos, válidos solo en modo `compilar`:

| Campo | Valores | Argumentos que genera |
|---|---|---|
| `nivel` | `'O0'` (por defecto), `'O1'`, `'O2'` | Nada, `-O1`, `-O2` |
| `pasadas` | Lista con elementos de `PASADAS = ('unroll', 'rename', 'dce', 'reorder', 'spill')`; solo con `nivel = 'O1'` | `--opt=` con los nombres en el orden de `PASADAS`, separados por coma |
| `unroll` | 0 o un entero de 2 a 16; solo si `'unroll'` está en `pasadas` (o con `O2`) | `--unroll=N` si no es 0 |
| `metricas` | `True` o `False`; solo con nivel distinto de `O0` | `--metrics <destino sin extensión>.metricas.json`; ese archivo se agrega a las salidas previstas |

```
validar(datos):
    …lo actual…
    nivel ← datos.nivel si está en ('O0','O1','O2') y modo = 'compilar', si no 'O0'
    pasadas ← [p de PASADAS si p está en datos.pasadas]  si nivel = 'O1', si no []
    unroll ← datos.unroll si es entero en 2..16 y ('unroll' en pasadas o nivel = 'O2'), si no 0
    metricas ← datos.metricas es True y nivel ≠ 'O0'

preparar(datos, fuente):
    …lo actual…
    si nivel = 'O2': args += ['-O2']
    si nivel = 'O1': args += ['--opt=' + ','.join(pasadas)]  si pasadas, si no ['-O1']
    si unroll: args += ['--unroll=%d' % unroll]
    si metricas: args += ['--metrics', ruta];  salidas += [ruta]
```

Con esto `validar` no deja pasar ninguna combinación que `ccom` rechace con código 2.

**`ide/interfaz/opciones_compilacion.py`.** Una lista desplegable "Optimización" con los tres
niveles; cinco casillas de pasadas, habilitadas solo con `O1`; un campo numérico "Factor de
unrolling" (0 = automático); la casilla "Guardar métricas". `datos()` los incluye.

### 13.3 Herramienta de medición

Archivo nuevo: `tools/medir.py` (Python estándar). Objetivo de Make: `metricas`.

```
python3 tools/medir.py --pasada dce --salida build/metricas pruebas/optimizador/dce/*.cls
```

**Funciones**

```python
def compilar(ccom, fuente, carpeta, opciones) -> dict     # corre ccom con -s y --metrics; devuelve el JSON
def simular(asm, mem) -> tuple[int, bytes]                # usa Simulator de tools/sim_asm.py; (ciclos, memoria final)
def estado_observable(asm, memoria) -> bytes              # tramos de globales y de main, leídos de la tabla del .asm
def medir(ccom, fuente, pasada, salida, factor=0) -> dict # una prueba, antes y después
def main(argv) -> int                                     # 0 si todas equivalentes; 1 si alguna no; 2 mal uso
```

`estado_observable` usa el mismo criterio que `pruebas/generador_o1/tac_emit/comparar.py`
(las direcciones de las variables globales y de `main`, tomadas del listado `.asm`).

**`medir`, paso a paso**

1. `antes ← compilar(-O1)`.
2. `despues ← compilar(--opt=<pasada>)`, con `--unroll=N` si se pidió.
3. Simular los dos `.asm`: ciclos y memoria final.
4. `equivalente ←` los dos estados observables coinciden.
5. Escribir `<salida>/<programa>.<pasada>.json`:

```json
{
  "prueba": "d1_asignaciones.cls", "pasada": "dce", "equivalente": true,
  "antes":   {"emitidas": 0, "bundles": 0, "bytes": 0, "ciclos": 0, "tiempo_ms": 0.0},
  "despues": {"emitidas": 0, "bundles": 0, "bytes": 0, "ciclos": 0, "tiempo_ms": 0.0},
  "contadores": {"eliminadas": 0, "bloques_eliminados": 0}
}
```

`emitidas`, `bundles`, `bytes` y `tiempo_ms` salen del JSON de `ccom`; `ciclos`, del
simulador; `contadores`, del JSON de después. Cubre las cinco métricas del enunciado (8.4).

### 13.4 Gráficas en el IDE

Archivo nuevo: `ide/interfaz/metricas.py`.

| Clase | Responsabilidad |
|---|---|
| `PanelMetricas(QWidget)` | Lista de pruebas medidas, selector de pasada, botón "Medir", área de gráficas y tabla de contadores |
| `GraficaBarras(QWidget)` | Dibuja con `QPainter` un par de barras (antes y después) por métrica, con su valor y la variación en porcentaje |
| `MedidorMetricas(QObject)` | Lanza `tools/medir.py` con `QProcess` (como F6 lanza el simulador), informa avance y errores, y al terminar pide recargar |

- Lee los archivos de `build/metricas/` de la carpeta del proyecto.
- Cuatro gráficas por prueba: instrucciones emitidas, bytes, ciclos, tiempo de compilación.
- Si `equivalente` es falso, la prueba se marca en rojo.
- Sin dependencias nuevas: solo PySide6, que la interfaz ya usa.
- Entrada "Métricas de optimización" en el menú de `ide/interfaz/ventana.py`.

## 14. Las 15 pruebas obligatorias (tarea C4.1)

Carpeta `pruebas/optimizador/<pasada>/`. Cada programa se verifica igual: equivalencia contra
`-O0` en el simulador, y el contador esperado en el JSON de métricas. Los esperados se calculan
sobre el TAC real (4.5); cada PR guarda junto al programa su listado HIGH (o ISA virtual) y, si
el número medido difiere del de esta parte, lo corrige con el listado como evidencia.

### 14.1 Unrolling

`u1_constante.cls`: N constante.

```
Vm suma : int = 0 ;

F main [] : void ;
    S [ i , 0 , 4 , 1 ] ;
        suma = suma + i ;
    .
    E ;
.
```

Esperado: `suma = 6`. Desenrollado total, `desenrollados = [{linea 4, factor 4}]`; sin la
prueba del `S` ni salto de retroceso (se conservan el `goto` de entrada y los saltos entre
copias).

`u2_variable.cls`: N variable pequeña.

```
Vm suma : int = 0 ;
Vm n : int = 6 ;

F main [] : void ;
    S [ i , 0 , n , 1 ] ;
        suma = suma + i ;
    .
    E ;
.
```

Esperado: `suma = 15`. `bound` = `$limite_S0`: parcial con factor 4, una ronda y 2 vueltas de
residuo.

`u3_grande.cls`: N grande.

```
Vm suma : int = 0 ;

F main [] : void ;
    S [ i , 0 , 100 , 1 ] ;
        suma = suma + i ;
    .
    E ;
.
```

Esperado: `suma = 4950`. N = 100 > 8: parcial; con el factor por defecto, 25 rondas y residuo
vacío; con `--unroll=8`, 12 rondas y residuo de 4.

Las mejoras de bundles y ciclos frente a `-O1` se reportan medidas, no se dan por supuestas.

### 14.2 Renombramiento

`r1_waw.cls`

```
Vm salida : int = 0 ;

F f [ int a , int b ] : int r ;
    Vm x : int ;
    x = a + b ;
    r = x * 2 ;
    x = a - b ;
    r = r + x ;
    E ;
.

F main [] : void ;
    salida = f [ 5 , 3 ] ;
    E ;
.
```

Esperado: `salida = 18`; `renombradas = 2`. En el HIGH, `x` tiene tres definiciones en el
bloque (`x = 0` implícita, `x = t0`, `x = t2`): se renombran las dos primeras.

`r2_war.cls`

```
Vm g : int = 0 ;

F f [ int a , int b ] : void ;
    Vm x : int = a ;
    g = x ;
    x = b ;
    g = g + x ;
    x = a + b ;
    g = g + x ;
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: `g = 20`; `renombradas = 2` (tres definiciones de `x`). Con `--opt=rename,reorder`
se reporta la diferencia de bundles frente a `reorder` solo.

`r3_limites.cls`

```
Vm g : int = 0 ;

F f [ int a , int b ] : int r ;
    a = 1 ; a = 2 ; g = 1 ; g = 2 ; r = 1 ; r = 2 ;
    E ;
.

F main [] : void ;
    Vm x : int = 1 ;
    Vm y : int = 8 / x ;
    x = 2 ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: `renombradas = 0`. Parámetro, global y retorno no son candidatos; en `main`, `x`
está protegida por la división que la lee entre sus dos definiciones (decisión 4.6.3). La
división es entre `x`, que vale 1, así que el programa no se detiene.

### 14.3 DCE

`d1_asignaciones.cls`

```
Vm g : int = 0 ;

F tocar [ int k ] : void ;
    g = g + k ;
    E ;
.

F calcular [ int a ] : int r ;
    Vm x : int = a * 2 ;
    Vm y : int = a + 1 ;
    tocar [ a ] ;
    r = y ;
    E ;
.

F main [] : void ;
    Vm v : int ;
    v = calcular [ 3 ] ;
    E ;
.
```

Esperado: `g = 3`, `v = 4`. `eliminadas = 3`: en `calcular`, `r = 0`, `x = t0` y `t0 = a * 2`; la
llamada a `tocar` se conserva; en `main`, `v = 0` se conserva porque la llamada lee las raíces.
`bloques_eliminados = 0`.

`d2_inalcanzable.cls`: unión inalcanzable después de un `C` que retorna en todas sus ramas.

```
Vm g : int = 0 ;

F f [ int a , int b ] : void ;
    C [ a == b ] ;
        g = 1 ;
        E ;
    A ;
        g = 2 ;
        E ;
    .
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: `g = 2`; `bloques_eliminados = 1` (el bloque del `E` final de `f`); `eliminadas = 0`.

`d3_efectos.cls`

```
Vm datos : int [ 2 ] ;

F f [ int a , int b ] : void ;
    Vm x : int = a / 4 ;
    Vm y : int = a / b ;
    autenticar [ a ] ;
    datos [ 0 ] = a ;
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: `eliminadas = 3`: la división entre 4 y su copia, y la copia del resultado de `a / b`.
Se conservan la división entre `b` (puede fallar), la operación CRYPTO y la escritura al
arreglo con el cálculo de su dirección.

**Resultados ejecutados C3.2.** Los tres programas anteriores se conservan literalmente
en `pruebas/optimizador/dce/`, con listados `.antes.tac` y `.despues.tac` y los
estados y contadores exigidos en `esperados.json`. Los contadores
medidos en el JSON de `--metrics` son, respectivamente, `3/0`, `0/1` y `3/0`
(`eliminadas/bloques_eliminados`), sin diferencias con estos esperados. En d3,
`datos = [8, 0]`; con memoria externa cero y con el patrón `0..255` repetido,
`AUTH 8` termina con autenticación falsa y excepción CRYPTO activa, igual que sin DCE.
El simulador comprueba además `g = 3, v = 4` en d1 y `g = 2` en d2, junto con el
resto del estado observable, memoria externa y estado CRYPTO. Los ciclos se miden
con el simulador; `ciclos_sim` del JSON del compilador sigue siendo `null`.

### 14.4 Reordenamiento

`o1_cadenas.cls`: dos cálculos independientes escritos uno tras otro.

```
F f [ int a , int b , int c , int d ] : void ;
    Vm p : int ; Vm q : int ; Vm u : int ; Vm w : int ;
    p = a + b ; q = p + p ; u = c + d ; w = u + u ;
    E ;
.

F main [] : void ;
    f [ 1 , 2 , 3 , 4 ] ;
    E ;
.
```

Esperado: `reordenadas > 0` (en el ISA virtual, `f` tiene un bloque de 21 instrucciones con dos
cadenas de carga, suma y store); el valor exacto y los bundles frente a `-O1` se fijan con el
listado del PR.

`o2_saltos.cls`: un `C` con cálculos independientes antes del salto y dentro de cada rama.

```
F f [ int a , int b ] : void ;
    Vm p : int = a + b ;
    Vm q : int = a - b ;
    C [ p > q ] ;
        p = a + 1 ; q = b + 2 ;
    A ;
        p = a + 3 ; q = b + 4 ;
    .
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: reordena dentro de cada bloque; ninguna instrucción cambia de bloque.

`o3_funciones.cls`: una llamada entre cálculos, y una escritura seguida de una lectura de la
misma posición de un arreglo.

```
Vm datos : int [ 2 ] ;

F tocar [ int k ] : void ;
    E ;
.

F f [ int a , int b ] : void ;
    Vm x : int = a + b ;
    tocar [ a ] ;
    x = x + b ;
    datos [ 0 ] = x ;
    x = datos [ 0 ] ;
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
```

Esperado: nada cruza el `jas`; la escritura y la lectura del arreglo (base calculada)
conservan su orden.

### 14.5 Spilling

`s1_expresion.cls`: una expresión anidada a la derecha con 13 productos.

```
F f [ int a1 , int b1 , int a2 , int b2 , int a3 , int b3 , int a4 , int b4 , int a5 , int b5 ,
      int a6 , int b6 , int a7 , int b7 , int a8 , int b8 , int a9 , int b9 , int a10 , int b10 ,
      int a11 , int b11 , int a12 , int b12 , int a13 , int b13 ] : int r ;
    r = a1 * b1 + ( a2 * b2 + ( a3 * b3 + ( a4 * b4 + ( a5 * b5 + ( a6 * b6 + ( a7 * b7 +
        ( a8 * b8 + ( a9 * b9 + ( a10 * b10 + ( a11 * b11 + ( a12 * b12 + ( a13 * b13 ) ) ) ) ) ) ) ) ) ) ) ) ;
    E ;
.

F main [] : void ;
    f [ 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 ] ;
    E ;
.
```

Esperado: con `-O1`, error "sin registros temporales disponibles" (14 temporales vivos a la
vez, 15 intervalos solapados); con `--opt=spill`, compila, `r = 26` y `derramados ≥ 1`. Los
derramados y las cargas y guardados exactos se fijan con el listado del PR.

`s2_lazo.cls`: la misma presión dentro del cuerpo de un `S`. Muchos locales no bastan: cada
variable vive en memoria y se carga al usarse, así que no crean presión de temporales.

```
Vm g : int = 0 ;

F f [ int a1 , int b1 , int a2 , int b2 , int a3 , int b3 , int a4 , int b4 , int a5 , int b5 ,
      int a6 , int b6 , int a7 , int b7 , int a8 , int b8 , int a9 , int b9 , int a10 , int b10 ,
      int a11 , int b11 , int a12 , int b12 , int a13 , int b13 ] : void ;
    S [ i , 0 , 3 , 1 ] ;
        g = g + a1 * b1 + ( a2 * b2 + ( a3 * b3 + ( a4 * b4 + ( a5 * b5 + ( a6 * b6 + ( a7 * b7 +
            ( a8 * b8 + ( a9 * b9 + ( a10 * b10 + ( a11 * b11 + ( a12 * b12 + ( a13 * b13 ) ) ) ) ) ) ) ) ) ) ) ) ;
    .
    E ;
.

F main [] : void ;
    f [ 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 ] ;
    E ;
.
```

Esperado: `-O1` falla; con `--opt=spill`, compila y `g = 78`. Los costos de los temporales del
cuerpo se multiplican por 10; el PR reporta cuáles se derramaron.

`s3_compleja.cls`: como `s1`, con los productos combinados por multiplicaciones, para que los
derramados sean fuentes de operaciones complejas.

```
F f [ int a1 , int b1 , int a2 , int b2 , int a3 , int b3 , int a4 , int b4 , int a5 , int b5 ,
      int a6 , int b6 , int a7 , int b7 , int a8 , int b8 , int a9 , int b9 , int a10 , int b10 ,
      int a11 , int b11 , int a12 , int b12 , int a13 , int b13 ] : int r ;
    r = a1 * b1 * ( a2 * b2 * ( a3 * b3 * ( a4 * b4 * ( a5 * b5 * ( a6 * b6 * ( a7 * b7 *
        ( a8 * b8 * ( a9 * b9 * ( a10 * b10 * ( a11 * b11 * ( a12 * b12 * ( a13 * b13 ) ) ) ) ) ) ) ) ) ) ) ) ;
    E ;
.

F main [] : void ;
    f [ 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 ] ;
    E ;
.
```

Esperado: `-O1` falla; con `--opt=spill`, compila, `r = 8192`, y las cargas de derrame alimentan
operaciones `MUL` con registros prestados.

## 15. Avisos para el cerebro

- Este documento reúne el diseño (fase 3) y la brecha (fase 4) de C6, y el plan de pasos de la
  fase 5. `plan_trabajo.md` y `estado_compi.md` siguen nombrando a
  `diseno_camino_optimizante.md` como contrato vigente: sus decisiones están en el anexo A.
- A `diseno_camino_optimizante.md` le faltaba su §11 (se perdió en una reescritura del 12/11);
  está recuperada en A.5.
- Las "fugas del parser con errores de sintaxis" (plan §1.3) están resueltas por los
  destructores que agregó la integración del IDE: confirmado en P0 y con AddressSanitizer en la
  validación final (9.17).
- `make check-ide` da 30 fallos y 1 error (última validación, con P15). Respecto de la fase 1
  hay uno más de la terminal integrada (`test_terminales_sesiones_independientes_e_interrupcion`);
  `test_copiar_pegar_terminal_y_ctrl_c_sin_seleccion` falla o pasa según el entorno (largo del
  prompt). La suite necesita `XDG_DATA_HOME` aislado; sin él, las pruebas de papelera real se
  cuelgan. Para la revisión del IDE.
- El mensaje de ROM excedida sale como `archivo:0:0: error de generacion` también con `-x`,
  donde no hay generación. Para la revisión de los errores.
- P13 (cascadas) se pospuso para después de las optimizaciones (9.14).
- Alineación con el curso (4.4): P15 cambia el TAC (variables directo, como el curso). El
  spilling se hace con el coloreo de Chaitin (`--opt=spill`, `-O2`); `-O1` conserva su
  asignación, sin derrame. C3.4 cambia de contenido.
- Revisión del 15/11 (4.5, 4.6): el diseño de las optimizaciones se corrigió contra el TAC real
  y contra lo que Calser permite. Cambian esperados (por ejemplo, `d1` elimina 3 y `r1` renombra
  2 por las inicializaciones implícitas), DCE conserva la eliminación de bloques inalcanzables
  (6 programas del corpus tienen uniones muertas), el unrolling omite los `S` con salida
  anticipada y el reordenamiento toma siempre la distancia mayor.
- Pendiente para la etapa del IDE o de cierre: `ccom -i` escribe el TAC inicial, mientras 13.1
  dice que el `.tac` refleja el TAC después de las pasadas de alto nivel.
- P14 corrigió un error de diseño: la liveness hacía cúbica la asignación de -O1 (9.15).
- Pregunta nueva para el profesor: qué se espera por tablas "eficientes" (§4.1).
- `docs/compiler-integration.md` de arqui dice que `ccom` no usa `rg`; pasa a ser el registro
  auxiliar del derrame.
- El IDE no reconoce el archivo de tres direcciones (`.tac`) ni puede generarlo con F5; el
  diseño está en 13.1.
- El IDE no puede pedir nivel, pasadas, factor ni métricas en F5. Diseño en la parte 13; es
  trabajo del IDE, no del refactor.
- El enlazado (parte 12) toca la frontera con el IDE en dos puntos: `--module-dir` en la
  comprobación automática (`ide/interfaz/comprobacion.py`) y el tipo de token `cadena`
  (`ide/ccom.py`, `ide/interfaz/editor.py`).
- Regla nueva para DCE y renombramiento (11.1): lo que puede fallar, y las llamadas, leen el
  estado observable. Extiende la regla de "puede fallar" del diseño anterior, que solo cubría
  el reordenamiento.
- El paso P1 mueve todos los fuentes, cabeceras y suites a carpetas: los inventarios del
  proyecto quedan con rutas viejas.
- Dos tareas del plan cambian de contenido: la clonación de bloques (estaba en C3.1) y la
  base del reordenamiento para fijar un orden (estaba en C3.3) las deja el refactor (P11). El
  grafo de dependencias sigue en C3.3.

## Anexo A. Decisiones vigentes del camino optimizante

Tomadas de `claude/diseno_camino_optimizante.md` (cerrado el 11/11, revisado contra el código).
Siguen vigentes salvo lo que la parte 4 cambia. Fuentes fuera de Aho: Cooper y Torczon
(*Engineering a Compiler*), Appel (*Modern Compiler Implementation*), Muchnick, Poletto y
Sarkar (1999, *linear scan*), Goodman y Hsu (1988), Fisher, Faraboschi y Young.

### A.1 Decisiones

| Tema | Decisión | Fundamento |
|---|---|---|
| Registros virtuales | Temporales virtuales ilimitados; los registros físicos se deciden una sola vez | "Registros simbólicos" (Aho §8.8.4), pseudorregistros (Aho §10.2.3) |
| Lectura de variables (cambia con P15) | La variable se usa directo (`E.place = id.place`); solo se copia a un temporal si una llamada posterior en la misma expresión podría modificarla (9.18). En alto nivel una variable es una variable, no un acceso a memoria | Aho §6.4; presentación *Optimización de Código*, diapositiva 7. La copia restante conserva el orden de evaluación |
| Destino distinto de las fuentes en `*`, `/`, `%`, `^` | La rutina exige destino, operandos y 8 auxiliares físicos todos distintos. En ISA virtual, si el destino coincide con una fuente, la bajada usa un temporal nuevo y agrega una copia. En la asignación, las fuentes de esas instrucciones se consideran vivas también en la posición de escritura | Restricción de la rutina del Proyecto I |
| Asignación de registros | `-O1`: *linear scan* sobre intervalos de vida (liveness, Aho §9.2.5), sin derrame. Con `--opt=spill` o `-O2`: coloreo de grafos de Chaitin con derrame (7.6) | Poletto y Sarkar (1999) para el *linear scan*; Aho §8.8.4 y la presentación *Spilling de Registros* para Chaitin; Briggs para el coloreo optimista |
| Política de elección de registro | El registro libre desde hace más tiempo; si empatan, el menor | Aho §10.2.3 |
| Renombramiento | Local al bloque básico, en alto nivel, sobre variables escalares modificables declaradas en la función. Quedan fuera: globales, arreglos, parámetros, variable de retorno, contador de `S` y su límite | Aho §10.2.1; la forma global (SSA, Cytron et al. 1991) queda fuera del alcance |
| Dos calendarizaciones | El mismo algoritmo (calendarización por lista, Aho §10.3) antes de asignar (reordenamiento) y después de expandir (empaquetado, `cg_schedule`) | Aho §10.2.4; Goodman y Hsu (1988) |
| Empaquetado en bundles | `cg_schedule` del Proyecto I en todos los niveles | Verificado; respeta los bundles fijos de CRYPTO |
| Lazos: dos fuentes | Marcas de `S` y `L` al construir, solo para el unrolling; y análisis de lazos naturales recalculable en cualquier etapa (profundidad de cada bloque, para el spilling) | Aho §9.6.4 y algoritmo 9.46. Los lazos estructurados de Calser dan grafos reducibles |
| Estado observable | Variables globales, variables de `main` (escalares y arreglos), memoria de datos externa (`0x1000`–`0xFFFF`) y estado CRYPTO. Las variables de otras funciones, la pila y las casillas de preservación no son observables | Es lo que comprueban las pruebas y el procesador |
| Derrame | Extensión propia por función, al final de los datos (`spill_base`, `spill_size`). Solo con `--opt=spill` o `-O2` | Appel; Cooper y Torczon |
| Pila en `-O1`/`-O2` | `mov rx, <fin de datos>` leído después de la última reserva de derrame, repitiendo antes la comprobación de 256 bytes libres | Igual que el Proyecto I, con el fin de datos definitivo |
| Enlace de retorno | Alrededor de cada `jas`, la emisión guarda `rl` en la pila y lo recupera | Sin esto una función que llama a otra pierde su retorno |
| Casillas de preservación | El registro `r` se guarda en `temp_base + 4·(r−4)`. Las preservaciones nunca están anidadas | Una casilla por registro |
| Registros fijos de CRYPTO | Los argumentos se llevan a sus registros fijos con una permutación paralela | Las plantillas del Proyecto I los esperan ahí |
| Convención de llamadas | La del Proyecto I: la función llamada puede destruir `t0`–`t11`; el llamador guarda los vivos | — |
| Mensajes de error | Todo error de `-O1`/`-O2` lleva el nombre real del archivo | Defecto C0.1 del Proyecto I |
| Entrada al camino | Solo con un AST aceptado y semántica sin errores | Contrato del parser |
| Regla de "puede fallar" | `/`, `%` y `^` no cambian de orden relativo con stores, llamadas ni CRYPTO, en ninguna dirección: si una división entre cero salta al manejador de error, el estado observable en ese punto debe ser el mismo que sin optimizar | — |

### A.2 Niveles y banderas

| Bandera | Qué hace |
|---|---|
| `-O0` | Generador y organizador del Proyecto I |
| `-O1` | Camino por la representación sin optimizaciones: la línea base del "antes" |
| `-O2` | Equivale a `--opt=unroll,rename,dce,reorder,spill` (reservado, código 2, hasta C3.5) |
| `--opt=lista` | Sobre `-O1`, activa solo las pasadas listadas, siempre en el orden fijo |
| `--unroll=N` | Factor del unrolling, N entre 2 y 16 |
| `--metrics archivo.json` | Escribe las métricas de la compilación |
| `-i`, `--tac` | Escribe la representación de alto nivel |
| `-m` | En `-O1`/`-O2`, tabla de las etiquetas emitidas con sus direcciones finales |
| `-p` | Bundles en ensamblador y en binario: la bandera para ver la calendarización (R14). C3.5 verifica que muestre slots y NOPs por paquete |

Errores de uso (código 2): `--opt=` junto con `-O0`; pasada desconocida; `--unroll=N` sin
`unroll` activo o fuera de 2–16; `-O2` hasta C3.5. Error interno (código 1): validación
fallida después de una pasada, con su nombre.

### A.3 Definición de cada métrica

| Métrica | Definición |
|---|---|
| "Antes y después" del §6 | Antes = `-O1`; después = `-O1` con la pasada activa. Se compara con las instrucciones emitidas |
| Instrucciones de alto nivel | De la representación de alto nivel, sin etiquetas de bloque, antes y después de cada pasada de alto nivel |
| ISA antes de expandir | De la representación justo antes de emitir; cada operación compleja cuenta 1 |
| Emitidas | Operaciones que no son NOP y llegan al organizador, incluidas expansiones y preservación |
| Bundles, NOPs, bytes | Los que entrega el organizador; NOPs = 4 × bundles − emitidas; bytes = 16 × bundles |
| Demanda y presión | Máximo de valores vivos antes de asignar; máximo de registros ocupados después |
| Tiempo | Reloj monótono por pasada; total como `--time` |

### A.4 Formato del JSON de métricas

```json
{
  "programa": "pruebas/p02_factorial.cls",
  "nivel": "O1",
  "pasadas": ["dce"],
  "alto_nivel": {"inicial": 0, "despues": {"dce": 0}},
  "isa_antes_expandir": 0,
  "emitidas": 0,
  "bundles": 0, "nops": 0, "bytes": 0,
  "demanda": 0, "presion": 0, "mascara_registros": "0xfff0",
  "tiempo_ms": {"total": 0.0, "pasadas": {"dce": 0.0}},
  "contadores": {
    "desenrollados": [], "renombradas": 0, "eliminadas": 0, "bloques_eliminados": 0,
    "reordenadas": 0, "derramados": 0, "cargas_derrame": 0, "guardados_derrame": 0
  },
  "ciclos_sim": null
}
```

### A.5 Definiciones de las optimizaciones (la §11 recuperada)

| Tema | Definición |
|---|---|
| Heurística del unrolling | Si inicio y fin llegan como enteros a la marca y N vale 8 o menos: desenrollado total. Si no: factor 4 (o el de `--unroll=N`), con lazo de residuo para N no múltiplo del factor. No se desenrollan los `S` con salida anticipada ni los que contienen otro lazo. Nunca superar el tope de 4096 bundles |
| Prueba de que `/`, `%` y `^` no fallan (DCE) | Solo por constantes: divisor constante distinto de cero, o exponente constante mayor o igual que cero. Sin esa prueba no se eliminan |
| Independencia de memoria (reordenamiento) | Dos accesos son independientes solo si los dos son `rz` más constante, con constantes distintas. Cualquier dirección calculada (por ejemplo, un elemento de arreglo) se trata como dependiente de todo otro acceso |

### A.6 Regla para ambigüedades durante la implementación

- Las resuelve el chat de la tarea, con la opción más simple, y las documenta: nombres,
  detalles internos, mensajes, estructuras privadas de un módulo.
- Van al cerebro: lo que cambie la arquitectura, el contrato de una pasada (parte 7), las
  banderas o el formato de salida visible, el comportamiento de `-O0`, o lo ya integrado.
