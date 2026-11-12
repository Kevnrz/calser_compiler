"""Acceso contextual al mismo motor C de sugerencias, sin perder edición estándar."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import unittest
from unittest.mock import patch
from PySide6.QtCore import QTimer
from PySide6.QtGui import QContextMenuEvent, QTextCursor
from ide.pruebas import test_carpetas as fixtures


class MenuEditor(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def menu(self):
        menu = self.window.editor().crear_menu_contextual()
        self.addCleanup(menu.deleteLater)
        return menu

    def sugerencias(self, menu):
        return next(a for a in menu.actions() if a.text().startswith('Sugerencias de código'))

    def test_conserva_menu_estandar_y_agrega_atajo(self):
        menu=self.menu()
        self.assertGreater(len(menu.actions()), 5)
        accion=self.sugerencias(menu)
        self.assertIn('Ctrl+Espacio', accion.text())
        self.assertTrue(accion.isEnabled())

    def test_respeta_preferencia_lectura_y_asm_sin_desactivar_global(self):
        editor=self.window.editor()
        self.window.asistencia.configurar({'habilitada': False})
        self.assertFalse(self.sugerencias(self.menu()).isEnabled())
        self.window.asistencia.configurar({'habilitada': True})
        editor.setReadOnly(True)
        self.assertFalse(self.sugerencias(self.menu()).isEnabled())
        editor.setReadOnly(False)
        editor.path=Path('/tmp/entrada.asm')
        self.assertFalse(self.sugerencias(self.menu()).isEnabled())
        self.assertTrue(self.window.sugerencias_action.isEnabled())
        editor.path=None

    def test_clic_derecho_consulta_real_y_no_modifica_texto(self):
        self.window.show(); self.window.activateWindow()
        editor=self.window.editor();editor.setPlainText('F main [] : vo')
        cursor=editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End)
        editor.setTextCursor(cursor);editor.setFocus();fixtures.APP.processEvents()
        menu=editor.crear_menu_contextual()
        punto=editor.cursorRect().center()
        accion=self.sugerencias(menu)
        def activar():
            accion.trigger()
            menu.close()
        QTimer.singleShot(0,activar)
        with patch.object(editor,'crear_menu_contextual',return_value=menu):
            editor.contextMenuEvent(QContextMenuEvent(QContextMenuEvent.Reason.Mouse,punto,editor.mapToGlobal(punto)))
        self.esperar(lambda:self.window.asistencia.popup.isVisible())
        self.assertEqual(editor.toPlainText(),'F main [] : vo')
        opcion=self.window.asistencia.popup.lista.item(0).data(256)
        self.assertEqual(opcion.texto,'void')

    def test_menu_por_teclado_conserva_cursor(self):
        editor=self.window.editor();editor.setPlainText('F main [] : vo')
        cursor=editor.textCursor();cursor.setPosition(8);editor.setTextCursor(cursor)
        menu=self.menu()
        QTimer.singleShot(0,menu.close)
        punto=editor.cursorRect().center()
        with patch.object(editor,'crear_menu_contextual',return_value=menu):
            editor.contextMenuEvent(QContextMenuEvent(QContextMenuEvent.Reason.Keyboard,punto,editor.mapToGlobal(punto)))
        self.assertEqual(editor.textCursor().position(),8)
