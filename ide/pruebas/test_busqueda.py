"""Búsqueda literal del proyecto y navegación sin alterar los documentos."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
from PySide6.QtCore import Qt, QTimer
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QPushButton
from ide.busqueda import Coincidencia, ResultadoBusqueda, buscar_proyecto
from ide.pruebas import test_carpetas as fixtures

APP = fixtures.APP


class MotorBusqueda(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.raiz = Path(self.tmp.name)
        self.carpeta = self.raiz / 'ejemplos'; self.carpeta.mkdir()
        self.archivo = self.carpeta / 'suma.cls'
        self.archivo.write_text('F main [] : void ;\n# Total total a+b\nE ; .\n')

    def test_nombres_rutas_y_carpetas(self):
        self.assertEqual([h.archivo for h in buscar_proyecto(self.raiz, 'suma', 'archivos').coincidencias], [self.archivo])
        self.assertEqual([h.archivo for h in buscar_proyecto(self.raiz, 'ejemplos', 'carpetas').coincidencias], [self.carpeta])
        self.assertEqual(len(buscar_proyecto(self.raiz, 'ejemplos/suma', 'archivos').coincidencias), 1)

    def test_codigo_literal_y_mayusculas(self):
        resultado = buscar_proyecto(self.raiz, 'total', 'codigo')
        self.assertEqual([(h.linea, h.columna) for h in resultado.coincidencias], [(2, 3), (2, 9)])
        self.assertEqual(len(buscar_proyecto(self.raiz, 'total', 'codigo', True).coincidencias), 1)
        self.assertEqual(len(buscar_proyecto(self.raiz, 'a+b', 'codigo').coincidencias), 1)
        self.assertFalse(buscar_proyecto(self.raiz, 'a.*b', 'codigo').coincidencias)

    def test_buffers_predominan_y_documentos_con_ruta_aun_ausente(self):
        nuevo = self.carpeta / 'nuevo.cls'
        buffers = {self.archivo: '# local sin guardar', nuevo: '# local nuevo'}
        self.assertEqual(len(buscar_proyecto(self.raiz, 'local', 'codigo', buffers=buffers).coincidencias), 2)
        self.assertFalse(buscar_proyecto(self.raiz, 'Total', 'codigo', buffers=buffers).coincidencias)
        self.assertFalse(nuevo.exists())

    def test_excluye_dependencias_enlaces_y_otras_extensiones(self):
        (self.raiz / 'build').mkdir(); (self.raiz / 'build/oculto.cls').write_text('aguja')
        (self.raiz / 'oculto.txt').write_text('aguja')
        (self.raiz / 'enlace.cls').symlink_to(self.archivo)
        (self.raiz / 'bucle').symlink_to(self.raiz, target_is_directory=True)
        resultado = buscar_proyecto(self.raiz, 'aguja', buffers={self.raiz / 'build/otro.cls': 'aguja'})
        self.assertFalse(resultado.coincidencias)

    def test_limites_y_archivos_no_utf8_se_informan(self):
        self.archivo.write_text('x ' * 30)
        resultado = buscar_proyecto(self.raiz, 'x', 'codigo', limite=4)
        self.assertEqual(len(resultado.coincidencias), 4); self.assertTrue(resultado.avisos)
        resultado = buscar_proyecto(self.raiz, 'x', 'codigo', max_archivo=2)
        self.assertFalse(resultado.coincidencias); self.assertTrue(resultado.avisos)
        self.archivo.write_bytes(b'\xff\xfe')
        self.assertTrue(buscar_proyecto(self.raiz, 'x', 'codigo').avisos)

    def test_limite_recorrido_y_cancelacion(self):
        resultado = buscar_proyecto(self.raiz, 'x', max_entradas=0)
        self.assertTrue(resultado.avisos)
        self.assertTrue(buscar_proyecto(self.raiz, 'x', cancelar=lambda: True).cancelado)
        self.assertFalse(buscar_proyecto(self.raiz, ' ').coincidencias)


class InterfazBusqueda(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def preparar(self):
        self.sub = self.folder / 'ejemplos'; self.sub.mkdir()
        self.path = self.sub / 'suma.cls'
        self.path.write_text('F main [] : void ;\n# 😀 total total\nE ; .\n')
        self.panel = self.window.busqueda_proyecto
        self.window.resize(1000, 700); self.window.show()
        self.esperar(lambda: self.window.tree.indice_ruta(self.sub).isValid())
        QTest.qWait(150)

    def buscar(self, consulta, modo='todo'):
        self.panel.modo.setCurrentIndex(self.panel.modo.findData(modo))
        self.panel.campo.setText(consulta)
        self.panel.iniciar()
        self.esperar(lambda: self.panel.ultimo is not None and self.panel.task is None and not self.panel.timer.isActive())
        return self.panel.ultimo.coincidencias

    def test_reemplaza_boton_y_conserva_abrir_carpeta_en_menu(self):
        self.preparar()
        self.assertFalse(any(b.text() == 'Abrir carpeta…' for b in self.window.sidebar.findChildren(QPushButton)))
        archivo = self.window.menus.actions()[0].menu()
        self.assertIn('Abrir carpeta…', [a.text() for a in archivo.actions()])
        self.window.files_button.setChecked(False)
        self.window.enfocar_busqueda_proyecto()
        self.assertTrue(self.window.sidebar.isVisible())
        self.assertIs(self.panel.campo, self.window.focusWidget())

    def test_modos_y_abrir_archivo(self):
        self.preparar()
        hits = self.buscar('suma', 'archivos'); self.assertEqual(len(hits), 1)
        self.window.ir_resultado_busqueda(hits[0])
        self.assertEqual(self.window.editor().path, self.path)
        hits = self.buscar('total', 'codigo'); self.assertEqual(len(hits), 2)
        self.assertTrue(all(h.tipo == 'codigo' for h in hits))

    def test_navegacion_unicode_y_cambio_sin_guardar(self):
        self.preparar()
        editor = self.window.abrir(self.path)
        hits = self.buscar('total', 'codigo')
        self.window.ir_resultado_busqueda(hits[1])
        self.assertEqual(editor.textCursor().selectedText(), 'total')
        self.assertEqual(editor.textCursor().blockNumber(), 1)
        editor.insertPlainText('resultado')
        self.esperar(lambda: self.panel.ultimo is not None and not self.panel.timer.isActive()
                     and self.panel.task is None and len(self.panel.ultimo.coincidencias) == 1)
        self.assertTrue(editor.document().isModified())
        self.assertEqual(self.path.read_text().count('total'), 2)
        self.assertEqual(editor.toPlainText().count('total'), 1)

    def test_resultado_obsoleto_no_selecciona_otra_linea(self):
        self.preparar()
        editor = self.window.abrir(self.path)
        hit = self.buscar('total', 'codigo')[0]
        editor.setPlainText('# ahora es otro texto\n')
        self.window.ir_resultado_busqueda(hit)
        self.assertFalse(editor.textCursor().hasSelection())
        self.assertIn('cambió', self.window.statusBar().currentMessage())

    def test_carpeta_vuelve_al_arbol_y_la_revela(self):
        self.preparar()
        hit = self.buscar('ejemplos', 'carpetas')[0]
        self.window.ir_resultado_busqueda(hit)
        self.assertEqual(self.panel.campo.text(), '')
        self.assertIs(self.panel.paginas.currentWidget(), self.window.tree)
        self.esperar(lambda: self.window.tree.ruta_actual() == self.sub)
        self.assertTrue(self.window.tree.isExpanded(self.window.tree.indice_ruta(self.sub)))

    def test_limpiar_escape_y_sin_coincidencias(self):
        self.preparar(); self.buscar('inexistente')
        self.assertIn('Sin coincidencias', self.panel.estado.text())
        self.panel.campo.setFocus(); QTest.keyClick(self.panel.campo, Qt.Key.Key_Escape)
        self.assertEqual(self.panel.campo.text(), '')
        self.assertIs(self.panel.paginas.currentWidget(), self.window.tree)

    def test_cambio_proyecto_ignora_resultados_previos(self):
        self.preparar(); self.buscar('total')
        anterior = self.panel.generacion
        otro = self.base / 'otro'; otro.mkdir()
        self.window.cargar_carpeta(otro)
        self.panel.recibir(anterior, ResultadoBusqueda((Coincidencia('archivo', self.path),)))
        self.esperar(lambda: self.panel.ultimo is not None and self.panel.task is None and not self.panel.timer.isActive()
                     and not self.panel.ultimo.coincidencias)
        self.assertEqual(self.panel.raiz, otro)

    def test_worker_responsivo_cancelado_al_limpiar(self):
        self.preparar()
        iniciado = threading.Event()
        def lento(**kwargs):
            iniciado.set()
            while not kwargs['cancelar'](): time.sleep(.005)
            return ResultadoBusqueda(cancelado=True)
        with patch('ide.interfaz.busqueda.buscar_proyecto', side_effect=lento):
            self.panel.campo.setText('total'); self.panel.iniciar()
            self.assertTrue(iniciado.wait(2))
            latidos = []; QTimer.singleShot(0, lambda: latidos.append(True)); APP.processEvents()
            self.assertTrue(latidos)
            self.panel.campo.clear()
            self.esperar(lambda: self.panel.task is None)
            self.assertIsNone(self.panel.ultimo)
            self.assertIs(self.panel.paginas.currentWidget(), self.window.tree)
