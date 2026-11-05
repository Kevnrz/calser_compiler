# Contexto de implementación: motor de sugerencias

Responsable: compañero de sugerencias. Corresponde al motor de E5/F11; Saddy implementa el parser predictivo.

## Resultado esperado

Dado el texto y el cursor, devolver opciones sintácticamente justificadas para continuar Calser. No se solicita crear ventanas ni conectar Ctrl+Espacio: CALSIDE integrará los resultados posteriormente.

## Contexto compartido y límites

Proyecto: CALSIDE, IDE nativo de Calser hecho con Python/PySide6. Repositorio de trabajo: `/home/saddygr/Documentos/GitHub/calser_compiler`. Fecha de este reparto: 2026-11-05.

Ya existen el editor, pestañas, guardado, compilación F5, diagnósticos gráficos, clones Tipo I/II y terminales. E3 conecta el lexer Python al editor. E4 ejecuta `ccom --check` sobre una copia temporal del texto para validar sintaxis y semántica sin guardar ni generar artefactos. El parser LL(1) complementará esa ruta; no sustituirá la semántica ni la compilación de ccom.

El núcleo recibido contenía lexer, gramática LL(1), FIRST/FOLLOW y tabla. **Saddy ya implementó el parser predictivo** en `ide/nucleo/parser.py`; sugerencias y reparaciones siguen pendientes. La API real y sus ejemplos están en [parser_predictivo.md](parser_predictivo.md). Los nombres propuestos para el motor de este documento todavía no están implementados; adaptar ese motor a la API del parser disponible.

Reparto decidido por Saddy:

- Saddy: parser predictivo (completar la ejecución de LL(1)).
- Un mismo compañero: motor de sugerencias.
- Ese mismo compañero: motor de reparaciones.
- Integración posterior en CALSIDE: adaptadores, coordinación de tareas y widgets de E5/E6.

Los motores deben ser Python puro, sin PySide6, acceso a widgets, lectura obligatoria de archivos, procesos ccom ni escritura del documento. Reciben texto en memoria y devuelven datos. Deben funcionar con documentos sin guardar. No implementar en cada motor otro lexer o una gramática paralela.

Lecturas del repositorio (rutas desde su raíz):

- `ide/nucleo/lexico.py`: `analizar(texto) -> (tokens, errores)`; `Token.tipo`, `lexema`, `linea`, `columna`, `columna_fin`, `valor`.
- `ide/nucleo/gramatica.py`: `CALSER`, `Gramatica`, `Produccion`, `FIN = "$"`, `EPSILON = "ε"`. Una producción vacía representa epsilon.
- `ide/nucleo/tabla.py`: `construir_tabla(CALSER)`, `TablaLL1.entrada(A, a)`, `.fila(A)`, `.primero`, `.siguiente`, `.es_ll1`.
- `ide/docs/gramatica_ll1.md` y `ide/docs/gramatica_calser.md`: transformaciones y lenguaje.
- `grammar/parser.y` y `grammar/lexer.l`: referencia del compilador real; investigar y documentar discrepancias, no modificar el lenguaje silenciosamente.
- `ide/docs/plan_ide.md`, `ide/docs/integrar_nucleo.md`: fases y arquitectura.

### Convenciones que deben compartir los tres módulos

Contrato del parser implementado: offsets globales en caracteres de Python, desde 0, con rangos `[inicio, fin)`; cursor entre 0 y `len(texto)`. Inserción: `inicio == fin`. Línea/columna de presentación: desde 1; una tabulación cuenta un carácter. El lexer actual usa fin de columna inclusivo: convertir expresamente. Qt usa UTF-16 y ccom bytes UTF-8; esa conversión pertenece al adaptador del IDE, no al motor.

El IDE captura `(documento, revision, texto, cursor)` y asocia ese contexto con cada tarea; descarta resultados antiguos. Los motores pueden recibir solo texto/cursor, pero toda respuesta aplicada debe corresponder al mismo texto y revisión. Ningún módulo decide que una respuesta antigua sigue siendo válida.

No compartir pilas mutables entre llamadas. Límites explícitos de pasos/resultados y estados de cancelación o límite alcanzado, sin presentarlos como análisis correcto. Si se añade cancelación cooperativa, acordar la misma convención en las APIs. Errores del usuario se devuelven como datos; fallos internos o contratos inválidos no deben ocultarse como «sin errores».

`DiagnosticoSintactico`, `ContextoPredictivo`, `SimboloEntrada` y `ResultadoSintactico` ya existen en `ide/nucleo/parser.py`. Importarlos desde ahí. No se creó `modelos.py`; `EdicionTexto` pertenece al motor de reparaciones. El compañero implementa ambos motores y coordina sus tipos comunes sin duplicar los modelos del parser.

### Entrega y validación común

