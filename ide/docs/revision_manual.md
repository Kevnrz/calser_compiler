# Revisión manual de CALSIDE

Estado: **pendiente de ejecución humana**. Esta guía es el registro activo de la revisión; completar cada caso al probarlo en la aplicación. No confundir las 384 pruebas automatizadas previas con resultados manuales aprobados.

## Preparación

Desde la raíz del repositorio:

```sh
make
calside_revision_dir="$(mktemp -d /tmp/calside-revision.XXXXXX)"
cp -R ide/ejemplos/revision_manual/. "$calside_revision_dir/"
printf 'Carpeta de pruebas: %s\n' "$calside_revision_dir"
make ide
```

Si usas el entorno virtual, el último comando es `make ide PYTHON=.venv/bin/python`. Si la aplicación ya estaba abierta antes de compilar, ciérrala y vuelve a abrirla. En **Archivo → Abrir carpeta** elige la carpeta temporal indicada. Cambia únicamente esas copias, no los ejemplos versionados. Para empezar otra ronda crea una copia nueva; no borres automáticamente carpetas anteriores que contengan observaciones.

| Dato | Valor de esta ronda |
| --- | --- |
| Fecha | Pendiente |
| Rama/commit | Pendiente |
| Sistema y arquitectura | Pendiente |
| Pantalla/resolución/escalado | Pendiente |
| Ruta de carpeta temporal | Pendiente |
| Ejecutable ccom utilizado | Pendiente |

Marca cada caso **Pendiente / OK / Fallo / No aplica**. Un fallo debe describir pasos, esperado y observado; no marcar OK después de un arreglo sin repetir el caso. Los problemas de legibilidad/distribución también se anotan.

## Programas adicionales: arreglos, matrices y recorridos completos

El conjunto [recorridos y resultados esperados](../ejemplos/revision_manual/recorridos/README.md) añade **12 programas válidos e independientes**, numerados del 01 al 12: operaciones, funciones, condiciones, ciclos, recursión, arreglos, búsqueda, suma/resta de matrices, transposición, producto rectangular y booleanos con corto circuito. La guía incluye variaciones para probar sugerencias, reparaciones y diagnósticos sobre copias.

Se verificaron con `ccom --check`, compilación y ejecución en el simulador: **12/12**, comparando las variables finales con los valores documentados. Esto valida los ejemplos; su revisión interactiva humana queda **pendiente**. No altera los estados R01–R24. Empieza por `recorridos/01_operaciones.cls` y avanza en orden; las operaciones matriciales son los casos 09–11.

## Bloque A: archivos y edición — empezar aquí

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R01 | Abrir la carpeta temporal y `programas/suma.cls`. Abrir una segunda pestaña y alternar. | Jerarquía, iconos `.cls`, colores, números de línea y contenido independiente; sin errores en suma. | OK — confirmado por el usuario (2026-11-11) |
| R02 | Crear un documento, escribir `# prueba manual`, guardar como `programas/nota.cls`, editar sin guardar y cerrar. Elegir Cancelar; repetir con Descartar. | Cancelar conserva pestaña/texto; Descartar conserva la última versión guardada. El indicador de cambios coincide. | OK — confirmado por el usuario (2026-11-11) |
| R03 | Crear carpeta `prueba archivos`; copiar/cortar/pegar, renombrar y mover copias con pestañas abiertas. Probar Ctrl/Shift y arrastre. Eliminar una copia desde el clic derecho del árbol, editor o pestaña; pulsar Ctrl+Z inmediatamente, también después de hacer clic en el editor. Escribir después de eliminar y comprobar que Ctrl+Z deshace esa escritura. Repetir con carpeta y selección múltiple. | Se restauran las rutas y los bytes guardados, manteniendo cambios sin guardar e historial del editor. Un destino ocupado bloquea la restauración sin sobrescribir. Las rutas se actualizan sin perder texto ni Deshacer. No sobrescribe destinos sin el flujo previsto. Soltar entre hermanos solo reordena. | OK — confirmado por el usuario (2026-11-11) |
| R04 | Buscar un archivo, una carpeta y texto de una copia modificada sin guardar. Seleccionar resultados. | Resultados correctos, buffer reciente, navegación correcta; al cambiar consulta no reaparecen resultados antiguos. | OK — confirmado por el usuario (2026-11-11) |

