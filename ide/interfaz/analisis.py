"""Cola serial de snapshots: una tarea activa y una pendiente por documento."""
from PySide6.QtCore import QObject, QThread, Qt, Signal, Slot

from ide.adaptador_nucleo import analizar_documento


class TareaLexica(QThread):
    resultado = Signal(object)
    fallo = Signal(str, int, str)

    def __init__(self, documento, revision, texto, parent):
        super().__init__(parent)
        self.documento, self.revision, self.texto = documento, revision, texto

    def run(self):
        try:
            if self.isInterruptionRequested():
                return
            result = analizar_documento(self.documento, self.revision, self.texto)
            if not self.isInterruptionRequested():
                self.resultado.emit(result)
        except Exception as error:
            if not self.isInterruptionRequested():
                self.fallo.emit(self.documento, self.revision, str(error))


class CoordinadorAnalisis(QObject):
    resultado = Signal(object)
    fallo = Signal(str, int, str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.pendientes = {}
        self.task = None
        self.cerrando = False

    @Slot(str, int, str)
    def solicitar(self, documento, revision, texto):
        if not self.cerrando:
            self.pendientes[documento] = (revision, texto)
            self.iniciar()

    def olvidar(self, documento):
        self.pendientes.pop(documento, None)

    def iniciar(self):
        if self.task or self.cerrando or not self.pendientes:
            return
        document = next(iter(self.pendientes))
        revision, text = self.pendientes.pop(document)
        self.task = TareaLexica(document, revision, text, self)
        self.task.resultado.connect(self.recibir, Qt.ConnectionType.QueuedConnection)
        self.task.fallo.connect(self.error, Qt.ConnectionType.QueuedConnection)
        self.task.finished.connect(self.terminar)
        self.task.start()

    @Slot(object)
    def recibir(self, result):
        if not self.cerrando:
            self.resultado.emit(result)

    @Slot(str, int, str)
    def error(self, documento, revision, message):
        if not self.cerrando:
            self.fallo.emit(documento, revision, message)

    @Slot()
    def terminar(self):
        task, self.task = self.task, None
        if task:
            task.deleteLater()
        self.iniciar()

    def detener(self):
        self.cerrando = True
        self.pendientes.clear()
        if self.task:
            self.task.requestInterruption()
            if not self.task.wait(3000):
                self.cerrando = False
                return False
        return True
