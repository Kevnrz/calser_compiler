"""Texto → tokens/errores reales del núcleo → rangos UTF-16 de presentación.

No importa Qt ni implementa otro lexer/parser. Los comentarios se recorren
solo para colorearlos, porque analizar() no los incluye en sus tokens.
"""
from array import array
from dataclasses import dataclass

from ide.diagnosticos import Problema
from ide.nucleo.lexico import analizar


@dataclass(frozen=True)
class Tramo:
    inicio: int
    longitud: int
    categoria: str


@dataclass(frozen=True)
class ResultadoLexico:
    documento: str
    revision: int
    tramos: dict
    problemas: tuple
    fatal: bool


def comentarios(texto):
    """Rangos Unicode [inicio, fin), con las mismas fronteras #/#[ que Calser."""
    position = 0
    while position < len(texto):
        start = texto.find("#", position)
        if start < 0:
            return
        if texto.startswith("#[", start):
            close = texto.find("]#", start + 2)
            end = len(texto) if close < 0 else close + 2
        else:
            close = texto.find("\n", start)
            end = len(texto) if close < 0 else close
        yield start, end
        position = end


def analizar_documento(documento, revision, texto):
    if len(texto) > 2_000_000:
        raise ValueError("El documento supera 2 millones de caracteres; el análisis en vivo está pausado. Puedes compilarlo con F5.")
    tokens, errors = analizar(texto)
    lines = texto.split("\n")
    maps = []
    for line in lines:
        offsets = array("I", [0])
        for char in line:
            offsets.append(offsets[-1] + (2 if ord(char) > 0xFFFF else 1))
        maps.append(offsets)
    spans = {}

    def append(line, start, end, category):
        positions = maps[line]
        start, end = min(start, len(positions) - 1), min(end, len(positions) - 1)
        if end > start:
            spans.setdefault(line, []).append(Tramo(positions[start], positions[end] - positions[start], category))

    for token in tokens:
        category = "reservada"
        if token.tipo == "id":
            category = "identificador"
        elif token.tipo == "num":
            category = "numero"
        elif token.tipo in ("int", "bool", "void"):
            category = "tipo"
        elif token.tipo in ("verdadero", "falso"):
            category = "booleano"
        elif not token.tipo[0].isalpha():
            category = "operador"
        append(token.linea - 1, token.columna - 1, token.columna_fin, category)

    # Recorrido lineal para comentarios multilínea, incluidas líneas vacías.
    line, line_start = 0, 0
    for start, end in comentarios(texto):
        while line_start + len(lines[line]) < start:
            line_start += len(lines[line]) + 1
            line += 1
        while line_start < end:
            line_end = line_start + len(lines[line])
            append(line, max(start, line_start) - line_start, min(end, line_end) - line_start, "comentario")
            if end <= line_end:
                break
            line_start = line_end + 1
            line += 1
    problems = tuple(Problema(None, e.linea, e.columna, "error lexico: " + e.mensaje, e.largo, "núcleo", "unicode", documento) for e in errors)
    return ResultadoLexico(documento, revision, spans, problems, any(e.fatal for e in errors))
