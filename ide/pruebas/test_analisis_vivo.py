"""E3: lexer real, Unicode y ciclo asíncrono sin guardar el documento."""
import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

from PySide6.QtWidgets import QApplication
from ide.adaptador_nucleo import analizar_documento
from ide.interfaz.ventana import Ventana

ROOT = Path(__file__).resolve().parents[2]


class AdaptadorNucleo(unittest.TestCase):
    def test_tokens_reales_y_comentarios_multilinea(self):
        result = analizar_documento("doc", 1, "#[ café 😀\n\n]# Vmx : int = 3 ; # fin\n")
        categories = [span.categoria for span in result.tramos[2]]
        self.assertIn("tipo", categories)
        self.assertIn("numero", categories)
        self.assertIn("comentario", categories)
        # El lexer reconoce Vm incluso pegado a x; el antiguo regex no.
        self.assertTrue(any(s.categoria == "reservada" and s.longitud == 2 for s in result.tramos[2]))
        self.assertEqual(result.problemas, ())

    def test_fatal_y_final_de_documento(self):
        result = analizar_documento("doc", 1, "F main [] : void ;\n#[ sin cerrar\n\n")
        self.assertTrue(result.fatal)
        self.assertEqual(result.problemas[0].longitud, 2)
        self.assertEqual(result.problemas[0].unidad, "unicode")
        self.assertTrue(all(s.categoria == "comentario" for s in result.tramos[1]))
        self.assertEqual(analizar_documento("doc", 2, "").tramos, {})

    def test_limite_explicito(self):
        with self.assertRaisesRegex(ValueError, "análisis en vivo está pausado"):
            analizar_documento("doc", 1, " " * 2_000_001)


