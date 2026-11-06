"""Sugerencias predictivas de Calser. Python puro, sin efectos secundarios.

Las sugerencias son continuaciones de escritura, NO reparaciones ni garantías
de corrección semántica. Los rangos usan offsets Python [inicio, fin).
"""
from dataclasses import dataclass
from typing import Callable

from .gramatica import FIN
from .lexico import analizar as analizar_lexico
from .parser import ContextoPredictivo, contexto_en


PALABRAS = frozenset({
    "int", "bool", "void", "verdadero", "falso",
    "autenticar", "cerrar_sesion", "cargar_llave", "sellar",
    "cambiar_ancla", "cifrar", "descifrar",
    "Vm", "Vi", "F", "C", "A", "L", "S", "E", "R",
})
OPERADORES = frozenset({
    "==", "!=", "<=", ">=", "<", ">", "+", "-", "*", "/", "%",
    "^", "!", "&", "|", "=",
})
DELIMITADORES = frozenset({"[", "]", "(", ")", ":", ";", ",", "."})
NO_CONCRETOS = frozenset({"id", "num", FIN})


@dataclass(frozen=True)
class Sugerencia:
    codigo: str
    etiqueta: str
    texto: str
    inicio: int
    fin: int
    categoria: str
    explicacion: str
    automatica_segura: bool = False


@dataclass(frozen=True)
class ResultadoSugerencias:
    estado: str
    cursor: int
    inicio_fragmento: int
    fin_fragmento: int
    esperados: tuple[str, ...]
    sugerencias: tuple[Sugerencia, ...]
    contexto: ContextoPredictivo
    limite_alcanzado: bool = False
    motivo: str = ""


def _lineas(texto: str) -> list[int]:
    return [0] + [i + 1 for i, c in enumerate(texto) if c == "\n"]


def _fragmento(texto: str, cursor: int) -> tuple[int, int]:
    """Encuentra el token que contiene el cursor, sin reimplementar el lexer."""
    tokens, _ = analizar_lexico(texto)
    lineas = _lineas(texto)
    for token in tokens:
        inicio = lineas[token.linea - 1] + token.columna - 1
        fin = inicio + len(token.lexema)
        if inicio < cursor <= fin and token.tipo in PALABRAS | {"id", "num"}:
            return inicio, fin
        if inicio <= cursor < fin and token.tipo in OPERADORES:
            return inicio, fin
    return cursor, cursor


def sugerir(texto: str, cursor: int, *, max_resultados: int = 24,
            cancelar: Callable[[], bool] | None = None) -> ResultadoSugerencias:
    """Sugiere terminales concretos válidos en el prefijo del cursor.

    No inventa nombres ni literales. Un resultado no modifica el documento.
    El IDE debe comprobar documento/revisión/texto antes de aplicar.
    """
    if not isinstance(texto, str):
        raise TypeError("texto debe ser str")
    if type(cursor) is not int or not 0 <= cursor <= len(texto):
        raise ValueError("cursor fuera de rango")
    if type(max_resultados) is not int or max_resultados < 1:
        raise ValueError("max_resultados debe ser entero positivo")
    inicio, fin = _fragmento(texto, cursor)
    # Un fragmento completo solo se usa si es válido para contexto_en.
    fragmento = texto[inicio:cursor]
    if fragmento and not (
        (fragmento[0].isalpha() and fragmento[0].isascii() and
         all(c.isascii() and (c.isalnum() or c == "_") for c in fragmento))
        or (fragmento[0] == "_" and all(c.isascii() and
            (c.isalnum() or c == "_") for c in fragmento))
        or (fragmento.isascii() and fragmento.isdecimal())
        or (all(c in "=!<>" for c in fragmento) and len(fragmento) <= 2)
    ):
        inicio = fin = cursor
        fragmento = ""
    ctx = contexto_en(texto, cursor, inicio_fragmento=inicio if inicio < cursor else None,
                      cancelar=cancelar)
    estado = ctx.estado
    if estado != "prefijo_valido":
        return ResultadoSugerencias(
            estado, cursor, inicio, fin, ctx.esperados, (), ctx,
            estado == "limite", ctx.resultado.motivo or
            ("Cursor dentro de comentario" if estado == "comentario" else
             "El prefijo contiene errores; no se proponen continuaciones"))
    opciones = []
    for terminal in ctx.esperados:
        if terminal in NO_CONCRETOS:
            continue
        if terminal not in PALABRAS | OPERADORES | DELIMITADORES:
            continue
        if fragmento and not terminal.startswith(fragmento):
            continue
        # Nunca reemplazar el sufijo de un identificador por puntuación.
        if inicio < cursor and fin > cursor and terminal in DELIMITADORES:
            continue
        categoria = ("palabra" if terminal in PALABRAS else
                     "operador" if terminal in OPERADORES else "delimitador")
        opciones.append(Sugerencia(
            "SUG_TERMINAL_" + terminal, terminal, terminal, inicio, fin,
            categoria, f"El parser LL(1) permite {terminal!r} en este contexto."))
    # Orden estable: palabras, delimitadores, operadores; luego alfabético.
    prioridad = {"palabra": 0, "delimitador": 1, "operador": 2}
    opciones.sort(key=lambda s: (prioridad[s.categoria], s.etiqueta))
    truncado = len(opciones) > max_resultados
    return ResultadoSugerencias(
        estado, cursor, inicio, fin, ctx.esperados,
        tuple(opciones[:max_resultados]), ctx, truncado,
        "max_resultados" if truncado else "")


def proponer_sugerencias(texto: str, cursor: int, **kwargs) -> ResultadoSugerencias:
    """Alias público para uso desde adaptadores del IDE."""
    return sugerir(texto, cursor, **kwargs)