En R02 se puede usar `# prueba manual`: un documento con solo comentarios válidos debe quedar sin problemas automáticos y con estado «Sin código para analizar». Un comentario sin cerrar sí debe señalar error léxico. Al pulsar F5, un comentario válido muestra «El archivo solo contiene comentarios; no hay código para compilar» en Salida, sin subrayado de error ni artefactos. Un archivo vacío recibe el aviso equivalente; un comentario sin cerrar conserva sus errores. Guardar no debe añadir rutas ni cambiar el texto; comprobar también Cancelar/Descartar.

No continuar automáticamente al siguiente bloque si aparece pérdida de datos o selección/ruta incorrecta. Registrar el fallo y conservar la copia temporal para reproducirlo.

## Bloque B: diagnósticos y recuperación

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R05 | Abrir `errores/dos_sintacticos.cls`; esperar análisis sin F5. | **2 errores sintácticos**, líneas 3 y 4; cada uno permite navegar al `;` inesperado. | OK — confirmado por el usuario (2026-11-11) |
| R06 | Completar `x =` con `1` y después `y =` con `2`, conservando declaraciones. Alternar con la pestaña suma. | Primero queda un problema y luego ninguno. Las pestañas no mezclan diagnósticos; el disco no cambia hasta guardar. | OK — confirmado por el usuario (2026-11-11) |
| R07 | En una copia válida añadir `@`; borrarlo. Añadir un comentario `#[` sin cierre y después cerrarlo con `]#`. | Errores léxicos reales; al corregir desaparecen. El comentario abierto no produce una cascada de errores sintácticos. | OK — confirmado por el usuario (2026-11-11) |
| R08 | Abrir `errores/semantico.cls`. Sustituir `desconocida` por `2`. | Error semántico por nombre no declarado; al corregir desaparece. Con sintaxis que quinválida no se anuncian errores semánticos de un AST recuperado. | OK — confirmado por el usuario (2026-11-11) |

