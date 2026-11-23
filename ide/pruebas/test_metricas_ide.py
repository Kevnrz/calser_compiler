"""Acción de métricas con procesos falsos; no ejecuta tools/medir.py."""
import sys
import unittest
from unittest.mock import Mock, patch

from PySide6.QtCore import QProcess, QObject, Signal
from ide.pruebas import test_carpetas as fixtures


class ProcesoFalso(QObject):
    ProcessChannelMode = QProcess.ProcessChannelMode
    ProcessError = QProcess.ProcessError
    ExitStatus = QProcess.ExitStatus
    readyReadStandardOutput = Signal()
    finished = Signal(int, QProcess.ExitStatus)
    errorOccurred = Signal(QProcess.ProcessError)

    def __init__(self, parent):
        super().__init__(parent)
        self.start = Mock()
        self.kill = Mock()
        self.setWorkingDirectory = Mock()
        self.setProcessChannelMode = Mock()
        self.salida = b''

    def readAllStandardOutput(self):
        salida, self.salida = self.salida, b''
        return salida

    def errorString(self):
        return 'Python no disponible'


class MetricasIDE(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    terminar = fixtures.Carpetas.terminar
    esperar = fixtures.Carpetas.esperar

    def preparar(self):
        w = self.window
        w.root = self.base
        (self.base / 'tools').mkdir()
        (self.base / 'tools/medir.py').write_text('# no se ejecuta')
        self.reporte = self.base / 'build/metricas/reporte.html'
        self.reporte.parent.mkdir(parents=True)
        self.reporte.write_text('<html>reporte de prueba</html>')

    def test_dos_comandos_y_apertura_solo_al_terminar(self):
        self.preparar()
        w = self.window
        with patch('ide.interfaz.metricas.QProcess', ProcesoFalso), patch(
                'ide.interfaz.metricas.QDesktopServices.openUrl', return_value=True) as abrir:
            w.metricas_action.trigger()
            primero = w.metricas_process
            primero.start.assert_called_once_with(sys.executable,
                [str(w.root / 'tools/medir.py'), '--modo', 'optimizacion'])
            primero.setWorkingDirectory.assert_called_once_with(str(w.root))
            self.assertIs(w.panels.currentWidget(), w.metricas_output)
            self.assertTrue(w.stop_button.isEnabled())
            self.assertFalse(w.compilar())
            self.assertFalse(w.ejecutar())
            self.assertFalse(w.ejecutar_metricas())
            primero.salida = 'avance y errores\n'.encode()
            primero.readyReadStandardOutput.emit()
            primero.finished.emit(0, QProcess.ExitStatus.NormalExit)
            segundo = w.metricas_process
            segundo.start.assert_called_once_with(sys.executable,
                [str(w.root / 'tools/medir.py'), '--modo', 'nivel'])
            abrir.assert_not_called()
            segundo.finished.emit(0, QProcess.ExitStatus.NormalExit)
            self.assertIsNone(w.metricas_process)
            self.assertEqual(abrir.call_args.args[0].toLocalFile(), str(self.reporte))
            self.assertIn('avance y errores', w.metricas_output.toPlainText())
            self.assertTrue(w.compile_action.isEnabled())
            self.assertFalse(w.stop_button.isEnabled())

    def test_fallo_cancelacion_e_inicio_fallido_no_abren_reporte(self):
        self.preparar()
        w = self.window
        with patch('ide.interfaz.metricas.QProcess', ProcesoFalso), patch(
                'ide.interfaz.metricas.QDesktopServices.openUrl') as abrir:
            for etapa in (1, 2):
                w.ejecutar_metricas()
                if etapa == 2:
                    w.metricas_process.finished.emit(0, QProcess.ExitStatus.NormalExit)
                w.metricas_process.finished.emit(1, QProcess.ExitStatus.NormalExit)
                self.assertIsNone(w.metricas_process)
                self.assertIn('Métricas fallidas', w.metricas_output.toPlainText())
            w.ejecutar_metricas()
            proceso = w.metricas_process
            self.assertFalse(w.close())
            w.cancelar()
            proceso.kill.assert_called_once()
            # Incluso un 0 recibido después de Cancelar no lanza la segunda etapa.
            proceso.finished.emit(0, QProcess.ExitStatus.NormalExit)
            self.assertIn('Métricas canceladas', w.metricas_output.toPlainText())
            w.ejecutar_metricas()
            w.metricas_process.errorOccurred.emit(QProcess.ProcessError.FailedToStart)
            self.assertIn('Python no disponible', w.metricas_output.toPlainText())
            self.assertIsNone(w.metricas_process)
            self.assertTrue(w.compile_action.isEnabled())
            abrir.assert_not_called()
