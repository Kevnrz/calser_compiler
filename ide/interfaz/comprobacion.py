"""Front-end ccom sobre copias temporales: cola serial y QProcess cancelable."""
from dataclasses import dataclass, replace
from pathlib import Path
import tempfile

from PySide6.QtCore import QObject, QProcess, QTimer, Signal
from ide import ccom


@dataclass(frozen=True)
class SolicitudCcom:
    documento: str
    revision: int
    texto: str
    fuente: Path | None
    carpeta: Path


@dataclass(frozen=True)
class ResultadoCcom:
    solicitud: SolicitudCcom
    problemas: tuple
    codigo: int
    salida: str


class ComprobadorCcom(QObject):
    resultado = Signal(object)
    fallo = Signal(object, str, str)
    estado = Signal(object, str)

    def __init__(self, root, parent=None):
        super().__init__(parent)
        self.root = Path(root)
        self.configured = ""
        self.habilitado = True
        self.suspendido = False
        self.cerrando = False
        self.demora = 600
        self.timeout_ms = 5000
        self.temporizadores = {}
        self.pendientes = {}
        self.process = None
        self.actual = None
        self.temporary = None
        self.cancelado = False
        self.motivo = ""
        self.stdout = bytearray()
        self.stderr = bytearray()
        self.limite = QTimer(self)
        self.limite.setSingleShot(True)
        self.limite.timeout.connect(lambda: self.abortar("Tiempo límite de análisis alcanzado."))

    def programar(self, request):
        self.invalidar(request.documento)
        if not self.habilitado or self.cerrando:
            return
        timer = QTimer(self)
        timer.setSingleShot(True)
        self.temporizadores[request.documento] = timer

        def ready():
            if self.temporizadores.get(request.documento) is not timer:
                return
            self.temporizadores.pop(request.documento)
            timer.deleteLater()
            self.pendientes[request.documento] = request
            self.iniciar()

        timer.timeout.connect(ready)
        timer.start(self.demora)
        self.estado.emit(request, "Esperando…")

    def invalidar(self, document):
        timer = self.temporizadores.pop(document, None)
        if timer:
            timer.stop()
            timer.deleteLater()
        self.pendientes.pop(document, None)
        if self.actual and self.actual.documento == document and self.process:
            self.cancelado = True
            self.process.kill()

    def activar(self, enabled):
        self.habilitado = enabled
        if not enabled:
            for doc in list(self.temporizadores):
                self.invalidar(doc)
            self.pendientes.clear()
            if self.actual:
                self.invalidar(self.actual.documento)

    def suspender(self, paused):
        self.suspendido = paused
        if paused and self.actual and not self.cancelado:
            # F5 puede pertenecer a otra pestaña. Reanudar el snapshot de esta
            # después de compilar; invalidar(doc) del llamador ya excluyó el
            # documento compilado o cualquier revisión obsoleta.
            self.pendientes[self.actual.documento] = self.actual
            self.estado.emit(self.actual, "Esperando…")
            self.cancelado = True
            self.process.kill()
        if not paused:
            self.iniciar()

    def iniciar(self):
        if self.process or self.suspendido or not self.habilitado or self.cerrando:
            return
        while self.pendientes and not self.process:
            doc = next(iter(self.pendientes))
            request = self.pendientes.pop(doc)
            executable = ccom.ejecutable(self.root, self.configured)
            if not executable:
                self.fallo.emit(request, "ccom no disponible; ejecuta make o configura su ruta.", "")
                continue
            try:
                self.temporary = tempfile.TemporaryDirectory(prefix="calside_check_")
                self.snapshot = Path(self.temporary.name) / "documento.cls"
                self.snapshot.write_text(request.texto, encoding="utf-8", newline="\n")
            except (OSError, UnicodeError) as error:
                self.limpiar_temporal()
                self.fallo.emit(request, "No se pudo preparar la copia temporal.", str(error))
                continue
            self.actual = request
            self.cancelado = False
            self.motivo = ""
            self.stdout, self.stderr = bytearray(), bytearray()
            self.process = QProcess(self)
            cwd = request.fuente.parent if request.fuente and request.fuente.parent.is_dir() else request.carpeta
            self.cwd = cwd if cwd.is_dir() else self.root
            self.process.setWorkingDirectory(str(self.cwd))
            self.process.readyReadStandardOutput.connect(self.leer_stdout)
            self.process.readyReadStandardError.connect(self.leer_stderr)
            self.process.errorOccurred.connect(self.error_proceso)
            self.process.finished.connect(self.finalizar)
            self.estado.emit(request, "Analizando…")
            self.limite.start(self.timeout_ms)
            self.process.start(executable, ["--check", str(self.snapshot)])

    def agregar(self, destination, data):
        if len(destination) + len(data) > 1_000_000:
            self.abortar("La salida del análisis supera 1 MB. Reduce el documento o revisa ccom.")
        else:
            destination.extend(data)

    def leer_stdout(self):
        if self.process and self.process.isOpen():
            self.agregar(self.stdout, bytes(self.process.readAllStandardOutput()))

    def leer_stderr(self):
        if self.process and self.process.isOpen():
            self.agregar(self.stderr, bytes(self.process.readAllStandardError()))

    def abortar(self, message):
        if self.process:
            self.motivo = message
            self.process.kill()

    def error_proceso(self, error):
        if self.process and error == QProcess.ProcessError.FailedToStart:
            self.finalizar(-1, QProcess.ExitStatus.CrashExit)

    def finalizar(self, code, status):
        if not self.process:
            return
        self.limite.stop()
        self.leer_stdout()
        self.leer_stderr()
        request = self.actual
        error_text = self.stderr.decode("utf-8", errors="replace")
        out_text = self.stdout.decode("utf-8", errors="replace")
        display = str(request.fuente or f"Sin título [{request.documento[:6]}]")
        log = (out_text + error_text).replace(str(self.snapshot), display)
        message = self.motivo
        if not message and status == QProcess.ExitStatus.CrashExit:
            message = f"No se completó el análisis: {self.process.errorString()}"
        elif not message and code == 2:
            message = "ccom no admite --check o rechazó sus opciones. Ejecuta make o configura una versión compatible."
        elif not message and code not in (0, 1):
            message = f"ccom terminó con código inesperado {code}."
        problems = []
        if not message:
            for problem in ccom.diagnosticos(error_text, self.snapshot, self.cwd):
                if problem.archivo == self.snapshot:
                    problem = replace(problem, archivo=request.fuente, documento=request.documento)
                problems.append(replace(problem, origen="ccom automático"))
            if code == 1 and not problems:
                message = "ccom rechazó el documento sin un diagnóstico reconocible; consulta Análisis de ccom."
        cancelled = self.cancelado or self.cerrando or not self.habilitado
        process, self.process = self.process, None
        self.actual = None
        self.limpiar_temporal()
        process.deleteLater()
        if not cancelled:
            if message:
                self.fallo.emit(request, message, log)
            else:
                self.resultado.emit(ResultadoCcom(request, tuple(problems), code, log))
        self.iniciar()

    def limpiar_temporal(self):
        if self.temporary:
            self.temporary.cleanup()
            self.temporary = None

    def detener(self):
        self.cerrando = True
        self.activar(False)
        if self.process:
            process = self.process
            process.kill()
            if not process.waitForFinished(1500):
                self.cerrando = False
                self.habilitado = True
                return False
        self.limpiar_temporal()
        return True
