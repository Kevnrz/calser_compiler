"""Parser LL(1) sin Qt: reconocimiento completo y estado de un prefijo.

Estado M6: API de consulta/estudio y pruebas del LL(1), conservada por decisión
explícita. La interfaz productiva usa Flex/Bison en ccom, no este módulo.

La cima de todas las pilas públicas está a la DERECHA. Los rangos son offsets
Python [inicio, fin). No se recupera ni edita el texto después de un error.
"""
from bisect import bisect_right
from dataclasses import dataclass, replace
import re
from typing import Callable

from .gramatica import CALSER, EPSILON, FIN, Gramatica, Produccion
from .lexico import ErrorLexico, analizar as analizar_lexico
from .tabla import construir_tabla, primero_cadena


@dataclass(frozen=True)
class SimboloEntrada:
    tipo: str
    lexema: str
    inicio: int
    fin: int
    linea: int
    columna: int


@dataclass(frozen=True)
class DiagnosticoSintactico:
    codigo: str
    mensaje: str
    encontrado: SimboloEntrada
    esperados: tuple[str, ...]
    pila: tuple[str, ...]

    @property
    def inicio(self):
        return self.encontrado.inicio

    @property
    def fin(self):
        return self.encontrado.fin

    @property
    def linea(self):
        return self.encontrado.linea

    @property
    def columna(self):
        return self.encontrado.columna


@dataclass(frozen=True)
class PasoParser:
    numero: int
    accion: str
    pila: tuple[str, ...]  # antes de ejecutar la acción
    indice_token: int
    encontrado: SimboloEntrada
    produccion: Produccion | None = None


@dataclass(frozen=True)
class ResultadoSintactico:
    estado: str
    pila: tuple[str, ...]
    esperados: tuple[str, ...]
    diagnosticos: tuple[DiagnosticoSintactico, ...]
    errores_lexicos: tuple[ErrorLexico, ...]
    tokens_consumidos: int
    offset_alcanzado: int
    pasos: int
    traza: tuple[PasoParser, ...]
    traza_truncada: bool
    motivo: str = ""

    @property
    def aceptado(self) -> bool:
        return self.estado == "aceptado"


@dataclass(frozen=True)
class ContextoPredictivo:
    cursor: int
    punto: int  # fin del prefijo entregado al parser, antes del fragmento
    inicio_fragmento: int
    prefijo: str
    tipo_comentario: str | None
    resultado: ResultadoSintactico

    @property
    def estado(self):
        return self.resultado.estado

    @property
    def pila(self):
        return self.resultado.pila

    @property
    def esperados(self):
        return self.resultado.esperados

    @property
    def diagnosticos(self):
        return self.resultado.diagnosticos

    @property
    def errores_lexicos(self):
        return self.resultado.errores_lexicos

    @property
    def valido(self):
        return self.estado == "prefijo_valido"

    @property
    def puede_terminar(self):
        return self.valido and FIN in self.esperados


