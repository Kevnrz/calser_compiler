"""Inicio del prototipo: python3 -m ide.main [archivo.cls]."""
import sys
from pathlib import Path

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication

from ide.interfaz.ventana import Ventana


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("CALSIDE")
    app.setOrganizationName("Calser")
    app.setStyle("Fusion")
    root = Path(__file__).resolve().parents[1]
    app.setWindowIcon(QIcon(str(root / "assets/branding/calser-file-icon.png")))
    window = Ventana(root)
    if len(sys.argv) > 1:
        window.cargar_carpeta(Path(sys.argv[1]).resolve().parent)
    for argument in sys.argv[1:]:
        window.abrir(Path(argument))
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
