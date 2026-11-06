"""Sondeo de versiones de archivos fuera del hilo de interfaz."""
from PySide6.QtCore import QObject, QThread, QTimer, Signal
from ide import archivos


class LecturaDisco(QThread):
    resultado = Signal(object)

    def __init__(self, solicitudes, parent=None):
        super().__init__(parent)
        self.solicitudes = solicitudes

    def run(self):
        for documento, path, base in self.solicitudes:
            if self.isInterruptionRequested():
                return
            try:
                estado = archivos.estado(path, cancelar=self.isInterruptionRequested)
                error = ''
            except InterruptedError:
                return
            except OSError as fallo:
                estado, error = None, str(fallo)
            self.resultado.emit((documento, path, base, estado, error))


class MonitorDisco(QObject):
    resultado = Signal(object)

    def __init__(self, solicitudes, parent=None):
        super().__init__(parent)
        self.solicitudes = solicitudes
        self.task = None
        self.timer = QTimer(self)
        self.timer.setInterval(1500)
        self.timer.timeout.connect(self.comprobar)
        self.timer.start()

    def comprobar(self):
        if self.task is not None:
            return
        solicitudes = self.solicitudes()
        if not solicitudes:
            return
        self.task = LecturaDisco(solicitudes, self)
        self.task.resultado.connect(self.resultado)
        self.task.finished.connect(self.terminar)
        self.task.start()

    def terminar(self):
        task, self.task = self.task, None
        if task:
            task.deleteLater()

    def detener(self):
        self.timer.stop()
        if self.task:
            self.task.requestInterruption()
            if not self.task.wait(1500):
                self.timer.start()
                return False
        return True
