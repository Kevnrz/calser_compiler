"""Propuestas conservadoras de reparación sintáctica para CALSER.

Este módulo es Python puro: no accede a archivos, Qt, ccom ni al documento
abierto. Todas las ediciones usan offsets Python [inicio, fin) del texto
original. La validación reanaliza una copia completa con el parser LL(1).

La primera versión solo propone inserciones de terminales de cierre (; . ] )
que el parser espera en el primer error. No elimina ni inventa código.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
from typing import Iterable

from .parser import DiagnosticoSintactico, ResultadoSintactico, analizar_sintaxis


@dataclass(frozen=True)
class EdicionTexto:
    """Edición sobre el texto original; inserción si inicio == fin."""
    inicio: int
    fin: int
    texto: str


@dataclass(frozen=True)
class ValidacionReparacion:
    """Resultado de volver a analizar el candidato completo."""
    estado: str
    error_resuelto: bool
    aceptado: bool
    diagnostico_resultante: DiagnosticoSintactico | None
    offset_alcanzado: int
    motivo: str = ""


@dataclass(frozen=True)
class Reparacion:
    codigo: str
    descripcion: str
    explicacion: str
    diagnostico_objetivo: DiagnosticoSintactico
    ediciones: tuple[EdicionTexto, ...]
    automatica_segura: bool
    razones: tuple[str, ...]
    precondiciones: tuple[str, ...]
    validacion: ValidacionReparacion
    huella_texto: str
    longitud_texto: int


@dataclass(frozen=True)
class ResultadoReparaciones:
    estado: str
    propuestas: tuple[Reparacion, ...]
    diagnostico: DiagnosticoSintactico | None
    limites: tuple[str, ...] = ()
    motivo: str = ""
    huella_texto: str = ""
    longitud_texto: int = 0


# Orden explícito y estable para resultados reproducibles.
_TERMINALES_REPARABLES = (";", ".", "]", ")")
_DESCRIPCIONES = {
    ";": ("Insertar punto y coma", "Falta el terminador ';' en esta posición."),
    ".": ("Insertar punto final", "La construcción de función/control requiere un '.' de cierre."),
    "]": ("Cerrar corchete", "La construcción abierta requiere un ']' de cierre."),
    ")": ("Cerrar paréntesis", "La expresión requiere un ')' de cierre."),
}


def _huella(texto: str) -> str:
    return hashlib.sha256(texto.encode("utf-8")).hexdigest()


def aplicar_ediciones(texto: str, ediciones: Iterable[EdicionTexto]) -> str:
    """Aplica ediciones no solapadas de offsets originales, derecha a izquierda.

    Dos inserciones en el mismo offset se rechazan: su orden semántico no puede
    inferirse. El llamador debe fusionarlas explícitamente en una sola edición.
    """
    if not isinstance(texto, str):
        raise TypeError("texto debe ser str")
    cambios = tuple(ediciones)
    for e in cambios:
        if not isinstance(e, EdicionTexto):
            raise TypeError("cada edición debe ser EdicionTexto")
        if type(e.inicio) is not int or type(e.fin) is not int:
            raise TypeError("los offsets deben ser enteros")
        if not 0 <= e.inicio <= e.fin <= len(texto):
            raise ValueError("rango de edición fuera del texto original")
        if not isinstance(e.texto, str):
            raise TypeError("el texto de reemplazo debe ser str")
    ordenadas = sorted(cambios, key=lambda e: (e.inicio, e.fin))
    for anterior, actual in zip(ordenadas, ordenadas[1:]):
        if actual.inicio < anterior.fin or (
            actual.inicio == anterior.inicio and
            (actual.inicio == actual.fin or anterior.inicio == anterior.fin)
        ):
            raise ValueError("las ediciones se solapan o insertan en el mismo offset")
    resultado = texto
    for e in sorted(cambios, key=lambda e: (e.inicio, e.fin), reverse=True):
        resultado = resultado[:e.inicio] + e.texto + resultado[e.fin:]
    return resultado


def _validar_candidato(texto: str, diagnostico: DiagnosticoSintactico,
                       edicion: EdicionTexto) -> ValidacionReparacion:
    candidato = aplicar_ediciones(texto, (edicion,))
    resultado = analizar_sintaxis(candidato)
    if resultado.estado in {"limite", "cancelado"}:
        return ValidacionReparacion(resultado.estado, False, False,
                                    resultado.diagnosticos[0] if resultado.diagnosticos else None,
                                    resultado.offset_alcanzado, resultado.motivo)
    if resultado.errores_lexicos:
        return ValidacionReparacion("error_lexico", False, False,
                                    resultado.diagnosticos[0] if resultado.diagnosticos else None,
                                    resultado.offset_alcanzado,
                                    "El candidato contiene errores léxicos.")

    siguiente = resultado.diagnosticos[0] if resultado.diagnosticos else None
    # La edición se inserta justo antes del símbolo encontrado. Para considerar
    # resuelto el error original, el parser debe consumir también ese símbolo.
    umbral = edicion.inicio + len(edicion.texto)
    # En EOF no hay un token original que consumir: basta con que el parser
    # haya consumido la inserción y haya llegado al EOF posterior.
    avanzo_mas_alla = (resultado.offset_alcanzado >= umbral if diagnostico.encontrado.tipo == "$"
                       else resultado.offset_alcanzado > umbral)
    resuelto = resultado.aceptado or avanzo_mas_alla
    return ValidacionReparacion(
        resultado.estado, resuelto, resultado.aceptado, siguiente,
        resultado.offset_alcanzado,
        "El candidato fue aceptado." if resultado.aceptado else
        ("El parser superó el símbolo que causaba el error original." if resuelto else
         "El parser no consiguió superar el punto del error original."),
    )


def _es_caso_auto(texto: str, resultado: ResultadoSintactico,
                  diagnostico: DiagnosticoSintactico,
                  reparacion: Reparacion, cantidad_validas: int) -> tuple[bool, tuple[str, ...]]:
    """Política deliberadamente estrecha para automatización."""
    motivos = []
    token = diagnostico.encontrado
    starters_no_ambiguos = {
        "F", "C", "A", "L", "S", "E", "R", "Vm", "Vi",
        "autenticar", "cerrar_sesion", "cargar_llave", "sellar",
        "cambiar_ancla", "cifrar", "descifrar",
    }
    if reparacion.codigo != "INSERCION_PUNTO_COMA":
        motivos.append("Solo la inserción de ';' tiene una política automática inicial.")
    if token.tipo == "$":
        motivos.append("El error está en EOF; el usuario podría seguir escribiendo.")
    if token.tipo not in starters_no_ambiguos:
        motivos.append("El token siguiente también podría pertenecer a una expresión o continuación.")
    if cantidad_validas != 1:
        motivos.append("Hay más de un candidato validado; no existe una elección inequívoca.")
    if not reparacion.validacion.aceptado:
        motivos.append("El programa completo todavía no queda sintácticamente aceptado.")
    if resultado.errores_lexicos:
        motivos.append("El texto original contiene errores léxicos.")
    if diagnostico.esperados.count(";") != 1:
        motivos.append("El parser no identifica ';' como una expectativa única de esta posición.")
    if not motivos:
        motivos.append("Único ';' validado, antes de un inicio inequívoco de sentencia, y el candidato completo es aceptado.")
        return True, tuple(motivos)
    return False, tuple(motivos)


def proponer_reparaciones(texto: str, *, max_candidatos: int = 4) -> ResultadoReparaciones:
    """Propone reparaciones para el primer error sintáctico fiable.

    No repara errores léxicos, estados de límite/cancelación ni errores de
    sintaxis posteriores. La API no conoce documento/revisión del IDE; devuelve
    SHA-256 y longitud para que el adaptador compruebe que sigue siendo el mismo
    texto antes de mostrar o aplicar una propuesta.
    """
    if not isinstance(texto, str):
        raise TypeError("texto debe ser str")
    if type(max_candidatos) is not int or max_candidatos < 1:
        raise ValueError("max_candidatos debe ser un entero positivo")
    huella = _huella(texto)
    base = dict(huella_texto=huella, longitud_texto=len(texto))
    resultado = analizar_sintaxis(texto)

    if resultado.estado == "aceptado":
        return ResultadoReparaciones("sin_error", (), None, motivo="El programa ya es sintácticamente válido.", **base)
    if resultado.estado in {"limite", "cancelado"}:
        return ResultadoReparaciones("bloqueado", (), None, (resultado.motivo,),
                                     "El parser no completó el análisis.", **base)
    if resultado.errores_lexicos or resultado.estado == "error_lexico":
        return ResultadoReparaciones("bloqueado", (), resultado.diagnosticos[0] if resultado.diagnosticos else None,
                                     ("errores_lexicos",),
                                     "No se proponen cambios sobre una tokenización con errores.", **base)
    if resultado.estado != "error_sintactico" or not resultado.diagnosticos:
        return ResultadoReparaciones("bloqueado", (), None, (),
                                     f"Estado del parser no reparable: {resultado.estado}.", **base)

    diagnostico = resultado.diagnosticos[0]
    candidatos = [s for s in _TERMINALES_REPARABLES if s in diagnostico.esperados]
    limites = ()
    if len(candidatos) > max_candidatos:
        candidatos = candidatos[:max_candidatos]
        limites = ("max_candidatos",)

    propuestas = []
    for terminal in candidatos:
        edicion = EdicionTexto(diagnostico.inicio, diagnostico.inicio, terminal)
        validacion = _validar_candidato(texto, diagnostico, edicion)
        descripcion, explicacion = _DESCRIPCIONES[terminal]
        codigo = {
            ";": "INSERCION_PUNTO_COMA", ".": "INSERCION_PUNTO_FINAL",
            "]": "INSERCION_CORCHETE_CIERRE", ")": "INSERCION_PARENTESIS_CIERRE",
        }[terminal]
        razones = [validacion.motivo]
        if not validacion.error_resuelto:
            razones.append("La propuesta no supera el error original y no se recomienda aplicarla.")
        reparacion = Reparacion(
            codigo, descripcion, explicacion, diagnostico, (edicion,), False,
            tuple(razones),
            ("El offset se interpreta sobre el texto original.",
             "Verificar huella y revisión del documento antes de aplicar.",
             "La reparación no garantiza intención ni corrección semántica."),
            validacion, huella, len(texto),
        )
        if validacion.error_resuelto:
            propuestas.append(reparacion)

    # Orden determinista: aceptación completa, luego candidato que avanza;
    # los empates conservan el orden fijo de terminales del catálogo.
    propuestas.sort(key=lambda p: (not p.validacion.aceptado,
                                   -p.validacion.offset_alcanzado,
                                   _TERMINALES_REPARABLES.index(p.ediciones[0].texto)))
    cantidad_validas = len(propuestas)
    clasificadas = []
    for p in propuestas:
        segura, razones_auto = _es_caso_auto(texto, resultado, diagnostico, p, cantidad_validas)
        clasificadas.append(Reparacion(
            p.codigo, p.descripcion, p.explicacion, p.diagnostico_objetivo,
            p.ediciones, segura, p.razones + razones_auto, p.precondiciones,
            p.validacion, p.huella_texto, p.longitud_texto,
        ))

    estado = "propuestas" if clasificadas else "sin_propuesta"
    motivo = ("Propuestas generadas y reanalizadas sobre copias." if clasificadas else
              "No se encontró una inserción de cierre que supere el error original.")
    return ResultadoReparaciones(estado, tuple(clasificadas), diagnostico,
                                 limites, motivo, **base)
