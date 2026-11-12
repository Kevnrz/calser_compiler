"""Regresiones R02: comentarios sin programa y archivos MIME no son código."""
from pathlib import Path
import unittest
from unittest.mock import patch
from PySide6.QtCore import QMimeData, QPoint, QPointF, QUrl, Qt
from PySide6.QtGui import QDragEnterEvent, QDropEvent
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QFileDialog
from ide.pruebas import test_comprobacion as fixtures
from ide import archivos


class ComentariosYArchivos(unittest.TestCase):
    setUp = fixtures.ComprobacionAutomatica.setUp
    cleanup = fixtures.ComprobacionAutomatica.cleanup
    esperar = fixtures.ComprobacionAutomatica.esperar
    escribir = fixtures.ComprobacionAutomatica.escribir

    def mime_archivo(self):
        mime=QMimeData()
        mime.setUrls([QUrl.fromLocalFile(str(self.folder/'nota.cls'))])
        return mime

    def test_comentarios_validos_no_piden_sintaxis_y_limpian_diagnosticos(self):
        editor=self.escribir(fixtures.SEMANTICO)
        self.esperar(lambda:bool(self.window.auto_results.get(editor)))
        consultas=[]
        self.window.comprobador.estado.connect(lambda request,state:consultas.append(state))
        for texto in ('# prueba manual', '# café 😀\n# otro', ' #[ bloque\n café 😀 ]# \n', '# #[ texto de comentario de linea'):
            self.escribir(texto)
            self.esperar(lambda:editor.analisis_revision==editor.revision and editor.estado_ccom=='Sin código para analizar')
            self.assertIs(editor.contiene_codigo,False)
            self.assertFalse(editor.problemas)
            self.assertNotIn(editor,self.window.auto_results)
            self.assertEqual(self.window.problems.topLevelItemCount(),0)
            self.assertEqual(editor.toPlainText(),texto)
        QTest.qWait(50)
        self.assertEqual(consultas,[],'Los comentarios no deben llegar al parser automático')

    def test_comentario_abierto_y_caracter_invalido_siguen_siendo_errores(self):
        for texto in ('#[ sin cerrar\n😀','# comentario\n@'):
            editor=self.escribir(texto)
            self.esperar(lambda:editor.analisis_revision==editor.revision and bool(editor.errores_lexicos))
            self.assertEqual(editor.estado_ccom,'Corrige el léxico')
            self.assertTrue(all(p.categoria=='Léxico' for p in editor.problemas))
        self.escribir('#[ sin cerrar\n😀 ]#')
        self.esperar(lambda:editor.estado_ccom=='Sin código para analizar')
        self.assertFalse(editor.problemas)

    def test_uri_mime_no_se_inserta_ni_arrastra_y_texto_plano_si(self):
        editor=self.escribir('# prueba manual')
        revision=editor.revision
        mime=self.mime_archivo()
        self.assertFalse(editor.canInsertFromMimeData(mime))
        editor.insertFromMimeData(mime)
        drag=QDragEnterEvent(QPoint(10,10),Qt.DropAction.CopyAction,mime,Qt.MouseButton.LeftButton,Qt.KeyboardModifier.NoModifier)
        fixtures.QApplication.sendEvent(editor.viewport(),drag)
        self.assertFalse(drag.isAccepted())
        drop=QDropEvent(QPointF(10,10),Qt.DropAction.CopyAction,mime,Qt.MouseButton.LeftButton,Qt.KeyboardModifier.NoModifier)
        fixtures.QApplication.sendEvent(editor.viewport(),drop)
        self.assertEqual(editor.toPlainText(),'# prueba manual')
        self.assertEqual(editor.revision,revision)
        texto=QMimeData();texto.setText('\n# otra linea')
        self.assertTrue(editor.canInsertFromMimeData(texto))
        editor.insertFromMimeData(texto)
        self.assertEqual(editor.toPlainText(),'# prueba manual\n# otra linea')
        editor.undo();self.assertEqual(editor.toPlainText(),'# prueba manual')

    def test_portapapeles_de_archivo_no_pega_uri_y_texto_si(self):
        clipboard=self.app.clipboard();self.addCleanup(clipboard.clear)
        editor=self.escribir('# prueba manual')
        clipboard.setMimeData(self.mime_archivo())
        editor.paste()
        self.assertEqual(editor.toPlainText(),'# prueba manual')
        self.assertTrue(clipboard.mimeData().hasUrls(),'No destruir portapapeles de archivos')
        clipboard.setText('file:///ruta/pegada/como/texto')
        editor.paste()
        self.assertEqual(editor.toPlainText(),'# prueba manualfile:///ruta/pegada/como/texto')
        editor.undo();self.assertEqual(editor.toPlainText(),'# prueba manual')

    def test_guardar_comentario_no_modifica_texto_y_descarta_uri_posterior(self):
        editor=self.escribir('# prueba manual')
        path=self.folder/'nota con espacios.cls'
        with patch.object(QFileDialog,'getSaveFileName',return_value=(str(path),'Calser (*.cls)')):
            self.assertTrue(self.window.guardar())
        self.assertEqual(path.read_text(),'# prueba manual')
        self.assertEqual(editor.toPlainText(),'# prueba manual')
        self.assertFalse(editor.document().isModified())
        editor.insertFromMimeData(self.mime_archivo())
        self.assertEqual(editor.toPlainText(),'# prueba manual')
        self.assertFalse(editor.document().isModified())
        self.esperar(lambda:editor.estado_ccom=='Sin código para analizar')
        self.assertFalse(editor.problemas)

    def test_f5_sin_codigo_informa_sin_marcar_error_y_flex_no_oculta_errores(self):
        path=self.folder/'comentario.cls'
        self.window.seleccionar_modo('analizar')
        for texto in ('# prueba manual', '#[ comentario\nmultilínea ]#', '  \n'):
            path.write_text(texto)
            editor=self.window.abrir(path)
            editor.setPlainText(texto)
            editor.document().setModified(False)
            self.window.establecer_version(editor, archivos.estado(path))
            self.window.compilar()  # F5 inmediato: no depender del debounce.
            self.esperar(lambda:'no hay código para compilar' in self.window.output.toPlainText())
            self.assertIsNone(self.window.process)
            self.assertFalse(self.window.results.get(editor))
            self.assertFalse(editor.problemas)
            self.assertNotIn('fin de archivo', self.window.output.toPlainText())
            self.assertEqual(path.read_text(),texto)
            self.assertFalse(list(self.folder.rglob('*.bin')))
            self.window.output.clear()
        self.escribir('#[ comentario sin cerrar')
        self.window.compilar()
        self.esperar(lambda:bool(self.window.results.get(editor)))
        self.assertTrue(editor.errores_lexicos)
        self.assertNotIn('no hay código para compilar',self.window.output.toPlainText())
        self.esperar(lambda:self.window.process is None)
        self.escribir(fixtures.VALIDO)
        self.window.compilar()
        self.esperar(lambda:'Análisis correcto' in self.window.output.toPlainText())
        self.esperar(lambda:self.window.process is None)
        self.assertFalse(self.window.results.get(editor))



if __name__=='__main__':unittest.main()
