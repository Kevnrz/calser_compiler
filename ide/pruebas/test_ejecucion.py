"""Compilar y ejecutar separados, con ccom y el simulador reales."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import unittest
from unittest.mock import patch
from PySide6.QtCore import QTimer
from ide.pruebas import test_carpetas as fixtures

APP = fixtures.APP
SUMA = '''F suma [ int a, int b ] : int c ;
    c = a + b ;
    E ;
.
F main [] : void ;
    Vm resultado : int ;
    resultado = suma [ 2, 3 ] ;
    E ;
.
'''


class Ejecucion(unittest.TestCase):
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def setUp(self):
        fixtures.Carpetas.setUp(self)
        self.path = self.folder / 'suma con espacios.cls'
        self.path.write_text(SUMA)
        self.editor = self.window.abrir(self.path)

    def limpiar(self):
        if self.window.run_process:
            self.window.cancelar()
            self.esperar(lambda: self.window.run_process is None)
        if self.window.process:
            self.window.cancelar()
            self.esperar(lambda: self.window.process is None)
        fixtures.Carpetas.limpiar(self)

    def compilar(self):
        self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        self.assertIn(self.editor, self.window.compilados)
        return self.window.compilados[self.editor]

    def ejecutar(self):
        self.assertTrue(self.window.ejecutar())
        self.esperar(lambda: self.window.run_process is None)
        return self.window.run_output.toPlainText()

    def simulador_falso(self, texto):
        root = self.base / 'simulador'; (root / 'tools').mkdir(parents=True, exist_ok=True)
        (root / 'tools/sim_asm.py').write_text(texto)
        self.window.root = root

    def test_compilar_no_ejecuta_y_ejecutar_muestra_resultado_real(self):
        info = self.compilar()
        self.assertIsNone(self.window.run_process)
        self.assertEqual(self.window.run_output.toPlainText(), '')
        self.window.results_action.setChecked(False)
        salida = self.ejecutar()
        self.assertIn('Ejecución terminada correctamente', salida)
        self.assertIn('resultado = 5', salida)
        self.assertIn('Estado final de las variables:', salida)
        self.assertIn('Registros:', salida)
        self.assertIn('llamada', salida)
        self.assertTrue(self.window.results_action.isChecked())
        self.assertIs(self.window.panels.currentWidget(), self.window.run_output)
        self.assertTrue(self.window.run_trace.is_file())
        self.assertEqual((self.window.run_trace.parent / info.asm.name).read_bytes(), info.asm.read_bytes())
        self.assertTrue(self.window.run_button.isEnabled())
        self.assertFalse(self.window.stop_button.isEnabled())

    def test_sin_compilar_no_guarda_ni_lanza(self):
        with patch.object(self.window, 'error') as error, patch.object(self.window, 'guardar') as guardar:
            self.assertFalse(self.window.ejecutar())
            error.assert_called_once(); guardar.assert_not_called()
        self.assertIsNone(self.window.run_process)

    def test_edicion_sin_guardar_requiere_recompilar(self):
        self.compilar(); self.editor.insertPlainText('# nuevo\n')
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())
        self.assertTrue(self.editor.document().isModified())
        self.assertNotIn('# nuevo', self.path.read_text())

    def test_fuente_externa_modificada_no_ejecuta_version_vieja(self):
        self.compilar(); self.path.write_text('# externo\n' + SUMA)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())

    def test_artefactos_modificados_o_ausentes_requieren_recompilar(self):
        info = self.compilar(); contenido = info.asm.read_bytes()
        info.asm.write_bytes(contenido + b'\n# externo\n')
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())
        info = self.compilar(); info.memoria.unlink()
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())

    def test_compilacion_fallida_descarta_ejecutable_previo(self):
        self.compilar(); self.editor.setPlainText('F incorrecto !')
        self.window.guardar(); self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        self.assertNotIn(self.editor, self.window.compilados)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())

    def test_cancelar_y_protecciones_durante_ejecucion(self):
        self.compilar(); self.simulador_falso('import time\ntime.sleep(60)\n')
        self.assertTrue(self.window.ejecutar())
        self.assertFalse(self.window.compile_button.isEnabled())
        self.assertFalse(self.window.run_button.isEnabled())
        self.assertTrue(self.window.stop_button.isEnabled())
        self.assertFalse(self.window.close())
        antes = self.window.tabs.count(); self.window.cerrar_tab(self.window.tabs.currentIndex())
        self.assertEqual(self.window.tabs.count(), antes)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.mover_archivo(self.path, self.folder / 'otro.cls'))
            self.assertFalse(self.window.mover_archivo(self.folder, self.base / 'movida'))
        latido = []; QTimer.singleShot(0, lambda: latido.append(True)); APP.processEvents()
        self.assertTrue(latido)
        self.window.cancelar(); self.esperar(lambda: self.window.run_process is None)
        self.assertIn('Ejecución cancelada', self.window.run_output.toPlainText())
        self.assertIsNone(self.window.process)
        self.assertTrue(self.window.compile_button.isEnabled())

    def test_limite_tiempo_distingue_cancelacion(self):
        self.compilar(); self.simulador_falso('import time\ntime.sleep(60)\n')
        self.assertTrue(self.window.ejecutar())
        self.window.run_timer.start(10)
        self.esperar(lambda: self.window.run_process is None)
        self.assertIn('límite de 30 segundos', self.window.run_output.toPlainText())
        self.assertFalse(self.window.stop_button.isEnabled())

    def test_error_y_avisos_simulador_se_muestran(self):
        self.compilar()
        self.simulador_falso('import sys\nprint("Aviso de prueba", file=sys.stderr)\nsys.exit(1)\n')
        salida = self.ejecutar()
        self.assertIn('Aviso de prueba', salida); self.assertIn('Ejecución con avisos', salida)
        self.simulador_falso('import sys\nprint("sim_asm: error de prueba", file=sys.stderr)\nsys.exit(2)\n')
        salida = self.ejecutar()
        self.assertIn('error de prueba', salida); self.assertIn('Ejecución fallida · código 2', salida)

    def test_repetir_ejecucion_no_reutiliza_traza_previa(self):
        self.compilar(); self.ejecutar(); anterior = self.window.run_trace
        self.simulador_falso('import sys\nsys.exit(2)\n')
        salida = self.ejecutar()
        self.assertNotEqual(self.window.run_trace, anterior)
        self.assertTrue(anterior.is_file())
        self.assertNotIn('resultado = 5', salida)
        self.assertIn('No se generó una traza', salida)

    def test_editar_durante_ejecucion_informa_version_anterior(self):
        self.compilar()
        self.simulador_falso('import time\ntime.sleep(.15)\nprint("terminó")\n')
        self.assertTrue(self.window.ejecutar())
        self.editor.insertPlainText('# cambio durante ejecución\n')
        self.esperar(lambda: self.window.run_process is None)
        self.assertIn('versión compilada anterior', self.window.run_output.toPlainText())
        self.assertTrue(self.editor.document().isModified())

    def test_no_disponible_o_fallo_al_iniciar_restauran_controles(self):
        self.compilar(); original = self.window.root
        self.window.root = self.base / 'ausente'
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())
        self.window.root = original
        with patch('ide.interfaz.ejecucion.sys.executable', str(self.base / 'python-ausente')):
            self.assertTrue(self.window.ejecutar())
            self.esperar(lambda: self.window.run_process is None)
        self.assertIn('No se pudo iniciar el simulador', self.window.run_output.toPlainText())
        self.assertTrue(self.window.run_button.isEnabled())
        self.assertFalse(self.window.stop_button.isEnabled())

    def test_compilar_otro_documento_no_ejecuta_programa_equivocado(self):
        self.compilar()
        otra = self.folder / 'otra.cls'; otra.write_text('F main [] : void ; E ; .')
        self.window.abrir(otra)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())
        self.window.tabs.setCurrentWidget(self.editor)
        self.assertIn('resultado = 5', self.ejecutar())
