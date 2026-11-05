"""
tabla.py — PRIMERO, SIGUIENTE y tabla de análisis predictivo M.

Implementa los algoritmos de Aho et al. §4.4.2 (cálculo de PRIMERO y
SIGUIENTE por punto fijo) y el Algoritmo 4.31 (construcción de M) de
§4.4.3. Sirve para cualquier Gramatica, no solo para Calser: las pruebas
lo comparan con los ejemplos del libro (Ej. 4.30 y Fig. 4.17).

Ejemplo con la gramática de Aho Ej. 4.28 (E -> T E', E' -> + T E' | ε, ...):
  PRIMERO(E) = { (, id }      SIGUIENTE(E') = { ), $ }
  M[E', +] = E' -> + T E'     M[E', )] = M[E', $] = E' -> ε
"""

from dataclasses import dataclass, field
from typing import Dict, FrozenSet, Iterable, List, Set, Tuple

from .gramatica import EPSILON, FIN, Gramatica, Produccion


def calcular_primero(g: Gramatica) -> Dict[str, FrozenSet[str]]:
    """
    PRIMERO(X) de cada no terminal (Aho §4.4.2, reglas 1-3). Incluye ε si
    X ⇒* ε. Se repite hasta que ningún conjunto crece (punto fijo).
    """
    primero: Dict[str, Set[str]] = {a: set() for a in g.no_terminales}
    cambio = True
    while cambio:
        cambio = False
        for p in g.producciones:
            nuevo = _primero_cadena(g, primero, p.cuerpo)
            if not nuevo <= primero[p.cabeza]:
                primero[p.cabeza] |= nuevo
                cambio = True
    return {a: frozenset(s) for a, s in primero.items()}


def _primero_cadena(g: Gramatica, primero, simbolos: Iterable[str]) -> Set[str]:
    """PRIMERO(X1 X2 ... Xn): se avanza mientras Xi sea anulable."""
    resultado: Set[str] = set()
    for x in simbolos:
        conjunto = primero[x] if g.es_no_terminal(x) else {x}
        resultado |= conjunto - {EPSILON}
        if EPSILON not in conjunto:
            return resultado
    resultado.add(EPSILON)            # todos anulables (o cadena vacía)
    return resultado


def primero_cadena(g: Gramatica, primero, simbolos: Iterable[str]) -> FrozenSet[str]:
    """Versión pública de PRIMERO para una cadena de símbolos."""
    return frozenset(_primero_cadena(g, primero, simbolos))


def calcular_siguiente(g: Gramatica, primero) -> Dict[str, FrozenSet[str]]:
    """
    SIGUIENTE(A) de cada no terminal (Aho §4.4.2, reglas 1-3):
      1. $ ∈ SIGUIENTE(inicial).
      2. A -> α B β: PRIMERO(β) - {ε} ⊆ SIGUIENTE(B).
      3. A -> α B, o A -> α B β con β ⇒* ε: SIGUIENTE(A) ⊆ SIGUIENTE(B).
    """
    siguiente: Dict[str, Set[str]] = {a: set() for a in g.no_terminales}
    siguiente[g.inicial].add(FIN)
    cambio = True
    while cambio:
        cambio = False
        for p in g.producciones:
            for i, b in enumerate(p.cuerpo):
                if not g.es_no_terminal(b):
                    continue
                resto = _primero_cadena(g, primero, p.cuerpo[i + 1:])
                nuevo = resto - {EPSILON}
                if EPSILON in resto:
                    nuevo |= siguiente[p.cabeza]
                if not nuevo <= siguiente[b]:
                    siguiente[b] |= nuevo
                    cambio = True
    return {a: frozenset(s) for a, s in siguiente.items()}


@dataclass
class TablaLL1:
    """
    Tabla M[A, a] -> producción, más los datos con que se construyó.
    conflictos: (A, a, producción ya puesta, producción nueva) por cada
    celda que recibiría dos producciones. Vacía <=> la gramática es LL(1).
    """
    gramatica: Gramatica
    primero: Dict[str, FrozenSet[str]]
    siguiente: Dict[str, FrozenSet[str]]
    m: Dict[Tuple[str, str], Produccion] = field(default_factory=dict)
    conflictos: List[Tuple[str, str, Produccion, Produccion]] = field(
        default_factory=list)

    @property
    def es_ll1(self) -> bool:
        return not self.conflictos

    def entrada(self, no_terminal: str, terminal: str):
        """M[A, a] o None si la celda está vacía (error)."""
        return self.m.get((no_terminal, terminal))

    def fila(self, no_terminal: str) -> Dict[str, Produccion]:
        """Fila M[A, ·]: terminales con entrada y su producción."""
        return {a: p for (x, a), p in self.m.items() if x == no_terminal}

    def anulables(self) -> FrozenSet[str]:
        """No terminales A con A ⇒* ε."""
        return frozenset(a for a, s in self.primero.items() if EPSILON in s)


def construir_tabla(g: Gramatica) -> TablaLL1:
    """
    Algoritmo 4.31 (Aho §4.4.3). Para cada producción A -> α:
      1. para cada terminal a en PRIMERO(α), M[A, a] = A -> α;
      2. si ε ∈ PRIMERO(α), para cada b en SIGUIENTE(A) (incluido $),
         M[A, b] = A -> α.
    Si una celda recibe dos producciones distintas, se registra el conflicto
    y se conserva la primera.
    """
    primero = calcular_primero(g)
    siguiente = calcular_siguiente(g, primero)
    tabla = TablaLL1(g, primero, siguiente)
    for p in g.producciones:
        f = _primero_cadena(g, primero, p.cuerpo)
        destino = f - {EPSILON}
        if EPSILON in f:
            destino |= siguiente[p.cabeza]
        for a in sorted(destino):
            previa = tabla.m.get((p.cabeza, a))
            if previa is None:
                tabla.m[(p.cabeza, a)] = p
            elif previa != p:
                tabla.conflictos.append((p.cabeza, a, previa, p))
    return tabla
