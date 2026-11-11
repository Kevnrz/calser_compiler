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

## Uso principal

- Archivo: abrir carpeta, nuevo, guardar, guardar como y cerrar con protección de cambios.
- Explorador: jerarquía, menú contextual, selección múltiple, arrastre y búsqueda. Ctrl+B oculta/recupera el panel.
- Editor: colores y diagnósticos desde C, sin guardar; cada problema navega a su ubicación. El parser puede recuperar varios errores sintácticos, con límites conservadores.
- Ctrl+Espacio o clic derecho: sugerencias de continuaciones válidas. Alt+Enter o clic derecho: reparaciones, vista previa y aplicación con Deshacer.
- Editar → Preferencias de asistencia: pausa opcional y corrección automática. Esta última viene apagada; requiere seleccionar la regla revisada de `;`.
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

Reparaciones: cuatro cierres; automatización: una regla optativa de `;`. Sugerencias: terminales concretos, sin inventar nombres ni instrucciones completas. Clones: equivalencia léxica, no semántica. Recuperación sintáctica: puede detenerse o saltar fragmentos sin anclas fiables. Terminal: Linux, sin emulación de pantalla completa. Esquina morada estática; animaciones y predicción gris son mejoras futuras.
