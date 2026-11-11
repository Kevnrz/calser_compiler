"""Recorrido gráfico focalizado con ccom real: gris, Tab, cancelación y cierres."""
from pathlib import Path
import unittest
from PySide6.QtCore import Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide.pruebas import test_carpetas as base
from ide.interfaz.preferencias_asistencia import PreferenciasAsistencia


class SugerenciaInline(unittest.TestCase):
    setUp = base.Carpetas.setUp
    limpiar = base.Carpetas.limpiar
    esperar = base.Carpetas.esperar
    terminar = base.Carpetas.terminar

    def preparar(self, texto, posicion=None):
        self.editor = self.window.editor()
        self.editor.setPlainText(texto)
        cursor = self.editor.textCursor()
        indice = len(texto) if posicion is None else posicion
        cursor.setPosition(len(texto[:indice].encode('utf-16-le')) // 2)
        self.editor.setTextCursor(cursor)
        self.editor.document().setModified(False)
        self.window.show(); self.window.activateWindow()
        self.editor.setFocus(); QTest.qWait(20)
        self.control = self.window.asistencia
        self.control.configurar({**self.control.configuracion, 'sugerencias_inline': True, 'pausa_ms': 300})

    def gris(self):
        self.esperar(lambda:self.editor.sugerencia_inline.isVisible())

    def test_completa_en_gris_tab_y_undo_sin_modificar_preview(self):
        self.preparar('# café 😀\nF main [] : v')
        QTest.keyClicks(self.editor, 'o')
        contexto = self.editor.contexto_asistencia()
        self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, 'id')
        QTest.keyClick(self.editor, Qt.Key.Key_BracketRight, Qt.KeyboardModifier.AltModifier)
        self.assertTrue(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(), contexto)
        self.assertFalse(self.control.popup.isVisible())
        self.window.grab().save('/tmp/calside-sugerencia-inline.png')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertTrue(self.editor.toPlainText().endswith('void'))
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(self.editor.toPlainText(), contexto.texto)

    def test_escape_enter_y_foco_descartan_sin_aceptar(self):
        self.preparar('F main [] : v')
        for tecla in (Qt.Key.Key_Escape, Qt.Key.Key_Return):
            self.editor.setPlainText('F main [] : v')
            cursor = self.editor.textCursor(); cursor.movePosition(QTextCursor.MoveOperation.End); self.editor.setTextCursor(cursor)
            QTest.keyClicks(self.editor, 'o'); self.gris()
            QTest.keyClick(self.editor, tecla)
            self.assertFalse(self.editor.sugerencia_inline.isVisible())
            self.assertNotIn('void', self.editor.toPlainText())
        self.editor.setPlainText('F main [] : v')
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        QTest.keyClicks(self.editor, 'o'); self.gris()
        self.window.busqueda_proyecto.campo.setFocus(); QTest.qWait(20)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())

    def test_comentario_y_cambio_de_pestana_no_insertan_propuestas(self):
        self.preparar('# nota')
        QTest.keyClicks(self.editor, ' vo')
        QTest.qWait(500)
        self.esperar(lambda:self.control.coordinador.task is None)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.toPlainText(), '# nota vo')
        self.editor.setPlainText('F main [] : v')
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        QTest.keyClicks(self.editor, 'o')
        self.window.nuevo(); QTest.qWait(500)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.window.editor().toPlainText(), '')

    def test_reparacion_punto_y_coma_inline_sin_panel_ni_guardado(self):
        texto = 'F main [] : void ;\n    Vm x : int = \n    E ;\n.\n'
        self.preparar(texto, texto.index('\n', texto.index('Vm')))
        QTest.keyClicks(self.editor, '3')
        antes = self.editor.toPlainText()
        self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, ';')
        self.assertEqual(self.editor.toPlainText(), antes)
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertIn('3;', self.editor.toPlainText())
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), antes)
        self.assertFalse(list(self.folder.rglob('*.cls')))

    def test_auto_enter_regla_real_con_una_activacion_y_deshacer(self):
        texto = 'F main [] : void ;\n    Vm x : int = 3\n    E ;\n.\n'
        self.preparar(texto, texto.index('\n', texto.index('Vm')))
        dialogo=PreferenciasAsistencia(self.control.configuracion, self.control.reglas, self.window)
        dialogo.automaticas.setChecked(True)
        datos=dialogo.datos(); self.assertEqual(datos['reglas_automaticas'], ['INSERCION_PUNTO_COMA'])
        self.control.configurar(datos);dialogo.deleteLater()
        QTest.keyClick(self.editor, Qt.Key.Key_Return)
        antes=self.editor.toPlainText()
        self.esperar(lambda:'3;' in self.editor.toPlainText())
        self.editor.undo();self.assertEqual(self.editor.toPlainText(), antes)
        QTest.qWait(450);self.assertEqual(self.editor.toPlainText(), antes)

    def test_manual_ctrl_espacio_conserva_lista_y_tab(self):
        self.preparar('F main [] : vo')
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda:self.control.popup.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(),'F main [] : void')

    def test_diagnostico_tardio_no_cancela_gris_y_tab_sigue_funcionando(self):
        self.preparar('F main [] : v')
        self.window.auto_action.setChecked(True)
        self.window.comprobador.demora = 800
        QTest.keyClicks(self.editor, 'o')
        self.gris()
        contexto = self.editor.contexto_asistencia()
        self.esperar(lambda:bool(self.window.auto_results.get(self.editor)))
        QTest.qWait(50)  # Procesar el cambio de tamaño al aparecer el aviso.
        self.assertTrue(self.window.diagnostic_message.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(), contexto)
        self.assertTrue(self.editor.sugerencia_inline.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F main [] : void')

    def test_main_completo_propone_corchetes_y_tab_deja_cursor_dentro(self):
        self.preparar('F mai')
        QTest.keyClicks(self.editor, 'n')
        self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, '[]')
        self.assertEqual(self.editor.toPlainText(), 'F main')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F main[]')
        self.assertEqual(self.editor.textCursor().position(), len('F main['))
        self.editor.undo();self.assertEqual(self.editor.toPlainText(), 'F main')

    def test_linea_vacia_no_inventa_C_y_prefijo_V_si_completa(self):
        self.preparar('F main[]: void;\n   ')
        QTest.keyClicks(self.editor, ' ')
        self.esperar(lambda:not self.control.timer_pausa.isActive())
        self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.toPlainText(), 'F main[]: void;\n    ')
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda:self.control.popup.isVisible())
        self.assertIn('C', [self.control.popup.lista.item(i).data(Qt.ItemDataRole.UserRole).texto
                            for i in range(self.control.popup.lista.count())])
        QTest.keyClick(self.editor, Qt.Key.Key_Escape)
        QTest.keyClicks(self.editor, 'V')
        self.gris()
        self.assertIn(self.editor.sugerencia_inline.texto, ('i','m'))
