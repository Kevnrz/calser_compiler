# E7: archivos externos, proyecto y preferencias

Implementado en `feat/calside`, 2026-11-05. Independiente de los motores de sugerencias y reparaciones. El parser se conserva sin cambios.

## Archivos que cambian fuera de CALSIDE

Cada documento abierto registra la versión leída: dispositivo/inodo, tamaño, fecha en nanosegundos y SHA-256 de los bytes. La lectura comprueba que el archivo no fue reemplazado durante la operación. La huella detecta cambios incluso si otro editor conserva la fecha y el tamaño.

Un trabajador comprueba las fuentes abiertas cada 1,5 segundos. También se solicita una comprobación al volver a la ventana. Solo hay un sondeo activo; el hash y el recorrido de archivos no se ejecutan en el hilo gráfico. Los resultados llevan documento, ruta y versión base; se ignoran después de guardar, recargar, Guardar como o cerrar la pestaña.

Un cambio, borrado o error de lectura produce un aviso para la pestaña activa y un indicador `⚠` en su título. **No se recarga ni se descarta el texto automáticamente.**

- **Recargar:** lee el disco. Si hay ediciones locales, pide confirmación antes de descartarlas. Un fallo de lectura/UTF-8 conserva íntegramente el editor. Atajo: **Ctrl+Shift+R**.
- **Conservar:** mantiene el buffer y lo marca como pendiente de guardar. Oculta ese aviso concreto, pero no adopta la versión ajena como base ni autoriza sobrescribirla. Otro cambio externo vuelve a avisar.
- **Guardar como:** permite guardar una copia sin reemplazar el archivo original. Un destino existente exige confirmación; un archivo abierto en otra pestaña no puede usarse como destino.

Al guardar, se comprueba el disco aunque el sondeo aún no haya detectado el cambio. Si difiere de la versión leída, se ofrece sobrescribir/recrear explícitamente, recargar, guardar una copia o cancelar. La opción por defecto y Escape cancelan. Si el archivo cambia otra vez durante la decisión o la preparación de la escritura, se cancela el guardado conservando las dos versiones.

Si una fuente abierta fue sustituida por un enlace simbólico, el guardado condicionado se rechaza y pide Guardar como, para no sobrescribir otro destino indirectamente.

El guardado prepara un temporal en el mismo directorio, vacía sus datos, conserva los permisos de un destino existente, comprueba de nuevo la versión y hace un reemplazo atómico. Ante fallo conserva el archivo anterior y elimina el temporal. La versión registrada al guardar corresponde a lo escrito por CALSIDE; un escritor inmediatamente posterior se detectará como cambio externo.

**Límite del sistema de archivos:** la comprobación y el reemplazo son operaciones distintas; no hay un bloqueo exclusivo o compare-and-swap portable frente a todos los programas externos. Existe una pequeña ventana entre la comprobación final y el reemplazo. No se promete exclusión mutua con otro editor que escriba simultáneamente.

F5 también comprueba que disco y editor correspondan a la misma versión antes de compilar, incluso si el documento no tiene cambios locales. Una compilación cuyo archivo cambió durante el proceso no publica diagnósticos como si fueran actuales. El análisis automático de E4 continúa trabajando sobre el buffer del editor.

## Explorador de proyecto

`Explorador` usa `QFileSystemModel` y un filtro: carga directorios mediante el modelo asíncrono de Qt y expande subcarpetas según se necesitan. No hace un recorrido recursivo completo desde Python. Muestra carpetas y archivos `.cls`; usa iconos nativos de archivos/carpetas.

Se excluyen `.git`, `.venv`, `venv`, `__pycache__`, `build`, `dist`, `node_modules`, `.pytest_cache`, `.mypy_cache`, `.ruff_cache`, `.idea` y `.tox`. No sigue ni muestra enlaces simbólicos. Una carpeta que el usuario abrió explícitamente puede servir como raíz aunque su nombre esté excluido; también se permite llegar a una raíz dentro de un ancestro excluido.

