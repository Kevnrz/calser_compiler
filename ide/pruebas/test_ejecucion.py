"""Compilar y ejecutar separados, con ccom y el simulador reales."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import unittest
from unittest.mock import patch
from PySide6.QtCore import QTimer
from ide.pruebas import test_carpetas as fixtures
from ide import archivos, preferencias
from ide.interfaz.ejecucion import argumentos_simulador, CompilacionEjecutable
from PySide6.QtCore import QSettings

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


class SelectorSimulador(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    terminar = fixtures.Carpetas.terminar
    esperar = fixtures.Carpetas.esperar

    def test_menu_y_argumentos_para_cada_simulador_y_latencia(self):
        w = self.window
        self.assertTrue(w.acciones_simulador['sin_cache'].isChecked())
        self.assertTrue(w.acciones_latencia[4].isChecked())
        self.assertFalse(w.latencia_menu.isEnabled())
        comunes = ['programa.asm', '--data-image', 'programa.mem', '--traza', 'traza.txt',
                   '--max-bundles', '100000', '--traza-max', '5000']
        for cache in ('sin_cache', '0', '1', '2'):
            w.acciones_simulador[cache].trigger()
            self.assertEqual(sum(a.isChecked() for a in w.acciones_simulador.values()), 1)
            self.assertEqual(w.latencia_menu.isEnabled(), cache != 'sin_cache')
            for divisor in (2, 4, 5):
                w.seleccionar_simulacion('mem_div', divisor)
                args = argumentos_simulador(w.root, 'programa.asm', 'programa.mem',
                                            'traza.txt', w.configuracion_simulacion)
                script = 'sim_asm.py' if cache == 'sin_cache' else 'sim_cache.py'
                extra = [] if cache == 'sin_cache' else ['--cache', cache, '--mem-div', str(divisor)]
                self.assertEqual(args, [str(w.root / 'tools' / script)] + comunes + extra)

    def test_preferencia_persiste_al_reabrir(self):
        settings = QSettings(str(self.base / 'preferencias.ini'), QSettings.Format.IniFormat)
        self.window.settings = settings
        self.window.seleccionar_simulacion('simulador', '2')
        self.window.seleccionar_simulacion('mem_div', 5)
        self.window.guardar_preferencias()
        otra = fixtures.Ventana(fixtures.ROOT, preferencias=QSettings(
            str(self.base / 'preferencias.ini'), QSettings.Format.IniFormat))
        try:
            self.assertEqual(otra.configuracion_simulacion, {'simulador': '2', 'mem_div': 5})
            self.assertTrue(otra.acciones_simulador['2'].isChecked())
            self.assertTrue(otra.acciones_latencia[5].isChecked())
            self.assertTrue(otra.latencia_menu.isEnabled())
        finally:
            otra.monitor.timer.stop()
            otra.close()
            otra.deleteLater()
        self.assertEqual(preferencias.validar_simulacion({'simulador': 'extra', 'mem_div': True}),
                         {'simulador': 'sin_cache', 'mem_div': 4})

    def test_f6_cache_muestra_salida_completa_y_codigos(self):
        fuente = self.folder / 'programa.cls'
        fuente.write_text('F main [] : void ; E ; .')
        editor = self.window.abrir(fuente)
        asm, mem = self.folder / 'programa.asm', self.folder / 'programa.mem'
        asm.write_text('ensamblador de prueba')
        mem.write_text('memoria de prueba')
        self.window.compilados[editor] = CompilacionEjecutable(fuente, editor.revision,
            archivos.estado(fuente), asm, mem, archivos.estado(asm), archivos.estado(mem))
        root = self.base / 'simulador'
        (root / 'tools').mkdir(parents=True)
        self.window.root = root
        self.window.seleccionar_simulacion('simulador', '2')
        for codigo, mensaje in ((0, 'terminada correctamente'), (1, 'con avisos'), (2, 'fallida · código 2')):
            (root / 'tools/sim_cache.py').write_text(
                f'import sys\nprint("Métricas de caché: prueba")\nprint("aviso stderr", file=sys.stderr)\nsys.exit({codigo})\n')
            self.assertTrue(self.window.ejecutar())
            self.esperar(lambda: self.window.run_process is None)
            salida = self.window.run_output.toPlainText()
            self.assertIn('Simulador: Caché: L1 + L2 · MEM_DIV=4 (53 ciclos)', salida)
            self.assertIn('Métricas de caché: prueba', salida)
            self.assertIn('aviso stderr', salida)
            self.assertIn(mensaje, salida)


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
