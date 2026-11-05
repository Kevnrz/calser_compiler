"""Operaciones de archivos independientes de la interfaz gráfica."""
import os
import tempfile
from pathlib import Path


def leer(path):
    return Path(path).read_text(encoding="utf-8-sig")


def guardar(path, text):
    """Reemplazo atómico en el mismo directorio; no trunca ante un fallo."""
    path = Path(path).resolve()
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(text.replace("\r\n", "\n").replace("\r", "\n"))
        if path.exists():
            os.chmod(temporary, path.stat().st_mode & 0o777)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
