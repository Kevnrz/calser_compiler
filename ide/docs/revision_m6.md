# Revisión y limpieza de CALSIDE · M6

Fecha: 2026-11-11. M1–M6 implementados; M7 queda pendiente.
Este documento registra el mapa vigente y el alcance de la revisión. La explicación completa del sistema está en [guia_sistema_calside.md](guia_sistema_calside.md).

## Decisión sobre el código anterior

| Pieza | Decisión y motivo |
| --- | --- |
| `ide/nucleo/asistencia.py` | Productivo: modelos y validación estructural, sin importar analizadores ni Qt. Mantener su ubicación evita romper contratos del aplicador y las entregas. |
| `ide/nucleo/lexico.py`, `gramatica.py`, `tabla.py`, `parser.py` | Conservar API de consulta, estudio y pruebas del LL(1), autorizada por Saddy. Cabeceras/README explican que no es backend gráfico ni respaldo de ccom. |
| Sugerencias/reparaciones Python de la entrega | No se cargan en producción; los proveedores efectivos son `ide.sugerencias_ccom` y `ide.reparaciones_ccom`. No se fusiona ni se borra la rama del compañero desde esta revisión. |
| Regex/firma/comparación de clones Python | Sustituidos en M5 por Flex/`src/clones.c`. El helper `tokenizar` restante consulta Flex y solo se conserva para inspección/pruebas, no para el detector. |
| `ide/adaptador_nucleo.py` | Conservar nombre por compatibilidad: hoy adapta Flex/JSON a Qt. No importa el lexer del núcleo. |
| Regex de `ide/ccom.py:diagnosticos` | Conservar para leer la salida textual de F5. No reconoce código Calser ni determina reglas del lenguaje. |
| `ide/servicio_ccom.py` y `interfaz/comprobacion.py` | Conservar ambos: transporte Popen bloqueante en trabajadores M2–M5 y QProcess dirigido por señales para M1. Sus ciclos de vida son distintos; no hay dos analizadores. |
| Simulador Python | Conservado como herramienta externa autorizada. F6 ejecuta los artefactos de F5. |

No se eliminan módulos por estar escritos en Python: se retira el uso redundante del lenguaje en los recorridos productivos, preservando lo que sigue teniendo consumidores reales.

## Cambios de limpieza

- `ccom.informe_json` centraliza UTF-8, duplicados, constantes no JSON, versión, operación, identidad de fuente y código real del proceso. Lo reutilizan los seis servicios; cada adaptador conserva su validación específica de estados y contenido.
- `lexico_json` reutiliza el informe decodificado para diagnósticos y tokens. Se elimina la segunda lectura JSON del mismo mensaje.
- Se retira `_informe` de reparaciones y las copias de la cabecera en sugerencias/clones.
- `lexer_parse_prefix` pasa a llamarse `lexer_parse_text`: su función sirve tanto al prefijo de M3 como a la copia completa de M4. Se actualizan declaración, definición y consumidores; no cambia la gramática ni los tokens.
- Se retira el vector inverso de valores de clones que nunca se leía. La marca `seen_b` ya garantiza la unicidad inversa de la biyección; sus pruebas negativas se conservan.
- Se actualizan cabeceras/comentarios de servicios C, interfaces públicas, observadores Flex/Bison y adaptadores Python: consumidores, estado global/local, propiedad de memoria, rangos, errores y cancelación.
- Se actualizan las guías antiguas para que no recomienden volver a integrar un backend Python ya sustituido.

## Recorridos productivos

| Acción | Entrada Python | Decisión en C | Salida/interfaz |
| --- | --- | --- | --- |
| Léxico/colores | `adaptador_nucleo.analizar_documento` | `--tokens-json`, Flex | `convertir_lexico`, `ResultadoLexico`, `Editor.aplicar_analisis`. |
| Sintaxis/semántica automática | `ComprobadorCcom` | `--check-json`, Flex/Bison/semántico | `ccom.diagnosticos_json`, Problemas y marcas. |
| Sugerencias | `AsistenciaEditor` → `sugerencias_ccom.sugerir` | `--suggest-json`, Flex y Bison/LAC | Modelos `Sugerencia`, popup, aplicador con revisión vigente. |
| Reparaciones | `ReparacionesVentana` → `reparaciones_ccom.proponer_reparaciones` | `--repair-json`, candidatos y reanálisis C | Vista previa, motivos, aplicación y Deshacer. |
| Validación automática de reparación | `politica_asistencia.evaluar_automatizacion` | `--syntax-json` original y candidato | Permisos/regla/vigencia en Python; no guardar ni encadenar. |
| Clones | `PanelClones` → `ide.clones.detectar` | `--clones-json`, Flex y comparación Tipo I/II | Fragmentos, tabla, comparación y navegación vigilada. |
| Compilación/ejecución | Ventana, perfil F5 y controlador F6 | ccom genera artefactos | Simulador externo Python, salida y traza. |

