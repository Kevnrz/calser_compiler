"""

Estado M6: API de consulta/estudio y pruebas del LL(1), conservada por decisión
explícita. La interfaz productiva usa Flex/Bison en ccom, no este módulo.

gramatica.py — gramática LL(1) de Calser como datos.

Fuente: ide/docs/gramatica_ll1.md §3, obtenida de grammar/parser.y quitando
la recursión por la izquierda (Aho §4.3.3) y factorizando (Aho §4.3.4).
Los no terminales nuevos llevan "_p" donde el documento usa apóstrofo
(disy_p = disy').

Formato (Aho §4.2.1-4.2.2): cada línea es "cabeza -> alt1 | alt2 | ...";
los símbolos van separados por espacios; ε es el cuerpo vacío. Un símbolo
es no terminal si es cabeza de alguna línea; si no, es terminal. El
terminal | se escribe entre comillas ('|') para no confundirlo con el
separador de alternativas.
"""

from dataclasses import dataclass
from typing import Dict, List, Tuple

FIN = "$"          # marcador de fin de entrada (Aho §4.4.2)
EPSILON = "ε"


@dataclass(frozen=True)
class Produccion:
    """cabeza -> cuerpo; numero = posición en la gramática (desde 1)."""
    numero: int
    cabeza: str
    cuerpo: Tuple[str, ...]

    def __str__(self) -> str:
        return "%s -> %s" % (self.cabeza, " ".join(self.cuerpo) or EPSILON)


class Gramatica:
    """Terminales, no terminales, símbolo inicial y producciones (§4.2.1)."""

    def __init__(self, texto: str):
        self.producciones: List[Produccion] = []
        self.no_terminales: List[str] = []        # en orden de aparición
        cuerpos: List[Tuple[str, List[str]]] = []

        for linea in texto.strip().splitlines():
            linea = linea.strip()
            if not linea or linea.startswith("//"):
                continue
            cabeza, _, resto = linea.partition("->")
            cabeza = cabeza.strip()
            if not cabeza or not resto:
                raise ValueError("línea mal formada: %r" % linea)
            if cabeza in self.no_terminales:
                raise ValueError("cabeza repetida: %s" % cabeza)
            self.no_terminales.append(cabeza)
            for alternativa in resto.split(" | "):
                simbolos = [s.strip("'") if s == "'|'" else s
                            for s in alternativa.split()]
                if simbolos == [EPSILON]:
                    simbolos = []
                cuerpos.append((cabeza, simbolos))

        for numero, (cabeza, simbolos) in enumerate(cuerpos, start=1):
            self.producciones.append(
                Produccion(numero, cabeza, tuple(simbolos)))

        self.inicial = self.no_terminales[0]
        conjunto_nt = set(self.no_terminales)
        self.terminales: List[str] = sorted({
            s for p in self.producciones for s in p.cuerpo
            if s not in conjunto_nt})
        self._por_cabeza: Dict[str, List[Produccion]] = {
            a: [p for p in self.producciones if p.cabeza == a]
            for a in self.no_terminales}

    def es_no_terminal(self, simbolo: str) -> bool:
        return simbolo in self._por_cabeza

    def alternativas(self, cabeza: str) -> List[Produccion]:
        return self._por_cabeza[cabeza]


# --------------------------------------------------------------------------
# Gramática LL(1) de Calser (gramatica_ll1.md §3), 60 no terminales,
# 118 producciones, 47 terminales. El orden sigue al documento.
# --------------------------------------------------------------------------
TEXTO_CALSER = """
// Programa y funciones
programa -> globales funciones
globales -> decl globales | ε
funciones -> funcion funciones_p
funciones_p -> funcion funciones_p | ε
funcion -> F id [ params ] : retorno ; cuerpo .
retorno -> tipo id | void
params -> lista_param | ε
lista_param -> param lista_param_p
lista_param_p -> , param lista_param_p | ε
param -> modif_opt tipo id dims_opt
modif -> Vm | Vi
modif_opt -> modif | ε
tipo -> int | bool
dims_opt -> dims | ε
dims -> [ num ] dims_p
dims_p -> [ num ] | ε

// Cuerpo y sentencias
cuerpo -> sentencia cuerpo | ε
sentencia -> decl | sent_id | condicional | ciclo_l | ciclo_s | emit | rewind | crypto
decl -> modif id : tipo dims_opt decl_p
decl_p -> ; | = inicial ;
inicial -> expr | lista_init
lista_init -> [ elems ]
elems -> elem elems_p
elems_p -> , elem elems_p | ε
elem -> expr | lista_init
sent_id -> id sent_id_p
sent_id_p -> = expr ; | [ args ] tras_args
tras_args -> = expr ; | [ expr ] = expr ; | ;

// Control: C/A (alts y residual unidos y factorizados), L, S
condicional -> C [ expr ] ; cuerpo resto_c .
resto_c -> A tras_a | ε
tras_a -> [ expr ] ; cuerpo resto_c | ; cuerpo
ciclo_l -> L [ expr ] ; cuerpo .
ciclo_s -> S [ id , expr , expr , paso ] ; cuerpo .
paso -> num | - num

// E, R y criptografía
emit -> E ;
rewind -> R id [ args ] ;
crypto -> autenticar [ expr ] ; | cerrar_sesion [ ] ; | cargar_llave [ num , num , expr ] ; | sellar [ ] ; | cambiar_ancla [ expr ] ; | cifrar [ expr , expr , expr , num ] ; | descifrar [ expr , expr , expr , num ] ;

// Argumentos
args -> lista_arg | ε
lista_arg -> expr lista_arg_p
lista_arg_p -> , expr lista_arg_p | ε

// Expresiones (un nivel por precedencia; E -> T E' de Aho Ej. 4.28)
expr -> disy
disy -> conj disy_p
disy_p -> '|' conj disy_p | ε
conj -> igualdad conj_p
conj_p -> & igualdad conj_p | ε
igualdad -> relacion igualdad_p
igualdad_p -> == relacion igualdad_p | != relacion igualdad_p | ε
relacion -> aditivo relacion_p
relacion_p -> < aditivo relacion_p | > aditivo relacion_p | <= aditivo relacion_p | >= aditivo relacion_p | ε
aditivo -> multiplicativo aditivo_p
aditivo_p -> + multiplicativo aditivo_p | - multiplicativo aditivo_p | ε
multiplicativo -> unario multiplicativo_p
multiplicativo_p -> * unario multiplicativo_p | / unario multiplicativo_p | % unario multiplicativo_p | ε
unario -> - unario | ! unario | potencia
potencia -> primario potencia_p
potencia_p -> ^ unario | ε
primario -> ( expr ) | acceso | num | verdadero | falso
acceso -> id acceso_p
acceso_p -> [ args ] indice | ε
indice -> [ expr ] | ε
"""

CALSER = Gramatica(TEXTO_CALSER)