class AnalisisVivo(unittest.TestCase):
    def setUp(self):
        self.app = QApplication.instance() or QApplication([])
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder = Path(self.directory.name)
        self.window = Ventana(ROOT)
        self.window.auto_action.setChecked(False)  # Esta suite aísla el lexer E3.
        self.addCleanup(self.cleanup)

    def cleanup(self):
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        self.app.processEvents()

    def esperar(self, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        while not predicate() and time.monotonic() < deadline:
            self.app.processEvents()
            time.sleep(.005)
        self.assertTrue(predicate())

    def analizado(self, editor):
        self.esperar(lambda: editor.analisis_revision == editor.revision)

    def test_error_sin_guardar_navegacion_y_correccion(self):
        editor = self.window.editor()
        editor.insertPlainText("Vm x : int = 3 ; @")
        self.assertEqual(editor.analisis_timer.interval(), 300)
        self.assertTrue(editor.analisis_timer.isActive())
        self.analizado(editor)
        self.assertIsNone(editor.path)
        self.assertIsNone(self.window.process)
        self.assertEqual(len(editor.errores_lexicos), 1)
        self.assertEqual(self.window.problems.topLevelItem(0).text(5), "núcleo")
        self.window.ir_problema(self.window.problems.topLevelItem(0), 0)
        self.assertEqual(editor.textCursor().selectedText(), "@")
        self.assertIn("núcleo · Léxico", self.window.diagnostic_message.text())
        editor.insertPlainText("")
        self.analizado(editor)
        self.assertEqual(self.window.problems.topLevelItemCount(), 0)
        self.assertTrue(editor.document().isModified())

    def test_unicode_y_comentarios_no_producen_errores_falsos(self):
        editor = self.window.editor()
        editor.insertPlainText("#[ café 😀\n]#\tVm x : int = 1 ;\n😀 @")
        self.analizado(editor)
        self.assertEqual(len(editor.errores_lexicos), 2)
        emoji, at = editor.errores_lexicos
        self.assertEqual(editor.rango_problema(emoji).selectedText(), "😀")
        self.assertEqual(editor.rango_problema(at).selectedText(), "@")
        block = editor.document().findBlockByNumber(2)
        self.assertEqual(editor.rango_problema(at).selectionStart(), block.position() + 3)
        self.assertTrue(any(s.categoria == "comentario" for s in editor.highlighter.tramos[0]))

    def test_resaltado_no_modifica_disco_revision_ni_deshacer(self):
        path = self.folder / "guardado.cls"
        original = "Vm x : int = 3 ;"
        path.write_text(original)
        editor = self.window.abrir(path)
        revision = editor.revision
        self.analizado(editor)
        self.assertEqual(editor.revision, revision)
        self.assertFalse(editor.document().isModified())
        self.assertFalse(editor.document().isUndoAvailable())
        editor.insertPlainText("@")
        self.analizado(editor)
        self.assertTrue(editor.document().isModified())
        self.assertEqual(path.read_text(), original)
        editor.undo()
        self.analizado(editor)
        self.assertEqual(editor.toPlainText(), original)
        self.assertEqual(editor.errores_lexicos, [])

    def test_pestanas_independientes_y_sintaxis_aun_no_analizada(self):
        first = self.window.editor()
        first.insertPlainText("@")
        second = self.window.nuevo()
        second.insertPlainText("Vm x : int")  # Falta ;, pero no es un error léxico.
        self.analizado(first)
        self.analizado(second)
        self.assertEqual(len(first.problemas), 1)
        self.assertEqual(second.problemas, [])
        self.assertIs(self.window.editor(), second)
        self.window.ir_problema(self.window.problems.topLevelItem(0), 0)
        self.assertIs(self.window.editor(), first)

    def test_resultado_tardio_se_descarta_y_worker_no_toca_qt(self):
        started, release = threading.Event(), threading.Event()
        self.addCleanup(release.set)
        seen = []

        def delayed(*args):
            seen.append(threading.current_thread() is not threading.main_thread())
            started.set()
            release.wait(2)
            return analizar_documento(*args)

        editor = self.window.editor()
        with patch("ide.interfaz.analisis.analizar_documento", side_effect=delayed):
            editor.insertPlainText("@")
            editor.analisis_timer.stop()
            editor.emitir_analisis()
            self.esperar(started.is_set)
            old_revision = editor.revision
            editor.selectAll(); editor.insertPlainText("Vm x : int = 1 ;")
            release.set()
            self.analizado(editor)
        self.assertGreater(editor.revision, old_revision)
        self.assertEqual(editor.problemas, [])
        self.assertTrue(all(seen))

    def test_cerrar_pestana_durante_analisis_no_aplica_a_otra(self):
        started, release = threading.Event(), threading.Event()
        self.addCleanup(release.set)

        def delayed(*args):
            started.set(); release.wait(2)
            return analizar_documento(*args)

        with patch("ide.interfaz.analisis.analizar_documento", side_effect=delayed):
            editor = self.window.editor()
            editor.insertPlainText("@")
            editor.document().setModified(False)
            editor.analisis_timer.stop(); editor.emitir_analisis()
            self.esperar(started.is_set)
            old_id = editor.documento_id
            self.window.cerrar_tab(0)
            new = self.window.editor()
            release.set()
            self.analizado(new)
        self.assertNotEqual(new.documento_id, old_id)
        self.assertEqual(new.problemas, [])

    @unittest.skipUnless((ROOT / "ccom").exists(), "Construir ccom para la integración")
    def test_ccom_y_nucleo_no_duplican_error_lexico(self):
        path = self.folder / "incorrecto.cls"
        path.write_text("F main [] : void ;\n    @\n    E ;\n.\n")
        editor = self.window.abrir(path)
        self.analizado(editor)
        self.assertEqual(self.window.problems.topLevelItemCount(), 1)
        self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        self.assertEqual(self.window.problems.topLevelItemCount(), 1)
        self.assertEqual(self.window.problems.topLevelItem(0).text(5), "ccom")

    def test_fallo_de_analisis_visible_y_recuperacion(self):
        editor = self.window.editor()
        with patch("ide.interfaz.analisis.analizar_documento", side_effect=ValueError("fallo de prueba")):
            editor.insertPlainText("@")
            self.esperar(lambda: "No disponible" in editor.estado_analisis)
        self.assertIn("fallo de prueba", self.window.analysis_state.text())
        editor.selectAll(); editor.insertPlainText("Vm x : int = 1 ;")
        self.analizado(editor)
        self.assertEqual(editor.estado_analisis, "Sin errores léxicos")

    def test_cierre_aplazado_por_clones_no_desactiva_analisis(self):
        with patch.object(self.window.clones_panel, "detener", return_value=False):
            self.window.close()
        self.assertFalse(self.window.analizador.cerrando)
        editor = self.window.editor()
        editor.insertPlainText("@")
        self.analizado(editor)
        self.assertEqual(len(editor.problemas), 1)


if __name__ == "__main__":
    unittest.main()