El modelo detecta archivos nuevos, borrados y cambios de directorio. Abrir o cambiar de proyecto no cierra las pestañas existentes. Los errores de acceso de la raíz se muestran sin cambiar el proyecto; un fallo de acceso al expandir una carpeta se informa en la barra de estado.

- **Ctrl+B:** mostrar/ocultar explorador.
- **Ctrl+Shift+E:** mostrarlo y darle foco.
- Flechas: navegar y expandir; **Enter** o doble clic sobre archivo: abrirlo.
- **Ctrl+Alt+J:** alternar resultados abajo/derecha.

## Preferencias

Se guardan al cerrar normalmente mediante `QSettings("Calser", "CALSIDE")`, bajo la configuración del usuario. En Linux normalmente corresponde a `~/.config/Calser/CALSIDE.conf`, respetando `XDG_CONFIG_HOME`. No se escriben dentro del repositorio ni hay que agregarlas a `.gitignore`.

Se recuerdan carpeta de proyecto, ruta de ccom, familia/tamaño de fuente, tamaño de ventana, visibilidad del explorador y resultados, orientación/tamaños de paneles y activación del análisis automático.

**Ver → Fuente del editor…** cambia la fuente de las pestañas actuales y futuras sin modificar su texto ni Deshacer. Se admiten tamaños de 6 a 40 puntos. El tema oscuro actual se mantiene; selector de temas y tiempo de compilación configurable siguen fuera de esta entrega.

El JSON de configuración tiene versión y validación de campos/rangos. Valores inválidos usan los predeterminados; una carpeta desaparecida vuelve a la raíz del proyecto y una ruta inválida de ccom vuelve a su búsqueda habitual. No se restauran buffers, pestañas de documentos, terminales ni procesos. Las pruebas usan configuración temporal o `Ventana(root, preferencias=False)` y no alteran las preferencias del usuario.

## Organización de código

- `ide/archivos.py`: estados de disco, lectura consistente y escritura condicionada/atómica.
- `ide/preferencias.py`: validación y persistencia de configuración.
- `ide/interfaz/disco.py`: trabajador y temporizador de sondeo.
- `ide/interfaz/explorador.py`: modelo de archivos y exclusiones.
- `ide/interfaz/proyecto.py`: avisos, decisiones, recarga, fuente y preferencias de la ventana.
- `ide/interfaz/ventana.py`: integración de esas operaciones con edición, procesos y cierre.

## Validación

```bash
make check-ide
```

Resultado: **124 pruebas combinadas correctas**, incluidas 28 nuevas de E7. Se cubren fechas/tamaños iguales, reemplazo/borrado, cambios durante guardado, recuperación tras errores de lectura, Guardar como, resultados tardíos, F5 con disco ajeno, cierre con sondeo activo, fuentes, exclusiones/enlaces, archivos nuevos, preferencias dañadas y atajos reales.

Revisión visual local: 1250×800 y mínimo 800×520, con escala normal y 150 %. Conserva los botones del aviso y la edición; los paneles se pueden redimensionar. La comprobación en otra máquina y el equipo de defensa permanece en E8/E9.

Prueba manual reproducible:

1. Abrir una carpeta con una subcarpeta y un `.cls`. Navegar hasta el archivo y abrirlo.
2. Editarlo en CALSIDE y cambiarlo desde otro editor. Esperar el aviso; comprobar que el buffer local no cambió.
3. Pulsar Conservar y luego Guardar: debe pedir resolver el conflicto. Cancelar debe conservar ambas versiones.
4. Guardar como copia; comprobar que el archivo modificado externamente conserva su contenido.
5. Volver a abrirlo, modificarlo externamente y Recargar. Con cambios locales, comprobar primero la cancelación de la confirmación.
6. Borrar un archivo abierto: conservar el buffer, guardar una copia o recrearlo solo con confirmación.
7. Cambiar fuente, mover paneles, ocultar explorador y cerrar/reabrir normalmente. Comprobar preferencias y navegación con teclado.
