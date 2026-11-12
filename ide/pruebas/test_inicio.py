"""Arranque: barrera visual/carga, omisión, argumentos y preferencias aisladas."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QRectF, QSettings, Qt
from PySide6.QtGui import QImage, QPainter
from PySide6.QtTest import QSignalSpy, QTest
from PySide6.QtWidgets import QApplication, QWidget

from ide import preferencias
from ide.interfaz.inicio import Inicio, MarcaInicio, Presentacion

ROOT = Path(__file__).resolve().parents[2]
APP = QApplication.instance() or QApplication([])


class VentanaControlada(QWidget):
    def __init__(self, root, *, preferencias=None):
        super().__init__()
        self.carpetas = []
        self.archivos = []

    def cargar_carpeta(self, path):
        self.carpetas.append(path)

    def abrir(self, path):
        self.archivos.append(path)


class Arranque(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.settings = QSettings(str(Path(self.tmp.name) / 'estado.ini'), QSettings.Format.IniFormat)
        self.inicios = []

    def crear(self, argumentos=()):
        inicio = Inicio(APP, ROOT, argumentos, settings=self.settings, fabrica=VentanaControlada)
        self.inicios.append(inicio)
        return inicio

    def tearDown(self):
        for inicio in self.inicios:
            inicio.timer.stop()
            if inicio.presentacion:
                inicio.presentacion.animacion.stop()
                inicio.presentacion.hide()
                inicio.presentacion.deleteLater()
            if inicio.ventana:
                inicio.ventana.hide()
                inicio.ventana.deleteLater()
            inicio.deleteLater()
        APP.processEvents()

    def cargar(self, inicio):
        # Pasos reales, sin depender de la velocidad del equipo.
        inicio.timer.stop()
        while not inicio.lista:
            inicio.paso()
            inicio.timer.stop()

    def test_carga_primero_espera_animacion_y_muestra_una_vez(self):
        inicio = self.crear()
        inicio.iniciar()
        self.cargar(inicio)
        self.assertFalse(inicio.ventana.isVisible())
        presentacion = inicio.presentacion
        with patch.object(inicio.ventana, 'show', wraps=inicio.ventana.show) as mostrar:
            presentacion.animacion.setCurrentTime(Presentacion.DURACION)
            presentacion.omitir()
            inicio.mostrar_si_lista()
            self.assertEqual(mostrar.call_count, 1)
        self.assertTrue(inicio.ventana.isVisible())
        self.assertIsNone(inicio.presentacion)

    def test_escape_antes_de_cargar_no_cancela_archivos(self):
        rutas = [str(Path(self.tmp.name) / 'con espacios.cls'), '/tmp/otro.cls']
        inicio = self.crear(rutas)
        inicio.iniciar()
        inicio.timer.stop()
        QTest.keyClick(inicio.presentacion, Qt.Key.Key_Escape)
        self.assertTrue(inicio.visual_terminada)
        self.assertFalse(inicio.mostrada)
        self.assertFalse(inicio.presentacion.animacion.state() == inicio.presentacion.animacion.State.Running)
        self.cargar(inicio)
        self.assertEqual(inicio.ventana.carpetas, [Path(rutas[0]).parent])
        self.assertEqual(inicio.ventana.archivos, list(map(Path, rutas)))
        self.assertTrue(inicio.mostrada)

    def test_clic_emite_una_vez_y_no_espera_reloj(self):
        inicio = self.crear()
        inicio.iniciar()
        inicio.timer.stop()
        espia = QSignalSpy(inicio.presentacion.terminada)
        QTest.mouseClick(inicio.presentacion, Qt.MouseButton.LeftButton)
        QTest.mouseClick(inicio.presentacion, Qt.MouseButton.LeftButton)
        self.assertEqual(espia.count(), 1)
        self.assertFalse(inicio.lista)
        self.cargar(inicio)
        self.assertTrue(inicio.ventana.isVisible())

    def test_preferencia_persistente_evade_presentacion(self):
        self.assertTrue(preferencias.guardar(self.settings, {'version': 1, 'animacion_inicio': False}))
        otro = QSettings(self.settings.fileName(), QSettings.Format.IniFormat)
        self.assertIs(preferencias.cargar(otro)['animacion_inicio'], False)
        inicio = self.crear()
        inicio.iniciar()
        self.assertIsNone(inicio.presentacion)
        self.cargar(inicio)
        self.assertTrue(inicio.ventana.isVisible())

    def test_preferencia_invalida_conserva_valor_predeterminado(self):
        self.assertNotIn('animacion_inicio', preferencias.validar({'version': 1, 'animacion_inicio': 'false'}))
        inicio = self.crear()
        inicio.iniciar()
        self.assertIsNotNone(inicio.presentacion)

    def test_eventos_inicializan_sin_esperar_la_presentacion(self):
        inicio = self.crear()
        inicio.iniciar()
        for _ in range(10):
            APP.processEvents()
        self.assertTrue(inicio.lista)
        self.assertFalse(inicio.visual_terminada)
        self.assertFalse(inicio.ventana.isVisible())

    def test_final_conserva_geometria_y_colores_sin_salto_en_1x_y_2x(self):
        marca = MarcaInicio(ROOT)
        for escala in (1, 2):
            with self.subTest(escala=escala):
                imagenes = []
                for tiempo in (marca.DURACION - 1, marca.DURACION):
                    imagen = QImage(1600 * escala, 900 * escala,
                                    QImage.Format.Format_ARGB32_Premultiplied)
                    imagen.setDevicePixelRatio(escala)
                    pintor = QPainter(imagen)
                    marca.dibujar(pintor, QRectF(0, 0, 1600, 900), tiempo)
                    pintor.end()
                    imagenes.append(imagen)
                # El último milisegundo aún gira: admitir su cambio subpíxel,
                # pero detectar una recolocación brusca al SVG estático final.
                antes = imagenes[0].copy(220 * escala, 280 * escala, 330 * escala, 340 * escala)
                despues = imagenes[1].copy(220 * escala, 280 * escala, 330 * escala, 340 * escala)
                a, b = bytes(antes.constBits()), bytes(despues.constBits())
                diferencia = sum(abs(x - y) for x, y in zip(a, b)) / len(a)
                self.assertLess(diferencia, 1.)
                final = imagenes[1]
                # Interior de tres fragmentos distintos: solo el segundo morado.
                for x, y, color in ((385, 303, '#f3ad3d'), (512, 410, '#ab83df'),
                                    (512, 500, '#f3ad3d'), (1090, 450, '#ab83df')):
                    self.assertEqual(final.pixelColor(x * escala, y * escala).name(), color)

    def test_hexagono_sigue_girando_durante_letras_y_al_final(self):
        marca = MarcaInicio(ROOT)
        for inicio in (2400, marca.FIN_LETRAS + 100):
            with self.subTest(inicio=inicio):
                imagenes = []
                for tiempo in (inicio, inicio + 100):
                    imagen = QImage(1600, 900, QImage.Format.Format_ARGB32_Premultiplied)
                    pintor = QPainter(imagen)
                    marca.dibujar(pintor, QRectF(0, 0, 1600, 900), tiempo)
                    pintor.end()
                    # El desplazamiento ya terminó y el recorte no contiene
                    # letras: el cambio debe provenir del giro del hexágono.
                    imagenes.append(imagen.copy(210, 270, 350, 360))
                self.assertNotEqual(imagenes[0], imagenes[1])

    def test_fallo_de_inicializacion_informa_y_sale(self):
        inicio = self.crear()
        inicio.fabrica = lambda *a, **kw: (_ for _ in ()).throw(RuntimeError('fallo controlado'))
        inicio.iniciar()
        inicio.timer.stop()
        with patch('ide.interfaz.inicio.QMessageBox.critical') as mensaje, patch.object(APP, 'exit') as salir, patch('traceback.print_exc'):
            inicio.paso()
        self.assertEqual(mensaje.call_args.args[-1], 'fallo controlado')
        salir.assert_called_once_with(1)
        self.assertFalse(inicio.presentacion.isVisible())

    def test_accion_real_guarda_y_restaura_sin_perder_estado(self):
        from ide.interfaz.ventana import Ventana
        ventanas = []
        try:
            ventana = Ventana(ROOT, preferencias=self.settings)
            ventanas.append(ventana)
            ventana.animacion_inicio_action.setChecked(False)
            self.assertIs(preferencias.cargar(self.settings)['animacion_inicio'], False)
            self.assertIn('asistencia', preferencias.cargar(self.settings))
            otra = Ventana(ROOT, preferencias=self.settings)
            ventanas.append(otra)
            self.assertFalse(otra.animacion_inicio_action.isChecked())
            otra.animacion_inicio_action.setChecked(True)
            self.assertTrue(preferencias.cargar(self.settings)['animacion_inicio'])
        finally:
            for ventana in ventanas:
                ventana.close()
                ventana.deleteLater()
            APP.processEvents()


if __name__ == '__main__':
    unittest.main()
