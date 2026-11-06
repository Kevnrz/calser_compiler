"""Punto de entrada de CALSIDE: crea la aplicación y muestra su ventana.

Se inicia desde la raíz del repositorio con ``python3 -m ide.main``. También
acepta rutas de archivos después del nombre del módulo para abrirlos al iniciar.

Este archivo prepara Qt y entrega el control a ``Ventana``. La edición, los
análisis y las llamadas al compilador se coordinan en los componentes de esa
ventana; aquí no se compila ni se analiza el código del usuario.
"""
import sys
from pathlib import Path

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication

from ide.interfaz.ventana import Ventana


def main():
    """Inicializa la interfaz y devuelve el código de salida del bucle de Qt.

    QApplication debe existir antes de crear widgets. Su bucle de eventos
    mantiene la ventana disponible y distribuye clics, teclas, temporizadores
    y respuestas de los procesos/trabajadores a sus métodos correspondientes.
    """
    app = QApplication(sys.argv)
    # Identifican la aplicación; ProyectoVentana utiliza también Calser/CALSIDE
    # al crear QSettings para almacenar las preferencias del usuario.
    app.setApplicationName("CALSIDE")
    app.setOrganizationName("Calser")
    app.setStyle("Fusion")

    # Resolver desde este archivo permite encontrar herramientas y recursos
    # aunque el usuario inicie CALSIDE desde otro directorio de trabajo.
    root = Path(__file__).resolve().parents[1]
    app.setWindowIcon(QIcon(str(root / "assets/branding/calser-file-icon.png")))
    window = Ventana(root)

    # Con archivos de entrada, el primero determina la carpeta del explorador.
    # Después se abre cada ruta en una pestaña, incluso si pertenecen a carpetas
    # distintas. Ventana se encarga de la lectura y de informar sus errores.
    if len(sys.argv) > 1:
        window.cargar_carpeta(Path(sys.argv[1]).resolve().parent)
    for argument in sys.argv[1:]:
        window.abrir(Path(argument))
    window.show()

    # Esta llamada permanece activa hasta cerrar la aplicación; no es un bucle
    # que tengamos que escribir manualmente para actualizar los widgets.
    return app.exec()


if __name__ == "__main__":
    # Ejecutar el módulo arranca la aplicación. Importarlo desde otro módulo
    # solo define main; no abre una ventana. sys.exit propaga el código de Qt.
    sys.exit(main())
