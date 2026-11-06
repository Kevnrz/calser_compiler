"""Diagnóstico común. La unidad de columna se conserva hasta llegar a Qt."""
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Problema:
    archivo: Path | None
    linea: int | None
    columna: int | None
    mensaje: str
    longitud: int = 1
    origen: str = "ccom"
    unidad: str = "utf8"
    documento: str | None = None

    @property
    def tiene_posicion(self):
        return (self.linea is not None and self.columna is not None
                and self.linea > 0 and self.columna > 0)

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
