"""E4: ccom real sobre buffers y ciclo de procesos/temporales."""
import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

from PySide6.QtWidgets import QApplication
from ide.interfaz.ventana import Ventana

ROOT = Path(__file__).resolve().parents[2]
VALIDO = "F main [] : void ;\n    Vm total : int = 3 ;\n    E ;\n.\n"
SEMANTICO = "# café 😀\nF main [] : void ;\n    Vm total : int = 3 ;\n    total = desconocida + 1 ;\n    E ;\n.\n"


@unittest.skipUnless((ROOT / "ccom").exists(), "Construir ccom con make para E4")
class ComprobacionAutomatica(unittest.TestCase):
    def setUp(self):
        self.app = QApplication.instance() or QApplication([])
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder = Path(self.directory.name)
        self.window = Ventana(ROOT, preferencias=False)
        self.window.project = self.folder
        self.window.comprobador.demora = 10
        self.window.editor().analisis_timer.setInterval(10)
        self.addCleanup(self.cleanup)

    def cleanup(self):
        if self.window.process:
            self.window.cancelar()
            self.window.process.waitForFinished(1000)
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        self.app.processEvents()

    def esperar(self, predicate):
        end = time.monotonic() + 5
        while not predicate() and time.monotonic() < end:
            self.app.processEvents()
            time.sleep(.005)
        self.assertTrue(predicate(), self.window.auto_output.toPlainText())

    def escribir(self, text):
        editor = self.window.editor()
        editor.analisis_timer.setInterval(10)
        editor.selectAll(); editor.insertPlainText(text)
        return editor

    def comprobar(self, editor):
        self.esperar(lambda: editor in self.window.auto_results)

    def fake(self, body):
        path = self.folder / "ccom_prueba"
        path.write_text(f"#!{sys.executable}\n" + body)
        path.chmod(0o755)
        self.window.configured_ccom = str(path)
        return path

    def test_sintaxis_y_semantica_sin_f5_ni_guardar(self):
        editor = self.escribir("F main [] : void ;\n    vs\n    E ;\n.\n")
        self.comprobar(editor)
        problem = self.window.auto_results[editor][0]
        self.assertEqual(problem.categoria, "Sintáctico")
        self.assertEqual(problem.linea, 3)
        self.assertIsNone(editor.path)
        self.assertIsNone(self.window.process)
        self.window.ir_problema(self.window.problems.topLevelItem(0), 0)
        self.assertEqual(editor.textCursor().selectedText(), "E")
        editor = self.escribir(SEMANTICO)
        self.comprobar(editor)
        self.assertEqual(self.window.auto_results[editor][0].categoria, "Semántico")
        self.assertEqual(self.window.problems.topLevelItem(0).text(5), "ccom automático")
        self.assertTrue(editor.document().isModified())

    def test_original_y_artefactos_intactos_y_temporal_eliminado(self):
        source = self.folder / "fuente con espacios.cls"
        source.write_text(VALIDO)
        originals = {source: source.read_bytes()}
        for ext in (".bin", ".asm", ".mem", ".hex"):
            target = source.with_suffix(ext); target.write_bytes(b"conservar")
            originals[target] = target.read_bytes()
        temporary = []
        self.window.comprobador.estado.connect(lambda request, state: temporary.append(self.window.comprobador.snapshot.parent) if state == "Analizando…" else None)
        editor = self.window.abrir(source)
        self.escribir(SEMANTICO)
        self.comprobar(editor)
        self.assertEqual(set(self.folder.iterdir()), set(originals))
        self.assertTrue(all(p.read_bytes() == data for p, data in originals.items()))
        self.assertTrue(temporary and all(not p.exists() for p in temporary))
        self.assertEqual(self.window.auto_results[editor][0].archivo, source)
        self.assertNotIn("calside_check_", self.window.auto_output.toPlainText())

    def test_corregir_elimina_diagnostico_y_mantiene_pestanas_independientes(self):
        first = self.escribir(SEMANTICO)
        self.comprobar(first)
        second = self.window.nuevo(); self.escribir(VALIDO)
        self.comprobar(second)
        self.assertEqual(len(first.problemas), 1)
        self.assertEqual(second.problemas, [])
        self.window.tabs.setCurrentWidget(first)
        self.escribir(VALIDO)
        self.assertEqual(first.problemas, [])
        self.comprobar(first)
        self.assertEqual(first.estado_ccom, "Sin errores")

    def test_errores_lexicos_no_lanzan_ccom_y_toggle_limpia_resultados(self):
        editor = self.escribir("@")
        self.esperar(lambda: editor.analisis_revision == editor.revision)
        self.assertEqual(editor.estado_ccom, "Corrige el léxico")
        self.assertNotIn(editor, self.window.auto_results)
        self.assertIsNone(self.window.comprobador.process)
        self.escribir(SEMANTICO); self.comprobar(editor)
        self.window.auto_action.setChecked(False)
        self.assertEqual(editor.estado_ccom, "Desactivado")
        self.assertEqual(editor.problemas, [])
        self.window.auto_action.setChecked(True)
        self.comprobar(editor)
        self.assertEqual(len(editor.problemas), 1)

    def test_editar_cancela_proceso_antiguo_y_limpia_copia(self):
        self.fake("import time\ntime.sleep(20)\n")
        editor = self.escribir(SEMANTICO)
        self.esperar(lambda: self.window.comprobador.process is not None)
        old = self.window.comprobador.snapshot.parent
        self.window.configured_ccom = str(ROOT / "ccom")
        self.escribir(VALIDO)
        self.comprobar(editor)
        self.assertFalse(old.exists())
        self.assertEqual(editor.problemas, [])

    def test_cerrar_pestana_cancela_comprobacion_sin_contaminar_la_nueva(self):
        self.fake("import time\ntime.sleep(20)\n")
        editor = self.escribir(SEMANTICO)
        self.esperar(lambda: self.window.comprobador.process is not None)
        old = self.window.comprobador.snapshot.parent
        editor.document().setModified(False)
        self.window.cerrar_tab(0)
        self.esperar(lambda: self.window.comprobador.process is None)
        self.assertFalse(old.exists())
        self.assertIsNot(self.window.editor(), editor)
        self.assertEqual(self.window.editor().problemas, [])

    def test_timeout_visible_no_se_presenta_como_documento_valido(self):
        self.fake("import time\ntime.sleep(20)\n")
        self.window.comprobador.timeout_ms = 60
        editor = self.escribir(VALIDO)
        self.esperar(lambda: editor.estado_ccom == "No disponible")
        self.assertIn("Tiempo límite", self.window.auto_output.toPlainText())
        self.assertNotIn(editor, self.window.auto_results)
        self.assertIsNone(self.window.comprobador.temporary)
        self.window.configured_ccom = str(ROOT / "ccom")
        self.window.comprobador.timeout_ms = 5000
        self.escribir(VALIDO + "\n")
        self.comprobar(editor)

    def test_ejecutable_antiguo_y_error_sin_posicion_son_fallos_visibles(self):
        self.fake("import sys\nprint('opcion desconocida --check', file=sys.stderr)\nsys.exit(2)\n")
        editor = self.escribir(VALIDO)
        self.esperar(lambda: editor.estado_ccom == "No disponible")
        self.assertIn("versión compatible", self.window.auto_output.toPlainText())
        self.assertNotIn(editor, self.window.auto_results)
        self.fake("import sys\nprint('error sin posicion', file=sys.stderr)\nsys.exit(1)\n")
        self.escribir(VALIDO + "\n")
        self.esperar(lambda: editor.estado_ccom == "No disponible")
        self.assertIn("sin un diagnóstico con posición", self.window.auto_output.toPlainText())
        self.assertEqual(editor.problemas, [])

    def test_f5_tiene_prioridad_y_generacion_solo_ocurre_manualmente(self):
        source = self.folder / "programa.cls"; source.write_text(VALIDO)
        self.fake("import time\ntime.sleep(20)\n")
        editor = self.window.abrir(source); editor.analisis_timer.setInterval(10)
        editor.programar_analisis()
        self.esperar(lambda: self.window.comprobador.process is not None)
        old = self.window.comprobador.snapshot.parent
        self.window.configured_ccom = str(ROOT / "ccom")
        self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        self.assertIn("Compilación correcta", self.window.output.toPlainText())
        self.esperar(lambda: not old.exists())
        self.assertTrue(list((self.folder / "build/ide").rglob("*.bin")))
        self.assertFalse(self.window.comprobador.suspendido)

    def test_compilar_no_duplica_el_error_automatico(self):
        source = self.folder / "programa.cls"; source.write_text(SEMANTICO)
        editor = self.window.abrir(source);editor.analisis_timer.setInterval(10)
        editor.programar_analisis();self.comprobar(editor)
        self.window.compilar();self.esperar(lambda: self.window.process is None)
        self.assertEqual(len(editor.problemas), 1)
        self.assertEqual(editor.problemas[0].origen, "ccom")

    def test_f5_en_otra_pestana_reanuda_el_analisis_pendiente(self):
        self.fake("import time\ntime.sleep(20)\n")
        first = self.escribir(SEMANTICO)
        self.esperar(lambda: self.window.comprobador.process is not None)
        source = self.folder / "otro.cls"; source.write_text(VALIDO)
        self.window.abrir(source)
        self.window.configured_ccom = str(ROOT / "ccom")
        self.window.comprobador.configured = self.window.configured_ccom
        self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        self.comprobar(first)
        self.assertEqual(self.window.auto_results[first][0].categoria, "Semántico")

    def test_guardar_como_durante_analisis_remapea_al_nombre_nuevo(self):
        self.fake("import time\ntime.sleep(20)\n")
        editor = self.escribir(SEMANTICO)
        self.esperar(lambda: self.window.comprobador.process is not None)
        old = self.window.comprobador.snapshot.parent
        self.window.configured_ccom = str(ROOT / "ccom")
        source = self.folder / "nuevo nombre.cls"
        with patch("ide.interfaz.ventana.QFileDialog.getSaveFileName", return_value=(str(source), "Calser (*.cls)")):
            self.assertTrue(self.window.guardar(como=True))
        self.comprobar(editor)
        self.assertEqual(self.window.auto_results[editor][0].archivo, source)
        self.assertFalse(old.exists())
        self.assertEqual(source.read_text(), SEMANTICO)
        self.assertFalse(editor.document().isModified())

    def test_fallo_de_arranque_limpia_temporal(self):
        path = self.fake("esto no es un ejecutable\n")
        path.write_bytes(b"\x00ELF-invalido")
        editor = self.escribir(VALIDO)
        self.esperar(lambda: editor.estado_ccom == "No disponible")
        self.assertIsNone(self.window.comprobador.temporary)
        self.assertIsNone(self.window.comprobador.process)
        self.assertNotIn(editor, self.window.auto_results)

    def test_salida_excesiva_se_cancela_y_no_se_publica_como_valida(self):
        self.fake("import os\nos.write(2, b'x'*1100000)\n")
        editor = self.escribir(VALIDO)
        self.esperar(lambda: editor.estado_ccom == "No disponible")
        self.assertIn("supera 1 MB", self.window.auto_output.toPlainText())
        self.assertNotIn(editor, self.window.auto_results)
        self.assertIsNone(self.window.comprobador.temporary)


if __name__ == "__main__":
    unittest.main()
