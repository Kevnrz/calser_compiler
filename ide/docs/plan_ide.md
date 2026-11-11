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
| F11 | Sugerencias | Motor C conectado; vista gris por defecto con Tab/Esc, alternativas Alt+[ / Alt+] y lista Ctrl+Espacio. No inventa identificadores ni instrucciones completas. |
| F12 | Reparaciones propuestas | Motor C conectado; `;`, `.`, `]`, `)`, vista previa, precondiciones y Deshacer; cierres locales en gris aceptables con Tab. |
| F13 | Corrección automática | Optativa para `;`, con una sola activación en preferencias y validación completa C. Comprobada con motor real; repetición humana de R12 pendiente. |
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
| F27 | Animación de inicio | Implementada: diseño geométrico aprobado, SVG/Qt, 4,4 s, clic/Esc y preferencia persistente. Revisión visual humana pendiente. |

Las funciones implementadas tienen pruebas automatizadas; aún deben pasar la revisión humana y la prueba en el equipo de entrega. Las limitaciones de M7, asistencia y terminal se conservan explícitas en las referencias técnicas.

## Etapas cerradas de implementación

- [x] E1–E4: base/editor, archivos, compilación, léxico y análisis automático.
- [x] E5–E6: sugerencias, reparaciones y automatización conservadora conectadas a C.
- [x] E7: archivos externos, proyecto y preferencias, verificados localmente.
- [x] M0–M7 y M1B: frontera C/Python, servicios reales, perfiles gráficos, motores, limpieza y recuperación sintáctica.

La evidencia y evolución están en la [bitácora](bitacora_ide.md). No hay motores pendientes de conectar en estos recorridos.

## Siguiente trabajo: revisión manual

- [ ] Copiar los ejemplos a un proyecto temporal, abrir CALSIDE y registrar plataforma/pantalla.
- [x] Bloque A de revisión manual: R01–R04 aprobados por el usuario (2026-11-11).
- [x] Bloque B de revisión manual: R05–R08 aprobados por el usuario (2026-11-11).
- [x] R09–R11 (sugerencias y reparación manual) aprobados por el usuario (2026-11-11).
- [ ] R12: ajustes de interacción implementados y comprobados con ccom real; pendiente repetir en la pantalla del usuario. La corrección automática es un requisito del enunciado, aunque la prueba se hubiera etiquetado opcional.
- [ ] Completar los bloques restantes de [revisión manual](revision_manual.md), anotando resultado y fallos; seguir desde R13. Las comprobaciones pendientes también pueden realizarse sobre el paquete E8.
- [ ] Corregir cada fallo y repetir su caso y las pruebas afectadas.
- [ ] Contrastar el alcance demostrado con los requisitos vigentes del proyecto y el profesor; anotar cualquier diferencia antes de afirmar cierre de requisitos.

No se marca un caso como aprobado solo porque tenga una prueba automática. Los fallos descubiertos se registran en la misma guía; su solución y evidencia pasan a la bitácora.

Según lo indicado por el usuario tras consultar al profesor (2026-11-11), informar un error a la vez satisface ese aspecto de la entrega. La detección entre fases y la recuperación más amplia no son un requisito adicional de cierre; permanecen como mejora futura. Esto no da por aprobados los demás requisitos del proyecto.

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

### Hallazgo pendiente de asistencia: R12

- [x] Replantear asistencia: sugerencias/cierres locales en gris con Tab; simplificar la activación de corrección automática y verificar el motor real. H03 conserva pendiente su repetición humana.
- [ ] Repetir R12 y la nueva vista gris con el usuario antes de marcar su cierre. Mantener el acceso manual a reparaciones y no ampliar reglas sin validación.

## Mejoras futuras, separadas del cierre

- Completar instrucciones o cuerpos de función mediante propuestas justificadas por C. La vista gris con Tab ya está implementada para terminales y cierres locales; no genera código arbitrario.
- Esquina morada que se dobla y revela controles: clic/teclado, movimiento reducido y conservación de mover/redimensionar/cerrar.
- Tema claro, preferencias adicionales y más reglas de reparación con positivos/negativos.
- Agrupar visualmente pares de clones equivalentes manteniendo los pares de archivos reales. Tres archivos pueden producir tres pares legítimos.
- Métricas/IR/optimizaciones cuando exista contrato de datos; recursos del compilador cuando estén implementados.
- Terminal con emulación completa, redimensionamiento PTY y otras plataformas.

### Diagnósticos de varias fases en una misma revisión

- [ ] Ampliar la recuperación de **ccom en C** para informar errores léxicos, sintácticos y semánticos de zonas independientes en una misma revisión. CALSIDE presenta los resultados mediante el contrato JSON, sin duplicar analizadores en Python.
- [ ] Permitir que el parser continúe después de tokens léxicos inválidos, conservando su ubicación y evitando mensajes sintácticos derivados del mismo fallo.
- [ ] Identificar nodos incompletos o inválidos del AST recuperado y analizar semánticamente solo las partes fiables. No basta con retirar los bloqueos actuales del IDE.
- [ ] Evitar diagnósticos en cascada o duplicados; mantener límites de errores, tiempo y recuperación. Un AST con errores sigue sin autorizar generación de código ni ejecución.
- [ ] Validar con un programa que combine un símbolo inválido, una asignación sin expresión y un identificador no declarado en instrucciones independientes. Mostrar los errores recuperables con su fase y ubicación, sin prometer detectar todos los errores de cualquier programa dañado.

Implementación gradual: recuperación léxica/sintáctica, semántica parcial segura y adaptación del contrato/interfaz si hace falta. Es una mejora futura; el flujo vigente sigue bloqueando las fases posteriores cuando encuentra errores en una fase previa.

## Documentación vigente

[Uso](../README.md) · [Sistema](guia_sistema_calside.md) · [Asistencia](asistencia.md) · [Servicios C](../../docs/ccom_servicios_ide.md) · [Revisión manual](revision_manual.md) · [Bitácora](bitacora_ide.md) · [LL(1) de consulta](../nucleo/README.md).
