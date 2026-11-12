# CALSIDE

IDE de escritorio de Calser con Python/PySide6 y ccom como backend C. M1–M7 están implementados y verificados localmente; empaquetado y validación final en otra máquina siguen pendientes.

## Abrir en desarrollo

Desde la raíz del repositorio, con Python 3.10 o posterior, GCC, Make, Flex y Bison:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r ide/requirements.txt
make
make ide PYTHON=.venv/bin/python
```

Si PySide6 ya está instalado:

```sh
make
make ide
```

También: `python3 -m ide.main ruta/al/programa.cls`. No depende de VS Code. La entrega empaquetada para abrir desde un icono es E8, todavía pendiente.

## Icono de la aplicación

CALSIDE usa `assets/branding/logo_programa.png` como icono de sus ventanas:
solo el hexágono exacto de la animación, con el segundo fragmento morado y
fondo hexagonal `#191c24` con vértices redondeados. El exterior del fondo es
transparente; su interior permanece opaco. El PNG mide 512×512 y su fuente
vectorial queda en `assets/branding/logo_programa.svg`.
En Wayland se identifica como `calside`; el escritorio resuelve el icono con
`calside.desktop`. En este equipo se registró una entrada de desarrollo en
`~/.local/share/applications/calside.desktop`, con `Icon` y `Path` apuntando a
este repositorio y `Exec` al Python utilizado. Permite abrir CALSIDE desde el
lanzador y mostrar su logo en el selector de ventanas. Si se traslada el
repositorio o cambia el intérprete, hay que actualizar esas rutas. Este registro
local no sustituye el empaquetado E8. Los iconos de archivos `.cls` conservan
su recurso propio, `calser-file-icon.png`.

## Animación de inicio

`python3 -m ide.main` (o `make ide`) muestra una secuencia de 4,4 s:
se dibujan los seis fragmentos del hexágono geométrico, que gira lentamente durante toda la secuencia y se desplaza
a la izquierda, y se trazan las letras de CALSIDE. El segundo fragmento (desde
arriba, en sentido horario) e IDE son morados; la D es el reflejo de la C. Clic o Esc omiten el tiempo visual
restante; si la ventana aún se está preparando, aparece «Preparando el editor…».

En **Ver → Animación al iniciar CALSIDE** se puede desmarcar la opción para los
próximos arranques y volver a activarla. Se guarda inmediatamente en
`QSettings('Calser', 'CALSIDE')`, clave JSON `estado_v1`, campo booleano
`animacion_inicio` (predeterminado `true`). La desactivación elimina toda espera
visual. Los archivos pasados por consola se siguen abriendo normalmente.

Para comprobarla: iniciar, observar el recorrido completo, repetir pulsando Esc
y con un clic, desmarcar la opción y reiniciar. Probar también
`python3 -m ide.main "ruta con espacios/programa.cls"` y comprobar su pestaña.
El diseño aprobado es `assets/branding/calside-inicio.svg`, completamente
vectorial. La variante CALSER queda conservada en
`assets/branding/calser-marca-geometrica.svg`. El reposo final coincide con el
SVG de CALSIDE. El giro mantiene unos 43°/s durante los 4,4 s (unos 189°), desde
el primer trazo hasta el final. Ritmo: 1,2 s de hexágono, 0,9 s de giro/desplazamiento, 2 s de
letras y 0,3 s finales de giro; la inicialización comienza durante la presentación.
Las pruebas focalizadas son:

```sh
QT_QPA_PLATFORM=offscreen python3 -m unittest ide.pruebas.test_inicio ide.pruebas.test_proyecto
```

## Uso principal

