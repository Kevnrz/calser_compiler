"""Carpetas: jerarquía lazy, operaciones asíncronas y buffers conservados."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

from PySide6.QtCore import QFile, QSettings, QTimer, Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication, QInputDialog, QMessageBox

from ide.interfaz.acciones_archivos import archivo_portapapeles
from ide.interfaz.operaciones_carpetas import CopiaCancelada, copiar_carpeta
from ide.interfaz.ventana import Ventana

ROOT=Path(__file__).resolve().parents[2]
APP=QApplication.instance() or QApplication([])
TEXTO='F main [] : void ; E ; .\n'


class Carpetas(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base=Path(self.tmp.name)
        self.folder=self.base/'proyecto';self.folder.mkdir()
        self.window=Ventana(ROOT,preferencias=False)
        self.window.auto_action.setChecked(False)
        self.window.monitor.timer.stop()
        self.window.cargar_carpeta(self.folder)
        self.addCleanup(self.limpiar)

    def esperar(self, condicion, timeout=6):
        limite=time.monotonic()+timeout
        while not condicion() and time.monotonic()<limite:
            APP.processEvents();time.sleep(.005)
        self.assertTrue(condicion())

    def terminar(self):
        self.esperar(lambda:self.window.operacion_carpetas is None)

    def limpiar(self):
        self.window.cancelar_copia_carpeta()
        self.terminar()
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close();self.window.deleteLater();APP.processEvents()
        APP.clipboard().clear()

    def arbol(self):
        raiz=self.folder/'ejemplos';(raiz/'sub').mkdir(parents=True)
        (raiz/'uno.cls').write_text(TEXTO)
        (raiz/'sub'/'dos.cls').write_text(TEXTO)
        return raiz

    def test_crear_carpeta_y_archivo_en_ella(self):
        with patch.object(QInputDialog,'getText',return_value=('nueva carpeta',True)):
            self.assertTrue(self.window.crear_entrada(self.folder,True))
        carpeta=self.folder/'nueva carpeta'
        with patch.object(QInputDialog,'getText',return_value=('suma',True)):
            self.assertTrue(self.window.crear_entrada(carpeta,False))
        archivo=carpeta/'suma.cls'
        self.assertEqual(archivo.read_text(),'')
        self.assertEqual(self.window.editor().path,archivo)
        self.esperar(lambda:self.window.tree.indice_ruta(archivo).isValid())

    def test_creacion_invalida_cancelada_o_existente_no_reemplaza(self):
        carpeta=self.folder/'existente';carpeta.mkdir()
        for nombre in ('existente','../fuera','a/b','a\\b'):
            with patch.object(QInputDialog,'getText',return_value=(nombre,True)),patch.object(self.window,'error'):
                self.assertFalse(self.window.crear_entrada(self.folder,True))
        with patch.object(QInputDialog,'getText',return_value=('',False)):
            self.assertFalse(self.window.crear_entrada(self.folder,True))
        archivo=self.folder/'uno.cls';archivo.write_text('conservar')
        with patch.object(QInputDialog,'getText',return_value=(archivo.name,True)),patch.object(self.window,'error'):
            self.assertFalse(self.window.crear_entrada(self.folder,False))
        self.assertEqual(archivo.read_text(),'conservar')

    def test_renombrar_carpeta_actualiza_todas_las_pestanas(self):
        origen=self.arbol();uno=self.window.abrir(origen/'uno.cls');dos=self.window.abrir(origen/'sub/dos.cls')
        uno.insertPlainText('# uno\n');dos.insertPlainText('# dos\n')
        textos=[uno.toPlainText(),dos.toPlainText()]
        revisiones=[uno.revision,dos.revision]
        with patch.object(QInputDialog,'getText',return_value=('renombrada',True)):
            self.assertTrue(self.window.renombrar_archivo(origen))
        self.terminar();destino=self.folder/'renombrada'
        self.assertFalse(origen.exists())
        self.assertEqual([uno.path,dos.path],[destino/'uno.cls',destino/'sub/dos.cls'])
        self.assertEqual([uno.toPlainText(),dos.toPlainText()],textos)
        self.assertEqual([uno.revision,dos.revision],revisiones)
        self.assertTrue(uno.document().isModified())
        self.assertTrue(dos.document().isModified())
        self.assertTrue(self.window.guardar(editor=uno))
        self.assertTrue(self.window.guardar(editor=dos))
        self.assertFalse(origen.exists())
        uno.undo();self.assertEqual(uno.toPlainText(),TEXTO)

    def test_cortar_pegar_carpeta_conserva_edicion_y_limpia_clipboard(self):
        origen=self.arbol();destino=self.folder/'destino';destino.mkdir()
        editor=self.window.abrir(origen/'sub/dos.cls');editor.insertPlainText('# local\n')
        self.assertTrue(self.window.copiar_archivo(origen,cortar=True))
        self.assertTrue(origen.exists())
        self.assertTrue(self.window.pegar_archivo(destino))
        self.terminar()
        self.assertFalse(origen.exists())
        self.assertEqual(editor.path,destino/'ejemplos/sub/dos.cls')
        self.assertTrue(editor.document().isModified())
        self.assertIsNone(archivo_portapapeles())
        self.assertTrue(self.window.guardar(editor=editor))

    def test_copiar_carpeta_completa_sin_seguir_enlaces(self):
        origen=self.arbol();destino=self.folder/'destino';destino.mkdir()
        (origen/'vacia').mkdir();(origen/'nota.txt').write_text('incluida')
        (origen/'.oculto').write_text('incluido')
        externo=self.base/'externo.txt';externo.write_text('externo')
        (origen/'enlace').symlink_to(externo)
        self.window.copiar_archivo(origen)
        self.assertTrue(self.window.pegar_archivo(destino));self.terminar()
        copia=destino/'ejemplos'
        self.assertEqual((copia/'sub/dos.cls').read_text(),TEXTO)
        self.assertEqual((copia/'nota.txt').read_text(),'incluida')
        self.assertTrue((copia/'vacia').is_dir())
        self.assertTrue((copia/'enlace').is_symlink())
        self.assertEqual(os.readlink(copia/'enlace'),str(externo))
        self.assertTrue(origen.exists())

    def test_destino_ocupado_y_carpeta_dentro_de_si_misma(self):
        origen=self.arbol();sub=origen/'sub'
        self.window.copiar_archivo(origen)
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.pegar_archivo(sub))
            self.assertFalse(self.window.mover_archivo(origen,sub/'ciclo'))
        self.assertFalse((sub/'ciclo').exists())
        with patch.object(QInputDialog,'getText',return_value=('duplicada',True)):
            self.assertTrue(self.window.pegar_archivo(self.folder))
        self.terminar()
        self.assertTrue((self.folder/'duplicada/sub/dos.cls').is_file())
        with patch.object(QInputDialog,'getText',return_value=('duplicada',True)),patch.object(self.window,'error'):
            self.assertFalse(self.window.pegar_archivo(self.folder))
        self.assertTrue((self.folder/'duplicada/sub/dos.cls').is_file())

    def test_mover_carpeta_no_adopta_cambios_externos(self):
        origen=self.arbol();path=origen/'uno.cls';editor=self.window.abrir(path)
        base=editor.estado_disco;path.write_text('# ajeno\n'+TEXTO)
        destino=self.folder/'otra'
        self.assertTrue(self.window.mover_archivo(origen,destino));self.terminar()
        self.assertEqual(editor.path,destino/'uno.cls')
        self.assertEqual(editor.estado_disco,base)
        self.assertTrue(self.window.tiene_cambio_externo(editor))
        with patch.object(self.window,'resolver_conflicto',return_value='cancelar'):
            self.assertFalse(self.window.guardar(editor=editor))
        self.assertTrue(editor.path.read_text().startswith('# ajeno'))

    def test_protege_raiz_compilacion_y_destinos_abiertos(self):
        origen=self.arbol();editor=self.window.abrir(origen/'uno.cls')
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.mover_archivo(self.folder,self.base/'movida'))
            self.assertFalse(self.window.eliminar_archivo(self.folder))
        self.window.process=object();self.window.compilation=(editor,editor.revision,editor.path)
        try:
            with patch.object(self.window,'error'):
                self.assertFalse(self.window.mover_archivo(origen,self.folder/'otra'))
        finally:
            self.window.process=None
        ocupado=self.folder/'ocupado';ocupado.mkdir();path=ocupado/'archivo.cls';path.write_text(TEXTO)
        self.window.abrir(path);path.unlink();ocupado.rmdir()
        with patch.object(self.window,'error'):
            self.assertFalse(self.window.mover_archivo(origen,ocupado))
        self.assertTrue(origen.exists())

    def test_papelera_carpeta_conserva_buffers_sin_borrado_permanente(self):
        origen=self.arbol();editor=self.window.abrir(origen/'sub/dos.cls');editor.insertPlainText('# local\n')
        texto=editor.toPlainText();trash=self.base/'papelera-simulada'
        def mover(path):
            Path(path).rename(trash);return True,str(trash)
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.Yes),patch.object(QFile,'moveToTrash',side_effect=mover):
            self.assertTrue(self.window.eliminar_archivo(origen));self.terminar()
        self.assertFalse(origen.exists());self.assertTrue(trash.exists())
        self.assertEqual(editor.toPlainText(),texto)
        self.assertTrue(editor.document().isModified())
        with patch.object(self.window,'resolver_conflicto',return_value='cancelar'):
            self.assertFalse(self.window.guardar(editor=editor))
        self.assertFalse(origen.exists())

    def test_papelera_fallida_y_copia_parcial_informan_sin_perder_origen(self):
        origen=self.arbol()
        with patch.object(QMessageBox,'exec',return_value=QMessageBox.StandardButton.Yes),patch.object(QFile,'moveToTrash',return_value=(False,'')),patch.object(self.window,'error') as error:
            self.assertTrue(self.window.eliminar_archivo(origen));self.terminar()
            error.assert_called_once()
        self.assertTrue(origen.exists())
        destino=self.folder/'parcial'
        with patch('ide.interfaz.operaciones_carpetas.shutil.copystat',side_effect=PermissionError('sin permisos')),patch.object(self.window,'error') as error:
            self.assertTrue(self.window.iniciar_operacion_carpeta('copiar',origen,destino));self.terminar()
            self.assertIn('incompleta',str(error.call_args))
        self.assertTrue(destino.exists());self.assertTrue(origen.exists())

    def test_copia_responsiva_cancelable_y_cierre_guardado_protegidos(self):
        origen=self.arbol();editor=self.window.abrir(origen/'uno.cls');editor.insertPlainText('# local\n')
        iniciado=threading.Event()
        def lento(src,dst,cancelar,avance,al_crear):
            iniciado.set()
            while not cancelar():
                time.sleep(.005)
            raise CopiaCancelada('cancelado por prueba')
        with patch('ide.interfaz.operaciones_carpetas.copiar_carpeta',side_effect=lento):
            try:
                self.assertTrue(self.window.iniciar_operacion_carpeta('copiar',origen,self.folder/'copia'))
                self.assertTrue(iniciado.wait(2))
                activo=[];QTimer.singleShot(0,lambda:activo.append(True));APP.processEvents()
                self.assertTrue(activo)
                self.assertFalse(self.window.close())
                with patch.object(self.window,'error'):
                    self.assertFalse(self.window.guardar(editor=editor))
                    self.assertIsNone(self.window.abrir(origen/'sub/dos.cls'))
            finally:
                self.window.cancelar_copia_carpeta();self.terminar()
        self.assertEqual((origen/'uno.cls').read_text(),TEXTO)
        self.assertTrue(editor.document().isModified())

    def test_expandir_todo_lazy_contraer_y_restaurar_al_cambiar_proyecto(self):
        (self.folder/'a/b/c').mkdir(parents=True)
        (self.folder/'x/y').mkdir(parents=True)
        tree=self.window.tree
        tree.expandir_todo()
        deseadas={self.folder/p for p in ('a','a/b','a/b/c','x','x/y')}
        self.esperar(lambda:deseadas <= tree.ramas)
        self.assertTrue(all(tree.isExpanded(tree.indice_ruta(p)) for p in deseadas))
        tree.contraer_todo();self.assertEqual(tree.ramas,set())
        tree.expandir_ruta(self.folder/'a/b')
        self.esperar(lambda:self.folder/'a/b' in tree.ramas)
        otra=self.base/'otra';otra.mkdir()
        self.window.cargar_carpeta(otra);self.window.cargar_carpeta(self.folder)
        self.esperar(lambda:tree.isExpanded(tree.indice_ruta(self.folder/'a/b')))

    def test_preferencias_restauran_solo_ramas_del_proyecto(self):
        (self.folder/'a/b').mkdir(parents=True)
        tree=self.window.tree;tree.expandir_ruta(self.folder/'a/b')
        self.esperar(lambda:self.folder/'a/b' in tree.ramas)
        settings=QSettings(str(self.base/'config.ini'),QSettings.Format.IniFormat)
        self.window.settings=settings;self.window.guardar_preferencias()
        otra=Ventana(ROOT,preferencias=settings)
        try:
            self.esperar(lambda:otra.tree.isExpanded(otra.tree.indice_ruta(self.folder/'a/b')))
            self.assertEqual(otra.project,self.folder)
            otra.tree.restaurar_ramas(['../fuera',str(self.base)])
            APP.processEvents()
            self.assertTrue(all(p.is_relative_to(self.folder) for p in otra.tree.ramas))
        finally:
            otra.close();otra.deleteLater();APP.processEvents()

    def test_atajos_del_explorador_y_revelar_archivo(self):
        origen=self.arbol();editor=self.window.abrir(origen/'sub/dos.cls')
        self.window.show();self.window.activateWindow()
        self.window.revelar_archivo_activo()
        self.esperar(lambda:self.window.tree.ruta_actual()==editor.path)
        tree=self.window.tree;tree.setFocus();APP.processEvents()
        with patch.object(QInputDialog,'getText',return_value=('creada por teclado',True)):
            QTest.keyClick(tree,Qt.Key.Key_N,Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier)
        self.assertTrue((editor.path.parent/'creada por teclado').is_dir())
        tree.revelar_ruta(origen);self.esperar(lambda:tree.ruta_actual()==origen)
        QTest.keyClick(tree,Qt.Key.Key_C,Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(archivo_portapapeles(),(origen,False))

    def test_copia_pura_cancelada_conserva_origen_y_parcial(self):
        origen=self.arbol();destino=self.folder/'cancelada'
        contador=iter([False]*4+[True])
        with self.assertRaises(CopiaCancelada):
            copiar_carpeta(origen,destino,lambda:next(contador,True),lambda n:None)
        self.assertTrue(destino.exists());self.assertTrue(origen.exists())
        self.assertEqual((origen/'sub/dos.cls').read_text(),TEXTO)
