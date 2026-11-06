"""E7: conflictos de disco, árbol lazy y preferencias aisladas del usuario."""
import json
import os
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QSettings, Qt
from PySide6.QtGui import QFont
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication, QFileDialog, QMessageBox, QFontDialog

from ide import archivos, preferencias
from ide.interfaz.ventana import Ventana

ROOT = Path(__file__).resolve().parents[2]
APP = QApplication.instance() or QApplication([])
TEXTO = 'F main [] : void ;\nE ;\n.\n'


class VersionesDisco(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'programa.cls'
        self.path.write_text(TEXTO)

    def test_huella_detecta_cambio_con_misma_fecha_y_tamano(self):
        estado = archivos.estado(self.path)
        fecha = self.path.stat()
        self.path.write_text(TEXTO.replace('main', 'otro'))
        os.utime(self.path, ns=(fecha.st_atime_ns, fecha.st_mtime_ns))
        actual = archivos.estado(self.path)
        self.assertEqual((actual.tamano, actual.modificado_ns), (estado.tamano, estado.modificado_ns))
        self.assertNotEqual(actual, estado)

    def test_reemplazo_y_borrado_sin_cambiar_texto(self):
        base = archivos.estado(self.path)
        otro = self.path.with_suffix('.tmp')
        otro.write_text(TEXTO)
        os.replace(otro, self.path)
        self.assertNotEqual(base, archivos.estado(self.path))
        self.path.unlink()
        self.assertEqual(archivos.estado(self.path), archivos.AUSENTE)
        with self.assertRaises(FileNotFoundError):
            archivos.cargar(self.path)

    def test_guardado_rechaza_version_ajena_y_limpia_temporal(self):
        base = archivos.estado(self.path)
        self.path.write_text('version ajena')
        with self.assertRaises(archivos.CambioExterno):
            archivos.guardar(self.path, 'version mia', esperado=base)
        self.assertEqual(self.path.read_text(), 'version ajena')
        self.assertEqual(list(self.path.parent.iterdir()), [self.path])

    def test_cambio_durante_preparacion_de_guardado(self):
        base = archivos.estado(self.path)
        original = tempfile.mkstemp
        def preparar(*args, **kwargs):
            respuesta = original(*args, **kwargs)
            self.path.write_text('cambio entre lectura y guardado')
            return respuesta
        with patch('ide.archivos.tempfile.mkstemp', side_effect=preparar):
            with self.assertRaises(archivos.CambioExterno):
                archivos.guardar(self.path, TEXTO, esperado=base)
        self.assertIn('entre lectura', self.path.read_text())
        self.assertEqual(len(list(self.path.parent.iterdir())), 1)

    def test_guardado_devuelve_su_version_no_la_del_escritor_posterior(self):
        reemplazar = os.replace
        def escribir_despues(src, dst):
            reemplazar(src, dst)
            self.path.write_text('otro escritor después del replace')
        with patch('ide.archivos.os.replace', side_effect=escribir_despues):
            escrita = archivos.guardar(self.path, TEXTO, esperado=archivos.estado(self.path))
        self.assertNotEqual(escrita, archivos.estado(self.path))
        self.assertIn('otro escritor', self.path.read_text())

    def test_reemplazo_por_enlace_no_sobrescribe_otro_archivo(self):
        otro = self.path.parent / 'ajeno.cls'
        otro.write_text('contenido ajeno')
        self.path.unlink()
        self.path.symlink_to(otro)
        with self.assertRaises(archivos.CambioExterno):
            archivos.guardar(self.path, TEXTO, esperado=archivos.estado(self.path))
        self.assertTrue(self.path.is_symlink())
        self.assertEqual(otro.read_text(), 'contenido ajeno')

    def test_carga_bom_y_guardado_permisos(self):
        self.path.write_bytes(b'\xef\xbb\xbf' + TEXTO.replace('\n', '\r\n').encode())
        self.path.chmod(0o640)
        texto, base = archivos.cargar(self.path)
        self.assertEqual(texto, TEXTO)
        nueva = archivos.guardar(self.path, texto, esperado=base)
        self.assertEqual(nueva, archivos.estado(self.path))
        self.assertEqual(self.path.stat().st_mode & 0o777, 0o640)


class Proyecto(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.window = Ventana(ROOT, preferencias=False)
        self.window.auto_action.setChecked(False)
        self.window.monitor.timer.stop()
        self.addCleanup(self.limpiar)
        self.path = self.folder / 'fuente.cls'
        self.path.write_text(TEXTO)
        self.editor = self.window.abrir(self.path)

    def limpiar(self):
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        APP.processEvents()

    def esperar(self, condicion, timeout=4):
        limite = time.monotonic() + timeout
        while not condicion() and time.monotonic() < limite:
            APP.processEvents()
            time.sleep(.005)
        self.assertTrue(condicion())

    def detectar(self):
        self.window.monitor.comprobar()
        self.esperar(lambda: self.window.tiene_cambio_externo(self.editor))

    def test_cambio_externo_avisa_sin_tocar_buffer_y_recarga(self):
        self.path.write_text('# versión externa\n' + TEXTO)
        self.detectar()
        self.assertEqual(self.editor.toPlainText(), TEXTO)
        self.assertFalse(self.editor.document().isModified())
        self.assertFalse(self.window.aviso_externo.isHidden())
        self.assertIn('⚠', self.window.tabs.tabText(0))
        self.assertTrue(self.window.recargar())
        self.assertTrue(self.editor.toPlainText().startswith('# versión externa'))
        self.assertFalse(self.window.tiene_cambio_externo(self.editor))
        self.assertFalse(self.editor.document().isModified())

    def test_conservar_no_adopta_version_ajena_y_guardar_cancelado(self):
        base = self.editor.estado_disco
        self.path.write_text('externo')
        self.detectar()
        self.window.conservar()
        self.assertTrue(self.editor.document().isModified())
        self.assertEqual(self.editor.estado_disco, base)
        self.assertTrue(self.window.aviso_externo.isHidden())
        with patch.object(self.window, 'resolver_conflicto', return_value='cancelar'):
            self.assertFalse(self.window.guardar())
        self.assertEqual(self.path.read_text(), 'externo')
        self.assertEqual(self.editor.toPlainText(), TEXTO)

    def test_guardar_detecta_conflicto_antes_del_monitor(self):
        self.editor.insertPlainText('# mío\n')
        self.path.write_text('ajeno')
        with patch.object(self.window, 'resolver_conflicto', return_value='cancelar') as resolver:
            self.assertFalse(self.window.guardar())
            resolver.assert_called_once()
        self.assertEqual(self.path.read_text(), 'ajeno')
        self.assertTrue(self.editor.document().isModified())

    def test_sobrescribir_explicito_y_recrear_borrado(self):
        for borrar in (False, True):
            if borrar:
                self.path.unlink()
            else:
                self.path.write_text('externo')
            with patch.object(self.window, 'resolver_conflicto', return_value='sobrescribir'):
                self.assertTrue(self.window.guardar())
            self.assertEqual(self.path.read_text(), TEXTO)
            self.assertEqual(self.editor.estado_disco, archivos.estado(self.path))

    def test_segundo_cambio_mientras_se_confirma_no_se_pierde(self):
        self.editor.insertPlainText('# mío\n')
        self.path.write_text('primera versión ajena')
        def confirmar(*args):
            self.path.write_text('segunda versión ajena')
            return 'sobrescribir'
        with patch.object(self.window, 'resolver_conflicto', side_effect=confirmar), patch.object(self.window, 'error'):
            self.assertFalse(self.window.guardar())
        self.assertEqual(self.path.read_text(), 'segunda versión ajena')
        self.assertTrue(self.editor.document().isModified())

    def test_recargar_dirty_cancelar_y_error_conservan_edicion(self):
        self.editor.insertPlainText('# mío\n')
        texto = self.editor.toPlainText()
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.No):
            self.assertFalse(self.window.recargar())
        self.assertEqual(self.editor.toPlainText(), texto)
        self.path.write_bytes(b'\xff')
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.Yes), patch.object(self.window, 'error'):
            self.assertFalse(self.window.recargar())
        self.assertEqual(self.editor.toPlainText(), texto)
        self.assertTrue(self.editor.document().isModified())

    def test_borrado_y_error_lectura_no_eliminan_buffer(self):
        self.path.unlink()
        self.detectar()
        self.assertIn('borrado', self.window.texto_externo.text())
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.recargar())
        self.assertEqual(self.editor.toPlainText(), TEXTO)
        dato = (self.editor.documento_id, self.path, self.editor.estado_disco, None, 'sin acceso')
        self.window.recibir_disco(dato)
        self.assertIn('sin acceso', self.window.texto_externo.text())

    def test_guardar_como_copia_preserva_original_y_descarta_sondeo_viejo(self):
        vieja = self.editor.estado_disco
        self.path.write_text('ajeno')
        copia = self.folder / 'copia.cls'
        with patch.object(QFileDialog, 'getSaveFileName', return_value=(str(copia), '')):
            self.assertTrue(self.window.guardar(como=True))
        self.assertEqual(self.path.read_text(), 'ajeno')
        self.assertEqual(copia.read_text(), TEXTO)
        self.window.recibir_disco((self.editor.documento_id, self.path, vieja, archivos.AUSENTE, ''))
        self.assertEqual(self.editor.path, copia)
        self.assertFalse(self.window.tiene_cambio_externo(self.editor))

    def test_guardar_como_destino_existente_y_duplicado(self):
        copia = self.folder / 'otra.cls'
        copia.write_text('conservar')
        with patch.object(QFileDialog, 'getSaveFileName', return_value=(str(copia), '')), patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.No):
            self.assertFalse(self.window.guardar(como=True))
        self.assertEqual(copia.read_text(), 'conservar')
        otro = self.window.abrir(copia)
        with patch.object(QFileDialog, 'getSaveFileName', return_value=(str(self.path), '')), patch.object(self.window, 'error'):
            self.assertFalse(self.window.guardar(como=True, editor=otro))

    def test_f5_no_compila_disco_distinto_del_editor_sin_decision(self):
        self.path.write_text('externo')
        with patch('ide.ccom.ejecutable', return_value='/bin/true'), patch.object(self.window, 'resolver_conflicto', return_value='cancelar'):
            self.window.compilar()
        self.assertIsNone(self.window.process)
        self.assertEqual(self.path.read_text(), 'externo')

    def test_resultado_viejo_despues_de_guardar_y_pestana_cerrada(self):
        base = self.editor.estado_disco
        self.editor.insertPlainText('# nueva\n')
        self.assertTrue(self.window.guardar())
        dato = (self.editor.documento_id, self.path, base, archivos.AUSENTE, '')
        self.window.recibir_disco(dato)
        self.assertFalse(self.window.tiene_cambio_externo(self.editor))
        self.window.cerrar_tab(0)
        self.window.recibir_disco(dato)
        self.assertIsNone(self.window.editor().path)

    def test_fuente_no_modifica_documento_y_se_aplica_a_nuevas_pestanas(self):
        revision = self.editor.revision
        with patch.object(QFontDialog, 'getFont', return_value=(True, QFont('monospace', 16))):
            self.window.configurar_fuente()
        self.assertEqual(self.editor.font().pointSize(), 16)
        self.assertEqual(self.window.nuevo().font().pointSize(), 16)
        self.assertEqual(self.editor.revision, revision)
        self.assertFalse(self.editor.document().isModified())

    def test_explorador_recursivo_excluye_dependencias_y_enlaces(self):
        sub = self.folder / 'ejemplos'
        sub.mkdir()
        nested = sub / 'suma.cls'
        nested.write_text(TEXTO)
        (self.folder/'build').mkdir()
        (self.folder/'build'/'oculto.cls').write_text(TEXTO)
        (self.folder/'leer.txt').write_text('otro')
        (self.folder/'bucle').symlink_to(self.folder, target_is_directory=True)
        self.window.cargar_carpeta(self.folder)
        tree = self.window.tree
        self.esperar(lambda: tree.filtro.rowCount(tree.rootIndex()) == 2)
        nombres = [tree.filtro.index(i, 0, tree.rootIndex()).data() for i in range(2)]
        self.assertEqual(set(nombres), {'ejemplos', 'fuente.cls'})
        carpeta = next(tree.filtro.index(i, 0, tree.rootIndex()) for i, n in enumerate(nombres) if n == 'ejemplos')
        tree.expand(carpeta)
        self.esperar(lambda: tree.filtro.rowCount(carpeta) == 1)
        archivo = tree.filtro.index(0, 0, carpeta)
        tree.abrir(archivo)
        self.assertEqual(self.window.editor().path, nested)
        self.assertEqual(self.editor.toPlainText(), TEXTO)
        # Cambiar de raíz no conserva índices de la anterior.
        self.window.cargar_carpeta(sub)
        self.esperar(lambda: tree.filtro.rowCount(tree.rootIndex()) == 1)

    def test_cierre_con_monitor_en_ejecucion(self):
        iniciado = threading.Event()
        def leer(path, *, cancelar=None):
            iniciado.set()
            while not cancelar():
                time.sleep(.002)
            raise InterruptedError('cancelado')
        with patch('ide.archivos.estado', side_effect=leer):
            self.window.monitor.comprobar()
            self.assertTrue(iniciado.wait(2))
            tarea = self.window.monitor.task
            self.assertTrue(self.window.close())
            self.assertFalse(tarea.isRunning())
        APP.processEvents()

    def test_error_de_acceso_a_carpeta_no_cambia_proyecto(self):
        anterior = self.window.project
        with patch('ide.interfaz.ventana.os.access', return_value=False), patch.object(self.window, 'error') as aviso:
            self.assertFalse(self.window.cargar_carpeta(self.folder))
            aviso.assert_called_once()
        self.assertEqual(self.window.project, anterior)

    def test_raiz_dentro_de_carpeta_excluida_y_archivo_nuevo(self):
        raiz = self.folder / 'build' / 'proyecto'
        raiz.mkdir(parents=True)
        self.window.cargar_carpeta(raiz)
        tree = self.window.tree
        self.assertTrue(tree.rootIndex().isValid())
        nuevo = raiz / 'nuevo.cls'
        nuevo.write_text(TEXTO)
        self.esperar(lambda: tree.filtro.rowCount(tree.rootIndex()) == 1)
        self.assertEqual(tree.filtro.index(0, 0, tree.rootIndex()).data(), 'nuevo.cls')

    def test_atajos_guardar_explorador_y_panel(self):
        self.window.show()
        self.window.activateWindow()
        self.editor.setFocus()
        APP.processEvents()
        self.editor.insertPlainText('# cambio por teclado\n')
        QTest.keyClick(self.editor, Qt.Key.Key_S, Qt.KeyboardModifier.ControlModifier)
        self.assertFalse(self.editor.document().isModified())
        self.assertIn('cambio por teclado', self.path.read_text())
        QTest.keyClick(self.editor, Qt.Key.Key_B, Qt.KeyboardModifier.ControlModifier)
        self.assertFalse(self.window.files_button.isChecked())
        QTest.keyClick(self.editor, Qt.Key.Key_E, Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier)
        self.assertTrue(self.window.tree.hasFocus())
        QTest.keyClick(self.window.tree, Qt.Key.Key_J, Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.AltModifier)
        self.assertEqual(self.window.workspace_splitter.orientation(), Qt.Orientation.Horizontal)

    def test_tamano_minimo_y_navegacion_por_teclado(self):
        self.window.show()
        self.window.resize(800, 520)
        APP.processEvents()
        self.assertLessEqual(self.window.width(), 800)
        self.window.files_button.setChecked(False)
        self.window.enfocar_explorador()
        self.assertFalse(self.window.sidebar.isHidden())
        self.assertTrue(self.window.tree.hasFocus())
        self.assertEqual(self.editor.toPlainText(), TEXTO)


