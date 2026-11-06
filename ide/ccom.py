"""Resolución de ccom y lectura conservadora de sus diagnósticos humanos."""
import os
import re
import shutil
from pathlib import Path
from ide.diagnosticos import Problema  # Mantiene ccom.Problema para consumidores existentes.


def ejecutable(root, configured=""):
    candidates = [configured, str(Path(root) / "ccom"), shutil.which("ccom")]
    for value in candidates:
        if value and Path(value).is_file() and os.access(value, os.X_OK):
            return str(Path(value).resolve())
    return None


def diagnosticos(text, source, cwd):
    """Extrae diagnósticos ubicados o generales; las demás líneas siguen en Salida."""
    result = []
    lines = text.splitlines()
    for index, line in enumerate(lines):
        match = re.match(r"^(.+?):(\d+):(\d+):\s*(.+)$", line)
        general = False
        if match:
            filename, row, column, message = match.groups()
        else:
            match = re.match(
                r"^(\S.*?):\s*(error(?: (?:lexico|léxico|sintactico|sintáctico|semantico|semántico))?:\s*.+)$",
                line, re.IGNORECASE)
            if not match:
                continue
            filename, message = match.groups()
            row = column = None
            general = True
        path = Path(source) if filename == "ccom" else Path(filename)
        if not path.is_absolute():
            path = Path(cwd) / path
        length = 0 if general else 1
        # ccom imprime primero el fuente y después su señalador. Solo usar
        # longitudes de ese formato; no deducir rangos a partir del mensaje.
        if not general and index + 2 < len(lines) and re.match(r"^\s*\d+\s*\|", lines[index + 1]):
            marker = re.match(r"^\s*\|[ \t]*(\^+)\s*$", lines[index + 2])
            if marker:
                length = len(marker.group(1))
        result.append(Problema(path.resolve(), None if general else max(1, int(row)),
                               None if general else max(1, int(column)), message, length))
    return result
