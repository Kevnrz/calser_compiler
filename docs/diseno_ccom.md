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
| 4 | Decisiones tomadas | Escrita |
| 5 | Diseño contra el código actual | Escrita |
| 6 | Efecto de la integración del IDE sobre el léxico y el parser | Escrita |
| 7 | Contrato de integración de las optimizaciones | Escrita |
| 8 | Conexión con el IDE y las métricas | Escrita |
| 9 | Plan de implementación del refactor (pasos P0 a P15) y su cierre | P0–P12, P14 y P15 integrados; P13 pospuesto |
| 10 | Lo que se queda como está | Escrita |
| 11 | Diseño técnico de las optimizaciones (estructuras, pseudocódigo, casos de prueba) | Escrita |
| 12 | Enlazado de archivos (`P`) | Escrita |
| 13 | IDE y medición (archivo `.tac`, nivel y pasadas en F5, medición, gráficas) | Escrita |
| 14 | Las 15 pruebas obligatorias | Escrita |
| 15 | Avisos para el cerebro | Escrita |
| A | Anexo: decisiones vigentes del camino optimizante (absorbe `diseno_camino_optimizante.md`) | Escrito |

**Estado del refactor (15/11).** P0 a P12 y P14 están integrados en la rama `refactor-c6`
(PR #18 a #23, commit `29f0665`) y pasaron la validación final (9.17). Después de revisar las
optimizaciones contra las presentaciones del curso se tomaron dos decisiones (4.4): P15 (lectura
de variables en el TAC al estilo del curso, antes del PR a `develop`) y Chaitin como ruta
alterna del spilling (se implementa con C3.4). P15 está integrado y validado en el árbol de
`feature/c6-tac-curso` (9.18); no se atribuye un PR ni un commit nuevos. Siguiente: un solo PR
de `refactor-c6` hacia `develop`; después, las optimizaciones (parte 7 y parte 11). P13
(cascadas) queda pospuesto para después de las optimizaciones, junto con la revisión del IDE
y de los errores (9.14).

Para retomar en otro chat: leer este apartado, la parte 4, 9.0 y 9.17.

**Base del análisis.** Código de `Kevnrz/calser_compiler`, rama `develop`, commit `6ad9fb4`,
leído directamente; comparado con `2268981` (antes de integrar el IDE). Bison 3.8.2 se ejecutó
sobre las dos gramáticas. **No se pudo compilar ni correr ninguna suite** en el entorno del
chat (no hay Flex): toda afirmación sobre comportamiento sale de leer el código, y cada paso
debe confirmarla con las protecciones.

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
| A5.3 Asignación de registros y spilling | Pasar de temporales a registros reales (*linear scan*, fuera de Aho). Si no alcanzan, derramar a memoria | ISA virtual, intervalos, profundidad de lazos | ISA física; plan de preservación; contadores de derrame | R10 |
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
| DCE | Local y global con variables vivas, repitiendo hasta punto fijo | 7.2, 11.2 | Alineado |
| Renombramiento | Nombre nuevo por temporal para liberar el bloque | 7.3, 11.3: los temporales ya nacen únicos; se renombran las variables locales | Alineado |
| Reordenamiento | Intercambio de instrucciones independientes; RAW, WAR, WAW | 7.5, 11.5 | Alineado |
| Unrolling | No está en las presentaciones; Aho §10.5.1 y notas de arqui §5.2.2 | 7.4, 11.4 | Alineado con el libro |
| TAC | `E → id { E.place = id.place }`: la variable se usa directo | Se copiaba toda lectura a un temporal | **Decisión 1: P15** (9.18) |
| Spilling | Coloreo de Chaitin; víctima por costo/grado con 10^profundidad | *Linear scan*; víctima por profundidad y fin | **Decisión 2: Chaitin como ruta alterna** (7.6) |

**Decisión 1.** El TAC usa la variable directo, como el curso. Solo se copia a un temporal
si queda una llamada a función por evaluar en la misma construcción, o en `R`, para conservar
la asignación simultánea de parámetros antes de sobrescribirlos. El fin de `S` se captura en
su casilla privada, sin agregar una copia a TEMP solo para la marca. Se implementa en P15
(9.18), antes del PR a `develop`.

**Decisión 2.** El *linear scan* sigue como ruta principal. Se agrega el coloreo de Chaitin en
su propio archivo, elegido con `--alloc=color`. Las dos estrategias entregan lo mismo (mapa
temporal→registro y lista de derramados) y comparten la víctima del curso. Después se comparan
bundles, ciclos y derramados para decidir cuál queda como principal. Se implementa con el
spilling (C3.4).

**Fuera del alcance** (lo enseña el curso, no lo pide el enunciado): subexpresiones comunes,
plegado y propagación de constantes, expresiones disponibles, definiciones alcanzables, código
invariante, reducción de fuerza, intercambio de ciclos, *tiling* y la prueba del GCD.

**Para la defensa.** Los bloques básicos se forman al construir el TAC, no con el algoritmo de
líderes de la presentación; el resultado cumple la misma definición (entrada solo por la
primera instrucción, salida solo por la última). El LL(1) (PRIMERO, SIGUIENTE, tabla M y
analizador predictivo) ya existe en `ide/nucleo/` y `ide/docs/ll1/`; su revisión queda para la
etapa del IDE.

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
| A5.3 Asignación | `tac_allocate.c`, `tac_allocate_sites.c` | P | Sin derrame: `no_register_available` solo da error. No recibe interruptor ni profundidad de lazos → P12. La lógica del derrame es la tarea C3.4 |
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

**Usa lo que existe:** `tac_live_analyze`, `tac_live_instruction`, `tac_live_contains`,
`tac_instruction_info`, `tac_definitions`, `tac_remove`, `tac_loops_analyze`,
`tac_loops_reachable`, `tac_block_remove`. **Le falta (P10):** `tac_observable_roots`.

Diseño interno: 11.2. Regla ante un fallo: 11.1.

**Algoritmo**

1. Calcular las raíces observables: variables escalares globales y de `main`.
2. Por función: calcular liveness (las raíces solo cuentan al salir de `main`).
3. Recorrer las instrucciones. Se elimina una instrucción si se cumple todo:
   - define un valor y ese valor no está vivo después de ella;
   - sus propiedades no la protegen: no es `nonremovable`, no escribe memoria, no toca el
     estado CRYPTO, no es barrera (llamada);
   - si puede fallar (`/`, `%`, `^`): el divisor es una constante distinta de cero, o el
     exponente es una constante mayor o igual que cero. Sin esa prueba no se elimina.
4. Si se eliminó algo, recalcular liveness y repetir desde 3.
5. Eliminar los bloques inalcanzables desde la entrada.
6. Anotar `removed` y `removed_blocks`.

**Ejemplo**

```
F f [ a : int ] : int r ;        alto nivel de f (con P15):
    Vm x : int = a * 2 ;           t1 = a * 2      x = t1
    Vm y : int = a + 1 ;           t2 = a + 1      y = t2
    r = y ;                        r = y
    E ;                            return
.
```

`x` no se lee nunca y `f` no es `main`, así que `x` no está viva después de `x = t1`.
Primera vuelta: se elimina `x = t1`. Segunda: `t1` ya no está viva, se elimina `t1 = a * 2`
(la multiplicación no puede fallar). Resultado: `removed = 2`.
Si la misma función fuera `main`, `x` sería raíz observable y no se eliminaría nada.

### 7.3 Renombramiento local (A4.3, tarea C3.3a)

**Usa lo que existe:** `tac_variable_is_rename_candidate` (excluye globales, arreglos,
parámetros, variable de retorno, contador de `S` y su límite), `tac_definitions`,
`tac_temp_new`, `tac_replace_reads`. **No le falta infraestructura.**

Diseño interno: 11.3. Una definición no se renombra si entre ella y la siguiente hay una
instrucción que puede fallar o una llamada que lea esa variable (11.1).

**Algoritmo**, por bloque básico:

1. Recorrer el bloque de arriba abajo.
2. Para cada variable candidata `v` con más de una definición en el bloque: cada definición
   **menos la última** pasa a escribir un temporal nuevo `t`.
3. Las lecturas de `v` entre esa definición y la siguiente pasan a leer `t`.
4. La última definición del bloque sigue escribiendo `v`: así la variable queda con su valor
   correcto al salir del bloque, sin agregar copias.
5. Anotar `renamed`: definiciones que pasaron a un temporal.

No cruza bloques: la información de otros caminos no se usa (forma local; la forma global
sería SSA, fuera del alcance).

**Ejemplo**

```
antes                         después
x = t1 + t2      (def 1)      t9 = t1 + t2
c = x * 2                     c = t9 * 2
x = t4 + 1       (def 2)      x = t4 + 1
```

La primera escritura de `x` y la segunda eran una dependencia WAW; la lectura intermedia y la
segunda escritura, una WAR. Ya no comparten nombre. Además desaparece una escritura a memoria,
porque toda variable se guarda en memoria en cada definición. `renamed = 1`.

### 7.4 Unrolling (A4.2, tarea C3.1)

**Usa lo que existe:** las marcas `TacLoopMark` que deja la construcción (para cada `S`:
bloque de cabecera, de cuerpo y de retorno; variable de control; límite; inicio, fin y paso ya
evaluados), `tac_loop_mark_state`, `tac_loops_analyze`, `tac_loops_contains`,
`tac_block_insert_after`, `tac_rebuild_edges`. **Le falta (P11):** `tac_blocks_clone`.

**Qué lazos.** Solo `S` cuya marca está en estado "con arista de retroceso". Los `L` no: no se
conoce su número de vueltas. El semántico ya prohíbe asignar la variable de un `S` dentro de
su cuerpo, así que el contador solo lo cambia el propio lazo.

**Heurística** (factor cuando no se da `--unroll=N`)

| Caso | Acción |
|---|---|
| Inicio, fin y paso constantes y N ≤ 8 vueltas | Desenrollado total: N copias del cuerpo, sin prueba ni salto de retroceso |
| Cualquier otro | Factor 4, con el lazo original como residuo |
| El crecimiento estimado haría pasar el programa de la ROM | No se desenrolla ese lazo |

Con `--unroll=N` (2 a 16) se usa ese factor en lugar de 4. La comprobación definitiva de la
ROM es la del ensamblado.

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

La entrada al lazo pasa a apuntar a `G`. Con paso negativo las comparaciones se invierten.
Cada copia se obtiene con `tac_blocks_clone` sobre los bloques del lazo (los que
`tac_loops_contains` reporta) menos la cabecera. Las salidas anticipadas (`E` dentro del
cuerpo) se conservan: sus destinos están fuera del conjunto clonado y no se renombran.

**Ejemplo, total.** `S [ i , 0 , 3 , 1 ] ;  s = s + i ;  .` tiene N = 3 ≤ 8:

```
s = s + i ; i = i + 1
s = s + i ; i = i + 1
s = s + i ; i = i + 1
```

Sin prueba ni saltos. Se anota `{linea del S, factor 3}`.

**Ejemplo, parcial.** Con fin 10 y factor 4: `G` deja pasar mientras `i + 3 < 10`; se ejecutan
dos rondas de 4 (i = 0..7) y el residuo hace las 2 vueltas restantes.

### 7.5 Reordenamiento (A4.5, tarea C3.3b)

**Usa lo que existe:** `tac_locations_visit` (qué ubicaciones lee y escribe cada instrucción,
incluidas las implícitas), `tac_instruction_info` (barrera, puede fallar, memoria), la unidad
funcional de cada instrucción (`isa.instruction->unit`, `isa_unit_slots`). **Le falta (P11):**
`tac_block_set_order`. El grafo de dependencias es lógica propia de la pasada.

**No puede reutilizar el organizador.** `cg_schedule_block` recibe lecturas y escrituras como
máscaras de los 16 registros reales; aquí los valores son temporales virtuales. Es la frontera
virtual/física. Lo que sí comparte son las reglas de distancia, que salen del ISA.

**Algoritmo**, por bloque (calendarización por lista, Aho §10.3–10.4):

1. Construir el grafo de dependencias. Hay arista de `a` a `b` (con `a` antes que `b`) si:

   | Caso | Distancia |
   |---|---|
   | `b` lee lo que `a` escribe (RAW) | 2 |
   | `b` escribe lo que `a` lee (WAR) | 1 |
   | `a` y `b` escriben lo mismo (WAW) | 1 |
   | Uno escribe memoria y el otro la lee o la escribe, salvo que las dos direcciones sean `rz` más constantes distintas | 1 |
   | `a` o `b` es barrera (llamada `jas`, CRYPTO) | 1, con todas |
   | Uno puede fallar (`/`, `%`, `^`) y el otro es un store, una llamada o CRYPTO | 1 |
   | `b` es el salto final del bloque | 1, con todas |

2. Prioridad de cada instrucción: camino más largo hasta el final del bloque.
3. Simular bundles: por ciclo caben 2 de ALU, 1 de salto y 1 de memoria. Se toma, entre las
   instrucciones listas, la de mayor prioridad; en empate, la que estaba antes.
4. El orden nuevo es por ciclo simulado y, dentro del ciclo, por posición original.
5. Aplicarlo con `tac_block_set_order`. Anotar `reordered`.

Las operaciones complejas (`*`, `/`, `%`, `^`) son una instrucción indivisible y ocupan su
ciclo solas.

**Por qué va antes de asignar.** Después de asignar, dos valores sin relación pueden compartir
registro y aparecer como dependientes (WAR y WAW falsas). Con temporales virtuales esas
dependencias no existen, y la política de la asignación (el registro libre hace más tiempo)
conserva el paralelismo expuesto.

**Ejemplo**

```
1: v1 = lw a(rz)            orden original: 1 2 3 4
2: v2 = add v1, v1          dependencias: 1→2 (RAW), 3→4 (RAW); memoria: solo lecturas
3: v3 = lw b(rz)
4: v4 = add v3, v3          simulación: ciclo 0: 1 · ciclo 1: 3 · ciclo 2: 2 · ciclo 3: 4
                            orden nuevo: 1 3 2 4      reordered = 2
```

Las dos cargas quedan en ciclos seguidos y cada suma espera su distancia de 2 mientras se
ejecuta la otra carga.

### 7.6 Spilling (dentro de A5.3, tarea C3.4)

**Decisión del 15/11 (4.4).** Además del *linear scan*, que sigue como ruta principal, se
implementa el coloreo de Chaitin, como en el curso, en `src/generador_o1/tac_allocate_color.c`,
elegido con `--alloc=color` (`TacAllocationOptions` gana el campo de la estrategia). Chaitin
construye el grafo de interferencia con los intervalos (dos temporales interfieren si sus
intervalos se solapan), simplifica con k = 12, elige víctima cuando se atasca y selecciona
colores. Las dos estrategias entregan el mismo mapa temporal→registro y la lista de
derramados; todo lo demás (plan por instrucción, código de derrame con `rg`, preservación,
operaciones complejas, emisión) es común. A diferencia del curso, no hace falta repetir el
algoritmo después de un spill: el código de derrame usa `rg`, registro reservado que no entra
al grafo y cumple el papel de los "temporales frescos". Después se comparan bundles, ciclos y
derramados para decidir cuál queda como principal.

**Dónde.** En el recorrido del *linear scan*, en el punto que hoy es `no_register_available`
(`tac_allocate.c`). No es una pasada del gestor.

**Interfaz que deja el refactor (P12)**

```c
typedef struct { int spill; } TacAllocationOptions;        /* NULL o spill = 0: sin derrame */

TacAllocationResult *tac_allocate_registers(TacProgram *program, const char *source_name,
    const TacAllocationOptions *options, char *error, size_t capacity);

/* TacInterval gana: */        size_t loop_depth;   /* mayor profundidad de lazo donde aparece */
/* TacAllocationMetrics gana: */ size_t spilled, spill_loads, spill_stores; double spill_ms;
```

Sin el interruptor, todo sigue como hoy: error "sin registros temporales disponibles".

**Usa lo que existe:** `data_area_reserve_spill` (una reserva por función), `tac_loops_depth`,
el plan por instrucción que ya arma la asignación (`TacEmissionSite`, `tac_allocate_sites`) y
`cg_preserve` (P3).

**Algoritmo** (Poletto y Sarkar 1999, fuera de Aho)

1. Al llegar a un intervalo sin registro libre, los candidatos son los 12 activos y el nuevo.
2. Víctima (la del curso, común a las dos estrategias): el de menor costo/grado, con
   costo = Σ 10^profundidad sobre cada definición y uso del temporal, y grado = cantidad de
   intervalos que se solapan con el suyo. En empate, el que termina más lejos.
3. La víctima queda derramada para toda su vida (no hay partición de intervalos). Si era un
   activo, su registro pasa al intervalo nuevo.
4. Al terminar la función: reservar `4 × derramados` bytes con `data_area_reserve_spill`. La
   casilla del derramado número `k` es `spill_base + 4·k`.
5. Plan por instrucción. Las cargas y guardados **no se insertan como instrucciones nuevas**:
   se anotan en el plan de la instrucción, igual que la preservación, y la emisión los escribe.
   - lectura de un derramado: el plan anota "cargar la casilla en `aux`" y el operando pasa a
     `aux`;
   - escritura de un derramado: el operando pasa a `aux` y el plan anota "guardar `aux` en la
     casilla".

   La emisión escribe, para toda instrucción con plan, en este orden: guardar los preservados,
   cargas de derrame, la instrucción, guardados de derrame, restaurar los preservados. El
   orden importa: si un registro prestado tiene un valor vivo, hay que guardarlo **antes** de
   cargarle el valor derramado; por eso la carga no puede ser una instrucción suelta anterior.
6. Elección de `aux`:

   | Situación | Auxiliar |
   |---|---|
   | Un solo operando derramado | `rg` |
   | Dos fuentes derramadas y el destino tiene registro | `rg` y el registro del destino |
   | Dos fuentes derramadas y no hay destino con registro (`sw`, salto condicional, o todo derramado) | `rg` y un registro prestado: uno de `t0`–`t11` que la instrucción no use; si está vivo, se guarda y se restaura en su casilla de `temp_base` |
   | Operación compleja aritmética (exige operandos en `t0`–`t11`) | Registros prestados, elegidos junto con sus 8 auxiliares |
   | CRYPTO | El valor se carga directo en su registro fijo |

**Ejemplo.** 13 valores vivos; `v13` aparece solo fuera de lazos y termina último: es la
víctima. Con `v2` en `t1` y `v14` en `t5`:

```
v14 = add v13, v2     →     lw  rg, casilla_v13(rz)
                            add t5, rg, t1

v13 = add v2, v7      →     add rg, t1, t6
                            sw  rg, casilla_v13(rz)
```

`spilled = 1`, y se cuenta una carga o un guardado por cada uso reescrito.

**Lo más delicado:** las operaciones complejas con operandos derramados, porque sus rutinas
exigen destino, fuentes y 8 auxiliares distintos entre `t0` y `t11`. El plan de auxiliares
(`tac_allocate_sites`) ya elige y preserva; hay que extenderlo para que también asigne
registro a los operandos derramados.

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

P0 no tiene PR: es la línea base, guardada fuera del repo en `refactor_ref/` (`ccom_ref`, `o0/`,
`consultas/`, `manifest.json`).

P15 está integrado en el árbol de `feature/c6-tac-curso`, con las comprobaciones completas de
9.18. Este cierre local no registra un PR ni un commit adicionales.

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

**Resultados finales (15/11; P15 integrado en este árbol).**

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
| `int tac_pass_dce(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel válido | 1; programa sin instrucciones muertas ni bloques inalcanzables; `counters.removed` y `removed_blocks` actualizados | 0 con texto en `detail` si falla un análisis o una edición |

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

remove_unreachable(f):
    lazos ← tac_loops_analyze(f);  si NULL: falló
    lista ← bloques b de f con tac_loops_reachable(lazos, b) = 0
    tac_loops_free(lazos)
    para cada b en lista, del último al primero:
        si tac_block_remove(f, b) = 0: falló
    si lista no vacía: tac_rebuild_edges(f)
    devolver tamaño de lista
```

Dentro de una vuelta la liveness queda desactualizada al borrar, pero solo en el sentido
seguro: puede dar por vivo algo que ya no lo está. La vuelta siguiente lo recalcula.

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Local muerta | Función con `Vm x : int = a * 2 ;` y `x` nunca leída | Se eliminan sus 2 instrucciones (`t = a * 2`, `x = t`); `removed = 2` |
| Llamada entre medias | Lo anterior con una llamada que modifica una global | La llamada se conserva; el resultado no cambia |
| División constante | Local muerta con `a / 4` | Se elimina |
| División variable | Local muerta con `a / b` | Se conserva |
| Observable | La misma local muerta, pero en `main` | No se elimina nada |
| Fallo en `main` | `x = 1 ; y = a / b ; x = 2 ;` en `main` | `x = 1` se conserva |
| Inalcanzable | Sentencias después de `E` | `removed_blocks ≥ 1` |
| Efectos | Escritura a arreglo y CRYPTO con resultado no usado | Se conservan |

### 11.3 Renombramiento (`src/optimizador/tac_pass_rename.c`)

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_rename(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel válido | 1; redefiniciones locales renombradas; `counters.renamed` actualizado | 0 si falla la liveness o un reemplazo |

**Estructura**

```c
typedef struct {
    const Symbol *variable;   /* candidata definida más de una vez en el bloque */
    size_t total;             /* cuántas definiciones tiene en el bloque */
    size_t seen;              /* cuántas se han recorrido */
    TacOperand current;       /* temporal que la reemplaza ahora; tac_none() si ninguno */
} RenameEntry;
```

**Pseudocódigo**

```
tac_pass_rename(contexto):
    para cada función f:
        vivos ← tac_live_analyze(f, raíces observables)
        para cada bloque b de f:
            contadores.renamed ← contadores.renamed + rename_block(contexto, f, b, vivos)
        tac_live_free(vivos)
    devolver 1

rename_block(contexto, f, b, vivos):
    tabla ← collect(contexto, f, b)          // candidatas con total ≥ 2
    si tabla vacía: devolver 0
    n ← 0
    para cada instrucción i de b, en orden:
        para cada entrada e con e.current definido:
            tac_replace_reads(b, i, tac_variable(e.variable), e.current)
        para cada definición d de i que sea la variable de una entrada e:
            e.seen ← e.seen + 1
            si e.seen < e.total y can_rename(vivos, i, e.variable):
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

can_rename(vivos, definicion, variable):
    para cada instrucción j después de definicion, hasta la siguiente definición de variable:
        si j lee variable sin tenerla como operando explícito:      // regla 11.1
            falso
    verdadero
```

"Lee sin tenerla como operando explícito" se comprueba con el conjunto `uses` de la
instrucción en la liveness (`TacLiveInstruction.uses`) contra `tac_uses`.

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| WAW y WAR | `x = a + b ; r = x * 2 ; x = a - b ; r = r + x ;` con `x` local | La primera `x` pasa a un temporal y su lectura también; la segunda escribe `x`. `renamed = 1` |
| Tres definiciones | `x` definida tres veces en un bloque | `renamed = 2` |
| Una definición | `x` definida una vez | `renamed = 0` |
| Parámetro, global, retorno | Redefiniciones de cada uno | `renamed = 0` |
| Entre bloques | `x` definida en un bloque y otra vez dentro de un `C` | `renamed = 0`: son bloques distintos |
| Fallo en `main` | `x = 1 ; y = a / b ; x = 2 ;` en `main` | `renamed = 0` |

### 11.4 Unrolling (`src/optimizador/tac_pass_unroll.c`)

**Forma de un lazo `S` en la representación** (la que construye `tac_build_flow.c`):

```
previo:  control = inicio ; limite = fin ; goto prueba
prueba:  if control >= limite goto salida        (paso > 0; con paso < 0 es <=)
cuerpo:  …
retorno: control = control + paso ; goto prueba   (último bloque del cuerpo)
salida:
```

La marca (`TacLoopMark`) da `header` (prueba), `body`, `latch` (retorno), `control`, `limit`,
`initial`, `bound` y `step`. El destino de un salto es su operando `dest`.

**Interfaz pública**

| Función | Entrada | Salida | Errores |
|---|---|---|---|
| `int tac_pass_unroll(TacPassContext *context, char *detail, size_t capacity)` | Programa de alto nivel con marcas de lazos; `context->unroll_factor` (0 o 2..16) | 1; lazos desenrollados; un elemento en `counters.unrolled` por lazo | 0 si falla un análisis, una clonación o la reconstrucción de aristas |

**Estructura**

```c
typedef struct {
    TacLoopMark mark;           /* copia: las marcas se descartan antes de editar */
    const TacBlock **blocks;    /* bloques del lazo sin la cabecera, en orden de lista */
    size_t block_count;
    size_t size;                /* instrucciones de esos bloques */
    TacBlock *previous;         /* único bloque de fuera del lazo que salta a la cabecera */
    TacBlock *exit;             /* destino de la prueba de la cabecera */
    int line;                   /* línea del S */
} LoopPlan;

enum { UNROLL_FULL_LIMIT = 8, UNROLL_DEFAULT_FACTOR = 4 };
#define UNROLL_BUDGET (ISA_ROM_BUNDLES / 4)   /* crecimiento total permitido, en instrucciones */
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
        si algún bloque de bloques es header de otra marca: saltar m        // solo los internos
        previous ← predecesor de m.header que no está en bloques
        exit ← destino (dest) de la última instrucción de m.header
        guardar plan
    tac_loops_free(lazos)

trip_count(p):
    // Solo INTEGER: NONE, TEMP y VARIABLE($limite_Sk) son no constantes.
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
    para cada b en p.blocks, del último al primero: tac_block_remove(f, b)
    tac_block_remove(f, header)
```

La guarda asume que `control + (k−1)·paso` no desborda 32 bits; es una precondición
documentada (los límites de un `S` en los programas de Calser son pequeños frente a ese rango).

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Total | `S [ i , 0 , 4 , 1 ]` | 4 copias, sin prueba ni salto de retroceso; `{línea, 4}` |
| Cero vueltas | `S [ i , 5 , 5 , 1 ]` | El lazo desaparece; `{línea, 0}` |
| Parcial exacto | Fin en una variable que vale 8 | Guarda, 4 copias, residuo que no ejecuta ninguna vuelta |
| Parcial con residuo | Fin en una variable que vale 6 | Una ronda de 4 y 2 vueltas de residuo |
| Menos que el factor | Fin en una variable que vale 3 | La guarda salta directo al residuo |
| Paso negativo | `S [ i , 10 , 0 , -2 ]` | 5 vueltas, total |
| Factor pedido | `--unroll=8` con 100 vueltas | 12 rondas de 8 y residuo de 4 |
| Anidados | Un `S` dentro de otro | Solo se desenrolla el interno |
| Con `E` dentro | Cuerpo con salida anticipada | Cada copia conserva su salida |
| Sin presupuesto | Cuerpo muy grande | No se desenrolla; lista vacía |

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
    IsaUnit unit;                     /* ALU, salto, memoria */
    int alone;                        /* operación compleja: ocupa su ciclo sola */
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
    nodos[0..n) ← una por instrucción; llenar con tac_instruction_info y tac_locations_visit
    D ← matriz n × n en ceros
    para a de 0 a n−1, para c de a+1 a n−1:
        D[a][c] ← dependency(nodos[a], nodos[c])
    // prioridades, de atrás hacia adelante
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

dependency(a, c):                    // a está antes que c
    si c es el terminador del bloque: 1
    si a.info.barrier o c.info.barrier: 1
    si (a.info.may_trap y (c.writes_memory o c.info.barrier))
       o (c.info.may_trap y (a.writes_memory o a.info.barrier)): 1
    d ← 0
    si alguna ubicación de a.writes está en c.reads:  d ← 2          // RAW
    si alguna de a.reads está en c.writes:            d ← máx(d, 1)  // WAR
    si alguna de a.writes está en c.writes:           d ← máx(d, 1)  // WAW
    si a y c acceden memoria y al menos uno escribe:
        si no (a.memory_known y c.memory_known y a.memory_address ≠ c.memory_address):
            d ← máx(d, 1)
    devolver d
```

Las ubicaciones se comparan con `tac_location_equal`. `memory_known` vale 1 cuando la
instrucción es una carga o un guardado cuya base es el registro físico `rz`; la dirección es
su desplazamiento.

**Casos de prueba**

| Caso | Entrada (ISA virtual de un bloque) | Esperado |
|---|---|---|
| Dos cadenas | `v1 = lw a ; v2 = add v1,v1 ; v3 = lw b ; v4 = add v3,v3` | Orden 1, 3, 2, 4; `reordered = 2` |
| Ya óptimo | Una sola cadena dependiente | Sin cambios; `reordered = 0` |
| Memoria igual | `sw v1, 64(rz)` seguido de `lw v2, 64(rz)` | Nunca se invierten |
| Memoria distinta | `sw v1, 64(rz)` y `lw v2, 68(rz)` | Pueden cambiar de orden |
| Dirección calculada | Guardado en un elemento de arreglo y una carga cualquiera | Conservan su orden |
| Barrera | Instrucciones a los dos lados de una llamada | Ninguna cruza la llamada |
| Puede fallar | Una división y un guardado | Conservan su orden relativo |
| Salto final | Bloque que termina en salto condicional | El salto queda último |

### 11.6 Spilling (en `tac_allocate.c`, `tac_allocate_sites.c` y la emisión)

**Interfaz** (la deja el refactor, P12)

```c
typedef struct { int spill; } TacAllocationOptions;        /* NULL o spill = 0: sin derrame */

TacAllocationResult *tac_allocate_registers(TacProgram *program, const char *source_name,
    const TacAllocationOptions *options, char *error, size_t capacity);
```

| Situación | Resultado |
|---|---|
| Alcanzan los registros | Igual que hoy, con o sin interruptor |
| No alcanzan, sin interruptor | NULL y "sin registros temporales disponibles", como hoy |
| No alcanzan, con interruptor | Resultado válido; planes con cargas y guardados; contadores de derrame |
| No queda espacio de datos para las casillas | NULL y error de memoria de datos |

**Estructuras**

```c
/* TacInterval (tac_intervals.h) gana: */
size_t loop_depth;        /* mayor profundidad de lazo de los bloques donde aparece */

/* TacFunctionAllocation (interna) gana: */
int   *spill_slot;        /* por temporal: índice de casilla, o -1 si tiene registro */
size_t spill_count;

/* TacEmissionSite (tac_allocate.h) gana: */
typedef struct { unsigned physical_register; int address; } TacSpillMove;
TacSpillMove loads[3];    size_t load_count;     /* después de preservar, antes de la instrucción */
TacSpillMove stores[1];   size_t store_count;    /* después de la instrucción, antes de restaurar */

/* TacAllocationMetrics gana: */
size_t spilled, spill_loads, spill_stores;  double spill_ms;
```

**Pseudocódigo**

```
// dentro de allocate_function, cuando no hay registro libre para el intervalo nuevo
spill_under_pressure(estado, nuevo):
    víctima ← choose_victim(estado, nuevo)
    f.spill_slot[víctima.temp_id] ← f.spill_count;  f.spill_count ← f.spill_count + 1
    f.registers[víctima.temp_id] ← 0
    si víctima = nuevo: devolver 0 y marcar "sin registro"      // el recorrido sigue
    registro ← el que tenía la víctima
    quitar víctima de estado.active
    devolver registro

choose_victim(estado, nuevo):           // víctima del curso, común a scan y Chaitin
    entre estado.active ∪ {nuevo}:
        costo(v) ← Σ sobre cada definición y uso de v de 10^profundidad del bloque
        grado(v) ← cantidad de intervalos que se solapan con el de v
        menor costo/grado;  empate: mayor end;  empate: mayor temp_id

// al terminar el recorrido de la función
si f.spill_count > 0: data_area_reserve_spill(símbolo de f, 4 × f.spill_count)
dirección(casilla k) ← frame.spill_base + 4 × k

// en tac_allocate_sites, por cada instrucción con algún operando derramado
plan_spill_site(f, vivos, i, sitio):
    fuentes ← operandos leídos de i que están derramados, sin repetir
    destino ← operando escrito de i si está derramado
    usados ← registros de los operandos no derramados de i
    disponibles ← lista de auxiliares según el tipo de i:
        ISA normal:  [rg]  +  [registro del destino, si tiene y no es fuente]  +  prestados
        compleja aritmética:  prestados
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

Caben siempre: una operación compleja usa como máximo 3 operandos y 8 auxiliares, 11 de los
12 temporales.

**Casos de prueba**

| Caso | Entrada | Esperado |
|---|---|---|
| Sin presión | Cualquier programa del corpus con `--opt=spill` | Mismo `.asm` que `-O1`; `derramados = 0` |
| 13 vivos | Expresión anidada a la derecha con 13 productos | Sin `spill`: error. Con `spill`: compila; mismo estado que el cálculo a mano; `derramados ≥ 1` |
| Una fuente | Suma con una fuente derramada | `lw rg, …` antes; la suma lee `rg` |
| Dos fuentes | Suma con las dos fuentes derramadas | `rg` y el registro del destino |
| Sin destino | Salto condicional o guardado con dos fuentes derramadas | `rg` y un prestado; si estaba vivo, se guarda y se restaura |
| Destino derramado | Definición de un derramado | `sw rg, …` después |
| Dentro de un lazo | Valores vivos dentro y fuera de un `S` | Se derraman los de fuera |
| Operación compleja | Multiplicación con operandos derramados | Operandos en temporales prestados; resultado correcto |
| CRYPTO | Argumento derramado de `cifrar` | Carga directa al registro fijo |

### 11.7 Composición de `-O2` (tarea C3.5)

`parse_options` en `main.c` deja de rechazar `-O2`: activa las cinco pasadas y fija el nivel 2.
`pruebas/optimizador/tac_passes/probar_cli.py` hoy espera código 2 para `-O2`; esa aserción
cambia en C3.5.

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
`-O0` en el simulador, y el contador esperado en el JSON de métricas.

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

Esperado: `suma = 6`. Desenrollado total, `desenrollados = [{linea 4, factor 4}]`; sin saltos
dentro de `main`; menos ciclos que `-O1`.

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

Esperado: `suma = 15`. Parcial con factor 4: una ronda y 2 vueltas de residuo.

`u3_grande.cls`: N grande. El mismo lazo con fin 100. Esperado: `suma = 4950`; con el factor
por defecto, 25 rondas y residuo vacío; con `--unroll=8`, 12 rondas y residuo de 4. Más bytes
y menos ciclos que `-O1`.

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

Esperado: `salida = 18`; `renombradas = 1`.

`r2_war.cls`: una local leída y redefinida tres veces en el mismo bloque. Esperado:
`renombradas = 2`; con `--opt=rename,reorder`, menos bundles que con `reorder` solo.

`r3_limites.cls`: las mismas redefiniciones sobre un parámetro, una global, la variable de
retorno, y en `main` con una división en medio. Esperado: `renombradas = 0`.

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

Esperado: `g = 3`, `v = 4`. Se eliminan las 2 instrucciones de `x` (con P15); la llamada a
`tocar` se conserva. `eliminadas = 2`.

`d2_inalcanzable.cls`: sentencias después de `E` y una rama `C [ falso ]`. Esperado:
`bloques_eliminados ≥ 1`.

`d3_efectos.cls`: resultado sin usar de una división entre constante, una división entre
variable, una operación CRYPTO y una escritura a un arreglo. Esperado: solo se elimina la
división entre constante.

### 14.4 Reordenamiento

`o1_cadenas.cls`: en una función, dos cálculos independientes escritos uno tras otro
(`p = a + b ; q = p + p ; u = c + d ; w = u + u ;`). Esperado: `reordenadas > 0`; menos
bundles que `-O1`.

`o2_saltos.cls`: un `C` con cálculos independientes antes del salto y dentro de cada rama.
Esperado: reordena dentro de cada bloque; ninguna instrucción cambia de bloque.

`o3_funciones.cls`: llamadas a funciones entre cálculos, y una escritura seguida de una
lectura de la misma posición de un arreglo. Esperado: nada cruza una llamada; la escritura y
la lectura conservan su orden.

### 14.5 Spilling

`s1_expresion.cls`: una expresión anidada a la derecha con 13 productos
(`a1 * b1 + ( a2 * b2 + ( … ) )`). Esperado: con `-O1`, error "sin registros temporales
disponibles"; con `--opt=spill`, compila y da el valor correcto; `derramados ≥ 1`.

`s2_lazo.cls`: muchos valores vivos, unos usados dentro de un `S` y otros solo después.
Esperado: los derramados son los que no se usan dentro del lazo.

`s3_compleja.cls`: una multiplicación y una llamada con la presión al máximo. Esperado:
resultado correcto con operandos derramados de una operación compleja.

Para el renombramiento conviene mostrar también la comparación contra `-O0`: antes de P15, en
`-O1`, 9 programas del corpus tenían más bundles que en `-O0`. Se vuelve a medir después de
P15.

## 15. Avisos para el cerebro

- Este documento reúne el diseño (fase 3) y la brecha (fase 4) de C6, y el plan de pasos de la
  fase 5. `plan_trabajo.md` y `estado_compi.md` siguen nombrando a
  `diseno_camino_optimizante.md` como contrato vigente: sus decisiones están en el anexo A.
- A `diseno_camino_optimizante.md` le faltaba su §11 (se perdió en una reescritura del 12/11);
  está recuperada en A.5.
- Las "fugas del parser con errores de sintaxis" (plan §1.3) están resueltas por los
  destructores que agregó la integración del IDE: confirmado en P0 y con AddressSanitizer en la
  validación final (9.17).
- `make check-ide` da 31 fallos y 1 error, no 29 y 1: los 2 nuevos son de la terminal integrada
  (`test_prototipo`, esperan el prompt `calser:`), fallan igual en `feat/calside` y dependen del
  entorno. La suite necesita `XDG_DATA_HOME` aislado; sin él, las pruebas de papelera real se
  cuelgan. Para la revisión del IDE.
- El mensaje de ROM excedida sale como `archivo:0:0: error de generacion` también con `-x`,
  donde no hay generación. Para la revisión de los errores.
- P13 (cascadas) se pospuso para después de las optimizaciones (9.14).
- Alineación con el curso (4.4): P15 cambia el TAC (variables directo, como el curso) y el
  spilling tendrá Chaitin como ruta alterna (`--alloc=color`). C3.4 cambia de contenido.
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
| Asignación de registros | *Linear scan* sobre intervalos de vida (liveness, Aho §9.2.5) | Poletto y Sarkar (1999). Alternativa del libro para el informe: coloreo de grafos (Aho §8.8.4) |
| Política de elección de registro | El registro libre desde hace más tiempo; si empatan, el menor | Aho §10.2.3 |
| Renombramiento | Local al bloque básico, en alto nivel, sobre variables escalares modificables declaradas en la función. Quedan fuera: globales, arreglos, parámetros, variable de retorno, contador de `S` y su límite | Aho §10.2.1; la forma global (SSA, Cytron et al. 1991) queda fuera del alcance |
| Dos calendarizaciones | El mismo algoritmo (calendarización por lista, Aho §10.3) antes de asignar (reordenamiento) y después de expandir (empaquetado, `cg_schedule`) | Aho §10.2.4; Goodman y Hsu (1988) |
| Empaquetado en bundles | `cg_schedule` del Proyecto I en todos los niveles | Verificado; respeta los bundles fijos de CRYPTO |
| Lazos: dos fuentes | Marcas de `S` y `L` al construir, solo para el unrolling; y análisis de lazos naturales recalculable en cualquier etapa (profundidad de cada bloque, para el spilling) | Aho §9.6.4 y algoritmo 9.46. Los lazos estructurados de Calser dan grafos reducibles |
| Estado observable | Variables globales, variables de `main` (escalares y arreglos), memoria de datos externa (`0x1000`–`0xFFFF`) y estado CRYPTO. Las variables de otras funciones, la pila y las casillas de preservación no son observables | Es lo que comprueban las pruebas y el procesador |
| Derrame | Extensión propia por función, al final de los datos (`spill_base`, `spill_size`). Solo en `-O1`/`-O2` | Appel; Cooper y Torczon |
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
| Heurística del unrolling | Si N es constante y vale 8 o menos: desenrollado total. Si no: factor 4 (o el de `--unroll=N`), con lazo de residuo para N no múltiplo del factor. Nunca superar el tope de 4096 bundles |
| Prueba de que `/`, `%` y `^` no fallan (DCE) | Solo por constantes: divisor constante distinto de cero, o exponente constante mayor o igual que cero. Sin esa prueba no se eliminan |
| Independencia de memoria (reordenamiento) | Dos accesos son independientes solo si los dos son `rz` más constante, con constantes distintas. Cualquier dirección calculada (por ejemplo, un elemento de arreglo) se trata como dependiente de todo otro acceso |

### A.6 Regla para ambigüedades durante la implementación

- Las resuelve el chat de la tarea, con la opción más simple, y las documenta: nombres,
  detalles internos, mensajes, estructuras privadas de un módulo.
- Van al cerebro: lo que cambie la arquitectura, el contrato de una pasada (parte 7), las
  banderas o el formato de salida visible, el comportamiento de `-O0`, o lo ya integrado.