**Límite conocido:** no se prometen todos los errores de un programa arbitrariamente dañado. Separadores faltantes pueden hacer que se salte parte del código hasta una ancla fiable. La recuperación puede detenerse; no es un defecto por sí solo si respeta lo documentado en [servicios C](../../docs/ccom_servicios_ide.md#recovery).

<a id="asistencia"></a>

## Bloque C: sugerencias y reparaciones

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R09 | En un documento nuevo escribir `F main [] : vo`; Ctrl+Espacio o clic derecho → Sugerencias. Aceptar `void`, Deshacer y repetir. | Reemplaza `vo` sin duplicar el sufijo; un Deshacer revierte la propuesta. No inventa `main`, nombres ni cuerpos de función. | OK — confirmado por el usuario (2026-11-11) |
| R10 | Abrir `errores/reparacion.cls`; Alt+Enter o clic derecho → Reparaciones. Revisar cambios y aplicar. | Propone `;` tras `3`, muestra motivo/precondiciones. Aplicar modifica el buffer; Ctrl+Z revierte; no guarda. | OK — confirmado por el usuario (2026-11-11) |
| R11 | Solicitar una reparación y luego editar/mover cursor/cambiar pestaña antes de aplicar. Solicitar sugerencias dentro de un comentario. | No aplica una respuesta antigua ni otra pestaña. El comentario bloquea sugerencias; cero opciones se explica sin inventarlas. | OK — confirmado por el usuario (2026-11-11) |
| R12 | Activar «Corregir al confirmar una línea con Enter»; la regla de `;` se selecciona inicialmente. En la copia de reparación, Enter justo después de `3`; esperar. Probar Ctrl+Z y luego deshabilitar la función. | Solo con permisos/regla y copia completa aceptable puede aplicar `;`. Informa el cambio, permite Deshacer y no entra en un ciclo de reparación. Sin permisos no edita solo. | Ajustado y comprobado con ccom real — pendiente repetición humana tras H03 |

Las condiciones precisas están en [asistencia](asistencia.md#politica). Un contexto incompleto, ambiguo o cambiado puede dejar una propuesta manual en lugar de corregir automáticamente.

### Repetición añadida: propuestas en gris

Con asistencia habilitada, escribir `F main [] : vo` al final de una línea y esperar: debe verse `id` gris sin cambiar el texto. Tab completa `void`; `F main` propone `[]: ` con cursor fuera y después `void`; otras funciones proponen `[]` con cursor dentro y segundo Tab para salir a `:`. Tras `:` se ofrecen tipos; aceptar `void` consulta después `;` y luego debe detenerse, sin proponer `.` o `F`; tras escribir `E;` no debe mostrar un punto pegado; al pulsar Enter sí debe proponer `.` en la línea siguiente y detenerse después del cierre; una línea vacía del cuerpo no sugiere `C` arbitrariamente. Ctrl+Z revierte; Esc descarta. Enter no acepta la vista. Probar también un `;` faltante al final de una declaración, comentarios, cambio de pestaña y Ctrl+Espacio para abrir la lista. Con análisis automático activado, esperar a que aparezca un diagnóstico: la propuesta gris debe seguir visible y Tab debe aceptarla si texto/cursor no cambiaron. La pausa por defecto es 350 ms; las preferencias anteriores pueden conservar otra espera. **Pendiente de revisión humana**, sin revocar los OK anteriores de la lista manual.

## Bloque D: compilar y ejecutar

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R13 | Abrir `programas/suma.cls`. Perfil Compilar y F5; después F6. | F5 guarda/compila sin ejecutar; F6 ejecuta y muestra **resultado = 5**, traza y estado de finalización. | Pendiente |
| R14 | Cambiar un literal sin guardar e intentar F6. | No ejecuta silenciosamente un artefacto viejo; solicita recompilar. Guardar/compilar permite ejecutar el resultado nuevo. | Pendiente |
| R15 | Seleccionar modo Analizar y F5 en una copia sin artefactos previos. | Solo diagnóstico; no genera ejecutable ni habilita F6 como nueva compilación completa. | Pendiente |
| R16 | Exportar AST JSON a una ruta con espacios; repetir con código sintácticamente inválido y un destino previamente existente. | Éxito sintáctico exporta JSON. Un error bloquea exportación y no trunca el destino existente. AST no certifica semántica. | Pendiente |
| R17 | Compilar con listado `.asm` y probar el modo Ensamblar sobre ese listado. Revisar destino/opciones desde la interfaz. | Los perfiles aceptan solo opciones compatibles; ensamblar genera artefactos desde `.asm`, sin asistencia Calser sobre esa entrada. | Pendiente |

Las salidas van bajo `build/ide` en la carpeta de la fuente, salvo destino explícito. Para un bucle largo/no terminante en una copia: probar Cancelar y comprobar que el editor sigue respondiendo; no confundir fin por límite con ejecución correcta.

## Bloque E: clones

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R18 | **Abrir como proyecto únicamente la subcarpeta `clones`**. Clones → Analizar carpeta, mínimo 20. | **3 pares:** base↔idéntico Tipo I; cada uno↔renombrado Tipo II. Tres pares entre tres archivos no son resultados duplicados. | Pendiente |
| R19 | Seleccionar pares, comparar y abrir fragmentos. Editar una copia sin guardar y volver a analizar; mover separadores y panel abajo/derecha. | Rangos correctos, buffers recientes, resultados anteriores invalidados y comparación legible en tu pantalla. No se normalizan operadores ni se afirma equivalencia semántica. | Pendiente |

No analizar la carpeta completa para esperar exactamente tres pares: suma y otras copias pueden añadir coincidencias legítimas. Tras este bloque volver a abrir la carpeta temporal raíz.

## Bloque F: paneles, terminales y persistencia

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R20 | Ctrl+B, × y Archivos; mover resultados abajo/derecha; ajustar la comparación de clones. | Controles sincronizados, pestañas/contenido conservados y distribuciones utilizables. | Pendiente |
| R21 | Abrir dos terminales; ejecutar `pwd` y `printf 'hola\n'`; seleccionar/copiar con Ctrl+C o Ctrl+Shift+C. En una ejecutar `sleep 30` e interrumpir sin selección. | Sesiones separadas, salida copiable; Ctrl+C interrumpe el comando cuando no hay selección. Sin exigir emulación de pantalla completa. | Pendiente |
| R22 | Cambiar tamaño de fuente y distribución; cerrar/reabrir normalmente. | Preferencias conservadas y proyecto accesible; no se prometen compilaciones F6 confiables recuperadas de otra sesión. | Pendiente |

## Bloque G: cambios externos y cierre seguro

| Caso | Acción | Resultado esperado | Estado |
| --- | --- | --- | --- |
| R23 | En una copia abierta, editar o borrar el archivo desde otro editor/gestor. Repetir con cambios locales sin guardar. | Aviso no destructivo para recargar/conservar/guardar copia. No pierde el buffer ni sobrescribe otra versión sin resolver el conflicto. | Pendiente |
| R24 | Cerrar con documento modificado; con una terminal ocupada; durante una operación cancelable. | Confirmaciones coherentes, Cancelar conserva trabajo, procesos/trabajadores terminan o el cierre espera según el caso. No hay bloqueo ni cierre silencioso con pérdida. | Pendiente |

## Registro de hallazgos

| ID | Caso | Pasos mínimos | Esperado / observado | Estado de arreglo y repetición |
| --- | --- | --- | --- | --- |
| H01 | R24 | La aplicación se cerró de golpe durante la revisión inicial. | Cierre inesperado; causa aún sin identificar. | Pendiente: reproducir y capturar salida de terminal. |
| H02 | R13 / R20 | Pasar el mouse y pulsar Compilar; comparar con Ejecutar. | Compilar no mostraba respuesta visual. | Corregido en estilos; 18 pruebas y capturas verificadas. Pendiente repetición humana. |
| H03 | R12 | Activar reparación automática y regla de `;`; pulsar Enter después de `3` en la copia de reparación. | El usuario informa que no funciona y considera rígida/poco útil la interacción. | Vista gris con Tab y activación más sencilla implementadas; comprobación con motor real correcta. Pendiente repetir R12 y validar usabilidad en la pantalla del usuario. |

Una captura ayuda para fallos visuales, pero registrar también archivo/código y pasos para reproducirlos. No poner datos personales en el corpus compartido.

### Ajuste visual durante la ronda

Flecha de Compilar con bloque de color irregular: estilo del subcontrol corregido y desplegable verificado. Cabeceras Archivo/Editar/Ver/Compilar con resaltado redondeado como Archivos. Pendiente comprobar ambos cambios en la pantalla del usuario, dentro de R13/R20.

### Regresión de R02: comentario y URL de archivo

El usuario informa que al guardar apareció una URL en el comentario. Se comprobó que el archivo guardado contenía solo `# prueba manual`: la URL estaba en el buffer después del guardado. Qt podía insertar datos MIME de archivos como texto; el editor ahora los rechaza sin alterar el portapapeles. Copiar una ruta explícitamente como texto plano sigue permitido. El análisis automático omite la sintaxis cuando Flex solo devuelve comentarios válidos; no se ocultan errores léxicos ni se cambia F5. Seis regresiones automatizadas correctas; **R02 pendiente de repetición humana**.

## Cierre de esta ronda

- [ ] Casos aplicables ejecutados; fallos registrados.
- [ ] Fallos de entrega corregidos y casos repetidos.
- [ ] Cambios funcionales verificados con pruebas pertinentes.
- [ ] Matriz de requisitos revisada con el enunciado/profesor.
- [ ] Se puede avanzar a empaquetado E8; la prueba en otra máquina todavía no se da por hecha.

[Plan vigente](plan_ide.md) · [Guía del sistema](guia_sistema_calside.md) · [Bitácora](bitacora_ide.md).
