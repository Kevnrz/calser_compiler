"""Puente entre los resultados del lexer y la presentación del editor.

TareaLexica entrega una copia del texto a analizar_documento. Este módulo llama
al lexer real del núcleo y devuelve colores y errores asociados a documento y
revisión; el editor comprobará esa identidad antes de presentarlos.

No importa Qt ni modifica widgets. Prepara tramos UTF-16 que el resaltador puede
usar directamente, mientras los errores conservan sus columnas Unicode para
que Editor.rango_problema los adapte al navegar o subrayar.

Los comentarios se recorren aquí únicamente para colorearlos: el lexer los
omite como tokens. Este recorrido visual no sustituye su análisis de errores.
"""
from array import array
from dataclasses import dataclass

from ide.diagnosticos import Problema
from ide.nucleo.lexico import analizar


@dataclass(frozen=True)
class Tramo:
    """Fragmento de color dentro de una línea, con inicio base 0 y largo UTF-16.

    La categoría es un nombre de estilo, como numero o comentario; el color
    concreto lo decide Resaltado en la interfaz.
    """
    inicio: int
    longitud: int
    categoria: str


@dataclass(frozen=True)
class ResultadoLexico:
    """Resultado de una instantánea, listo para ser recibido por el editor.

    tramos asocia número de línea base 0 con sus fragmentos de color. problemas
    conserva los diagnósticos; fatal indica si alguno impidió continuar el lexer.
    documento/revision permiten descartar resultados antiguos. La dataclass
    impide reasignar campos, aunque su diccionario interno sigue siendo mutable.
    """
    documento: str
    revision: int
    tramos: dict
    problemas: tuple
    fatal: bool


def comentarios(texto):
    """Genera rangos Python [inicio, fin) de comentarios para el resaltado.

    # abre comentario de línea; #[ abre uno de bloque que termina en ]#. Un
    bloque sin cierre se colorea hasta el final del texto; el error lo informa
    el lexer, no este generador. El salto final de un comentario de línea queda
    fuera del rango. Los rangos llegan ordenados y no se solapan.
    """
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
    """Devuelve colores y problemas del texto capturado, sin leer archivos.

        documento y revision se copian al resultado; aquí no se consulta el estado
        actual de la pestaña. El límite de texto protege el análisis en vivo y lanza
        ValueError cuando se supera. La compilación manual es un recorrido distinto.

    """
    if len(texto) > 2_000_000:
        raise ValueError("El documento supera 2 millones de caracteres; el análisis en vivo está pausado. Puedes compilarlo con F5.")
    tokens, errors = analizar(texto)
    lines = texto.split("\n")
    # Para cada línea, maps[i][j] traduce una frontera de caracteres Python a
    # unidades UTF-16. Por ejemplo: "a😀b" produce [0, 1, 3, 4]. El emoji ocupa
    # un carácter Python y dos unidades Qt; reutilizar mapas evita confundirlos.
    maps = []
    for line in lines:
        offsets = array("I", [0])
        for char in line:
            offsets.append(offsets[-1] + (2 if ord(char) > 0xFFFF else 1))
        maps.append(offsets)
    spans = {}

    def append(line, start, end, category):
        """Agrega un tramo convirtiendo fronteras Python base 0 a UTF-16."""
        positions = maps[line]
        start, end = min(start, len(positions) - 1), min(end, len(positions) - 1)
        if end > start:
            spans.setdefault(line, []).append(Tramo(positions[start], positions[end] - positions[start], category))

    # El lexer informa líneas/columnas desde 1; el resaltador identifica líneas
    # desde 0. La clasificación visual no añade significado semántico al token.
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

    # Un comentario de bloque puede atravesar varios bloques del editor. Se
    # divide en tramos por línea; line y line_start avanzan sin volver al inicio
    # por cada comentario, manteniendo lineal este recorrido de presentación.
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
    # No hay que inventar ruta para una pestaña sin guardar: documento la
    # identifica. Los rangos del error siguen en Unicode, a diferencia del color.
    problems = tuple(Problema(None, e.linea, e.columna, "error lexico: " + e.mensaje, e.largo, "núcleo", "unicode", documento) for e in errors)
    return ResultadoLexico(documento, revision, spans, problems, any(e.fatal for e in errors))
