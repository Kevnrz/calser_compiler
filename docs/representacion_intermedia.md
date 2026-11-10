# Representación intermedia de tres direcciones

C2.1a fija una forma construible a mano, sin generar desde el AST ni conectar
`main.c`. `include/tac.h` expone los contratos; `src/tac.c` arma y valida;
`src/tac_print.c` imprime. La lista de instrucciones del organizador existente
(`CGInstruction` en `codegen.c`) sigue independiente, sin renombrar código.
Referencias: Aho §6.2, §8.4, §10.2.3; enunciado §4.1.

## Operandos y cuádruplas

`TacOperand` distingue ausencia, temporal, variable (`const Symbol *` prestado),
entero de 64 bits y etiqueta local a una función. Los nombres impresos de las
variables vienen del símbolo; su identidad para análisis es el puntero, no el
nombre. El ámbito resuelto debe conservarse aunque dos símbolos se llamen igual.
Los temporales virtuales son pseudorregistros ilimitados en el sentido de Aho
§10.2.3: no hay techo de doce ni asignación física. Se numeran desde cero por
función con `size_t`; agotamiento del contador o memoria termina con diagnóstico.
El spilling los bajará a t0–t11. La anchura semántica del ISA se resolverá al
bajar las constantes, sin truncarlas en esta forma.

Cada `TacInstruction` guarda `op`, `dest`, `src1`, `src2` y enlace `next`.
`TAC_NONE` llena posiciones ausentes. La tabla inmutable consultada con
`tac_op_info` contiene máscaras de definición y lectura, tipos permitidos,
lectura/escritura de memoria, barrera, efecto lateral, flujo y formato de texto.
No hay estado global mutable: contadores y listas viven en los structs.

| Operación | dest / src1 / src2 | Define | Lee valores | Memoria | Barrera | No eliminable |
| --- | --- | --- | --- | --- | --- | --- |
| COPY | resultado / valor / — | dest | src1 | — | no | no |
| ADD SUB AND OR XOR EQ NE LT LE GT GE | resultado / valor / valor | dest | src1, src2 | — | no | no |
| NEG NOT | resultado / valor / — | dest | src1 | — | no | no |
| MUL DIV MOD POW | resultado / valor / valor | dest | src1, src2 | — | no | no |
| LOAD | resultado / base / índice | dest | src1, src2 | lectura | no | no |
| STORE | base / índice / valor | — | dest, src1, src2 | escritura | no | sí |
| CRYPTO | base destino / base origen / cantidad | — | dest, src1, src2 | lectura y escritura | sí | sí |
| PARAM | — / valor / — | — | src1 | — | no | sí |
| CALL | resultado opcional / símbolo función / aridad | dest si existe | — | lectura y escritura | sí | sí |
| RETURN | — / valor opcional / — | — | src1 si existe | — | sí | sí |
| GOTO | etiqueta / — / — | — | — | — | sí | sí |
| IF_EQ IF_NE IF_LT IF_LE IF_GT IF_GE | etiqueta / valor / valor | — | src1, src2 | — | sí | sí |

Resultado y base pueden ser temporal o variable; valor también puede ser
constante. CALL exige símbolo de función y aridad entera no negativa. Se usan
PARAM en orden de argumentos antes de CALL. La aridad y el símbolo de función
son metadatos, no usos de variables del programa. La comprobación de firmas y
el protocolo de argumentos corresponderán al productor futuro.

LOAD imprime `x = a[i]`; STORE, `a[i] = x` (Aho §6.2.1). El índice representa
un desplazamiento en bytes respecto de la base: el futuro productor calculará
el escalado y la linealización de matrices. STORE lee la dirección base, no
redefine su valor. CRYPTO representa una primitiva opaca de transformación de
memoria; su convenio manual es `crypto destino, origen, cantidad` (bytes).
El detalle de los intrínsecos de Calser, sus selectores y llaves se fijará al
construir desde el AST; esta tarea no implementa esa traducción. Siempre tiene
efectos, incluso si sus operandos parecen muertos.

Las variables son valores abstractos identificados por símbolo; las banderas de
memoria describen accesos indirectos y efectos implícitos, no domicilios físicos
de cada variable escalar. CALL es conservador sobre memoria y corta la movilidad
de instrucciones dentro del bloque, pero no lo termina. CRYPTO también es
barrera. Los análisis deben consultar memoria/barrera/efectos además de usos.
`tac_definitions` y `tac_uses` devuelven hasta tres operandos TEMP/VARIABLE sin
duplicados, en orden dest/src1/src2. No incluyen constantes, etiquetas, memoria
implícita ni el nombre del callee. Una escritura pura con resultado muerto
podrá eliminarse; esta forma no modela excepciones de aritmética.

MUL, DIV, MOD y POW permanecen como operaciones propias. El ISA no las tiene
(isa.md §6.1, §6.2, §8): expandirlas como hoy en `codegen_arith.c` introduce lazos
que partirían los bloques y estorbarían las optimizaciones. Se expanden al final.

