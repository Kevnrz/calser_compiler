"""M1B: invocaciones reales, destinos, preferencias y vigencia de F6."""
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QSettings, QTimer, Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QMessageBox, QFileDialog
from ide import compilacion, preferencias
from ide.interfaz.opciones_compilacion import OpcionesCompilacion
from ide.interfaz.ventana import Ventana
from ide.pruebas import test_ejecucion as ejecucion
APP = ejecucion.APP


class Perfiles(unittest.TestCase):
    def test_opciones_incompatibles_y_datos_invalidos(self):
        for modo in compilacion.MODOS:
            datos = compilacion.validar({'modo': modo, **{k: True for k in compilacion.OPCIONES}})
            for clave, (_, _, modos) in compilacion.OPCIONES.items():
                self.assertEqual(datos[clave], modo in modos)
        self.assertEqual(compilacion.validar({'modo': [], 'asm': 'yes', 'destino': 'a\0b'})['destino'], '')
        self.assertFalse(compilacion.validar({'asm': 'yes'})['asm'])

    def test_entradas_explicitas_sin_confundir_cls_y_asm(self):
        with self.assertRaises(ValueError):
            compilacion.preparar({'modo': 'ensamblar'}, Path('/tmp/programa.cls'))
        with self.assertRaises(ValueError):
            compilacion.preparar({'modo': 'compilar'}, Path('/tmp/programa.asm'))

    def test_nombres_iguales_no_colisionan_y_rutas_son_argumentos(self):
        a = compilacion.preparar({}, Path('/tmp/con espacios/a/uno.cls'))
        b = compilacion.preparar({}, Path('/tmp/con espacios/b/uno.cls'))
        self.assertNotEqual(a[1], b[1])
        self.assertIn('/tmp/con espacios/a/uno.cls', a[2])
        self.assertEqual(len(a[3]), 4)

    def test_dialogo_desactiva_casillas_y_destino_segun_modo(self):
        dialogo = OpcionesCompilacion({})
        self.addCleanup(dialogo.deleteLater)
        dialogo.modo.setCurrentIndex(dialogo.modo.findData('analizar'))
        self.assertFalse(dialogo.destino.isEnabled())
        self.assertFalse(dialogo.casillas['asm'].isEnabled())
        self.assertTrue(dialogo.casillas['tiempo'].isEnabled())
        self.assertFalse(dialogo.datos()['asm'])
        dialogo.modo.setCurrentIndex(dialogo.modo.findData('ast'))
        self.assertTrue(dialogo.destino.isEnabled())
        self.assertFalse(dialogo.casillas['tablas'].isEnabled())


