"""Solicitudes de asistencia seriales, cancelables e independientes de los widgets."""
from dataclasses import dataclass
import importlib
from PySide6.QtCore import QObject, QThread, QTimer, Qt, Signal
from ide.ediciones import MAX_TEXTO, aplicar_ediciones, validar_ediciones
from ide.nucleo.asistencia import (ContextoDocumento, EstadoAsistencia, ResultadoReparaciones,
                                  ResultadoSugerencias, validar_resultado, Reparacion, Sugerencia)

MODULOS = {'sugerencias': ('ide.nucleo.sugerencias', 'sugerir', ResultadoSugerencias),
           'reparaciones': ('ide.nucleo.correccion', 'proponer_reparaciones', ResultadoReparaciones)}


@dataclass(frozen=True)
class SolicitudAsistencia:
    numero: int
    tipo: str
    contexto: ContextoDocumento


@dataclass(frozen=True)
class RespuestaAsistencia:
    solicitud: SolicitudAsistencia
    resultado: ResultadoSugerencias | ResultadoReparaciones


def estado_resultado(tipo, estado, explicacion):
    return MODULOS[tipo][2](estado, explicacion=explicacion)


def cargar_motor(tipo):
    modulo, nombre, _ = MODULOS[tipo]
    try:
        biblioteca = importlib.import_module(modulo)
    except ModuleNotFoundError as error:
        if error.name == modulo:
            return None
        raise  # Una dependencia rota del motor es un fallo, no un motor ausente.
    motor = getattr(biblioteca, nombre)
    if not callable(motor):
        raise TypeError(f'{modulo}.{nombre} no es invocable')
    return motor


def validar_respuesta(solicitud, resultado):
    tipo, contexto = solicitud.tipo, solicitud.contexto
    if not isinstance(resultado, MODULOS[tipo][2]):
        raise ValueError('El motor devolvió un tipo ajeno al contrato compartido')
    campo, clase = ('sugerencias', Sugerencia) if tipo == 'sugerencias' else ('propuestas', Reparacion)
    validar_resultado(resultado, campo, clase)
    opciones = getattr(resultado, campo)
    if len(opciones) > (64 if tipo == 'sugerencias' else 32):
        return estado_resultado(tipo, EstadoAsistencia.LIMITE, 'El motor excedió el límite de opciones (64 sugerencias / 32 reparaciones)')
    for opcion in opciones:
        if tipo == 'sugerencias':
            if not opcion.inicio <= contexto.cursor <= opcion.fin:
                raise ValueError('La sugerencia no reemplaza el fragmento del cursor solicitado')
            validar_ediciones(contexto.texto, (opcion.edicion,))
        else:
            texto = aplicar_ediciones(contexto.texto, opcion.ediciones, opcion.precondiciones)
            if opcion.cursor_final is not None and opcion.cursor_final > len(texto):
                raise ValueError('Cursor de reparación fuera del resultado')
    return resultado


class TareaAsistencia(QThread):
    resultado = Signal(object)

    def __init__(self, solicitud, motor, parent):
        super().__init__(parent)
        self.solicitud, self.motor = solicitud, motor

    def run(self):
        s = self.solicitud
        try:
            if self.isInterruptionRequested():
                return
            motor = self.motor if self.motor is not None else cargar_motor(s.tipo)
            if motor is None:
                resultado = estado_resultado(s.tipo, EstadoAsistencia.BLOQUEADO,
                                             f'El motor de {s.tipo} todavía no está disponible')
            else:
                argumentos = (s.contexto.texto, s.contexto.cursor) if s.tipo == 'sugerencias' else (s.contexto.texto,)
                resultado = motor(*argumentos, cancelar=self.isInterruptionRequested)
                resultado = validar_respuesta(s, resultado)
        except Exception as error:
            resultado = estado_resultado(s.tipo, EstadoAsistencia.ERROR, f'{type(error).__name__}: {error}')
        if not self.isInterruptionRequested():
            self.resultado.emit(RespuestaAsistencia(s, resultado))


