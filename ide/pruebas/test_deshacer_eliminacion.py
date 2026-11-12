"""Restauración de papelera aislada, buffers y atajos de la interfaz real."""
from pathlib import Path
import time
import unittest
from unittest.mock import patch

from PySide6.QtCore import QFile, Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QMessageBox

from ide.pruebas import test_menu_archivos as base
APP, TEXTO = base.APP, base.TEXTO


class DeshacerEliminacion(unittest.TestCase):
    setUp = base.MenuArchivos.setUp
    limpiar = base.MenuArchivos.limpiar

    def papelera(self, path):
        """Mover de verdad en el directorio temporal, sin tocar la papelera personal."""
        path = Path(path)
        destino = self.folder / 'papelera de prueba' / path.name
        destino.parent.mkdir(exist_ok=True)
        path.rename(destino)
        return True, str(destino)

    def eliminar(self, path=None):
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes), \
             patch('ide.interfaz.papelera.enviar_a_papelera', side_effect=self.papelera):
            self.assertTrue(self.window.eliminar_archivo(path or self.path))

    def test_click_derecho_editor_y_explorador_ofrecen_eliminar(self):
        for menu in (self.editor.crear_menu_contextual(), self.window.crear_menu_archivo(self.path)):
            acciones = {a.text(): a for a in menu.actions()}
            self.assertTrue(acciones['Eliminar archivo…'].isEnabled())
            menu.deleteLater()
        self.window.nuevo()
        menu = self.window.editor().crear_menu_contextual()
        self.assertFalse(next(a for a in menu.actions() if a.text() == 'Eliminar archivo…').isEnabled())
        menu.deleteLater()

    def test_ctrl_z_en_explorador_restaura_archivo_y_pestana_limpia(self):
        self.window.show()
        self.window.activateWindow()
        APP.processEvents()
        self.eliminar()
        self.assertFalse(self.path.exists())
        self.assertTrue(self.editor.document().isModified())
        self.assertTrue(self.window.tree.hasFocus())
        QTest.keyClick(self.window.tree, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
        APP.processEvents()
        self.assertEqual(self.path.read_text(), TEXTO)
        self.assertFalse(self.editor.document().isModified())
        self.assertFalse(self.window.tiene_cambio_externo(self.editor))
        self.assertEqual(self.window._eliminaciones, [])

    def test_buffer_modificado_y_su_historial_se_conservan(self):
        self.editor.insertPlainText('# pendiente\n')
        texto = self.editor.toPlainText()
        self.eliminar()
        self.editor.insertPlainText('# posterior\n')
        posterior = self.editor.toPlainText()
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertEqual(self.path.read_text(), TEXTO)
        self.assertEqual(self.editor.toPlainText(), posterior)
        self.assertTrue(self.editor.document().isModified())
        self.editor.undo()
        self.assertNotEqual(self.editor.toPlainText(), posterior)
        self.assertTrue(self.editor.document().isModified())
        self.assertIn('pendiente', texto)

    def test_ctrl_z_en_codigo_sigue_deshaciendo_texto(self):
        self.window.show()
        self.window.activateWindow()
        self.eliminar()
        self.editor.setFocus()
        self.editor.insertPlainText('# escritura\n')
        APP.processEvents()
        QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
        self.assertNotIn('escritura', self.editor.toPlainText())
        self.assertFalse(self.path.exists())
        self.assertTrue(self.window._eliminaciones)

    def test_no_sobrescribe_destino_y_permite_reintentar(self):
        self.eliminar()
        self.path.write_text('archivo nuevo ajeno')
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.deshacer_eliminacion())
        self.assertEqual(self.path.read_text(), 'archivo nuevo ajeno')
        self.assertTrue(self.window._eliminaciones)
        self.path.rename(self.folder / 'ajeno.cls')
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertEqual(self.path.read_text(), TEXTO)

    def test_no_recrea_carpeta_padre_ni_sigue_sustitucion_por_enlace(self):
        sub = self.folder / 'sub'; sub.mkdir()
        destino = sub / 'uno.cls'
        self.assertTrue(self.window.mover_archivo(self.path, destino))
        self.eliminar(destino)
        sub.rmdir()
        otra = self.folder / 'otra'; otra.mkdir()
        sub.symlink_to(otra, target_is_directory=True)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.deshacer_eliminacion())
        self.assertEqual(list(otra.iterdir()), [])
        sub.unlink(); sub.mkdir()
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertEqual(destino.read_text(), TEXTO)

    def test_no_oculta_cambios_externos_previos(self):
        self.path.write_text('# externo\n' + TEXTO)
        self.eliminar()
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertTrue(self.window.tiene_cambio_externo(self.editor))
        self.assertEqual(self.editor.toPlainText(), TEXTO)
        self.assertTrue(self.editor.document().isModified())

    def test_seleccion_multiple_se_restaura_con_un_solo_paso(self):
        segundo = self.folder / 'dos.cls'; segundo.write_text('# segundo')
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes), \
             patch('ide.interfaz.papelera.enviar_a_papelera', side_effect=self.papelera):
            self.assertTrue(self.window.eliminar_seleccion([self.path, segundo]))
        self.assertEqual(len(self.window._eliminaciones), 1)
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertEqual(self.path.read_text(), TEXTO)
        self.assertEqual(segundo.read_text(), '# segundo')

    def test_carpeta_con_subcarpetas_y_buffer_abierto(self):
        carpeta = self.folder / 'programas'
        sub = carpeta / 'sub'; sub.mkdir(parents=True)
        path = sub / 'dos.cls'; path.write_text(TEXTO)
        editor = self.window.abrir(path)
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes), \
             patch('ide.interfaz.papelera.enviar_a_papelera', side_effect=self.papelera):
            self.assertTrue(self.window.eliminar_archivo(carpeta))
            limite = time.monotonic() + 5
            while self.window.operacion_carpetas is not None and time.monotonic() < limite:
                APP.processEvents(); QTest.qWait(5)
            self.assertIsNone(self.window.operacion_carpetas)
        self.assertFalse(carpeta.exists())
        self.assertTrue(self.window.deshacer_eliminacion())
        self.assertEqual(path.read_text(), TEXTO)
        self.assertEqual(editor.path, path)
        self.assertFalse(editor.document().isModified())

    def test_error_de_papelera_no_crea_historial_y_restauracion_fallida_no_pierde_archivo(self):
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes), \
             patch('ide.interfaz.papelera.enviar_a_papelera', return_value=(False, '')), patch.object(self.window, 'error'):
            self.assertFalse(self.window.eliminar_archivo(self.path))
        self.assertFalse(getattr(self.window, '_eliminaciones', []))
        self.eliminar()
        recuperable = self.window._eliminaciones[-1][-1]['papelera']
        with patch('ide.interfaz.historial_archivos.QFile') as archivo, patch.object(self.window, 'error'):
            archivo.return_value.rename.return_value = False
            self.assertFalse(self.window.deshacer_eliminacion())
        self.assertEqual(recuperable.read_text(), TEXTO)
        self.assertFalse(self.path.exists())
        self.assertTrue(self.window.deshacer_eliminacion())

    def test_papelera_real_y_ctrl_z_desde_editor_sin_cambiar_foco(self):
        """Regresión del overload real de PySide: no simular moveToTrash."""
        self.window.show()
        self.window.activateWindow()
        APP.processEvents()
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes):
            self.assertTrue(self.window.eliminar_archivo(self.path))
        try:
            self.assertFalse(self.path.exists())
            self.assertTrue(self.window._eliminaciones)
            self.assertFalse(self.window.boton_recargar_externo.isEnabled())
            with patch.object(self.window, 'error') as error:
                self.assertFalse(self.window.recargar())
                self.assertIn('ya no existe', error.call_args.args[1])
            self.editor.setFocus()
            APP.processEvents()
            QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
            self.assertEqual(self.path.read_text(), TEXTO)
            self.editor.insertPlainText('# escritura\n')
            QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
            self.assertEqual(self.editor.toPlainText(), TEXTO)
        finally:
            if not self.path.exists():
                self.window.deshacer_eliminacion()
