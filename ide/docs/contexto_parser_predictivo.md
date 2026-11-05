# Contexto de implementación: parser predictivo LL(1)

Responsable: Saddy. Habilita la base de E5 y los contextos que necesitan sugerencias/reparaciones.

## Resultado esperado

Completar el algoritmo que ejecuta la tabla LL(1) existente. Debe analizar un programa y poder detenerse en el cursor para explicar qué terminales son válidos a continuación. Tener FIRST/FOLLOW y la tabla no ejecuta todavía el análisis.

## Contexto compartido y límites

Proyecto: CALSIDE, IDE nativo de Calser hecho con Python/PySide6. Repositorio de trabajo: `/home/saddygr/Documentos/GitHub/calser_compiler`. Fecha de este reparto: 2026-11-05.

Ya existen el editor, pestañas, guardado, compilación F5, diagnósticos gráficos, clones Tipo I/II y terminales. E3 conecta el lexer Python al editor. E4 ejecuta `ccom --check` sobre una copia temporal del texto para validar sintaxis y semántica sin guardar ni generar artefactos. El parser LL(1) complementará esa ruta; no sustituirá la semántica ni la compilación de ccom.

El núcleo recibido contiene lexer, gramática LL(1), FIRST/FOLLOW y tabla predictiva. **Todavía no contiene parser predictivo, sugerencias ni reparaciones.** Este documento define trabajo por realizar; los nombres de API que propone no existen aún y deben acordarse entre los tres responsables antes de implementar dependencias.

Reparto decidido por Saddy:

- Saddy: parser predictivo (completar la ejecución de LL(1)).
- Compañero de sugerencias: motor de sugerencias.
- Compañero de reparaciones: motor de reparaciones.
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

Propuesta de contrato v1: offsets globales en caracteres de Python, desde 0, con rangos `[inicio, fin)`; cursor entre 0 y `len(texto)`. Inserción: `inicio == fin`. Línea/columna de presentación: desde 1; una tabulación cuenta un carácter. El lexer actual usa fin de columna inclusivo: convertir expresamente. Qt usa UTF-16 y ccom bytes UTF-8; esa conversión pertenece al adaptador del IDE, no al motor.

El IDE captura `(documento, revision, texto, cursor)` y asocia ese contexto con cada tarea; descarta resultados antiguos. Los motores pueden recibir solo texto/cursor, pero toda respuesta aplicada debe corresponder al mismo texto y revisión. Ningún módulo decide que una respuesta antigua sigue siendo válida.

No compartir pilas mutables entre llamadas. Límites explícitos de pasos/resultados y estados de cancelación o límite alcanzado, sin presentarlos como análisis correcto. Si se añade cancelación cooperativa, acordar la misma convención en las APIs. Errores del usuario se devuelven como datos; fallos internos o contratos inválidos no deben ocultarse como «sin errores».

Las clases comunes propuestas (`DiagnosticoSintactico`, `ContextoPredictivo`, `EdicionTexto`) pueden vivir en `ide/nucleo/modelos.py`. **Saddy coordina ese archivo** para evitar tres definiciones incompatibles. Acordar campos y fijar ejemplos serializables antes de que los compañeros dependan de ellos; no es necesario generar JSON para uso interno.

### Entrega y validación común

Entregar código, pruebas reproducibles y documentación de API con ejemplos de entrada/salida, límites conocidos y cambios respecto al contrato. Ejecutar `make check-ide`; referencia antes de estas entregas: 70 pruebas combinadas. Añadir pruebas en `ide/pruebas/`. Si se toca ccom o su gramática, ejecutar también las comprobaciones del compilador pertinentes. Coordinar cambios compartidos y comunicar rama/commit para integración; estos documentos no autorizan publicar, fusionar ni enviar mensajes automáticamente.

## Alcance propio

Crear `ide/nucleo/parser.py`. No construir un AST de compilación ni repetir análisis semántico. Reutilizar la tabla y el lexer actuales. No implementar el listado visual, los textos insertables ni el motor de reparaciones: esos pertenecen a los otros responsables.

### API propuesta a fijar primero

