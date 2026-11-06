"""Iconos compartidos con la identidad gráfica del plugin de VS Code."""
from functools import lru_cache
from pathlib import Path
from PySide6.QtGui import QIcon


@lru_cache(maxsize=1)
def icono_cls():
    # Es la misma imagen que editors/vscode/icons/calser.png; una única fuente
    # de branding evita depender del tema de archivos instalado en el escritorio.
    return QIcon(str(Path(__file__).resolve().parents[2] / 'assets/branding/calser-file-icon.png'))


def icono_archivo(path):
    return icono_cls() if path is not None and Path(path).suffix == '.cls' else QIcon()
