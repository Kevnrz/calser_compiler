"""Modelos productivos entre adaptadores C, coordinador y aplicador, sin Qt.

Este módulo no importa los analizadores del paquete nucleo. Valida estructura
(tipos, límites de rangos, estados y texto Unicode), no sintaxis de Calser.
Contextos, ediciones y precondiciones usan caracteres Python [inicio, fin);
los adaptadores convierten UTF-8 y el aplicador Qt convierte después a UTF-16.
Las dataclasses transportan una respuesta; no ejecutan ni autorizan cambios.
"""
from dataclasses import dataclass
from enum import Enum
from typing import Callable, Protocol


class EstadoAsistencia(str, Enum):
    OK = 'ok'
    SIN_OPCIONES = 'sin_opciones'
    BLOQUEADO = 'bloqueado'
    CANCELADO = 'cancelado'
    LIMITE = 'limite'
    ERROR = 'error'


def entero(valor, nombre):
    if type(valor) is not int or valor < 0:
        raise ValueError(f'{nombre} debe ser un entero no negativo')


def cadena(valor, nombre, *, no_vacia=False):
    if not isinstance(valor, str) or (no_vacia and not valor.strip()):
        raise ValueError(f'{nombre} debe ser texto' + (' no vacío' if no_vacia else ''))
    try:
        valor.encode('utf-16-le')
    except UnicodeError as error:
        raise ValueError(f'{nombre} contiene Unicode inválido') from error


@dataclass(frozen=True)
class ContextoDocumento:
    documento: str
    revision: int
    texto: str
    cursor: int
    ancla: int

    def __post_init__(self):
        cadena(self.documento, 'documento', no_vacia=True)
        entero(self.revision, 'revision')
        cadena(self.texto, 'texto')
        for nombre in ('cursor', 'ancla'):
            valor = getattr(self, nombre)
            entero(valor, nombre)
            if valor > len(self.texto):
                raise ValueError(f'{nombre} fuera del documento')


@dataclass(frozen=True)
class EdicionTexto:
    inicio: int
    fin: int
    texto: str

    def __post_init__(self):
        entero(self.inicio, 'inicio'); entero(self.fin, 'fin')
        if self.fin < self.inicio:
            raise ValueError('El fin del rango debe ser mayor o igual al inicio')
        cadena(self.texto, 'texto')


@dataclass(frozen=True)
class PrecondicionTexto:
    inicio: int
    fin: int
    texto_esperado: str

    def __post_init__(self):
        EdicionTexto(self.inicio, self.fin, self.texto_esperado)


@dataclass(frozen=True)
class Sugerencia:
    etiqueta: str
    texto: str
    inicio: int
    fin: int
    categoria: str
    detalle: str = ''
    cursor_relativo: int | None = None
    preferida_inline: bool = True

    def __post_init__(self):
        cadena(self.etiqueta, 'etiqueta', no_vacia=True)
        cadena(self.categoria, 'categoria', no_vacia=True)
        cadena(self.detalle, 'detalle')
        EdicionTexto(self.inicio, self.fin, self.texto)
        if type(self.preferida_inline) is not bool:
            raise ValueError('preferida_inline debe ser bool')
        if self.cursor_relativo is not None:
            entero(self.cursor_relativo, 'cursor_relativo')
            if self.cursor_relativo > len(self.texto):
                raise ValueError('cursor_relativo fuera del texto insertado')

    @property
    def edicion(self):
        return EdicionTexto(self.inicio, self.fin, self.texto)

    @property
    def cursor_final(self):
        return self.inicio + (len(self.texto) if self.cursor_relativo is None else self.cursor_relativo)


@dataclass(frozen=True)
class Reparacion:
    codigo: str
    descripcion: str
    ediciones: tuple[EdicionTexto, ...]
    automatica_segura: bool = False
    razones: tuple[str, ...] = ()
    precondiciones: tuple[PrecondicionTexto, ...] = ()
    validacion: str = ''
    cursor_final: int | None = None

    def __post_init__(self):
        cadena(self.codigo, 'codigo', no_vacia=True)
        cadena(self.descripcion, 'descripcion', no_vacia=True)
        cadena(self.validacion, 'validacion')
        object.__setattr__(self, 'ediciones', tuple(self.ediciones))
        object.__setattr__(self, 'razones', tuple(self.razones))
        object.__setattr__(self, 'precondiciones', tuple(self.precondiciones))
        if not self.ediciones or any(not isinstance(e, EdicionTexto) for e in self.ediciones):
            raise ValueError('Una reparación requiere EdicionTexto')
        if any(not isinstance(p, PrecondicionTexto) for p in self.precondiciones):
            raise ValueError('Precondición de tipo inválido')
        for razon in self.razones:
            cadena(razon, 'razon', no_vacia=True)
        if type(self.automatica_segura) is not bool:
            raise ValueError('automatica_segura debe ser bool')
        if self.automatica_segura and (not self.razones or not self.validacion.strip()):
            raise ValueError('Una reparación automática requiere razones y validación explícitas')
        if self.cursor_final is not None:
            entero(self.cursor_final, 'cursor_final')


def validar_resultado(instancia, campo, tipo):
    if not isinstance(instancia.estado, EstadoAsistencia):
        raise ValueError('estado debe ser EstadoAsistencia')
    cadena(instancia.explicacion, 'explicacion')
    opciones = tuple(getattr(instancia, campo))
    object.__setattr__(instancia, campo, opciones)
    if any(not isinstance(o, tipo) for o in opciones):
        raise ValueError('Tipo de opción inválido')
    if instancia.estado == EstadoAsistencia.OK and not opciones:
        raise ValueError('Usa SIN_OPCIONES para un resultado vacío')
    if instancia.estado != EstadoAsistencia.OK and opciones:
        raise ValueError('Solo OK puede contener propuestas aplicables')
    if instancia.estado in (EstadoAsistencia.BLOQUEADO, EstadoAsistencia.LIMITE, EstadoAsistencia.ERROR) and not instancia.explicacion.strip():
        raise ValueError('Este estado requiere una explicación')


@dataclass(frozen=True)
class ResultadoSugerencias:
    estado: EstadoAsistencia
    sugerencias: tuple[Sugerencia, ...] = ()
    explicacion: str = ''

    def __post_init__(self):
        validar_resultado(self, 'sugerencias', Sugerencia)


@dataclass(frozen=True)
class ResultadoReparaciones:
    estado: EstadoAsistencia
    propuestas: tuple[Reparacion, ...] = ()
    explicacion: str = ''

    def __post_init__(self):
        validar_resultado(self, 'propuestas', Reparacion)


class MotorSugerencias(Protocol):
    def __call__(self, texto: str, cursor: int, *, cancelar: Callable[[], bool] | None = None) -> ResultadoSugerencias: ...


class MotorReparaciones(Protocol):
    def __call__(self, texto: str, *, cancelar: Callable[[], bool] | None = None) -> ResultadoReparaciones: ...
