"""C1.2 parcial: lectura de TAC real sin servicios de Calser."""
from pathlib import Path
import re
import subprocess
import unittest
from unittest.mock import patch

from PySide6.QtCore import Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QFileDialog

from ide.interfaz.editor import Resaltado
from ide.interfaz.resaltado_tac import ResaltadorTac
from ide.pruebas import test_carpetas as base

APP = base.APP
ROOT = base.ROOT


class DocumentosTac(unittest.TestCase):
    setUp = base.Carpetas.setUp
    limpiar = base.Carpetas.limpiar
    terminar = base.Carpetas.terminar
    esperar = base.Carpetas.esperar

    def generar(self):
        destino = self.folder / 'arreglos.bin'
        resultado = subprocess.run(
            [str(ROOT / 'ccom'), '-O1', '-i',
             str(ROOT / 'pruebas/p05_arreglos_llamadas.cls'), '-o', str(destino)],
            capture_output=True, text=True, timeout=30)
        self.assertEqual(resultado.returncode, 0, resultado.stderr)
        return destino.with_suffix('.tac')

    def test_abrir_real_solo_lectura_sin_consultas_ni_guardado(self):
        path = self.generar()
        original = path.read_bytes()
        with patch.object(self.window.analizador, 'solicitar') as tokens, \
                patch.object(self.window.comprobador, 'programar') as analisis, \
                patch.object(self.window.asistencia.coordinador, 'solicitar') as asistencia:
            editor = self.window.abrir(path)
            self.window.auto_action.setChecked(True)
            self.window.show()
            self.window.activateWindow()
            editor.setFocus()
            QTest.keyClicks(editor, 'texto')
            editor.programar_analisis()
            editor.emitir_analisis()
            self.window.solicitar_analisis_editor(editor, editor.documento_id, editor.revision, editor.toPlainText())
            self.window.programar_comprobacion(editor)
            self.window.solicitar_sugerencias()
            self.window.solicitar_reparaciones()
            self.window.asistencia.programar(editor, 'escritura')
            QTest.qWait(650)
            self.assertTrue(editor.isReadOnly())
            self.assertFalse(editor.analisis_timer.isActive())
            self.assertIsInstance(editor.highlighter, ResaltadorTac)
            self.assertEqual(editor.toPlainText(), original.decode())
            self.assertFalse(editor.document().isModified())
            self.assertFalse(self.window.guardar())
            self.assertFalse(self.window.guardar(como=True))
            self.assertEqual(path.read_bytes(), original)
            self.assertEqual(editor.errores_lexicos, [])
            self.assertEqual(editor.problemas, [])
            self.assertEqual(self.window.problems.topLevelItemCount(), 0)
            tokens.assert_not_called()
            analisis.assert_not_called()
            asistencia.assert_not_called()
            self.assertIs(self.window.abrir(path), editor)

    def test_resaltado_tabla_en_listados_reales(self):
        paths = [self.generar(), *sorted((ROOT / 'pruebas/optimizador/unroll').glob('*.tac'))]
        reglas = (
            (r'^function \w+', '#efbb65'), (r'\bB\d+\b', '#8bd3cc'),
            (r'\bt\d+\b', '#d6dce7'), (r'\$\w+', '#909bac'),
            (r'-?\b\d+\b', '#c1a0eb'),
            (r'\b(if|goto|return|call|param|address)\b', '#efbb65'),
            (r'\b(successors|entry|initial_data)\b', '#909bac'),
        )
        for path in paths:
            editor = self.window.abrir(path)
            editor.highlighter.rehighlight()
            for patron, color in reglas:
                encontrados = 0
                for linea, texto in enumerate(editor.toPlainText().splitlines()):
                    block = editor.document().findBlockByNumber(linea)
                    for match in re.finditer(patron, texto):
                        inicio = len(texto[:match.start()].encode('utf-16-le')) // 2
                        longitud = len(match.group().encode('utf-16-le')) // 2
                        with self.subTest(archivo=path.name, patron=patron, linea=linea):
                            for posicion in range(inicio, inicio + longitud):
                                formatos = [f.format.foreground().color().name()
                                            for f in block.layout().formats()
                                            if f.start <= posicion < f.start + f.length]
                                self.assertEqual(formatos, [color])
                        encontrados += 1
                self.assertGreater(encontrados, 0, (path, patron))

    def test_f5_no_compila_tac_en_ningun_modo(self):
        editor = self.window.abrir(self.generar())
        self.window.show()
        self.window.activateWindow()
        editor.setFocus()
        QTest.qWait(30)
        with patch('ide.interfaz.ventana.ccom.ejecutable') as ejecutable, \
                patch.object(QFileDialog, 'getOpenFileName') as selector:
            for modo in ('compilar', 'analizar', 'ast', 'ensamblar'):
                self.window.seleccionar_modo(modo)
                QTest.keyClick(editor, Qt.Key.Key_F5)
                APP.processEvents()
                self.assertIn('listado TAC', self.window.statusBar().currentMessage())
                self.assertIsNone(self.window.process)
            ejecutable.assert_not_called()
            selector.assert_not_called()

    def test_dialogo_conserva_filtros_y_agrega_tac(self):
        with patch.object(QFileDialog, 'getOpenFileName', return_value=('', '')) as dialogo:
            self.window.elegir_archivo()
        self.assertEqual(dialogo.call_args.args[3],
                         'Calser (*.cls);;Tres direcciones (*.tac);;Todos los archivos (*)')

    def test_explorador_extensiones_y_exclusiones(self):
        for sufijo in ('cls', 'tac', 'bin', 'hex', 'mem'):
            (self.folder / ('archivo.' + sufijo)).write_text('')
        (self.folder / 'build').mkdir()
        (self.folder / 'build/oculto.tac').write_text('')
        tree = self.window.tree
        def nombres():
            return {tree.model().index(i, 0, tree.rootIndex()).data()
                    for i in range(tree.model().rowCount(tree.rootIndex()))}
        self.esperar(lambda: {'archivo.cls', 'archivo.tac'} <= nombres())
        self.assertEqual(nombres(), {'archivo.cls', 'archivo.tac'})
        for i in range(tree.model().rowCount(tree.rootIndex())):
            indice = tree.model().index(i, 0, tree.rootIndex())
            self.assertFalse(indice.data(Qt.ItemDataRole.DecorationRole).isNull())

    def test_cls_conserva_analisis_sugerencias_y_resaltado(self):
        path = self.folder / 'programa.cls'
        path.write_text('F main [] : void ; Vm x : int = 3 ; E ; .\n')
        editor = self.window.abrir(path)
        self.esperar(lambda: editor.analisis_revision == editor.revision)
        self.assertFalse(editor.isReadOnly())
        self.assertIsInstance(editor.highlighter, Resaltado)
        self.assertTrue(editor.highlighter.tramos)
        self.assertEqual(editor.errores_lexicos, [])
        with patch.object(self.window.asistencia.coordinador, 'solicitar') as asistencia, \
                patch.object(self.window.comprobador, 'programar') as analisis:
            self.window.solicitar_sugerencias()
            self.assertEqual(asistencia.call_args.args[0], 'sugerencias')
            self.window.auto_action.setChecked(True)
            self.window.programar_comprobacion(editor)
            self.assertTrue(analisis.called)
