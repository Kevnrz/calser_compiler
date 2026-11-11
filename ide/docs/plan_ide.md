# CALSIDE: estado actual y plan de cierre

Actualizado: 2026-11-11. Este es el único checklist vigente del IDE. El [historial](bitacora_ide.md) conserva entregas y decisiones anteriores; no se acumulan aquí anexos contradictorios.

## Frontera del producto

Python/PySide6 gestiona interfaz, documentos, archivos, permisos y procesos. C decide léxico, sintaxis, semántica, sugerencias, reparaciones y clones reutilizando ccom. LL(1) Python permanece como API de consulta/pruebas, fuera del flujo gráfico; el simulador Python externo está autorizado.

CALSIDE puede ejecutarse desde Python en desarrollo. Una aplicación empaquetada instalable sigue pendiente; no confundir esos estados. Completar el IDE tampoco certifica las optimizaciones, ISA o entregas de Arquitectura/Compiladores ajenas a su interfaz.

## Funcionalidades y estado

| ID | Funcionalidad | Estado real |
| --- | --- | --- |
| F01 | Aplicación de escritorio propia | Implementada con PySide6. |
| F02 | Archivos: nuevo/abrir/guardar/guardar como | Implementado, con protección ante cambios externos. |
| F03 | Pestañas y cambios sin guardar | Implementado; confirmación al cerrar y buffers independientes. |
| F04 | Editor y números de línea | Implementado. |
| F05 | Búsqueda/edición de texto | Implementado; búsqueda de proyecto incluye buffers. |
| F06 | Compilar con ccom | Implementado mediante F5 y perfiles. |
| F07 | Salida, cancelación y errores de procesos | Implementado para compilación/ejecución. |
| F08 | Diagnósticos gráficos y navegación | Implementado desde C; incluye varios errores sintácticos con recuperación conservadora. |
| F09 | Colores y errores léxicos | Flex real mediante `--tokens-json`. |
| F10 | Análisis automático | `--check-json` sobre copias sin guardar, por documento/revisión. |
| F11 | Sugerencias | Motor C conectado; Ctrl+Espacio, clic derecho y pausa opcional. No inventa identificadores ni instrucciones completas. |
| F12 | Reparaciones propuestas | Motor C conectado; `;`, `.`, `]`, `)`, vista previa, precondiciones y Deshacer. |
| F13 | Corrección automática | Implementada de forma conservadora para una regla de `;`, con habilitación y selección del usuario. Otras reglas requieren desarrollo/pruebas. |
| F14 | Clones Tipo I | Comparación C, formato/comentarios ignorados. |
| F15 | Clones Tipo II | Comparación C con biyección de identificadores y variación de literales por clase. No demuestra equivalencia semántica. |
| F16 | Recuperación ante fallos | Protecciones implementadas; validar manualmente los recorridos entregados. |
| F17 | Cambios externos | Monitor, recargar/conservar/copia y comprobación al guardar. No hay bloqueo exclusivo contra otro escritor. |
| F18 | Preferencias | Persistencia de fuente, proyecto, ccom, paneles y asistencia; tema/timeout configurables aplazados. |
| F19 | Opciones gráficas de compilación | Implementadas: análisis, compilación, AST y ensamblado; destino y banderas compatibles. |
| F20 | Métricas de optimización | Mejora futura; requiere contrato de datos del backend. |
| F21 | Ocultar/recuperar explorador | Implementado: ×, Archivos, Ctrl+B y menú Ver. |
| F22 | Resultados abajo/derecha | Implementado. |
| F23 | Varias terminales | Implementado en Linux; PTY sin emulación de pantalla completa. |
| F24 | Alineación y posición de menús | Implementado. |
| F25 | Aplicación empaquetada | Pendiente E8. |
| F26 | Esquina de papel animada | Mejora futura; esquina estática actual. |

Las funciones implementadas tienen pruebas automatizadas; aún deben pasar la revisión humana y la prueba en el equipo de entrega. Las limitaciones de M7, asistencia y terminal se conservan explícitas en las referencias técnicas.

