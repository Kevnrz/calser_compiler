"""Selección real con ratón/teclado y traslados de lotes sin perder buffers."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from PySide6.QtCore import QPoint, QPointF, QMimeData, Qt, QUrl
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QMessageBox
from ide.pruebas import test_carpetas as fixtures
from ide.interfaz.seleccion_archivos import archivos_portapapeles

APP = fixtures.APP


class Seleccion(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def mostrar(self):
        self.rutas = [self.folder / f'{nombre}.cls' for nombre in ('a', 'b', 'c')]
        for p in self.rutas:
            p.write_text(fixtures.TEXTO)
        self.destino = self.folder / 'destino'
        self.destino.mkdir()
        self.window.resize(1000, 700)
        self.window.show()
        self.tree = self.window.tree
        self.esperar(lambda: all(self.tree.indice_ruta(p).isValid() for p in self.rutas + [self.destino]))
        self.esperar(lambda: self.folder in self.tree._cargadas)
        QTest.qWait(180)  # QFileSystemModel ordena las nuevas filas con un temporizador.

    def click(self, p, mods=Qt.KeyboardModifier.NoModifier, button=Qt.MouseButton.LeftButton):
        QTest.mouseClick(self.tree.viewport(), button, mods, self.tree.visualRect(self.tree.indice_ruta(p)).center())

    def test_ctrl_shift_y_seleccionar_todo(self):
        self.mostrar()
        self.click(self.rutas[0])
        self.click(self.rutas[2], Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(set(self.tree.rutas_seleccionadas()), {self.rutas[0], self.rutas[2]})
        self.click(self.rutas[0])
        self.click(self.rutas[2], Qt.KeyboardModifier.ShiftModifier)
        self.assertEqual(set(self.tree.rutas_seleccionadas()), set(self.rutas))
        self.tree.setFocus()
        QTest.keyClick(self.tree, Qt.Key.Key_A, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(set(self.tree.rutas_seleccionadas()), set(self.rutas + [self.destino]))

    def test_rectangulo_y_copiar_con_teclado(self):
        self.mostrar()
        inferior = max(self.tree.visualRect(self.tree.indice_ruta(p)).bottom() for p in self.rutas + [self.destino])
        origen = QPoint(20, inferior + 45)
        final = QPoint(100, self.tree.visualRect(self.tree.indice_ruta(self.rutas[0])).top())
        QTest.mousePress(self.tree.viewport(), Qt.MouseButton.LeftButton, pos=origen)
        QTest.mouseMove(self.tree.viewport(), final)
        QTest.mouseRelease(self.tree.viewport(), Qt.MouseButton.LeftButton, pos=final)
        self.assertEqual(set(self.tree.rutas_seleccionadas()), set(self.rutas + [self.destino]))
        QTest.keyClick(self.tree, Qt.Key.Key_C, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(set(archivos_portapapeles()[0]), set(self.rutas + [self.destino]))

    def test_menu_no_pierde_seleccion(self):
        self.mostrar()
        self.click(self.rutas[0]); self.click(self.rutas[1], Qt.KeyboardModifier.ControlModifier)
        with patch.object(self.window, 'menu_archivo'):
            # Desconectar la señal para no abrir un menú modal en la prueba.
            self.tree.menuSolicitado.disconnect()
            self.tree.menu_contextual(self.tree.visualRect(self.tree.indice_ruta(self.rutas[0])).center())
        self.assertEqual(set(self.tree.rutas_seleccionadas()), set(self.rutas[:2]))
        menu = self.window.crear_menu_archivo(self.rutas[0])
        self.assertTrue(menu.actions()[0].text().startswith('Abrir archivos seleccionados'))
        self.assertNotIn('Renombrar…', [a.text() for a in menu.actions()])

    def test_traslado_archivos_con_buffers(self):
        self.mostrar()
        editores = [self.window.abrir(p) for p in self.rutas]
        for e in editores:
            e.insertPlainText('# sin guardar\n')
        self.assertTrue(self.window.trasladar_seleccion(self.rutas, self.destino))
        self.assertTrue(all(not p.exists() for p in self.rutas))
        for e, p in zip(editores, self.rutas):
            self.assertEqual(e.path, self.destino / p.name)
            self.assertTrue(e.document().isModified())
            self.assertIn('# sin guardar', e.toPlainText())
            e.undo(); self.assertEqual(e.toPlainText(), fixtures.TEXTO)

    def test_cortar_pegar_carpeta_y_archivo(self):
        self.mostrar()
        carpeta = self.folder / 'sub'; carpeta.mkdir()
        interno = carpeta / 'interno.cls'; interno.write_text(fixtures.TEXTO)
        editor = self.window.abrir(interno); editor.insertPlainText('# local\n')
        self.assertTrue(self.window.copiar_seleccion([carpeta, interno, self.rutas[0]], True))
        self.assertEqual(set(archivos_portapapeles()[0]), {carpeta, self.rutas[0]})
        self.assertTrue(self.window.pegar_archivo(self.destino)); self.terminar()
        self.assertIsNone(archivos_portapapeles())
        self.assertTrue((self.destino / self.rutas[0].name).exists())
        self.assertEqual(editor.path, self.destino / 'sub/interno.cls')
        self.assertTrue(editor.document().isModified())
        self.assertFalse(carpeta.exists())

    def test_copiar_varias_carpetas_secuencial(self):
        self.mostrar()
        carpetas = [self.folder / nombre for nombre in ('uno', 'dos')]
        for p in carpetas:
            p.mkdir(); (p / 'contenido.txt').write_text(p.name)
        self.window.copiar_seleccion(carpetas)
        self.assertTrue(self.window.pegar_archivo(self.destino)); self.terminar()
        for p in carpetas:
            self.assertEqual((self.destino / p.name / 'contenido.txt').read_text(), p.name)
            self.assertTrue(p.exists())
        self.assertIsNotNone(archivos_portapapeles())

    def test_conflicto_previo_no_mueve_ningun_archivo(self):
        self.mostrar()
        (self.destino / self.rutas[1].name).write_text('existente')
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.trasladar_seleccion(self.rutas, self.destino))
        self.assertTrue(all(p.exists() for p in self.rutas))
        self.assertFalse((self.destino / self.rutas[0].name).exists())
        self.assertEqual((self.destino / self.rutas[1].name).read_text(), 'existente')

    def test_ancestro_protegido_y_descendiente_rechazados(self):
        self.mostrar()
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.trasladar_seleccion([self.folder], self.destino))
            self.assertFalse(self.window.trasladar_seleccion([self.destino], self.destino))
        self.assertTrue(self.destino.exists())

    def test_fallo_worker_detiene_lote_y_conserva_clipboard(self):
        self.mostrar()
        carpeta = self.folder / 'uno'; carpeta.mkdir()
        self.window.copiar_seleccion([carpeta, self.rutas[0]], True)
        with patch('ide.interfaz.operaciones_carpetas.QDir.rename', return_value=False), patch.object(self.window, 'error'):
            self.assertTrue(self.window.pegar_archivo(self.destino)); self.terminar()
        self.assertTrue(carpeta.exists()); self.assertTrue(self.rutas[0].exists())
        self.assertIsNotNone(archivos_portapapeles())
        self.assertIsNone(self.window._lote_archivos)

    def test_papelera_multiple_una_confirmacion(self):
        self.mostrar()
        editores = [self.window.abrir(p) for p in self.rutas[:2]]
        def papelera(path):
            p = Path(path); destino = self.base / p.name; p.rename(destino)
            return True, str(destino)
        with patch.object(QMessageBox, 'exec', return_value=QMessageBox.StandardButton.Yes) as confirmacion, patch('ide.interfaz.seleccion_archivos.QFile.moveToTrash', side_effect=papelera):
            self.assertTrue(self.window.eliminar_seleccion(self.rutas[:2]))
            confirmacion.assert_called_once()
        self.assertTrue(all(e.document().isModified() for e in editores))
        self.assertTrue(self.rutas[2].exists())

    def test_drop_solo_interno_sobre_carpeta(self):
        self.mostrar()
        mime = QMimeData(); mime.setUrls([QUrl.fromLocalFile(str(self.rutas[0]))])
        evento = SimpleNamespace(source=lambda: self.tree, mimeData=lambda: mime,
             position=lambda: QPointF(self.tree.visualRect(self.tree.indice_ruta(self.destino)).center()),
             setDropAction=lambda a: None, accept=lambda: None, ignore=lambda: None)
        self.tree.dropEvent(evento)
        self.assertTrue((self.destino / self.rutas[0].name).exists())
        evento.source = lambda: None
        self.assertIsNone(self.tree.destino_arrastre(evento))
        evento.source = lambda: self.tree
        evento.position = lambda: QPointF(self.tree.visualRect(self.tree.indice_ruta(self.rutas[1])).center())
        self.assertIsNone(self.tree.destino_arrastre(evento))

    def test_drag_incluye_toda_seleccion(self):
        self.mostrar()
        self.click(self.rutas[0]); self.click(self.rutas[1], Qt.KeyboardModifier.ControlModifier)
        with patch('ide.interfaz.explorador.QDrag') as drag:
            inicio = self.tree.visualRect(self.tree.indice_ruta(self.rutas[0])).center()
            QTest.mousePress(self.tree.viewport(), Qt.MouseButton.LeftButton, pos=inicio)
            QTest.mouseMove(self.tree.viewport(), inicio + QPoint(35, 0))
            QTest.mouseRelease(self.tree.viewport(), Qt.MouseButton.LeftButton, pos=inicio + QPoint(35, 0))
            drag.return_value.exec.assert_called_once()
            mime = drag.return_value.setMimeData.call_args.args[0]
            self.assertEqual({u.toLocalFile() for u in mime.urls()}, set(map(str, self.rutas[:2])))
