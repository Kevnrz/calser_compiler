"""Orden visual de hermanos, intención del drop y previsualización con alfa."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from PySide6.QtCore import QPointF, QMimeData, QSettings, Qt, QUrl
from PySide6.QtTest import QTest
from ide import preferencias
from ide.pruebas import test_carpetas as fixtures
from ide.interfaz.ventana import Ventana

APP = fixtures.APP


class Orden(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def mostrar(self):
        self.rutas = [self.folder / n for n in ('alfa', 'beta', 'gamma')]
        for p in self.rutas:
            p.mkdir(); (p / 'uno.cls').write_text(fixtures.TEXTO)
        self.tree = self.window.tree
        self.window.resize(1000, 700); self.window.show()
        self.esperar(lambda: all(self.tree.indice_ruta(p).isValid() for p in self.rutas))
        self.esperar(lambda: self.folder in self.tree._cargadas)
        QTest.qWait(180)

    def nombres(self, carpeta=None):
        padre = self.tree.indice_ruta(carpeta or self.folder)
        return [self.tree._path(self.tree.filtro.index(n, 0, padre)).name
                for n in range(self.tree.filtro.rowCount(padre))]

    def evento(self, rutas, referencia, zona):
        rect = self.tree.visualRect(self.tree.indice_ruta(referencia))
        punto = rect.center()
        if zona == 'antes': punto.setY(rect.top() + 1)
        if zona == 'despues': punto.setY(rect.bottom() - 1)
        mime = QMimeData(); mime.setUrls([QUrl.fromLocalFile(str(p)) for p in rutas])
        return SimpleNamespace(source=lambda: self.tree, mimeData=lambda: mime,
             position=lambda: QPointF(punto), setDropAction=lambda a: None,
             accept=lambda: None, ignore=lambda: None)

    def test_antes_y_despues_cambian_orden_sin_mover_disco(self):
        self.mostrar()
        self.assertTrue(self.tree.reordenar([self.rutas[2]], self.rutas[0]))
        self.assertEqual(self.nombres(), ['gamma', 'alfa', 'beta'])
        self.assertTrue(self.tree.reordenar([self.rutas[2]], self.rutas[1], True))
        self.assertEqual(self.nombres(), ['alfa', 'beta', 'gamma'])
        self.assertTrue(all((p / 'uno.cls').is_file() for p in self.rutas))

    def test_drop_borde_reordena_y_centro_mueve(self):
        self.mostrar()
        with patch.object(self.window, 'trasladar_seleccion') as mover:
            # El borde no emite una petición de traslado a disco.
            self.tree.dropEvent(self.evento([self.rutas[2]], self.rutas[0], 'antes'))
            mover.assert_not_called()
        self.assertEqual(self.nombres(), ['gamma', 'alfa', 'beta'])
        self.tree.dropEvent(self.evento([self.rutas[2]], self.rutas[0], 'dentro'))
        self.terminar()
        self.assertTrue((self.rutas[0] / 'gamma/uno.cls').is_file())
        self.assertFalse(self.rutas[2].exists())

    def test_reordenar_bloque_respeta_su_orden_visual(self):
        self.mostrar()
        self.assertTrue(self.tree.reordenar([self.rutas[2], self.rutas[1]], self.rutas[0]))
        self.assertEqual(self.nombres(), ['beta', 'gamma', 'alfa'])

    def test_borde_no_cambia_carpeta_contenedora(self):
        self.mostrar()
        sub = self.rutas[0] / 'uno.cls'
        self.assertIsNone(self.tree.intencion_arrastre(self.evento([sub], self.rutas[1], 'antes')))
        self.assertFalse(self.tree.reordenar([sub], self.rutas[1]))
        self.assertTrue(sub.exists())

    def test_orden_se_conserva_al_cambiar_proyecto(self):
        self.mostrar()
        self.tree.reordenar([self.rutas[2]], self.rutas[0])
        otra = self.base / 'otra'; otra.mkdir()
        self.window.cargar_carpeta(otra); self.window.cargar_carpeta(self.folder)
        self.assertEqual(self.nombres(), ['gamma', 'alfa', 'beta'])

    def test_preferencias_restauran_orden_al_reabrir(self):
        self.mostrar(); self.tree.reordenar([self.rutas[2]], self.rutas[0])
        settings = QSettings(str(self.base / 'orden.ini'), QSettings.Format.IniFormat)
        self.window.settings = settings; self.window.guardar_preferencias()
        otra = Ventana(fixtures.ROOT, preferencias=settings)
        try:
            self.esperar(lambda: otra.tree.filtro.rowCount(otra.tree.rootIndex()) == 3)
            padre = otra.tree.rootIndex()
            nombres = [otra.tree._path(otra.tree.filtro.index(i, 0, padre)).name for i in range(3)]
            self.assertEqual(nombres, ['gamma', 'alfa', 'beta'])
        finally:
            otra.close(); otra.deleteLater(); APP.processEvents()

    def test_restaurar_alfabetico_conserva_carpetas_y_buffers(self):
        self.mostrar(); editor = self.window.abrir(self.rutas[0] / 'uno.cls')
        editor.insertPlainText('# local\n')
        self.tree.reordenar([self.rutas[2]], self.rutas[0])
        self.tree.orden_alfabetico(self.folder)
        self.assertEqual(self.nombres(), ['alfa', 'beta', 'gamma'])
        self.assertIn('# local', editor.toPlainText()); self.assertTrue(editor.document().isModified())

    def test_renombrar_preserva_orden_y_orden_de_descendientes(self):
        self.mostrar()
        dos = self.rutas[0] / 'dos.cls'; dos.write_text(fixtures.TEXTO)
        self.tree.expandir_ruta(self.rutas[0])
        self.esperar(lambda: self.tree.indice_ruta(dos).isValid())
        QTest.qWait(150)
        self.tree.reordenar([dos], self.rutas[0] / 'uno.cls')
        self.tree.reordenar([self.rutas[2]], self.rutas[0])
        destino = self.folder / 'renombrada'
        self.assertTrue(self.window.mover_archivo(self.rutas[0], destino)); self.terminar()
        self.esperar(lambda: self.nombres() == ['gamma', 'renombrada', 'beta'])
        self.assertEqual(self.nombres(), ['gamma', 'renombrada', 'beta'])
        self.tree.expandir_ruta(destino)
        self.esperar(lambda: self.tree.filtro.rowCount(self.tree.indice_ruta(destino)) == 2)
        self.assertEqual(self.nombres(destino), ['dos.cls', 'uno.cls'])

    def test_imagen_con_alpha_y_varias_entradas(self):
        self.mostrar()
        pixmap = self.tree.imagen_arrastre(self.rutas + self.rutas)
        self.assertFalse(pixmap.isNull())
        imagen = pixmap.toImage()
        self.assertTrue(imagen.hasAlphaChannel())
        alfa = {imagen.pixelColor(x, y).alpha() for x in range(imagen.width()) for y in range(imagen.height())}
        self.assertIn(0, alfa); self.assertTrue(any(0 < a < 255 for a in alfa))
        with patch('ide.interfaz.explorador.QDrag') as drag:
            self.tree.setCurrentIndex(self.tree.indice_ruta(self.rutas[0]))
            self.tree.startDrag(Qt.DropAction.MoveAction)
            drag.return_value.setPixmap.assert_called_once()
            drag.return_value.setHotSpot.assert_called_once()

    def test_validacion_y_limite_de_configuracion(self):
        limpio = preferencias.validar({'version': 1, 'orden_explorador': {
            '../fuera': ['uno'], '/absoluta': ['dos'], '.': ['alfa', '../fuera', 'a/b', 'alfa', 'beta']}})
        self.assertEqual(limpio['orden_explorador'], {'.': ['alfa', 'beta']})
        settings = QSettings(str(self.base / 'limite.ini'), QSettings.Format.IniFormat)
        datos = {'version': 1, 'proyecto': str(self.folder),
                 'orden_explorador': {f'carpeta{i}': ['x' * 200 + str(j) for j in range(100)] for i in range(30)},
                 'carpetas_abiertas': ['y' * 100 + str(i) for i in range(800)]}
        self.assertTrue(preferencias.guardar(settings, datos))
        self.assertLessEqual(len(settings.value('estado_v1')), 32768)
        self.assertEqual(preferencias.cargar(settings)['proyecto'], str(self.folder))