## Etapas cerradas de implementación

- [x] E1–E4: base/editor, archivos, compilación, léxico y análisis automático.
- [x] E5–E6: sugerencias, reparaciones y automatización conservadora conectadas a C.
- [x] E7: archivos externos, proyecto y preferencias, verificados localmente.
- [x] M0–M7 y M1B: frontera C/Python, servicios reales, perfiles gráficos, motores, limpieza y recuperación sintáctica.

La evidencia y evolución están en la [bitácora](bitacora_ide.md). No hay motores pendientes de conectar en estos recorridos.

## Siguiente trabajo: revisión manual

- [ ] Copiar los ejemplos a un proyecto temporal, abrir CALSIDE y registrar plataforma/pantalla.
- [ ] Completar los bloques de [revisión manual](revision_manual.md), anotando resultado y fallos.
- [ ] Corregir cada fallo y repetir su caso y las pruebas afectadas.
- [ ] Contrastar el alcance demostrado con los requisitos vigentes del proyecto y el profesor; anotar cualquier diferencia antes de afirmar cierre de requisitos.

No se marca un caso como aprobado solo porque tenga una prueba automática. Los fallos descubiertos se registran en la misma guía; su solución y evidencia pasan a la bitácora.

## E8: empaquetado y otra máquina

- [ ] Confirmar sistema operativo y arquitectura de la entrega. Linux es la plataforma probada; Windows/macOS requieren validación propia.
- [ ] Elegir herramienta y entorno de empaquetado reproducible.
- [ ] Incluir recursos y dependencias; decidir distribución/configuración de ccom y simulador.
- [ ] Abrir desde icono/acceso directo, sin VS Code ni comando Python manual.
- [ ] Probar edición, guardado, análisis, compilación, ejecución y rutas con espacios fuera del repositorio/directorio de desarrollo.
- [ ] Probar en otra máquina y documentar instalación, bibliotecas y límites reales.

Cierre E8: el paquete funciona como aplicación normal en la plataforma acordada y el recorrido es reproducible.

## E9: validación final y defensa

- [ ] Completar revisión manual y resolver los fallos de entrega.
- [ ] Ejecutar suites pertinentes sobre la versión final.
- [ ] Leer/comentar código, completar diagramas y documentación explicativa de la arquitectura final.
- [ ] Preparar programas de demostración y resultados esperados; ensayar con un compañero.
- [ ] Revisar alcance/responsabilidades con el resto del proyecto; no atribuir al IDE funciones backend no entregadas.
- [ ] Preparar revisión/PR/publicación cuando se solicite; no forma parte de una limpieza automática de documentación.

## Mejoras futuras, separadas del cierre

- Predicción gris dentro del editor, aceptable con Tab y cancelable; no modificar el buffer al previsualizar. Completar instrucciones completas requiere propuestas justificadas por C.
- Esquina morada que se dobla y revela controles: clic/teclado, movimiento reducido y conservación de mover/redimensionar/cerrar.
- Apertura animada: dibujar hexágono lado a lado, girarlo/desplazarlo a la izquierda y dibujar CALSIDE letra por letra. Objetivo inicial aproximado 2 s, carga en paralelo, saltable con clic/Esc, desactivable y sin espera obligatoria.
- Tema claro, preferencias adicionales y más reglas de reparación con positivos/negativos.
- Agrupar visualmente pares de clones equivalentes manteniendo los pares de archivos reales. Tres archivos pueden producir tres pares legítimos.
- Métricas/IR/optimizaciones cuando exista contrato de datos; recursos del compilador cuando estén implementados.
- Terminal con emulación completa, redimensionamiento PTY y otras plataformas.

## Documentación vigente

[Uso](../README.md) · [Sistema](guia_sistema_calside.md) · [Asistencia](asistencia.md) · [Servicios C](../../docs/ccom_servicios_ide.md) · [Revisión manual](revision_manual.md) · [Bitácora](bitacora_ide.md) · [LL(1) de consulta](../nucleo/README.md).
