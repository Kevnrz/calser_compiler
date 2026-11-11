# Programas para recorridos manuales

Doce programas independientes y válidos, de menor a mayor variedad. Cada archivo incluye sus resultados esperados. No dependen unos de otros ni se compilan todos juntos.

## Cómo probarlos

1. Trabaja en una copia temporal, siguiendo [la preparación de revisión manual](../../../docs/revision_manual.md#preparación).
2. Abre un `.cls`, espera el análisis y selecciona el perfil **Compilar**. Pulsa F5 y luego F6 para ejecutar.
3. Compara las variables finales de **Ejecución** con la tabla. Compilar solo genera artefactos; no ejecuta el programa. Estos ejemplos no imprimen con una instrucción de salida: los valores se observan en el estado final del simulador.
4. Modifica una entrada, calcula el resultado esperado y vuelve a compilar/ejecutar. Cambiar código sin recompilar no debe ejecutar silenciosamente una versión anterior.

Los diagnósticos de **Problemas** pueden incluir otras pestañas abiertas con errores. F5 compila el archivo activo. Cierra las pestañas inválidas si quieres comprobar el contador en cero para un ejemplo válido.

## Casos y resultados

Las listas de matrices se leen **por filas**: `[13, -16, -7, 22]` representa `[[13, -16], [-7, 22]]`. Los booleanos se observan como `0` (falso) y `1` (verdadero). Los índices empiezan en cero y el límite final de `S` es exclusivo.

| Archivo | Qué ejercita | Variables finales esperadas |
| --- | --- | --- |
| [01_operaciones](01_operaciones.cls) | Precedencia, division entera, residuo y potencia. | `expresion = 14, agrupada = 20, cociente = 3, residuo = 2, potencia = 32` |
| [02_funciones](02_funciones.cls) | Llamadas adelantadas y anidadas; los argumentos usan corchetes. | `resultado = 14` |
| [03_condiciones](03_condiciones.cls) | C y A: ejecutar tanto la rama verdadera como la alternativa. | `negativo = -1, cero = 0, positivo = 1` |
| [04_ciclos](04_ciclos.cls) | L mientras; S con fin exclusivo y paso positivo o negativo. | `suma_l = 15, suma_s = 15, descendente = 15, contador = 6` |
| [05_recursion](05_recursion.cls) | Factorial recursivo con R y caso base. | `resultado = 120, base = 1` |
| [06_arreglo_recorrido](06_arreglo_recorrido.cls) | Recorrer un arreglo de cinco elementos, sumar y hallar el mayor. | `datos = [3, -2, 7, 0, 4], total = 12, mayor = 7` |
| [07_arreglo_copia](07_arreglo_copia.cls) | Copiar un arreglo completo y modificar solo el destino desde una funcion. | `original = [1, -2, 3, 4], copia = [3, -6, 9, 12]` |
| [08_arreglo_busqueda](08_arreglo_busqueda.cls) | Buscar con retorno temprano; -1 representa un elemento ausente. | `encontrado = 2, ausente = -1` |
| [09_matriz_suma_resta](09_matriz_suma_resta.cls) | Sumar y restar matrices 2x2 elemento por elemento. | `suma = [1, 3, 2, 6], resta = [1, -7, 4, 2]` |
| [10_matriz_transpuesta](10_matriz_transpuesta.cls) | Transponer una matriz 2x3 para obtener una 3x2. | `original = [1, 2, 3, 4, 5, 6], transpuesta = [1, 4, 2, 5, 3, 6]` |
| [11_matriz_producto](11_matriz_producto.cls) | Producto 2x3 por 3x2, con tres ciclos y acumulador por celda. | `producto = [13, -16, -7, 22]` |
| [12_booleanos](12_booleanos.cls) | Corto circuito: las dos primeras expresiones no llaman a tocar. | `conjuncion = 0, disyuncion = 1, antes = 0, evaluada = 1, despues = 1` |

## Variaciones para asistencia y diagnósticos

Hazlas de una en una sobre una copia; después restaura el programa válido. No todas las situaciones ambiguas tienen una corrección automática.

- **Sugerencias:** borra un fragmento y vuelve a escribirlo. Prueba `F main` → `[]: ` → `void` → `;`. Tab acepta la propuesta gris; Esc la descarta; Ctrl+Espacio abre alternativas. Una línea vacía del cuerpo no debe inventar una sentencia. Después de `E;`, pulsa Enter para comprobar el punto sugerido en la siguiente línea.
- **Reparación:** en `01_operaciones`, elimina el `;` después de `2 + 3 * 4`; pide Reparaciones con Alt+Enter. Revisa la propuesta, aplícala y usa Ctrl+Z. La corrección al pulsar Enter depende de las preferencias y de que la regla pueda validarse.
- **Léxico:** inserta `@` fuera de un comentario. Debe aparecer un error léxico; bórralo antes del siguiente caso.
- **Sintaxis de llamadas:** en `02_funciones`, cambia `duplicar[3]` por `duplicar(3)`. Debe explicar que las llamadas usan corchetes. Los paréntesis aritméticos de `01_operaciones` sí son válidos.
- **Semántica:** en `06_arreglo_recorrido`, cambia un uso de `datos` por `dato_inexistente`. Con sintaxis válida debe denunciar el nombre no declarado.
- **Inmutabilidad:** en `07_arreglo_copia`, intenta agregar `original[0] = 99;` dentro de `main`; `Vi` protege la entrada. El programa original modifica únicamente `copia`.
- **Ejecución y cancelación:** en una copia de `04_ciclos`, quita el incremento de `contador`. Prueba Cancelar o el límite de ejecución; luego restaura la línea. Un fin por límite no cuenta como resultado correcto.

La revisión humana de estos recorridos sigue pendiente aunque sus resultados se hayan comprobado con ccom y el simulador. Para la prueba anterior de **tres pares de clones**, abre y analiza solo la subcarpeta `clones`; este conjunto completo contiene más similitudes y no tiene un número de pares fijado.