class CoordinadorAsistencia(QObject):
    resultado = Signal(object)

    def __init__(self, parent=None, *, plazo_ms=3000):
        super().__init__(parent)
        self.motores = {tipo: None for tipo in MODULOS}
        self.pendientes = {}
        self.ultimas = {}
        self.task = None
        self.numero = 0
        self.plazo_ms = plazo_ms
        self.timer = QTimer(self); self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.agotar)

    def establecer_motor(self, tipo, motor):
        if tipo not in MODULOS or (motor is not None and not callable(motor)):
            raise ValueError('Tipo de motor o proveedor inválido')
        self.invalidar(tipo=tipo)
        self.motores[tipo] = motor  # None recupera el módulo real por importación diferida.

    def solicitar(self, tipo, contexto):
        if tipo not in MODULOS or not isinstance(contexto, ContextoDocumento):
            raise ValueError('Solicitud de asistencia inválida')
        self.numero += 1
        solicitud = SolicitudAsistencia(self.numero, tipo, contexto)
        clave = (contexto.documento, tipo)
        self.ultimas[clave] = solicitud.numero
        self.pendientes.pop(clave, None)
        if self.task:
            activa = self.task.solicitud
            if (activa.contexto.documento, activa.tipo) == clave:
                self.task.requestInterruption()
        if len(contexto.texto) > MAX_TEXTO:
            self.resultado.emit(RespuestaAsistencia(solicitud, estado_resultado(tipo, EstadoAsistencia.LIMITE,
                                                'Documento de más de 2 millones de caracteres')))
            return solicitud
        if clave not in self.pendientes and len(self.pendientes) >= 32:
            self.resultado.emit(RespuestaAsistencia(solicitud, estado_resultado(tipo, EstadoAsistencia.LIMITE,
                                                'Cola de asistencia llena (32 solicitudes pendientes)')))
            return solicitud
        self.pendientes[clave] = solicitud
        self.iniciar()
        return solicitud

    def vigente(self, solicitud):
        return self.ultimas.get((solicitud.contexto.documento, solicitud.tipo)) == solicitud.numero

    def iniciar(self):
        if self.task or not self.pendientes:
            return
        clave = next(iter(self.pendientes))
        solicitud = self.pendientes.pop(clave)
        self.task = TareaAsistencia(solicitud, self.motores[solicitud.tipo], self)
        self.task.resultado.connect(self.recibir, Qt.ConnectionType.QueuedConnection)
        self.task.finished.connect(self.terminar, Qt.ConnectionType.QueuedConnection)
        self.timer.start(self.plazo_ms)
        self.task.start()

    def recibir(self, respuesta):
        if self.vigente(respuesta.solicitud):
            self.resultado.emit(respuesta)

    def terminar(self):
        self.timer.stop()
        tarea, self.task = self.task, None
        tarea.deleteLater()
        self.iniciar()

    def invalidar(self, documento=None, *, tipo=None):
        def coincide(clave):
            return (documento is None or clave[0] == documento) and (tipo is None or clave[1] == tipo)
        self.pendientes = {k: v for k, v in self.pendientes.items() if not coincide(k)}
        self.ultimas = {k: v for k, v in self.ultimas.items() if not coincide(k)}
        if self.task and coincide((self.task.solicitud.contexto.documento, self.task.solicitud.tipo)):
            self.task.requestInterruption()

    def agotar(self):
        if self.task:
            solicitud = self.task.solicitud
            self.task.requestInterruption()
            if self.vigente(solicitud):
                self.ultimas.pop((solicitud.contexto.documento, solicitud.tipo), None)
                self.resultado.emit(RespuestaAsistencia(solicitud, estado_resultado(solicitud.tipo,
                    EstadoAsistencia.LIMITE, 'El motor excedió el tiempo de asistencia (3 segundos)')))
            # Un motor que ignora cancelar ocupa su hilo hasta terminar. No se
            # arranca otro ni se destruye/termina por la fuerza ese hilo.

    def detener(self):
        self.timer.stop(); self.invalidar()
        return not self.task or self.task.wait(1500)