class Operaciones(unittest.TestCase):
    setUp = ejecucion.Ejecucion.setUp
    limpiar = ejecucion.Ejecucion.limpiar
    esperar = ejecucion.Ejecucion.esperar
    terminar = ejecucion.Ejecucion.terminar
    compilar = ejecucion.Ejecucion.compilar

    def operar(self, modo, **opciones):
        self.window.perfil_compilacion = compilacion.validar({'modo': modo, **opciones})
        self.window.compilar()
        self.esperar(lambda: self.window.process is None)
        return self.window.output.toPlainText()

    def test_analisis_no_toca_salidas_previas_ni_registra_f6(self):
        info = self.compilar()
        anterior = info.asm.stat().st_mtime_ns, info.asm.read_bytes()
        texto = self.operar('analizar', tiempo=True, progreso=True)
        self.assertIn('Análisis correcto', texto)
        self.assertEqual(anterior, (info.asm.stat().st_mtime_ns, info.asm.read_bytes()))
        self.assertNotIn(self.editor, self.window.compilados)
        self.assertEqual(self.window.compilation_outputs, [])

    def test_compilar_sin_s_no_reutiliza_asm_anterior(self):
        info = self.compilar()
        anterior = info.asm.read_bytes()
        self.assertIn('Compilación correcta', self.operar('compilar', asm=False))
        self.assertEqual(anterior, info.asm.read_bytes())
        self.assertNotIn(self.editor, self.window.compilados)
        with patch.object(self.window, 'error'):
            self.assertFalse(self.window.ejecutar())

    def test_ast_solo_sintaxis_aunque_haya_error_semantico(self):
        self.editor.setPlainText('F main [] : void ; x = 1 ; E ; .')
        self.window.guardar()
        texto = self.operar('ast', ast_texto=True)
        self.assertIn('semántica no comprobada', texto)
        destino = self.window.compilation_output
        self.assertIsInstance(json.loads(destino.read_text()), dict)
        self.assertFalse(destino.with_suffix('.bin').exists())
        self.assertNotIn(self.editor, self.window.compilados)

    def test_ensamblar_con_selector_y_destino_con_espacios(self):
        info = self.compilar()
        destino = self.folder / 'salida con espacios' / 'otro.bin'
        with patch.object(QFileDialog, 'getOpenFileName', return_value=(str(info.asm), '')):
            texto = self.operar('ensamblar', destino=str(destino), bundles=True, tiempo=True)
        self.assertIn('Ensamblado correcto', texto)
        self.assertTrue(destino.is_file())
        self.assertEqual(destino.read_bytes(), info.asm.with_suffix('.bin').read_bytes())
        self.assertNotIn(self.window.editor(), self.window.compilados)

    def test_destino_personalizado_f6_usa_el_nombre_elegido(self):
        destino = self.folder / 'otro nombre.bin'
        self.assertIn('Compilación correcta', self.operar('compilar', destino=str(destino)))
        info = self.window.compilados[self.editor]
        self.assertEqual(info.asm, destino.with_suffix('.asm'))
        self.assertTrue(info.vigente(self.editor))

    def test_protege_salidas_abiertas_y_enlaces_duros(self):
        destino = self.folder / 'protegido.bin'
        os.link(self.path, destino)
        with patch.object(self.window, 'error') as error:
            self.operar('compilar', destino=str(destino))
            error.assert_called_once()
        self.assertIsNone(self.window.process)
        self.assertEqual(self.path.read_text(), self.editor.toPlainText())

    def test_rechaza_enlaces_simbolicos_sin_escribir(self):
        destino = self.folder / 'protegido.bin'
        destino.symlink_to(self.path)
        with patch.object(self.window, 'error') as error:
            self.operar('compilar', destino=str(destino))
            error.assert_called_once()
        self.assertIsNone(self.window.process)

    def test_sobrescritura_custom_exige_eleccion(self):
        destino = self.folder / 'previo.json'
        destino.write_text('conservar')
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.No):
            self.operar('ast', destino=str(destino))
        self.assertEqual(destino.read_text(), 'conservar')
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.Yes):
            self.operar('ast', destino=str(destino))
        self.assertIsInstance(json.loads(destino.read_text()), dict)

    def test_boton_y_f5_ejecutan_el_mismo_perfil(self):
        self.window.seleccionar_modo('analizar')
        self.window.show()
        APP.processEvents()
        self.window.compile_button.click()
        self.esperar(lambda: self.window.process is None)
        self.assertIn('Análisis correcto', self.window.output.toPlainText())
        primera = self.window.output.toPlainText()
        self.window.output.clear()
        self.window.activateWindow()
        self.editor.setFocus()
        APP.processEvents()
        QTest.keyClick(self.editor, Qt.Key.Key_F5)
        self.esperar(lambda: self.window.process is None and bool(self.window.output.toPlainText()))
        self.assertEqual(primera, self.window.output.toPlainText())
        self.assertEqual(self.window.trabajo_compilacion['modo'], 'analizar')
        self.assertIn('--check', self.window.output.toPlainText())

    def test_opciones_reales_listados_y_tiempos(self):
        texto = self.operar('compilar', tablas=True, bundles=True, ast_texto=True,
                            tiempo=True, progreso=True, sin_schedule=True)
        self.assertIn('Compilación correcta', texto)
        self.assertIn(self.editor, self.window.compilados)

    def test_compilador_que_no_renueva_salidas_no_habilita_f6(self):
        self.compilar()
        falso = self.folder / 'falso ccom'
        falso.write_text('#!/bin/sh\nexit 0\n')
        falso.chmod(0o755)
        self.window.configured_ccom = str(falso)
        texto = self.operar('compilar')
        self.assertIn('no renovó', texto)
        self.assertNotIn(self.editor, self.window.compilados)

    def test_preferencias_restauran_y_filtran_perfil(self):
        settings = QSettings(str(self.base / 'prefs.ini'), QSettings.Format.IniFormat)
        self.window.settings = settings
        self.window.perfil_compilacion = compilacion.validar({'modo': 'ast', 'destino': str(self.folder / 'arbol.json'), 'tablas': True, 'tiempo': True})
        self.window.guardar_preferencias()
        self.assertFalse(preferencias.cargar(settings)['compilacion']['tablas'])
        otra = Ventana(self.window.root, preferencias=settings)
        try:
            self.assertEqual(otra.perfil_compilacion, self.window.perfil_compilacion)
            self.assertIn('Exportar AST', otra.compile_button.text())
        finally:
            for i in range(otra.tabs.count()):
                otra.tabs.widget(i).document().setModified(False)
            otra.close()
            otra.deleteLater()
            APP.processEvents()

    def test_cancelacion_y_edicion_no_registran_resultados(self):
        falso = self.folder / 'ccom lento'
        falso.write_text('#!/usr/bin/python3\nimport time\ntime.sleep(10)\n')
        falso.chmod(0o755)
        self.window.configured_ccom = str(falso)
        self.window.compilar()
        self.assertFalse(self.window.opciones_compilacion_action.isEnabled())
        self.editor.insertPlainText('# cambio')
        self.window.seleccionar_modo('analizar')
        self.assertEqual(self.window.perfil_compilacion['modo'], 'compilar')
        self.window.cancelar()
        self.esperar(lambda: self.window.process is None)
        self.assertNotIn(self.editor, self.window.compilados)
        self.assertTrue(self.window.opciones_compilacion_action.isEnabled())

    def test_asm_no_se_envia_al_analisis_calser(self):
        info = self.compilar()
        editor = self.window.abrir(info.asm)
        with patch.object(self.window.analizador, 'solicitar') as lexico, patch.object(self.window.comprobador, 'programar') as comprobar:
            editor.emitir_analisis()
            self.window.programar_comprobacion(editor)
            lexico.assert_not_called()
            comprobar.assert_not_called()
        self.assertIn('No aplica', editor.estado_analisis)
        self.assertEqual(editor.errores_lexicos, [])
