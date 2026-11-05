"""Diagnóstico común. La unidad de columna se conserva hasta llegar a Qt."""
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Problema:
    archivo: Path | None
    linea: int
    columna: int
    mensaje: str
    longitud: int = 1
    origen: str = "ccom"
    unidad: str = "utf8"
    documento: str | None = None

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
