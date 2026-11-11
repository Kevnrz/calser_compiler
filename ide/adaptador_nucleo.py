"""Presentación de tokens Flex: bytes UTF-8 del servicio → tramos Qt UTF-16.

Conserva su nombre por compatibilidad de importaciones. No importa el lexer del
núcleo, no reconoce palabras ni recorre comentarios: su clasificación y rangos
vienen de ccom --tokens-json. La tarea de fondo analiza una copia sin guardar.
"""
from array import array
from bisect import bisect_right
from dataclasses import dataclass, replace

from ide import ccom
from ide.servicio_ccom import consultar


@dataclass(frozen=True)
class Tramo:
    """Color dentro de una línea: inicio base 0 y longitud en unidades UTF-16."""
    inicio: int
    longitud: int
    categoria: str


@dataclass(frozen=True)
class ResultadoLexico:
    """Instantánea identificada por documento/revisión; nunca modifica widgets."""
    documento: str
    revision: int
    tramos: dict
    problemas: tuple
    fatal: bool


def convertir_lexico(documento, revision, texto, salida, fuente, codigo):
    """Valida el informe completo antes de convertir posiciones para Qt.

    Mapas por línea evitan decodificar desde su inicio por cada token. Las
    posiciones interiores de un carácter multibyte no son fronteras válidas.
    Los comentarios multilínea se dividen únicamente para pintar bloques Qt.
    """
    tokens, errores, fatal = ccom.lexico_json(salida, fuente, texto, codigo)
    bases, mapas = [], []
    base = 0
    for linea in texto.split('\n'):
        bases.append(base)
        mapa = array('i', [0])
        unidades = 0
        for char in linea:
            largo = len(char.encode('utf-8'))
            mapa.extend([-1] * (largo - 1))
            unidades += 2 if ord(char) > 0xffff else 1
            mapa.append(unidades)
        mapas.append(mapa)
        base += len(mapa)  # bytes de la línea + el salto LF
    tramos = {}
    for token in tokens:
        inicio, fin = token['start'], token['end']
        fila = bisect_right(bases, inicio) - 1
        while fila < len(bases) and bases[fila] < fin:
            mapa = mapas[fila]
            a = max(inicio - bases[fila], 0)
            b = min(fin - bases[fila], len(mapa) - 1)
            if b > a:
                tramos.setdefault(fila, []).append(Tramo(mapa[a], mapa[b] - mapa[a], token['kind']))
            fila += 1
    problemas = tuple(replace(p, archivo=None, documento=documento, origen='ccom léxico') for p in errores)
    return ResultadoLexico(documento, revision, tramos, problemas, fatal)


def analizar_documento(documento, revision, texto, *, ejecutable=None, cancelar=lambda: False):
    """Consulta Flex en un subproceso cancelable; llamar desde una tarea de fondo.

    El proceso recibe un temporal UTF-8 privado, sin guardar el documento. Se
    limita a 2 millones de caracteres, 5 s y 16 MB de respuesta. No hay fallback
    al lexer Python cuando ccom falta o devuelve un protocolo incompatible.
    """
    if len(texto) > 2_000_000:
        raise ValueError('El documento supera 2 millones de caracteres; el análisis en vivo está pausado. Puedes compilarlo con F5.')
    datos, fuente, codigo = consultar(texto, ['--tokens-json'], ejecutable=ejecutable,
                                      cancelar=cancelar, plazo=5, max_salida=16_000_000)
    return convertir_lexico(documento, revision, texto, datos, fuente, codigo)