class Preferencias(unittest.TestCase):
    def test_json_invalido_version_y_valores_fuera_de_rango(self):
        self.assertEqual(preferencias.validar([]), {})
        self.assertEqual(preferencias.validar({'version': 2}), {})
        d = preferencias.validar({'version': 1, 'puntos': -5, 'ancho': 'grande', 'panel_derecho': 'false', 'alto': 600,
                                 'ccom': '\x00', 'paneles': [1, 20], 'familia': 'monospace'})
        self.assertEqual(d, {'version': 1, 'alto': 600, 'familia': 'monospace'})

    def test_persistencia_aislada_y_recuperacion_de_configuracion(self):
        with tempfile.TemporaryDirectory() as carpeta:
            path = str(Path(carpeta)/'preferencias.ini')
            settings = QSettings(path, QSettings.Format.IniFormat)
            settings.setValue('estado_v1', '{mal json')
            self.assertEqual(preferencias.cargar(settings), {})
            w = Ventana(ROOT, preferencias=settings)
            try:
                w.cargar_carpeta(Path(carpeta))
                w.configured_ccom = '/ruta/no/disponible/ccom'
                w.files_button.setChecked(False)
                w.results_action.setChecked(False)
                w.mover_panel()
                w.auto_action.setChecked(False)
                w.fuente_editor = QFont('monospace', 15)
                self.assertTrue(w.close())
            finally:
                w.close()
                w.deleteLater()
                APP.processEvents()
            restaurada = Ventana(ROOT, preferencias=QSettings(path, QSettings.Format.IniFormat))
            try:
                self.assertEqual(restaurada.project, Path(carpeta))
                self.assertEqual(restaurada.configured_ccom, '')
                self.assertFalse(restaurada.files_button.isChecked())
                self.assertFalse(restaurada.results_action.isChecked())
                self.assertFalse(restaurada.auto_action.isChecked())
                self.assertEqual(restaurada.workspace_splitter.orientation(), Qt.Orientation.Horizontal)
                self.assertEqual(restaurada.editor().font().pointSize(), 15)
            finally:
                restaurada.close()
                restaurada.deleteLater()
                APP.processEvents()

    def test_proyecto_desaparecido_usa_raiz_y_cierre_cancelado_no_persiste(self):
        with tempfile.TemporaryDirectory() as carpeta:
            settings = QSettings(str(Path(carpeta)/'p.ini'), QSettings.Format.IniFormat)
            settings.setValue('estado_v1', json.dumps({'version': 1, 'proyecto': '/no/existe', 'puntos': 10000}))
            original = settings.value('estado_v1')
            w = Ventana(ROOT, preferencias=settings)
            try:
                self.assertEqual(w.project, ROOT)
                self.assertEqual(w.editor().font().pointSize(), 12)
                w.editor().insertPlainText('# sin guardar')
                with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.Cancel):
                    self.assertFalse(w.close())
                self.assertEqual(settings.value('estado_v1'), original)
            finally:
                w.editor().document().setModified(False)
                w.close()
                w.deleteLater()
                APP.processEvents()