- Archivo: abrir carpeta, nuevo, guardar, guardar como y cerrar con protección de cambios.
- Explorador: jerarquía, menú contextual, selección múltiple, arrastre y búsqueda. Ctrl+B oculta/recupera el panel.
- Editor: colores y diagnósticos desde C, sin guardar; cada problema navega a su ubicación. El parser puede recuperar varios errores sintácticos, con límites conservadores.
- Al escribir al final de una línea aparece una continuación en gris tras una pausa: Tab acepta, Esc descarta y Alt+[ / Alt+] cambia la propuesta. `vo` puede mostrar `id` para completar `void`; `F main` propone `[]: ` de una vez y deja el cursor fuera, seguido de `void` y `;`. Las funciones normales proponen `[]` con cursor dentro; otro Tab o flecha derecha sale y consulta `:`. Después de `:` se ofrecen tipos; `main` prioriza `void`, como exige ccom. Tras aceptar una propuesta se consulta la siguiente continuación sin exigir un espacio. Tras el `;` de la cabecera no se propone cerrar con `.` ni iniciar otra función con `F`. Después de un `E;` explícito, el punto se propone únicamente al pasar a una línea posterior con Enter; no se muestra pegado a `E;`. Una línea vacía no elige una instrucción arbitraria; Ctrl+Espacio permite consultar sus alternativas. La vista no modifica el texto ni lo guarda. Si ccom propone un cierre faltante en el cursor, también puede aceptarse con Tab.
- Ctrl+Espacio o clic derecho: lista de continuaciones válidas. Alt+Enter o clic derecho: reparaciones, vista previa y aplicación con Deshacer.
- Editar → Preferencias de asistencia: vista gris activada por defecto, pausa configurable (350 ms iniciales) y corrección automática con Enter. Esta última viene apagada: al activarla se selecciona la regla revisada de `;`, sin un segundo paso obligatorio. Una preferencia de espera ya guardada se conserva.
- Compilar ▾ / F5: modo análisis, compilación, AST o ensamblado. Opciones de compilación permite destino/banderas compatibles. F5 requiere guardar la fuente; no ejecuta.
- Ejecutar / F6: última compilación completa vigente del documento; muestra traza, variables y registros mediante el simulador. Si se editan fuente/artefactos, pide recompilar.
- Clones → Analizar carpeta: pares Tipo I/II con copias sin guardar, comparación y navegación. El umbral inicial es 20 tokens.
- Resultados: panel abajo/derecha, terminales nuevas con +; terminal PTY para Linux con copia, historial e interrupción.

Los artefactos de F5 se guardan en `<carpeta del fuente>/build/ide/<identificador de ruta>/`. El ejecutable se busca en Compilar → Configurar ccom, raíz del repositorio y PATH. Sin un ccom compatible se informa el fallo; no hay respaldo al LL(1) Python.

## Documentación

| Necesidad | Documento |
| --- | --- |
| Qué funciona y qué falta | [Plan vigente](docs/plan_ide.md) |
| Cómo se conecta todo y dónde leer código | [Guía del sistema](docs/guia_sistema_calside.md) |
| Modelos, aplicación y permisos de asistencia | [Asistencia](docs/asistencia.md) |
| Protocolos JSON, CCL1 y recuperación C | [Servicios de ccom](../docs/ccom_servicios_ide.md) |
| Probar el programa a mano | [Revisión manual](docs/revision_manual.md) |
| Evolución, decisiones y documentos sustituidos | [Bitácora](docs/bitacora_ide.md) |
| Estudiar el parser Python conservado | [LL(1) de consulta](nucleo/README.md) |

## Verificación

```sh
make check
make check-ide
make check-recorrido-ide
```

La última ejecución registrada incluye 400 pruebas IDE, suite del compilador y verificaciones de memoria; ver la bitácora. El recorrido gráfico automatizado no equivale a la revisión manual en tu pantalla.

## Límites de entrega

Reparaciones: cuatro cierres; automatización: una regla optativa de `;`. Sugerencias: terminales concretos, sin inventar nombres ni instrucciones completas. Clones: equivalencia léxica, no semántica. Recuperación sintáctica: puede detenerse o saltar fragmentos sin anclas fiables. Terminal: Linux, sin emulación de pantalla completa. Esquina morada estática; las instrucciones completas sugeridas siguen pendientes; la presentación de inicio está implementada y su revisión visual humana queda pendiente. La vista gris presenta propuestas de ccom al final de la línea; para reemplazos dentro de una línea se conserva la lista manual.