class ParserPredictivo:
    """Configuración reutilizable; cada llamada tiene su propia pila.

    La gramática es prestada y no debe modificarse después de construir el
    parser. Tabla privada calculada una vez; no se modifica durante consultas.
    Los límites son por llamada; la cancelación se consulta entre pasos.
    """
    def __init__(self, gramatica: Gramatica = CALSER, *, max_caracteres=2_000_000,
                 max_pasos=1_000_000, max_pila=4096, max_traza=500):
        for nombre, valor in (("max_caracteres", max_caracteres),
                              ("max_pasos", max_pasos), ("max_pila", max_pila),
                              ("max_traza", max_traza)):
            if type(valor) is not int or valor < 1:
                raise ValueError(f"{nombre} debe ser un entero positivo")
        if max_pila < 2:
            raise ValueError("max_pila debe permitir símbolo inicial y fin (mínimo 2)")
        self._tabla = construir_tabla(gramatica)
        if not self._tabla.es_ll1:
            raise ValueError("La gramática tiene conflictos LL(1)")
        self._gramatica = gramatica
        self.max_caracteres, self.max_pasos = max_caracteres, max_pasos
        self.max_pila, self.max_traza = max_pila, max_traza

    def _esperados(self, pila):
        # FIRST de TODA la continuación: una fila usa FOLLOW global, no el
        # contexto concreto. El FIN al fondo elimina epsilon del resultado.
        return tuple(sorted(primero_cadena(self._gramatica, self._tabla.primero,
                                          reversed(pila)) - {EPSILON}))

    def _inicial(self, estado, motivo):
        pila = (FIN, self._gramatica.inicial)
        return ResultadoSintactico(estado, pila, self._esperados(pila), (), (),
                                   0, 0, 0, (), False, motivo)

    def _preparar(self, texto, cancelar):
        if not isinstance(texto, str):
            raise TypeError("texto debe ser str")
        if len(texto) > self.max_caracteres:
            return self._inicial("limite", "max_caracteres")
        if cancelar is not None and cancelar():
            return self._inicial("cancelado", "cancelacion solicitada")
        return None

    def analizar_sintaxis(self, texto: str, *, traza=False,
                          cancelar: Callable[[], bool] | None = None):
        previo = self._preparar(texto, cancelar)
        return previo if previo is not None else self._ejecutar(texto, False, traza, cancelar)

    def contexto_en(self, texto: str, cursor: int, inicio_fragmento: int | None = None,
                    *, traza=False, cancelar: Callable[[], bool] | None = None):
        """Analiza texto[:punto], sin exigir EOF. No adivina palabras parciales.

        Sin inicio_fragmento se consumen todos los tokens de texto[:cursor].
        Para completar `vo|`, pasar el comienzo de `vo`. El rango a reemplazar
        (incluido cualquier sufijo a la derecha) lo decide sugerencias.
        """
        previo = self._preparar(texto, cancelar)
        if type(cursor) is not int or not 0 <= cursor <= len(texto):
            raise ValueError("cursor fuera de rango")
        punto = cursor if inicio_fragmento is None else inicio_fragmento
        if type(punto) is not int or not 0 <= punto <= cursor:
            raise ValueError("inicio_fragmento fuera de rango")
        if previo is not None:
            return ContextoPredictivo(cursor, punto, punto, texto[punto:cursor], None, previo)
        comentario = _comentario_en(texto[:cursor])
        if comentario is not None:
            # Un comentario abierto en el cursor es contexto de escritura, no
            # un EOF sintáctico ni un error fatal artificial del lexer.
            inicio, tipo = comentario
            resultado = self._ejecutar(texto[:inicio], True, traza, cancelar)
            if resultado.estado == "prefijo_valido":
                resultado = _cambiar_estado(resultado, "comentario", esperados=())
            return ContextoPredictivo(cursor, inicio, inicio, "", tipo, resultado)
        if punto < cursor:
            fragmento = texto[punto:cursor]
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*|[0-9]+|[=!<>]{1,2}", fragmento):
                raise ValueError("El fragmento debe ser una palabra, número u operador parcial")
            # Impedir que el llamador corte por la mitad un token anterior.
            tokens, _ = analizar_lexico(texto[:cursor])
            lineas = _inicios_linea(texto[:cursor])
            for token in tokens:
                inicio = lineas[token.linea - 1] + token.columna - 1
                if inicio < punto < inicio + len(token.lexema):
                    raise ValueError("inicio_fragmento corta un token")
        resultado = self._ejecutar(texto[:punto], True, traza, cancelar)
        return ContextoPredictivo(cursor, punto, punto, texto[punto:cursor], None, resultado)

    def _ejecutar(self, texto, prefijo, guardar_traza, cancelar):
        tokens, errores = analizar_lexico(texto)
        lineas = _inicios_linea(texto)
        entrada = [SimboloEntrada(t.tipo, t.lexema,
                    lineas[t.linea - 1] + t.columna - 1,
                    lineas[t.linea - 1] + t.columna - 1 + len(t.lexema),
                    t.linea, t.columna) for t in tokens]
        linea = bisect_right(lineas, len(texto))
        eof = SimboloEntrada(FIN, "", len(texto), len(texto), linea,
                             len(texto) - lineas[linea - 1] + 1)
        entrada.append(eof)
        primer_error = (lineas[errores[0].linea - 1] + errores[0].columna - 1
                        if errores else None)
        pila = [FIN, self._gramatica.inicial]
        indice = pasos = 0
        registro = []
        truncada = False
        alcanzado = 0

        def terminar(estado, motivo="", diagnosticos=(), esperados=None):
            return ResultadoSintactico(estado, tuple(pila),
                self._esperados(pila) if esperados is None else esperados,
                diagnosticos, tuple(errores), indice, alcanzado, pasos,
                tuple(registro), truncada, motivo)

        def registrar(accion, produccion=None):
            nonlocal truncada
            if guardar_traza:
                if len(registro) < self.max_traza:
                    registro.append(PasoParser(pasos + 1, accion, tuple(pila),
                                               indice, entrada[indice], produccion))
                else:
                    truncada = True

        while pila:
            if cancelar is not None and cancelar():
                return terminar("cancelado", "cancelacion solicitada")
            actual = entrada[indice]
            if primer_error is not None and primer_error <= actual.inicio:
                alcanzado = primer_error
                return terminar("error_lexico", "El lexer encontró un error antes de continuar")
            if prefijo and actual.tipo == FIN:
                alcanzado = len(texto)
                return terminar("prefijo_valido")
            if pasos >= self.max_pasos:
                return terminar("limite", "max_pasos")
            cima = pila[-1]
            produccion = (self._tabla.entrada(cima, actual.tipo)
                          if self._gramatica.es_no_terminal(cima) else None)
            if cima == actual.tipo:
                registrar("aceptar" if cima == FIN else "consumir")
                pila.pop()
                pasos += 1
                alcanzado = actual.fin
                if cima == FIN:
                    return terminar("aceptado", esperados=())
                indice += 1
            elif produccion is not None:
                if len(pila) - 1 + len(produccion.cuerpo) > self.max_pila:
                    return terminar("limite", "max_pila")
                registrar("expandir", produccion)
                pila.pop()
                pila.extend(reversed(produccion.cuerpo))
                pasos += 1
            else:
                esperados = self._esperados(pila)
                encontrado = "fin de archivo" if actual.tipo == FIN else repr(actual.lexema)
                opciones = ", ".join("fin de archivo" if x == FIN else repr(x) for x in esperados)
                diagnostico = DiagnosticoSintactico(
                    "SINTAXIS_EOF" if actual.tipo == FIN else "SINTAXIS_TOKEN",
                    f"Sintáctico: se encontró {encontrado}; se esperaba {opciones or 'una continuación válida'}",
                    actual, esperados, tuple(pila))
                registrar("error")
                pasos += 1
                alcanzado = actual.inicio
                return terminar("error_sintactico", diagnosticos=(diagnostico,))
        raise RuntimeError("Pila agotada sin aceptar el marcador de fin")


