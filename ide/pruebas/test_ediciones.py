"""Contrato y ediciones de asistencia: texto original, Unicode e historial Qt."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from dataclasses import FrozenInstanceError, replace
from pathlib import Path
import tempfile
import threading
import unittest
from PySide6.QtGui import QTextCursor
from PySide6.QtWidgets import QApplication
from shiboken6 import delete
from ide.nucleo.asistencia import (ContextoDocumento, EdicionTexto, EstadoAsistencia,
    PrecondicionTexto, Reparacion, ResultadoReparaciones, ResultadoSugerencias, Sugerencia)
from ide.ediciones import (MAX_EDICIONES, MAX_TEXTO, aplicar_ediciones, mapa_utf16,
                           python_a_utf16, utf16_a_python, validar_ediciones)
from ide.interfaz.ediciones import aplicar_reparacion, aplicar_sugerencia
from ide.interfaz.editor import Editor

APP = QApplication.instance() or QApplication([])
ROOT = Path(__file__).resolve().parents[2]


class Contrato(unittest.TestCase):
    def test_resultados_distinguen_vacio_bloqueo_limite_cancelacion_y_error(self):
        for estado in EstadoAsistencia:
            if estado == EstadoAsistencia.OK:
                continue
            r = ResultadoSugerencias(estado, explicacion='motivo')
            self.assertEqual(r.sugerencias, ())
        for estado in (EstadoAsistencia.BLOQUEADO, EstadoAsistencia.LIMITE, EstadoAsistencia.ERROR):
            with self.assertRaises(ValueError): ResultadoReparaciones(estado)
        with self.assertRaises(ValueError): ResultadoSugerencias(EstadoAsistencia.OK)
        with self.assertRaises(ValueError): ResultadoSugerencias('ok')

    def test_payload_inmutable_y_tipo_compartido(self):
        sugerencia = Sugerencia('int', 'int', 0, 1, 'tipo')
        opciones = [sugerencia]; r = ResultadoSugerencias(EstadoAsistencia.OK, opciones)
        opciones.clear(); self.assertEqual(r.sugerencias, (sugerencia,))
        with self.assertRaises(FrozenInstanceError): sugerencia.texto = 'float'
        with self.assertRaises(ValueError): ResultadoSugerencias(EstadoAsistencia.BLOQUEADO, (sugerencia,), 'error')
        with self.assertRaises(ValueError): ResultadoReparaciones(EstadoAsistencia.OK, (sugerencia,))

    def test_tipos_rangos_y_cursor_se_validan(self):
        for args in ((True, 1, ''), (-1, 0, ''), (3, 1, ''), (0, 0, '\ud800')):
            with self.assertRaises(ValueError): EdicionTexto(*args)
        with self.assertRaises(ValueError): ContextoDocumento('id', 1, 'abc', 4, 0)
        with self.assertRaises(ValueError): Sugerencia('int', 'int', 0, 0, 'tipo', cursor_relativo=4)

    def test_seguridad_automatica_no_es_implicita(self):
        e = EdicionTexto(1, 1, ';')
        self.assertFalse(Reparacion('terminador', 'Agregar ;', (e,)).automatica_segura)
        with self.assertRaises(ValueError): Reparacion('terminador', 'Agregar ;', (e,), True)
        r = Reparacion('terminador', 'Agregar ;', (e,), True, ('catálogo pendiente',), validacion='candidato reanalizado')
        self.assertTrue(r.automatica_segura)

    def test_ediciones_originales_sin_desplazar_rangos_posteriores(self):
        texto = 'abc def ghi'
        ediciones = (EdicionTexto(8, 11, 'G'), EdicionTexto(0, 3, 'ALFA'), EdicionTexto(4, 7, ''))
        self.assertEqual(aplicar_ediciones(texto, ediciones), 'ALFA  G')
        self.assertEqual(aplicar_ediciones('abcd', (EdicionTexto(0, 2, 'X'), EdicionTexto(2, 4, 'Y'))), 'XY')

    def test_solapamiento_inserciones_repetidas_y_limites_ambiguos(self):
        for ediciones in ((EdicionTexto(0, 3, ''), EdicionTexto(2, 4, '')),
                          (EdicionTexto(1, 1, 'x'), EdicionTexto(1, 1, 'y')),
                          (EdicionTexto(0, 1, 'x'), EdicionTexto(1, 1, 'y')),
                          (EdicionTexto(1, 1, 'y'), EdicionTexto(1, 2, 'z'))):
            with self.assertRaises(ValueError): aplicar_ediciones('abcd', ediciones)

    def test_precondiciones_y_limites_sin_propuestas_parciales(self):
        e = EdicionTexto(0, 1, 'x')
        self.assertEqual(aplicar_ediciones('abc', (e,), (PrecondicionTexto(1, 3, 'bc'),)), 'xbc')
        with self.assertRaises(ValueError): aplicar_ediciones('abc', (e,), (PrecondicionTexto(1, 3, 'xy'),))
        with self.assertRaises(ValueError): aplicar_ediciones('abc', (EdicionTexto(5, 5, '!'),))
        with self.assertRaises(ValueError): validar_ediciones('x' * (MAX_TEXTO + 1), ())
        with self.assertRaises(ValueError): validar_ediciones('x', (e,) * (MAX_EDICIONES + 1))

    def test_unicode_python_utf16_en_ambos_sentidos(self):
        texto = 'a😀é\n𝄞z'
        for p in range(len(texto) + 1):
            self.assertEqual(utf16_a_python(texto, python_a_utf16(texto, p)), p)
        self.assertEqual(mapa_utf16(texto, (0, 2, len(texto))), {0: 0, 2: 3, len(texto): 8})
        with self.assertRaises(ValueError): utf16_a_python(texto, 2)
        with self.assertRaises(ValueError): python_a_utf16(texto, len(texto) + 1)


class AplicadorQt(unittest.TestCase):
    def setUp(self):
        self.editor = Editor(ROOT)
        self.addCleanup(self.limpiar)

    def limpiar(self):
        self.editor.detener_analisis()
        self.editor.close(); self.editor.deleteLater(); APP.processEvents()

    def preparar(self, texto, cursor=0, ancla=None):
        self.editor.setPlainText(texto); self.editor.document().setModified(False)
        self.seleccionar(cursor, ancla)
        return self.editor.contexto_asistencia()

    def seleccionar(self, cursor, ancla=None):
        texto = self.editor.toPlainText()
        c = self.editor.textCursor()
        c.setPosition(python_a_utf16(texto, cursor if ancla is None else ancla))
        c.setPosition(python_a_utf16(texto, cursor), QTextCursor.MoveMode.KeepAnchor)
        self.editor.setTextCursor(c)

    def test_varias_ediciones_una_operacion_deshacer_y_rehacer(self):
        texto = 'Vm x : int = 1\nE\n.'
        ctx = self.preparar(texto)
        posiciones = [i for i, c in enumerate(texto) if c == '\n']
        resultado = self.editor.aplicar_ediciones(ctx, tuple(EdicionTexto(i, i, ' ;') for i in posiciones))
        self.assertTrue(resultado.aplicada)
        self.assertEqual(self.editor.toPlainText(), 'Vm x : int = 1 ;\nE ;\n.')
        self.assertTrue(self.editor.document().isModified())
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), texto)
        self.assertFalse(self.editor.document().isModified())
        self.editor.redo(); self.assertEqual(self.editor.toPlainText(), 'Vm x : int = 1 ;\nE ;\n.')

    def test_deshacer_no_absorbe_edicion_anterior(self):
        self.preparar('abc', 3)
        self.editor.insertPlainText('!')
        ctx = self.editor.contexto_asistencia()
        self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 1, 'A'), EdicionTexto(2, 3, 'C')))
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), 'abc!')
        self.assertTrue(self.editor.document().isModified())
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), 'abc')

    def test_sugerencia_reemplaza_palabra_completa_y_posiciona_cursor(self):
        ctx = self.preparar('Vm total', 5)
        s = Sugerencia('contador', 'contador', 3, 8, 'identificador')
        self.assertTrue(aplicar_sugerencia(self.editor, ctx, s).aplicada)
        self.assertEqual(self.editor.toPlainText(), 'Vm contador')
        self.assertEqual(self.editor.contexto_asistencia().cursor, len('Vm contador'))
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), 'Vm total')

    def test_cursor_relativo_en_plantilla_con_unicode(self):
        ctx = self.preparar('# 😀\nF', len('# 😀\nF'))
        s = Sugerencia('función', 'F main [] : void ;\n    \n.', 4, 5, 'plantilla', cursor_relativo=22)
        self.assertTrue(aplicar_sugerencia(self.editor, ctx, s).aplicada)
        self.assertEqual(self.editor.contexto_asistencia().cursor, s.cursor_final)
        self.assertTrue(self.editor.toPlainText().startswith('# 😀\nF main'))

    def test_seleccion_directa_e_invertida_se_conservan_y_desplazan(self):
        for cursor, ancla in ((6, 3), (3, 6)):
            ctx = self.preparar('😀 abc xyz', cursor, ancla)
            self.assertTrue(self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 0, '# '),)).aplicada)
            actual = self.editor.contexto_asistencia()
            self.assertEqual((actual.cursor, actual.ancla), (cursor + 2, ancla + 2))
            self.assertEqual(self.editor.textCursor().selectedText(), 'bc ')

    def test_reemplazo_dentro_seleccion_y_caret_en_texto_eliminado(self):
        ctx = self.preparar('abcdef', 2)
        self.editor.aplicar_ediciones(ctx, (EdicionTexto(1, 4, 'X'),))
        self.assertEqual(self.editor.toPlainText(), 'aXef')
        self.assertEqual(self.editor.contexto_asistencia().cursor, 2)
        ctx = self.preparar('abcdef', 5, 1)
        self.editor.aplicar_ediciones(ctx, (EdicionTexto(2, 4, '😀'),))
        self.assertEqual(self.editor.textCursor().selectedText(), 'b😀e')

    def test_reparacion_precondicion_y_cursor_final(self):
        ctx = self.preparar('E\n.', 0)
        r = Reparacion('terminador', 'Agregar ;', (EdicionTexto(1, 1, ' ;'),),
                        precondiciones=(PrecondicionTexto(0, 1, 'E'),), cursor_final=3)
        self.assertTrue(aplicar_reparacion(self.editor, ctx, r).aplicada)
        self.assertEqual(self.editor.toPlainText(), 'E ;\n.')
        self.assertEqual(self.editor.contexto_asistencia().cursor, 3)

    def test_otro_documento_o_revision_anterior_se_rechazan(self):
        ctx = self.preparar('abc')
        e = (EdicionTexto(0, 1, 'A'),)
        self.assertEqual(self.editor.aplicar_ediciones(replace(ctx, documento='otro'), e).estado, 'obsoleta')
        self.editor.insertPlainText('x'); self.editor.undo()
        self.assertEqual(self.editor.toPlainText(), ctx.texto)
        self.assertEqual(self.editor.aplicar_ediciones(ctx, e).estado, 'obsoleta')
        self.assertEqual(self.editor.toPlainText(), 'abc')

    def test_cambio_cursor_o_seleccion_sin_edicion_invalida_contexto(self):
        ctx = self.preparar('abc', 0)
        self.seleccionar(1)
        self.assertEqual(self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 0, '!'),)).estado, 'obsoleta')
        ctx = self.preparar('abc', 2, 0)
        self.seleccionar(0, 2)
        self.assertEqual(self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 0, '!'),)).estado, 'obsoleta')

    def test_invalida_no_toca_texto_cursor_modificado_ni_historial(self):
        ctx = self.preparar('abc', 1, 0)
        for ediciones, kw in (((EdicionTexto(9, 9, 'x'),), {}),
                              ((EdicionTexto(0, 1, 'X'),), {'cursor_final': 99}),
                              ((EdicionTexto(0, 1, 'X'),), {'precondiciones': (PrecondicionTexto(0, 1, 'z'),)}),
                              ((EdicionTexto(0, 1, '\r\n'),), {})):
            r = self.editor.aplicar_ediciones(ctx, ediciones, **kw)
            self.assertEqual(r.estado, 'invalida')
            self.assertEqual(self.editor.contexto_asistencia(), ctx)
            self.assertFalse(self.editor.document().isModified())
            self.assertFalse(self.editor.document().isUndoAvailable())

    def test_solo_lectura_y_sin_cambios_no_crean_undo(self):
        ctx = self.preparar('abc', 2)
        self.assertEqual(self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 1, 'a'),)).estado, 'sin_cambios')
        self.editor.setReadOnly(True)
        self.assertEqual(self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 1, 'A'),)).estado, 'solo_lectura')
        self.assertEqual(self.editor.contexto_asistencia(), ctx)
        self.assertFalse(self.editor.document().isUndoAvailable())

    def test_archivo_crlf_unicode_y_comentarios_no_se_guardan(self):
        with tempfile.TemporaryDirectory() as carpeta:
            path = Path(carpeta) / 'programa.cls'
            original = '# 😀 café\r\nVm x : int = 3\r\nE ;\r\n.\r\n'.encode()
            path.write_bytes(original)
            self.editor.path = path
            ctx = self.preparar(original.decode())
            pos = ctx.texto.index('\n', ctx.texto.index('Vm'))
            self.assertTrue(self.editor.aplicar_ediciones(ctx, (EdicionTexto(pos, pos, ' ;'),)).aplicada)
            self.assertTrue(self.editor.toPlainText().startswith('# 😀 café\n'))
            self.assertEqual(path.read_bytes(), original)
            self.editor.undo(); self.assertEqual(self.editor.toPlainText(), ctx.texto)

    def test_no_aplica_desde_un_hilo_trabajador(self):
        ctx = self.preparar('abc')
        resultados = []
        hilo = threading.Thread(target=lambda: resultados.append(
            self.editor.aplicar_ediciones(ctx, (EdicionTexto(0, 1, 'A'),))))
        hilo.start(); hilo.join(2)
        self.assertFalse(hilo.is_alive())
        self.assertEqual(resultados[0].estado, 'invalida')
        self.assertEqual(self.editor.toPlainText(), 'abc')

    def test_editor_destruido_se_rechaza_sin_acceder_widget(self):
        otro = Editor(ROOT); otro.setPlainText('abc'); ctx = otro.contexto_asistencia()
        otro.detener_analisis(); delete(otro)
        from ide.interfaz.ediciones import aplicar_ediciones_editor
        self.assertEqual(aplicar_ediciones_editor(otro, ctx, (EdicionTexto(0, 1, 'A'),)).estado, 'obsoleta')


class IntegracionVentana(unittest.TestCase):
    def test_aplicar_actualiza_pestana_y_diagnosticos_sin_guardar(self):
        from ide.pruebas import test_carpetas as fixtures
        from ide.diagnosticos import Problema
        caso = fixtures.Carpetas()
        caso.setUp()
        try:
            path = caso.folder / 'programa.cls'
            original = 'F main [] : void ; E ; .'
            path.write_text(original)
            editor = caso.window.abrir(path)
            caso.window.results[editor] = [Problema(path, 1, 1, 'error: anterior')]
            contexto = editor.contexto_asistencia()
            resultado = editor.aplicar_ediciones(contexto, (EdicionTexto(0, 0, '# prueba\n'),))
            self.assertTrue(resultado.aplicada)
            self.assertNotIn(editor, caso.window.results)
            self.assertIn('●', caso.window.tabs.tabText(caso.window.tabs.indexOf(editor)))
            self.assertEqual(path.read_text(), original)
            self.assertTrue(editor.analisis_timer.isActive())
            editor.undo()
            self.assertEqual(editor.toPlainText(), original)
        finally:
            caso.doCleanups()