## Bloques, grafo y recorrido

Un `TacProgram` posee funciones en orden de inserción. Cada `TacFunction` posee
bloques en ese orden, y su primer bloque es la entrada. Cada `TacBlock` posee
etiqueta, instrucciones y vectores explícitos de sucesores y predecesores.
`tac_connect` enlaza ambos sentidos, no duplica y rechaza cruzar funciones.
Puede haber ciclos del grafo, incluso aristas al mismo bloque; las listas
`next` deben seguir siendo acíclicas y contener memoria viva.

GOTO, IF y RETURN cierran bloque. GOTO tiene un sucesor; IF tiene primero el
salto tomado y luego la caída al bloque siguiente en la lista (si coinciden,
solo uno); RETURN no tiene sucesores. Sin terminador se cae al siguiente bloque,
con una arista explícita, o se sale si es el último. Así se permiten bloques
vacíos durante el armado. CALL puede tener instrucciones después dentro del
mismo bloque: es la excepción explícita solicitada al corte tradicional de
bloques por llamadas. El texto incluye sucesores en ese orden, etiquetas B0,
B1, etc. y temporales t0, t1, etc. sin direcciones del proceso ni fechas.

`tac_validate` devuelve 1 o 0 y un mensaje acotado. Comprueba operandos y etiquetas,
entrada, dueños, etiquetas repetidas, terminadores al final, destinos/caídas,
orden y cantidad de sucesores, reciprocidad y predecesores duplicados. No exige
SSA, alcanzabilidad ni definición antes del uso; no es verificación semántica CLS.
Las listas provienen de los constructores: no intenta recuperar ciclos de `next`
ni punteros liberados. El programa vacío es válido; una función sin entrada no.

Se itera con `program->first`, `function->next`, `function->first`, `block->next`
y `block->first`, `instruction->next`. Pueden editarse operandos y operación y
conectarse aristas durante el recorrido. Para borrar o reenlazar se guarda antes
`next` y se mantienen first/last, entrada y ambas direcciones del grafo. Se
revalida al acabar. Agregar al final permite que el recorrido visite lo agregado.
`tac_program_free` libera todos los propietarios, pero no los símbolos.

## Ejemplo paso a paso

Fragmento válido de Calser:

```text
F main [] : int res ;
    res = 3 ;
    C [ res < 10 ] ;
        res = res * 2 ;
    .
    E ;
.
```

1. Crear programa, función `main` y bloques B0 (condición), B1 (salida) y B2
   (cuerpo). Crear o tomar prestado el `Symbol` de `res`.
2. En B0 asignar 3 a `res`, luego IF_LT con destino B2, fuentes `res` y 10.
3. En B1 poner RETURN con fuente `res`. En B2 poner MUL con destino `res` y
   fuentes `res`, 2, seguido de GOTO B1.
4. Conectar B0→B2, B0→B1 y B2→B1, validar e imprimir:

```text
function main (entry B0)
B0: successors [B2, B1]
  res = 3
  if res < 10 goto B2
B1: successors []
  return res
B2: successors [B1]
  res = res * 2
  goto B1
```

El orden de lista no exige que la salida sea el último bloque; el grafo define
el control. Las etiquetas pertenecen a la función, no a todo el programa.

## Decisiones para las siguientes tareas

Orden fijado: **unrolling → DCE → reordenamiento/renombramiento/calendarización
→ asignación de registros con spilling → expansión final a ensamblador**.
`-O0` seguirá usando el generador actual, idéntico al P1. `-O1` y `-O2` pasarán
por la representación intermedia de tres direcciones. Esta tarea solo documenta
ese contrato futuro: no agrega banderas ni pasadas ni cambia el compilador.

Para Gabriel, Saddy y Kevin: usar `pruebas/tac/probar.c` como plantilla. Crear
programa con `tac_program_new`, funciones/bloques con `tac_function_new` y
`tac_block_new`, temporales con `tac_temp_new`, y cuádruplas con `tac_append`.
`TAC_NONE` se obtiene con `tac_none`; usar `tac_variable`, `tac_integer` y
`tac_label` para el resto. Conectar aristas después del armado, validar, ejecutar
la pasada, revalidar y comparar impresión. Para WAR/WAW reutilizar el mismo
operando temporal como destino varias veces: no es SSA. Consultar siempre tabla
y defs/usos en vez de deducir propiedades del texto impreso. Liberar el programa
antes de destruir los símbolos prestados.

`make check-tac` ejecuta aserciones de consultas/validación y compara exactamente
los cinco casos con `pruebas/tac/esperado.txt`, ejecutando el binario C y usando
`diff`, sin Python. La salida obtenida queda en `build/tac_obtenido.txt`.
Incluye memoria indexada, aristas
recíprocas, salto hacia atrás, llamada seguida de CRYPTO y dependencias WAR/WAW.
`make check` incluye ese target; las suites anteriores y `make ejercicios`
conservan su implementación.
