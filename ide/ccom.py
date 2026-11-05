"""Resolución de ccom y lectura conservadora de sus diagnósticos humanos."""
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Problema:
    archivo: Path
    linea: int
    columna: int
    mensaje: str
    longitud: int = 1

    @property
    def categoria(self):
        category = self.mensaje.partition(":")[0].lower()
        if "lexico" in category or "léxico" in category:
            return "Léxico"
        if "sintactico" in category or "sintáctico" in category:
            return "Sintáctico"
        if "semantico" in category or "semántico" in category:
            return "Semántico"
        return "Compilación"

    @property
    def descripcion(self):
        prefix, separator, rest = self.mensaje.partition(":")
        return rest.strip() if separator and prefix.lower().startswith("error") else self.mensaje


def ejecutable(root, configured=""):
    candidates = [configured, str(Path(root) / "ccom"), shutil.which("ccom")]
    for value in candidates:
        if value and Path(value).is_file() and os.access(value, os.X_OK):
            return str(Path(value).resolve())
    return None


def diagnosticos(text, source, cwd):
    """Los mensajes sin posición quedan íntegros en Salida, sin inventar rangos."""
    result = []
    lines = text.splitlines()
    for index, line in enumerate(lines):
        match = re.match(r"^(.+?):(\d+):(\d+):\s*(.+)$", line)
        if not match:
            continue
        filename, row, column, message = match.groups()
        path = Path(source) if filename == "ccom" else Path(filename)
        if not path.is_absolute():
            path = Path(cwd) / path
        length = 1
        # ccom imprime primero el fuente y después su señalador. Solo usar
        # longitudes de ese formato; no deducir rangos a partir del mensaje.
        if index + 2 < len(lines) and re.match(r"^\s*\d+\s*\|", lines[index + 1]):
            marker = re.match(r"^\s*\|[ \t]*(\^+)\s*$", lines[index + 2])
            if marker:
                length = len(marker.group(1))
        result.append(Problema(path.resolve(), max(1, int(row)), max(1, int(column)), message, length))
    return result
