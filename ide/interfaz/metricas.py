"""Mediciones secuenciales en segundo plano, sin bloquear el bucle de Qt."""
import codecs
import sys

from PySide6.QtCore import QProcess, QUrl
from PySide6.QtGui import QDesktopServices, QTextCursor


def comandos_metricas(root):
    return [[str(root / 'tools/medir.py'), '--modo', modo]
            for modo in ('optimizacion', 'nivel')]


class MetricasVentana:
    def iniciar_metricas(self):
        self.metricas_process = None

    def ejecutar_metricas(self):
        if self.process or self.run_process or self.metricas_process:
            return False
        if not (self.root / 'tools/medir.py').is_file():
            self.error('Métricas no disponibles', 'No se encontró tools/medir.py en la raíz del repositorio.')
            return False
        self.metricas_pendientes = comandos_metricas(self.root)
        self.metricas_canceladas = False
        self.metricas_output.setPlainText('Métricas de optimización · dos etapas\n')
        self.results_action.setChecked(True)
        self.panels.setCurrentWidget(self.metricas_output)
        self.controles_ejecucion(True)
        self.comprobador.suspender(True)
        self.siguiente_metrica()
        return True

    def escribir_metricas(self, texto):
        cursor = self.metricas_output.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(texto)
        self.metricas_output.setTextCursor(cursor)

    def siguiente_metrica(self):
        argumentos = self.metricas_pendientes.pop(0)
        self.escribir_metricas(f'\nComando: {sys.executable} {argumentos}\n')
        self.statusBar().showMessage(f'Métricas: {argumentos[-1]} · Cancelar detiene las mediciones')
        self.metricas_decoder = codecs.getincrementaldecoder('utf-8')(errors='replace')
        proceso = QProcess(self)
        self.metricas_process = proceso
        proceso.setWorkingDirectory(str(self.root))
        proceso.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        proceso.readyReadStandardOutput.connect(self.leer_metricas)
        proceso.finished.connect(self.terminar_metrica)
        proceso.errorOccurred.connect(self.error_metricas)
        proceso.start(sys.executable, argumentos)

    def leer_metricas(self):
        if self.metricas_process:
            self.escribir_metricas(self.metricas_decoder.decode(
                bytes(self.metricas_process.readAllStandardOutput())))

    def terminar_metrica(self, codigo, estado):
        if self.metricas_process is None:
            return
        self.leer_metricas()
        self.escribir_metricas(self.metricas_decoder.decode(b'', final=True))
        proceso, self.metricas_process = self.metricas_process, None
        proceso.deleteLater()
        if self.metricas_canceladas:
            self.finalizar_metricas('Métricas canceladas')
        elif estado == QProcess.ExitStatus.CrashExit or codigo != 0:
            self.finalizar_metricas(f'Métricas fallidas · código {codigo} · revisa la salida')
        elif self.metricas_pendientes:
            self.siguiente_metrica()
        else:
            reporte = self.root / 'build/metricas/reporte.html'
            if not reporte.is_file():
                self.finalizar_metricas(f'Métricas fallidas · no se encontró {reporte}')
            elif QDesktopServices.openUrl(QUrl.fromLocalFile(str(reporte))):
                self.finalizar_metricas(f'Métricas terminadas · reporte: {reporte}')
            else:
                self.finalizar_metricas(f'Métricas terminadas · no se pudo abrir el navegador: {reporte}')

    def error_metricas(self, error):
        if self.metricas_process and error == QProcess.ProcessError.FailedToStart:
            mensaje = self.metricas_process.errorString()
            self.metricas_process.deleteLater()
            self.metricas_process = None
            self.finalizar_metricas(f'No se pudieron iniciar las métricas: {mensaje}')

    def finalizar_metricas(self, mensaje):
        self.metricas_pendientes = []
        self.escribir_metricas(f'\n{mensaje}\n')
        self.statusBar().showMessage(mensaje)
        self.controles_ejecucion(False)
        self.comprobador.suspender(False)

    def cancelar_metricas(self):
        if self.metricas_process:
            self.metricas_canceladas = True
            self.metricas_process.kill()
