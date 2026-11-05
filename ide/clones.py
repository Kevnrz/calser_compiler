"""Clones léxicos entre archivos, independientes de ccom y del parser LL(1).

Tipo I: tokens idénticos omitiendo comentarios/espacios.
Tipo II: misma estructura con renombramiento biyectivo de identificadores y
cambios de literales del mismo tipo. No se normalizan operadores ni reservadas.
Se indexan ventanas, se verifican y extienden; no se compara cada par de tokens.
"""
from dataclasses import dataclass
from pathlib import Path
import re


RESERVADAS = frozenset("F Vm Vi int bool void C A L S E R autenticar cerrar_sesion cargar_llave sellar cambiar_ancla cifrar descifrar".split())
PATRON = re.compile(r'(?P<comentario>\#\[[\s\S]*?(?:\]\#|\Z)|\#(?!\[)[^\n]*)|(?P<espacio>\s+)|(?P<numero>[0-9]+)|(?P<cadena>"(?:\\.|[^"\\])*")|(?P<nombre>[A-Za-z_][A-Za-z0-9_]*)|(?P<simbolo>==|!=|<=|>=|.)')


class Cancelado(Exception):
    pass


class LimiteAnalisis(ValueError):
    pass


@dataclass(frozen=True)
class Token:
    clase: str
    valor: str
    inicio: int
    fin: int
    linea: int


@dataclass(frozen=True)
class Fragmento:
    archivo: Path
    inicio: int
    fin: int
    linea_inicio: int
    linea_fin: int


@dataclass(frozen=True)
class ParClon:
    tipo: str
    primero: Fragmento
    segundo: Fragmento
    tokens: int


def tokenizar(texto, cancelar=lambda: False):
    result = []
    line = 1
    for index, match in enumerate(PATRON.finditer(texto)):
        if index % 1024 == 0 and cancelar():
            raise Cancelado()
        kind, value = match.lastgroup, match.group()
        if kind not in ("comentario", "espacio"):
            if kind == "nombre":
                kind = "booleano" if value in ("verdadero", "falso") else ("reservada" if value in RESERVADAS else "identificador")
            result.append(Token(kind, value, match.start(), match.end(), line))
        line += value.count("\n")
    return result


def firma(tokens, parametrizada):
    identifiers = {}
    result = []
    for token in tokens:
        if parametrizada and token.clase == "identificador":
            result.append(("id", identifiers.setdefault(token.valor, len(identifiers))))
        elif parametrizada and token.clase in ("numero", "booleano", "cadena"):
            result.append((token.clase, "literal"))
        else:
            result.append((token.clase, token.valor))
    return tuple(result)


def compatibles(a, b, parametrizada, directo, inverso):
    if a.clase != b.clase:
        return False
    if not parametrizada:
        return a.valor == b.valor
    if a.clase == "identificador":
        if a.valor in directo:
            return directo[a.valor] == b.valor
        if b.valor in inverso:
            return False
        directo[a.valor], inverso[b.valor] = b.valor, a.valor
        return True
    return a.clase in ("numero", "booleano", "cadena") or a.valor == b.valor


def detectar(fuentes, minimo=20, cancelar=lambda: False):
    """Devuelve pares máximos por tipo, con offsets de caracteres Unicode.

    Límites explícitos para proyectos educativos: 200 archivos, 100000 tokens,
    50000 verificaciones y 500 pares. Al excederlos no se presentan resultados
    parciales como completos. El umbral mide tokens, no líneas ni similitud.
    """
    if minimo < 5:
        raise ValueError("El mínimo debe ser de al menos 5 tokens.")
    if len(fuentes) > 200:
        raise LimiteAnalisis("Más de 200 archivos. Selecciona una carpeta más pequeña.")
    data = []
    total = 0
    for path, text in sorted(fuentes.items(), key=lambda item: str(item[0])):
        if cancelar():
            raise Cancelado()
        tokens = tokenizar(text, cancelar)
        total += len(tokens)
        if total > 100000:
            raise LimiteAnalisis("Más de 100000 tokens. Reduce la carpeta analizada.")
        data.append((Path(path).resolve(), tokens))
    matches = []
    attempts = 0
    for parametrizada in (False, True):
        index = {}
        covered = {}
        for file_id, (path, tokens) in enumerate(data):
            pending = []
            for start in range(len(tokens) - minimo + 1):
                if start % 128 == 0 and cancelar():
                    raise Cancelado()
                key = firma(tokens[start:start + minimo], parametrizada)
                for other_id, other_start in index.get(key, ()):
                    if file_id == other_id:
                        continue
                    attempts += 1
                    if attempts % 128 == 0 and cancelar():
                        raise Cancelado()
                    if attempts > 50000:
                        raise LimiteAnalisis("Demasiados fragmentos repetidos. Aumenta el mínimo de tokens o reduce la carpeta.")
                    diagonal = (other_id, file_id, start - other_start)
                    if any(left <= other_start and other_start + minimo <= right for left, right in covered.get(diagonal, ())):
                        continue
                    other = data[other_id][1]
                    direct, inverse = {}, {}
                    if not all(compatibles(other[other_start + k], tokens[start + k], parametrizada, direct, inverse) for k in range(minimo)):
                        continue
                    left_a, left_b = other_start, start
                    right_a, right_b = other_start + minimo, start + minimo
                    while right_a < len(other) and right_b < len(tokens) and compatibles(other[right_a], tokens[right_b], parametrizada, direct, inverse):
                        right_a += 1
                        right_b += 1
                        if right_a % 1024 == 0 and cancelar():
                            raise Cancelado()
                    while left_a > 0 and left_b > 0 and compatibles(other[left_a - 1], tokens[left_b - 1], parametrizada, direct, inverse):
                        left_a -= 1
                        left_b -= 1
                    covered.setdefault(diagonal, []).append((left_a, right_a))
                    exact = all(a.valor == b.valor for a, b in zip(other[left_a:right_a], tokens[left_b:right_b]))
                    if parametrizada and exact:
                        continue
                    def fragmento(file_path, sequence, a, b):
                        return Fragmento(file_path, sequence[a].inicio, sequence[b - 1].fin, sequence[a].linea, sequence[b - 1].linea)
                    matches.append(ParClon("II" if parametrizada else "I", fragmento(data[other_id][0], other, left_a, right_a), fragmento(path, tokens, left_b, right_b), right_a - left_a))
                    if len(matches) > 500:
                        raise LimiteAnalisis("Más de 500 pares. Aumenta el mínimo de tokens o reduce la carpeta.")
                pending.append((key, start))
            # Publicar solo al terminar el archivo: no comparar ventanas del
            # mismo documento, incluso si tiene miles de secuencias repetidas.
            for key, start in pending:
                index.setdefault(key, []).append((file_id, start))
    # Una extensión anterior puede haber encontrado un subconjunto antes que
    # otra ventana lo amplíe; eliminar solo inclusiones dentro del mismo tipo.
    result = []
    for pair in sorted(matches, key=lambda p: -p.tokens):
        if any(p.tipo == pair.tipo and p.primero.archivo == pair.primero.archivo and p.segundo.archivo == pair.segundo.archivo and p.primero.inicio <= pair.primero.inicio and pair.primero.fin <= p.primero.fin and p.segundo.inicio <= pair.segundo.inicio and pair.segundo.fin <= p.segundo.fin for p in result):
            continue
        result.append(pair)
    return result