## Propiedad y unidades

C recibe copias; los servicios nunca editan documentos del usuario. En M2–M5 el transporte crea una carpeta privada y recoge el hijo antes de borrarla. QProcess hace lo equivalente mediante señales en M1. No se descartan silenciosamente errores de versión/transporte para inventar resultados vacíos.

Los servicios C producen bytes UTF-8 `[start,end)`, los contratos de ediciones usan caracteres Python y Qt usa UTF-16. Esas conversiones son adaptación de interfaz, no otra tokenización. No mezclar el token inesperado con el rango visual elegido para el diagnóstico ni con la posición de inserción de una reparación.

Los servicios C son seriales dentro de un proceso: Flex/Bison y observadores comparten estado global. CALSIDE usa hijos separados, así que varias tareas no comparten esos globales. Bison libera subárboles/cadenas de consulta mediante destructores; la compilación normal conserva la transferencia del AST a main. Clones libera sus tablas/textos y solo presta lexemas mientras vive ese contexto.

La autorización automática es independiente del resultado C: permiso de usuario, regla seleccionada, precondiciones y revisión vigente siguen en Python. C comprueba el lenguaje; Qt aplica una única operación de Deshacer sin guardar.

## Pruebas y recorrido gráfico

```sh
make check
make check-ide
make check-recorrido-ide
```

`test_arquitectura_ccom.py` verifica cabeceras de los seis servicios reales y respuestas cruzadas/UTF-16/duplicados inválidos. Ejecuta el recorrido gráfico en un proceso aislado donde importar `ide.nucleo.lexico`, `gramatica`, `tabla` o `parser` falla expresamente. No bloquea `asistencia.py`, que contiene los modelos productivos.

`tools/probar_recorrido_ide.py` realiza cinco grupos de acciones con Qt real, sin proveedores ficticios: dos pestañas con Unicode y análisis; clones y navegación; error sin guardar, reparación, Deshacer/Rehacer y disco intacto; sugerencias en documento sin nombre y cambio de pestaña; F5/F6 con resultado **5**. Cierra los trabajadores y limpia el proyecto temporal. Es un recorrido gráfico automatizado; no sustituye la valoración visual humana en otros tamaños de pantalla.

Para guardar una captura de ese recorrido:

```sh
QT_QPA_PLATFORM=offscreen python3 tools/probar_recorrido_ide.py --captura /tmp/calside-m6.png
```

Verificación final M6: **382 pruebas IDE correctas**, suite completa del compilador y recorrido gráfico. La compilación usa `-Wall -Wextra` sin advertencias nuevas. Las pruebas Python del LL(1) permanecen identificadas como estudio/diferenciales; las pruebas del lenguaje productivo consultan C. Los dobles de las suites de coordinación/transporte prueban fallos, plazos y descarte de respuestas, no demuestran reglas de Calser.

## Orden para leer y entender el código

1. `ide/main.py`, `ide/__init__.py`, `ide/interfaz/ventana.py`: arranque y conexiones.
2. `ide/ccom.py`, `ide/servicio_ccom.py`, `ide/interfaz/comprobacion.py`: ejecutable, cabecera común y procesos.
3. `src/main.c`, `include/diagnostics.h`, `src/diagnostics.c`, `src/diagnostics_json.c`: despacho y datos reales.
4. `grammar/lexer.l`, `include/lexer_service.h`, `ide/adaptador_nucleo.py`, `ide/interfaz/analisis.py`: tokens y colores.
5. `grammar/parser.y`, `src/suggestions.c`, `src/syntax_service.c`, `src/repairs.c`: LAC, candidatos y propiedad de AST.
6. `ide/nucleo/asistencia.py`, `ide/sugerencias_ccom.py`, `ide/reparaciones_ccom.py`, `ide/interfaz/asistencia.py`: contratos y coordinación.
7. `ide/politica_asistencia.py`, `ide/ediciones.py`, `ide/interfaz/ediciones.py`: permisos, vigencia y edición.
8. `src/clones.c`, `ide/clones.py`, `ide/interfaz/clones.py`: paquete, equivalencia y comparación visual.
9. `ide/nucleo/README.md`: LL(1) de consulta/pruebas, separado del recorrido anterior.

Los comentarios de M6 cubren los servicios/adaptadores de la migración y sus puntos de integración. No implican una reescritura ni auditoría total del generador, semántico, simulador o todos los widgets.

## Pendientes reales

**M7:** recuperación sintáctica con sincronización y varios errores fiables en una consulta. El formato JSON ya admite listas, pero la recuperación aún se detiene en el primer error sintáctico. Varios errores léxicos/semánticos disponibles no completan M7.

Empaquetado, animaciones, texto de predicción gris y mejoras visuales mantienen su planificación propia. No se añadieron como parte de M6.