def _inicios_linea(texto):
    return [0] + [i + 1 for i, c in enumerate(texto) if c == "\n"]


def _comentario_en(prefijo):
    """Contexto al final del prefijo según # y #[…]# (sin anidamiento)."""
    i = 0
    while True:
        inicio = prefijo.find("#", i)
        if inicio < 0:
            return None
        if prefijo.startswith("#[", inicio):
            fin = prefijo.find("]#", inicio + 2)
            if fin < 0:
                return inicio, "bloque"
            i = fin + 2
        else:
            fin = prefijo.find("\n", inicio)
            if fin < 0:
                return inicio, "linea"
            i = fin + 1


def _cambiar_estado(resultado, estado, **campos):
    return replace(resultado, estado=estado, **campos)


# Fachada de Calser. Construcción una sola vez; cada ejecución usa datos locales.
_CALSER = ParserPredictivo()


def analizar_sintaxis(texto: str, *, traza=False, cancelar=None) -> ResultadoSintactico:
    return _CALSER.analizar_sintaxis(texto, traza=traza, cancelar=cancelar)


def contexto_en(texto: str, cursor: int, inicio_fragmento: int | None = None,
                *, traza=False, cancelar=None) -> ContextoPredictivo:
    return _CALSER.contexto_en(texto, cursor, inicio_fragmento,
                             traza=traza, cancelar=cancelar)