Entregar código, pruebas reproducibles y documentación de API con ejemplos de entrada/salida, límites conocidos y cambios respecto al contrato. Ejecutar `make check-ide`; referencia después de implementar el parser: 96 pruebas combinadas. Añadir pruebas en `ide/pruebas/`. Si se toca ccom o su gramática, ejecutar también las comprobaciones del compilador pertinentes. Coordinar cambios compartidos y comunicar rama/commit para integración; estos documentos no autorizan publicar, fusionar ni enviar mensajes automáticamente.

## Dependencia y API propuesta

Crear `ide/nucleo/sugerencias.py`. Usar `contexto_en(texto, cursor, inicio_fragmento=None)` y `ContextoPredictivo` de `ide.nucleo.parser`, ya implementados. Solo ofrecer propuestas contextuales cuando `contexto.valido`; `comentario`, errores, límites y cancelación no son contexto válido. Los detalles y el primer error están en `contexto.resultado`. Usar pruebas con el parser real y dobles solo para aislar casos unitarios.

```python
sugerir(texto: str, cursor: int) -> ResultadoSugerencias
```

`ResultadoSugerencias`: estado, lista ordenada de sugerencias y explicación de bloqueo cuando corresponda. Una lista vacía por falta de opciones debe distinguirse de fallo interno, límite o contexto no disponible.

Cada `Sugerencia` debe contener:

- `etiqueta`: texto legible en la lista.
- `texto`: contenido insertable real, sin marcadores ficticios.
- `inicio`, `fin`: rango a reemplazar en el texto original.
- `categoria`: reservada, operador, delimitador, plantilla, etc.
- `detalle`: contexto breve; prioridad estable si se usa ranking.
- Opcional: cursor final relativo al texto insertado, con convención documentada.

El IDE adjuntará documento/revisión y validará que no cambiaron antes de aplicar.

## Comportamiento requerido

1. Obtener terminales válidos mediante el parser. Una fila aislada de la tabla no equivale al contexto completo; pedirle los esperados de la pila residual.
2. Reconocer el fragmento bajo el cursor. Para `Vm to|tal`, decidir y documentar que se reemplaza el identificador completo, sin dejar un sufijo duplicado. Para prefijos de palabras como `vo|`, analizar desde el comienzo del fragmento y filtrar por lo escrito.
3. No confundir tokens gramaticales con texto insertable: `id`, `num`, `ε` y `$` no son automáticamente propuestas literales. No inventar nombres de variables ni afirmar que están declaradas. La primera versión puede limitarse a reservadas, signos y plantillas justificadas; completar símbolos declarados exige un alcance adicional explícito.
4. No sugerir dentro de comentarios. Definir con Saddy cómo reconocer comentarios y bordes; no depender de que el lexer devuelva tokens de comentarios, porque los omite.
5. Distinguir prefijo válido de programa completo: el EOF temporal del cursor no implica un error definitivo. Si un error anterior bloquea el contexto, devolver bloqueo; evitar un listado global de palabras como si fuera contextual.
6. Usar orden determinista, eliminar duplicados y limitar resultados. Espacios, cursor fuera de rango y Unicode deben tener comportamiento documentado.
7. Las plantillas iniciales son opcionales. Si se incluyen, deben ser sintácticamente coherentes y permitir reemplazo local; no introducir variables arbitrarias ni saltar a otra parte del documento.
8. No aplicar ediciones, guardar, modificar cursor, realizar análisis semántico ni corregir errores previos. Esos pasos pertenecen al IDE u otros motores.

## Ejemplos y pruebas de aceptación

- Cursor al inicio: opciones justificadas por el símbolo inicial, no un catálogo completo.
- Contexto de tipo: sugerencias de tipo permitidas por esa producción; no asumir que `void` es válido en toda declaración.
- Prefijo de reservada y cursor en medio del fragmento: rango correcto, filtrado y ausencia de duplicación de sufijo.
- Tras una expresión: operadores/delimitadores realmente válidos para esa pila.
- Terminales internos `id`, `num`, `$`, `ε` nunca se insertan accidentalmente como código.
- Comentarios de línea/bloque/cerrado incompleto: resultado coherente con el contexto del parser.
- Error anterior al cursor, entrada vacía, final de documento, Unicode/CRLF/tabulaciones.
- Repetibilidad de orden, límite de opciones y ninguna mutación del texto.
- Pruebas unitarias con contexto controlado más pruebas integradas con el parser real antes de declarar terminada la entrega.

## Qué debe recibir CALSIDE

API y tipos estables, ejemplos completos de entrada/salida y catálogo de sugerencias soportadas. La interfaz se encargará de Ctrl+Espacio, activación por pausa, lista, flechas, Enter/Tab, Escape, revisión del documento y Deshacer. Entregar el motor independientemente de esa UI.