```python
analizar_sintaxis(texto: str) -> ResultadoSintactico
contexto_en(texto: str, cursor: int, inicio_fragmento: int | None = None) -> ContextoPredictivo
```

- `ResultadoSintactico`: aceptación sintáctica, análisis completo/interrumpido, errores léxicos separados, diagnósticos sintácticos y, opcionalmente, traza limitada.
- `DiagnosticoSintactico`: código estable, mensaje, inicio/fin, encontrado, terminales esperados y contexto suficiente para reparación.
- `ContextoPredictivo`: punto real hasta el que se analizó, pila residual inmutable, esperados, errores previos y estado (`valido`, `bloqueado`, `comentario`, `limite`, etc.; fijar enumeración).
- `inicio_fragmento`: cuando se completa un token parcial, analizar hasta su comienzo; el cursor marca hasta dónde llega lo escrito. No exigir que el token parcial ya sea una palabra válida.

Los otros motores necesitan conocer si el contexto es fiable. Si hay un error anterior que impide llegar al cursor, devolverlo y marcar el bloqueo. No adivinar un estado correcto después del error.

## Implementación y decisiones

1. Validar `tabla.es_ll1`; rechazar conflictos de configuración explícitamente.
2. Inicializar pila con fin de entrada y símbolo inicial; documentar qué extremo es la cima. Consumir terminales coincidentes y expandir no terminales con `M[A, lookahead]`, apilando el cuerpo en orden inverso. Epsilon no se apila.
3. Añadir fin de entrada virtual sin inventar una posición fuera del texto. Aceptar solamente cuando coinciden fin de pila y fin real de entrada. Un programa con errores léxicos no es globalmente válido aunque los tokens restantes encajen.
4. Primera versión: detenerse ante el primer error sintáctico y devolver estado útil. La recuperación con ediciones es responsabilidad del motor de reparaciones. Si se añade recuperación sin editar para mostrar varios errores, identificar sus contextos como recuperados y garantizar progreso.
5. Exponer estado de prefijo sin tratar el cursor como si siempre fuera el fin definitivo del programa. En un prefijo válido, EOF puede faltar legítimamente.
6. Calcular los esperados de **la secuencia residual de la pila**, incluyendo las continuaciones de símbolos anulables. No basta con tomar la fila del no terminal superior: sus entradas pueden depender del FOLLOW global y sobreestimar lo válido en ese punto concreto. Para una cima terminal, considerar el terminal requerido.
7. Definir tokens partidos por cursor, espacios, EOF, comentarios de línea/bloque y comentario sin cerrar. El lexer omite comentarios: mantener una detección de contexto coherente con sus reglas sin cambiar su API existente. Exponer esa información para que sugerencias no la reimplemente de otra forma.
8. No mutar `CALSER`, tokens o una tabla reutilizada. Limitar pasos/traza y evitar bucles con gramáticas inválidas. Documentar costes y comportamiento ante entradas grandes.

## Pruebas de aceptación

- Gramática pequeña de libro: expansión, epsilon, rechazo y consumo completo.
- Programas Calser válidos y casos inválidos con `;`, `.`, `[]`, `()` conforme a la gramática.
- El caso `vs` seguido por `E ;` devuelve encontrado/esperados/posición fieles, sin prometer mensajes semánticos.
- Entrada vacía, EOF incompleto y tokens sobrantes; no aceptar por haber agotado solo una de las dos partes.
- Cursor antes, dentro y después de un identificador/palabra parcial; continuación tras símbolos anulables.
- Comentarios, Unicode antes del error, CRLF y tabulaciones; offsets correctos.
- Error léxico anterior, límite de pasos y llamadas repetidas sin estado compartido accidental.
- Comparar aceptación sintáctica con Bison sobre un corpus, separando errores semánticos de diferencias reales de lenguaje; `ccom --check` también hace semántica y no basta comparar solo su código de salida.

## Coordinación inmediata

Publicar primero las dataclasses y ejemplos de `contexto_en` para que los compañeros puedan usar dobles de prueba. Pedir a sugerencias que precise el uso del fragmento parcial, y a reparaciones qué datos necesita del primer error. Avisar cuando cambie el contrato. Integrar con widgets será un paso posterior.
