"""Pruebas de archivos, navegación y proceso real sin servidor gráfico."""
import os
import sys
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from PySide6.QtWidgets import QApplication, QMessageBox
from PySide6.QtCore import Qt
from PySide6.QtTest import QTest

from ide import archivos, ccom
from ide.interfaz.ventana import Ventana

ROOT = Path(__file__).resolve().parents[2]
APP = QApplication.instance() or QApplication([])


class Prototipo(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder = Path(self.directory.name)
        self.window = Ventana(ROOT, preferencias=False)
        self.addCleanup(self.limpiar)

    def limpiar(self):
        for terminal in self.window.terminals:
            terminal.detener()
        self.window.terminals.clear()
        if self.window.process:
            self.window.cancelar()
            self.window.process.waitForFinished(3000)
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        APP.processEvents()

    def fuente(self, name="programa.cls", text="F main [] : void ;\n    E ;\n.\n"):
        path = self.folder / name
        archivos.guardar(path, text)
        return path

    def esperar(self):
        deadline = time.monotonic() + 10
        while self.window.process and time.monotonic() < deadline:
            APP.processEvents()
            time.sleep(.005)
        self.assertIsNone(self.window.process, "ccom no terminó")

    def test_archivos_unicode_y_saltos(self):
        path = self.fuente("tildes con espacios.cls", "# café 😀\r\nF main [] : void ;\r\nE ;\r\n.\r\n")
        self.assertIn("café 😀\n", archivos.leer(path))
        self.assertNotIn(b"\r", path.read_bytes())

    def test_pestanas_guardado_y_cancelar_cierre(self):
        path = self.fuente()
        editor = self.window.abrir(path)
        self.assertIs(editor, self.window.abrir(path.parent / "." / path.name))
        self.assertEqual(self.window.tabs.count(), 1)
        editor.insertPlainText("# cambio\n")
        self.assertTrue(editor.document().isModified())
        with patch.object(QMessageBox, "question", return_value=QMessageBox.StandardButton.Cancel):
            self.window.cerrar_tab(0)
        self.assertEqual(self.window.tabs.count(), 1)
        self.assertTrue(self.window.guardar())
        self.assertIn("# cambio", archivos.leer(path))
        self.assertFalse(editor.document().isModified())

    def test_guardado_fallido_conserva_estado(self):
        editor = self.window.abrir(self.fuente())
        editor.insertPlainText("# edición\n")
        with patch("ide.archivos.guardar", side_effect=PermissionError("sin acceso")), patch.object(self.window, "error"):
            self.assertFalse(self.window.guardar())
        self.assertTrue(editor.document().isModified())
        self.assertIn("edición", editor.toPlainText())

    def test_diagnosticos_rutas_y_mensajes_sin_posicion(self):
        text = "C:/mis fuentes/main.cls:3:12: error: falta ;\ntexto explicativo\nccom:5:2: sin registros\n"
        problems = ccom.diagnosticos(text, self.folder / "main.cls", self.folder)
        self.assertEqual(len(problems), 2)
        self.assertEqual(problems[0].linea, 3)
        self.assertTrue(str(problems[0].archivo).endswith("C:/mis fuentes/main.cls"))
        self.assertEqual(problems[1].archivo, self.folder / "main.cls")

    def test_diagnosticos_generales_no_capturan_resumenes_o_fragmentos(self):
        path = self.folder / 'fuente con espacios.cls'
        text = (f"{path}: error semantico: el archivo principal debe declarar 'main'\n"
                "Análisis semántico fallido: 1 error(es).\n"
                "     1 | # error: comentario\n"
                "mensaje explicativo\n"
                "ccom: error: no se pudo completar la operación\n")
        problems = ccom.diagnosticos(text, path, self.folder)
        self.assertEqual(len(problems), 2)
        self.assertEqual(problems[0].archivo, path)
        self.assertEqual(problems[0].categoria, 'Semántico')
        self.assertIsNone(problems[0].linea)
        self.assertIsNone(problems[0].columna)
        self.assertFalse(problems[0].tiene_posicion)
        self.assertEqual(problems[1].archivo, path)

    def test_problema_general_abre_archivo_sin_mover_cursor_ni_subrayar(self):
        path = self.fuente()
        editor = self.window.abrir(path)
        cursor = editor.textCursor()
        cursor.setPosition(3)
        editor.setTextCursor(cursor)
        problem = ccom.Problema(path, None, None, "error semantico: falta main")
        self.window.results[editor] = [problem]
        self.window.auto_results[editor] = [problem]
        self.window.actualizar_problemas()
        self.assertEqual(self.window.problems.topLevelItemCount(), 1)
        item = self.window.problems.topLevelItem(0)
        self.assertEqual((item.text(2), item.text(3)), ('—', '—'))
        self.assertIsNone(editor.rango_problema(problem))
        self.assertEqual(len(editor.extraSelections()), 1)  # Solo línea del cursor.
        self.assertIn('falta main', self.window.diagnostic_message.text())
        self.window.nuevo()
        self.window.ir_problema(item, 0)
        self.assertIs(self.window.editor(), editor)
        self.assertEqual(editor.textCursor().position(), 3)
        self.assertIn('falta main', self.window.diagnostic_message.text())

    def test_navegacion_columna_utf8(self):
        path = self.fuente(text="# café 😀 dato\n")
        from PySide6.QtCore import Qt
        from PySide6.QtWidgets import QTreeWidgetItem
        problem = ccom.Problema(path, 1, len("# café 😀 ".encode()) + 1, "ejemplo")
        item = QTreeWidgetItem(["ejemplo"])
        item.setData(0, Qt.ItemDataRole.UserRole, problem)
        self.window.ir_problema(item, 0)
        self.assertEqual(self.window.editor().textCursor().selectionStart(), len("# café 😀 ".encode("utf-16-le")) // 2)
        self.assertEqual(self.window.editor().textCursor().selectedText(), "d")

    def test_diagnostico_longitud_categoria_y_unicode(self):
        path = self.fuente(text="# café 😀 variable\n")
        column = len("# café 😀 ".encode()) + 1
        text = f"{path}:1:{column}: error semantico: variable inválida\n     1 | # café 😀 variable\n       |          ^^^^^^^^\n"
        problem = ccom.diagnosticos(text, path, self.folder)[0]
        self.assertEqual(problem.longitud, 8)
        self.assertEqual(problem.categoria, "Semántico")
        editor = self.window.abrir(path)
        editor.establecer_problemas([problem])
        self.assertEqual(editor.rango_problema(problem).selectedText(), "variable")
        self.assertEqual(len(editor.extraSelections()), 2)
        editor.moveCursor(editor.textCursor().MoveOperation.End)
        self.assertEqual(len(editor.extraSelections()), 2, "Mover el cursor no debe borrar las marcas")

    def test_error_al_final_de_linea_y_linea_vacia(self):
        editor = self.window.abrir(self.fuente(text="Vm x : int\n\n"))
        for row, column in [(1, 11), (2, 1), (1, 999)]:
            problem = ccom.Problema(editor.path, row, column, "error sintactico: falta ;")
            editor.establecer_problemas([problem])
            cursor = editor.rango_problema(problem)
            self.assertFalse(cursor.hasSelection())
            self.assertEqual(cursor.blockNumber(), row - 1)
            self.assertEqual(len(editor.extraSelections()), 2)
        invalid = ccom.Problema(editor.path, 999, 1, "error: sin fuente")
        self.assertIsNone(editor.rango_problema(invalid))

    def test_problemas_independientes_y_descripcion_visible(self):
        first = self.window.abrir(self.fuente("uno.cls", "x = 1 ;\n"))
        second = self.window.abrir(self.fuente("dos.cls", "y = 1 ;\n"))
        self.window.results[first] = [ccom.Problema(first.path, 1, 1, "error semantico: x no declarado")]
        self.window.results[second] = [ccom.Problema(second.path, 1, 1, "error semantico: y no declarado")]
        self.window.actualizar_problemas()
        self.assertEqual(self.window.problems.topLevelItemCount(), 2)
        self.assertIn("y no declarado", self.window.diagnostic_message.text())
        self.assertFalse(self.window.diagnostic_message.isHidden())
        self.window.tabs.setCurrentWidget(first)
        self.assertIn("x no declarado", self.window.diagnostic_message.text())
        first.insertPlainText("# cambio\n")
        self.assertEqual(first.problemas, [])
        self.assertEqual(len(second.problemas), 1)
        self.assertEqual(self.window.problems.topLevelItemCount(), 1)
        self.assertTrue(self.window.diagnostic_message.isHidden())

    def test_copiar_consolas_usa_foco_y_ambos_atajos(self):
        self.window.show()
        self.window.activateWindow()
        editor = self.window.editor()
        editor.insertPlainText('# texto del editor')
        editor.selectAll()
        for consola in (self.window.output, self.window.auto_output):
            self.window.panels.setCurrentWidget(consola)
            consola.setPlainText('resultado = 5')
            consola.selectAll()
            consola.setFocus()
            APP.processEvents()
            for modificadores in (Qt.KeyboardModifier.ControlModifier,
                                  Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier):
                APP.clipboard().setText('anterior')
                QTest.keyClick(consola, Qt.Key.Key_C, modificadores)
                self.assertEqual(APP.clipboard().text(), 'resultado = 5')
            self.assertEqual(editor.toPlainText(), '# texto del editor')
        editor.setFocus()
        editor.selectAll()
        QTest.keyClick(editor, Qt.Key.Key_C, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(APP.clipboard().text(), '# texto del editor')

    @unittest.skipUnless(sys.platform == 'linux', 'PTY Linux')
    def test_copiar_pegar_terminal_y_ctrl_c_sin_seleccion(self):
        self.window.show()
        self.window.activateWindow()
        editor = self.window.editor()
        editor.insertPlainText('# conservar')
        self.window.nueva_terminal()
        terminal = self.window.terminals[-1]
        limite = time.monotonic() + 5
        while 'calser:' not in terminal.screen.toPlainText() and time.monotonic() < limite:
            APP.processEvents()
            time.sleep(.005)
        self.assertIn('calser:', terminal.screen.toPlainText())
        terminal.screen.setPlainText('resultado = 5')
        terminal.screen.selectAll()
        terminal.screen.setFocus()
        APP.processEvents()
        QTest.keyClick(terminal.screen, Qt.Key.Key_C,
                       Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier)
        self.assertEqual(APP.clipboard().text(), 'resultado = 5')
        terminal.input.setFocus()
        terminal.input.setText('comando')
        terminal.input.selectAll()
        QTest.keyClick(terminal.input, Qt.Key.Key_C, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(APP.clipboard().text(), 'comando')
        terminal.input.clear()
        APP.clipboard().setText('printf hola')
        QTest.keyClick(terminal.input, Qt.Key.Key_V, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(terminal.input.text(), 'printf hola')
        self.assertEqual(editor.toPlainText(), '# conservar')
        terminal.input.deselect()
        with patch.object(terminal, 'escribir') as escribir:
            QTest.keyClick(terminal.input, Qt.Key.Key_C, Qt.KeyboardModifier.ControlModifier)
            escribir.assert_called_once_with(b'\x03')
        self.assertEqual(APP.clipboard().text(), 'printf hola')

    def test_panel_movil_y_explorador(self):
        from PySide6.QtCore import Qt
        self.window.show()
        APP.processEvents()
        editor = self.window.editor()
        editor.insertPlainText("# contenido que debe conservarse")
        self.window.files_button.setChecked(False)
        self.assertTrue(self.window.sidebar.isHidden())
        self.window.files_button.setChecked(True)
        self.assertFalse(self.window.sidebar.isHidden())
        self.window.mover_panel()
        self.assertEqual(self.window.workspace_splitter.orientation(), Qt.Orientation.Horizontal)
        self.window.mover_panel()
        self.assertEqual(self.window.workspace_splitter.orientation(), Qt.Orientation.Vertical)
        self.assertIn("conservarse", editor.toPlainText())
        menu_y = self.window.menus.mapTo(self.window.centralWidget(), self.window.menus.rect().center()).y()
        self.assertAlmostEqual(menu_y, 44 + 24, delta=2)

    @unittest.skipUnless(sys.platform == "linux", "PTY Linux")
    def test_terminales_sesiones_independientes_e_interrupcion(self):
        self.window.project = self.folder
        subdir = self.folder / "subcarpeta"
        subdir.mkdir()
        self.window.nueva_terminal()
        self.window.nueva_terminal()
        first, second = self.window.terminals

        def wait_for(predicate):
            deadline = time.monotonic() + 6
            while not predicate() and time.monotonic() < deadline:
                APP.processEvents()
                time.sleep(.01)
            self.assertTrue(predicate())

        wait_for(lambda: "calser:" in first.screen.toPlainText() and "calser:" in second.screen.toPlainText())
        first.input.setText("cd subcarpeta")
        first.enviar()
        first.input.setText("pwd")
        first.enviar()
        second.input.setText("pwd")
        second.enviar()
        wait_for(lambda: str(subdir) in first.screen.toPlainText() and str(self.folder) in second.screen.toPlainText())
        self.assertNotIn(str(subdir), second.screen.toPlainText())
        first.input.setText("sleep 30")
        first.enviar()
        wait_for(lambda: "sleep 30" in first.screen.toPlainText())
        first.escribir(b"\x03")
        first.input.setText("printf 'INTERRUPCION_OK\\n'")
        first.enviar()
        wait_for(lambda: "\nINTERRUPCION_OK\n" in first.screen.toPlainText())
        first.detener()
        self.assertFalse(first.activa())
        self.assertTrue(second.activa())

    @unittest.skipUnless((ROOT / "ccom").exists(), "Construir ccom con make para la integración")
    def test_compilacion_real_y_errores(self):
        path = self.fuente()
        self.window.abrir(path)
        self.window.compilar()
        self.esperar()
        self.assertIn("Compilación correcta", self.window.output.toPlainText())
        self.assertTrue(list((self.folder / "build/ide").rglob("*.bin")))
        bad = self.fuente("incorrecto.cls", "F main [] : void ;\n    x = 1 ;\n    E ;\n.\n")
        self.window.abrir(bad)
        self.window.compilar()
        self.esperar()
        self.assertIn("Compilación fallida", self.window.output.toPlainText())
        self.assertGreater(self.window.problems.topLevelItemCount(), 0)
        self.window.ir_problema(self.window.problems.topLevelItem(0), 0)
        self.assertEqual(self.window.editor().path, bad)
        self.assertEqual(len(self.window.editor().problemas), self.window.problems.topLevelItemCount())
        self.assertIn("ccom · Semántico", self.window.diagnostic_message.text())
        self.assertEqual(self.window.editor().textCursor().selectedText(), "x")
        self.window.editor().selectAll()
        self.window.editor().insertPlainText("F main [] : void ;\n    E ;\n.\n")
        self.assertEqual(self.window.editor().problemas, [])
        self.window.compilar()
        self.esperar()
        self.assertEqual(self.window.problems.topLevelItemCount(), 0)
        self.assertEqual(self.window.editor().problemas, [])
        self.assertIn("Compilación correcta", self.window.output.toPlainText())

    @unittest.skipUnless((ROOT / "ccom").exists(), "Construir ccom con make para la integración")
    def test_corpus_real_lexico_sintactico_semantico(self):
        cases = [
            ("e01_no_declarado.cls", "Semántico", 1),
            ("e16_falta_punto_coma.cls", "Sintáctico", 1),
            ("e52_caracter_invalido.cls", "Léxico", 1),
            ("e55_comentario_abierto.cls", "Léxico", 1),
            ("e73_parentesis_sin_cerrar.cls", "Sintáctico", 1),
            ("e15_varios_errores.cls", "Semántico", 3),
        ]
        for name, category, count in cases:
            with self.subTest(programa=name):
                path = self.fuente(name, (ROOT / "pruebas" / name).read_text())
                editor = self.window.abrir(path)
                self.window.compilar()
                self.esperar()
                self.assertEqual(len(editor.problemas), count)
                self.assertTrue(all(p.categoria == category for p in editor.problemas))
                self.assertTrue(all(editor.rango_problema(p) is not None for p in editor.problemas))
                if name == "e55_comentario_abierto.cls":
                    self.assertEqual(editor.rango_problema(editor.problemas[0]).selectedText(), "#[")

    @unittest.skipUnless((ROOT / "ccom").exists(), "Construir ccom con make para la integración")
    def test_resultado_anterior_no_se_aplica(self):
        self.window.abrir(self.fuente(text="F main [] : void ;\nx = 1 ;\nE ;\n.\n"))
        self.window.compilar()
        self.window.editor().insertPlainText("# cambio\n")
        self.esperar()
        self.assertEqual(self.window.problems.topLevelItemCount(), 0)
        self.assertIn("versión anterior", self.window.output.toPlainText())


if __name__ == "__main__":
    unittest.main()
