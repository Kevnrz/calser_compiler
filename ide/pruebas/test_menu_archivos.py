"""Menú del explorador: archivos, portapapeles y conservación de buffers."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from PySide6.QtCore import QFile, QPoint, QUrl
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import QApplication, QInputDialog, QMessageBox

from ide import archivos
from ide.interfaz.acciones_archivos import archivo_portapapeles
from ide.interfaz.ventana import Ventana

ROOT=Path(__file__).resolve().parents[2]
APP=QApplication.instance() or QApplication([])
TEXTO='F main [] : void ; E ; .\n'


class MenuArchivos(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder=Path(self.tmp.name)
        self.path=self.folder/'uno.cls'
        self.path.write_text(TEXTO)
        self.window=Ventana(ROOT, preferencias=False)
        self.window.auto_action.setChecked(False)
        self.window.monitor.timer.stop()
        self.editor=self.window.abrir(self.path)
        self.addCleanup(self.limpiar)

    def limpiar(self):
        APP.clipboard().clear()
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        APP.processEvents()

    def test_menu_archivo_y_fondo(self):
        menu=self.window.crear_menu_archivo(self.path)
        acciones={a.text():a for a in menu.actions()}
        self.assertTrue({'Abrir','Renombrar…','Enviar a la papelera…','Copiar','Cortar','Pegar','Copiar ubicación','Abrir ubicación','Propiedades'} <= acciones.keys())
        acciones['Copiar ubicación'].trigger()
        self.assertEqual(APP.clipboard().text(),str(self.path))
        menu.deleteLater()
        self.window.copiar_archivo(self.path)
        menu=self.window.crear_menu_archivo(self.folder)
        acciones={a.text():a for a in menu.actions()}
        self.assertTrue(acciones['Pegar'].isEnabled())
        self.assertTrue(acciones['Enviar a la papelera…'].isEnabled())
        menu.deleteLater()

    def test_renombrar_conserva_buffer_cursor_y_guardado(self):
        self.editor.insertPlainText('# cambios sin guardar\n')
        texto=self.editor.toPlainText()
        cursor=self.editor.textCursor().position()
        destino=self.folder/'nuevo nombre.cls'
        with patch.object(QInputDialog,'getText',return_value=(destino.name,True)):
            self.assertTrue(self.window.renombrar_archivo(self.path))
        self.assertFalse(self.path.exists())
        self.assertEqual(destino.read_text(),TEXTO)
        self.assertEqual(self.editor.path,destino)
        self.assertEqual(self.editor.toPlainText(),texto)
        self.assertEqual(self.editor.textCursor().position(),cursor)
        self.assertTrue(self.editor.document().isModified())
        self.assertTrue(self.window.guardar())
        self.assertEqual(destino.read_text(),texto)
        self.assertFalse(self.path.exists())

    def test_nombres_invalidos_destinos_ocupados_y_otras_pestanas(self):
        for nombre in ('../fuera.cls','sub/nombre.cls','..','', 'a\\b.cls'):
            with patch.object(QInputDialog,'getText',return_value=(nombre,True)),patch.object(self.window,'error'):
                self.assertFalse(self.window.renombrar_archivo(self.path))
            self.assertEqual(self.path.read_text(),TEXTO)
        ocupado=self.folder/'ocupado.cls';ocupado.write_text('ajeno')
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.mover_archivo(self.path,ocupado))
        self.assertEqual(ocupado.read_text(),'ajeno')
        self.window.abrir(ocupado)
        ocupado.unlink()
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.mover_archivo(self.path,ocupado))
        self.assertTrue(self.path.exists())

    def test_renombrar_no_oculta_conflicto_externo_ni_aplica_sondeo_antiguo(self):
        base=self.editor.estado_disco
        self.path.write_text('# ajeno\n'+TEXTO)
        destino=self.folder/'renombrado.cls'
        self.assertTrue(self.window.mover_archivo(self.path,destino))
        self.assertEqual(self.editor.estado_disco,base)
        self.assertTrue(self.window.tiene_cambio_externo(self.editor))
        self.window.recibir_disco((self.editor.documento_id,self.path,base,archivos.AUSENTE,''))
        self.assertEqual(self.editor.path,destino)
        with patch.object(self.window,'resolver_conflicto',return_value='cancelar'):
            self.assertFalse(self.window.guardar())
        self.assertTrue(destino.read_text().startswith('# ajeno'))

    def test_copiar_y_pegar_usan_disco_y_no_reemplazan_destino(self):
        self.editor.insertPlainText('# buffer\n')
        destino=self.folder/'sub';destino.mkdir()
        self.assertTrue(self.window.copiar_archivo(self.path))
        self.assertEqual(archivo_portapapeles(),(self.path,False))
        self.assertTrue(self.window.pegar_archivo(destino))
        self.assertEqual((destino/self.path.name).read_text(),TEXTO)
        self.assertEqual(self.editor.path,self.path)
        with patch.object(QInputDialog,'getText',return_value=(self.path.name,True)),patch.object(self.window,'error'):
            self.assertFalse(self.window.pegar_archivo(destino))
        self.assertEqual((destino/self.path.name).read_text(),TEXTO)
        with patch.object(QInputDialog,'getText',return_value=('copia.cls',True)):
            self.assertTrue(self.window.pegar_archivo(self.folder))
        self.assertTrue((self.folder/'copia.cls').is_file())

    def test_cortar_y_pegar_actualizan_pestana_y_limpian_portapapeles(self):
        destino=self.folder/'sub';destino.mkdir()
        self.editor.insertPlainText('# buffer\n')
        self.assertTrue(self.window.copiar_archivo(self.path,cortar=True))
        self.assertTrue(self.path.exists())
        self.assertEqual(archivo_portapapeles(),(self.path,True))
        self.assertTrue(self.window.pegar_archivo(destino))
        self.assertFalse(self.path.exists())
        self.assertEqual(self.editor.path,destino/self.path.name)
        self.assertTrue(self.editor.document().isModified())
        self.assertIsNone(archivo_portapapeles())
        self.assertTrue(self.window.guardar())
        self.assertIn('# buffer',self.editor.path.read_text())

    def test_no_mueve_o_borra_durante_compilacion(self):
        self.window.process=object()
        self.window.compilation=(self.editor,self.editor.revision,self.path)
        try:
            with patch.object(self.window,'error'):
                self.assertFalse(self.window.mover_archivo(self.path,self.folder/'otro.cls'))
                self.assertFalse(self.window.eliminar_archivo(self.path))
        finally:
            self.window.process=None
        self.assertTrue(self.path.exists())

    def test_papelera_cancelada_o_fallida_conserva_archivo(self):
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.No),patch.object(QFile,'moveToTrash') as trash:
            self.assertFalse(self.window.eliminar_archivo(self.path))
            trash.assert_not_called()
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.Yes),patch.object(QFile,'moveToTrash',return_value=(False,'')),patch.object(self.window,'error'):
            self.assertFalse(self.window.eliminar_archivo(self.path))
        self.assertEqual(self.path.read_text(),TEXTO)

    def test_papelera_conserva_texto_y_guardar_no_recrea_sin_confirmar(self):
        self.editor.insertPlainText('# edición\n')
        texto=self.editor.toPlainText()
        recuperable=self.folder/'papelera-simulada.cls'
        def trash(path):
            Path(path).rename(recuperable)
            return True,str(recuperable)
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.Yes),patch.object(QFile,'moveToTrash',side_effect=trash):
            self.assertTrue(self.window.eliminar_archivo(self.path))
        self.assertFalse(self.path.exists())
        self.assertTrue(recuperable.exists())
        self.assertEqual(self.editor.toPlainText(),texto)
        self.assertTrue(self.editor.document().isModified())
        with patch.object(self.window,'resolver_conflicto',return_value='cancelar'):
            self.assertFalse(self.window.guardar())
        self.assertFalse(self.path.exists())

    def test_ubicacion_propiedades_y_enlace_rechazado(self):
        with patch.object(QDesktopServices,'openUrl',return_value=True) as abrir:
            self.window.abrir_ubicacion(self.path)
            abrir.assert_called_once_with(QUrl.fromLocalFile(str(self.folder)))
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.Ok):
            self.window.propiedades_archivo(self.path)
        enlace=self.folder/'enlace.cls';enlace.symlink_to(self.path)
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.mover_archivo(enlace,self.folder/'otro.cls'))
        self.assertTrue(enlace.is_symlink())
