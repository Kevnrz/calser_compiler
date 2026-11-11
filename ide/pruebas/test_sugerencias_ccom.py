"""Motor M3 real: contrato, transporte UTF-8 y conexión del IDE sin dobles de propuestas."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide.sugerencias_ccom import sugerir, interpretar
from ide.nucleo.asistencia import EstadoAsistencia as E
from ide import ccom
from ide.pruebas import test_carpetas as fixtures

ROOT = fixtures.ROOT
APP = fixtures.APP
FUENTE = Path('/tmp/snapshot-suggest.cls')


def informe():
    return {'schema_version': 1, 'operation': 'suggest', 'offset_unit': 'utf8',
            'source': str(FUENTE), 'cursor': 14, 'exit_code': 0, 'status': 'ok', 'explanation': '',
            'suggestions': [{'label': 'void', 'text': 'void', 'start': 12, 'end': 14,
                             'category': 'palabra', 'detail': 'Bison/LAC'}]}


class MotorNativo(unittest.TestCase):
    def test_casos_del_motor_entregado_y_orden_estable(self):
        for texto, esperado in (('', ['F', 'Vi', 'Vm']), ('F main [] : vo', ['void']),
                                ('Vm dato : ', ['bool', 'int']), ('V', ['Vi', 'Vm'])):
            a = sugerir(texto, len(texto)); b = sugerir(texto, len(texto))
            self.assertEqual(a, b)
            self.assertEqual(a.estado, E.OK)
            self.assertEqual([s.texto for s in a.sugerencias], esperado)
        a = sugerir('F ', 2)
        self.assertEqual(a.estado, E.SIN_OPCIONES)
        self.assertFalse(a.sugerencias)

    def test_no_llama_lexer_gramatica_o_parser_python(self):
        with patch('ide.nucleo.lexico.analizar', side_effect=AssertionError('No usar Python')),              patch('ide.nucleo.parser.contexto_en', side_effect=AssertionError('No usar LL1')):
            a = sugerir('F main [] : vo', 14)
        self.assertEqual(a.sugerencias[0].texto, 'void')

    def test_cursor_medio_unicode_y_reemplazo_completo(self):
        texto = '# café 😀\r\nF main [] : void ; E ; .'
        cursor = texto.index('void') + 2
        a = sugerir(texto, cursor)
        s = a.sugerencias[0]
        self.assertEqual((s.inicio, s.fin), (texto.index('void'), texto.index('void') + 4))
        self.assertEqual(texto[:s.inicio] + s.texto + texto[s.fin:], texto)

    def test_comentarios_errores_previos_y_sufijo_posterior(self):
        for texto in ('# comentario', '#[ sin cerrar', '@ F ', 'F main [] : void ; Vm x : int = ; '):
            a = sugerir(texto, len(texto))
            self.assertEqual(a.estado, E.BLOQUEADO)
            self.assertTrue(a.explicacion)
            self.assertFalse(a.sugerencias)
        texto = 'F main [] : vo @ error posterior'
        self.assertEqual(sugerir(texto, 14).sugerencias[0].texto, 'void')
        self.assertEqual(sugerir('F main [] : void ; E ; . ', 25).estado, E.OK)

    def test_cancelacion_limite_y_argumentos(self):
        self.assertEqual(sugerir('F ', 2, cancelar=lambda: True).estado, E.CANCELADO)
        self.assertEqual(sugerir(' ' * 200001, 0).estado, E.LIMITE)
        for posicion in (-1, 100, True):
            with self.assertRaises(ValueError): sugerir('F ', posicion)
        with self.assertRaises(TypeError): sugerir(None, 0)

    def test_transporte_viejo_o_ausente_no_parece_sin_opciones(self):
        with patch.object(ccom, 'ejecutable', return_value=None):
            a=sugerir('', 0)
        self.assertEqual(a.estado, E.ERROR)
        with tempfile.TemporaryDirectory() as tmp:
            falso=Path(tmp)/'viejo'; falso.write_text(f'#!{sys.executable}\nprint("sin JSON")\n'); falso.chmod(0o755)
            a=sugerir('', 0, root=tmp, configurado=str(falso))
            self.assertEqual(a.estado, E.ERROR)
            self.assertIn('JSON', a.explicacion)

    def test_timeout_cancela_el_hijo_y_devuelve_limite(self):
        with tempfile.TemporaryDirectory() as tmp:
            falso=Path(tmp)/'lento'; falso.write_text(f'#!{sys.executable}\nimport time\ntime.sleep(20)\n'); falso.chmod(0o755)
            with patch('ide.servicio_ccom.time.monotonic', side_effect=[0, 4]):
                a=sugerir('', 0, root=tmp, configurado=str(falso))
            self.assertEqual(a.estado, E.LIMITE)

    def test_json_invalido_version_fuente_cursor_y_estado(self):
        for cambio in ({'schema_version': True}, {'schema_version': 99}, {'source': '/otra.cls'},
                       {'operation': 'check'}, {'offset_unit': 'unicode'}, {'cursor': True}, {'cursor': 13},
                       {'status': 'inventado'}, {'status': 'sin_opciones'}, {'exit_code': 1},
                       {'explanation': None}, {'suggestions': []}):
            d=informe(); d.update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo):
                interpretar(json.dumps(d), FUENTE, 'F main [] : vo', 14, 0)

    def test_rangos_partidos_duplicados_y_textos_invalidos(self):
        for cambio in ({'start': -1}, {'end': 100}, {'start': True}, {'start': 15}, {'text': ''},
                       {'text': 'x'*65}, {'category': []}, {'label': '\0'}, {'detail': ''}):
            d=informe(); d['suggestions'][0].update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo):
                interpretar(json.dumps(d), FUENTE, 'F main [] : vo', 14, 0)
        d=informe(); d['suggestions'] *= 2
        with self.assertRaises(ccom.ErrorProtocolo): interpretar(json.dumps(d), FUENTE, 'F main [] : vo', 14, 0)
        texto='# 😀\nF main [] : vo'; d=informe(); d['cursor']=len(texto.encode()); d['suggestions'][0].update(start=3,end=len(texto.encode()))
        with self.assertRaises(ccom.ErrorProtocolo): interpretar(json.dumps(d), FUENTE, texto, len(texto), 0)


class InterfazReal(unittest.TestCase):
    setUp=fixtures.Carpetas.setUp
    esperar=fixtures.Carpetas.esperar
    terminar=fixtures.Carpetas.terminar
    limpiar=fixtures.Carpetas.limpiar

    def preparar(self, texto='F main [] : vo'):
        self.window.show(); self.window.activateWindow()
        self.editor=self.window.editor(); self.editor.setPlainText(texto)
        cursor=self.editor.textCursor(); cursor.movePosition(QTextCursor.MoveOperation.End); self.editor.setTextCursor(cursor)
        self.editor.setFocus(); APP.processEvents()
        self.control=self.window.asistencia

    def pedir(self):
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda:self.control.popup.isVisible())

    def test_ctrl_espacio_real_tab_undo_y_sin_guardar(self):
        self.preparar(); self.pedir()
        self.assertEqual(self.control.popup.lista.item(0).data(Qt.ItemDataRole.UserRole).texto, 'void')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F main [] : void')
        self.assertIsNone(self.editor.path)
        self.editor.undo()
        self.assertEqual(self.editor.toPlainText(), 'F main [] : vo')

    def test_pausa_real_consulta_sin_aplicar(self):
        self.preparar()
        self.control.configurar({**self.control.configuracion, 'sugerencias_pausa': True, 'pausa_ms': 200})
        antes=self.editor.contexto_asistencia()
        self.control.programar(self.editor, 'escritura')
        self.esperar(lambda:self.control.popup.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(), antes)
        self.assertEqual(self.control.popup.lista.count(), 1)

    def test_motor_configurado_y_escape(self):
        self.preparar()
        marcador=self.folder/'usado'
        falso=self.folder/'ccom delegado'
        real=str(ROOT/'ccom')
        falso.write_text(f'#!{sys.executable}\nimport os,sys\nfrom pathlib import Path\n'
                         f'if "--suggest-json" in sys.argv: Path({str(marcador)!r}).write_text("si")\n'
                         f'os.execv({real!r}, [{real!r}, *sys.argv[1:]])\n')
        falso.chmod(0o755);self.window.configured_ccom=str(falso)
        antes=self.editor.contexto_asistencia();self.pedir()
        self.assertTrue(marcador.exists())
        QTest.keyClick(self.editor, Qt.Key.Key_Escape)
        self.assertFalse(self.control.popup.isVisible())
        self.assertEqual(antes,self.editor.contexto_asistencia())

    def test_asm_no_se_consulta_como_calser(self):
        self.preparar()
        p=self.folder/'entrada.asm';p.write_text('nop')
        self.editor=self.window.abrir(p)
        with patch.object(self.control.coordinador, 'solicitar') as llamar:
            self.window.solicitar_sugerencias()
            llamar.assert_not_called()
        self.assertIn('.asm', self.window.statusBar().currentMessage())

    def test_editar_cancela_hijo_nativo_y_descarta_respuesta(self):
        self.preparar()
        registro=self.folder/'proceso nativo'
        falso=self.folder/'ccom lento'
        real=str(ROOT/'ccom')
        falso.write_text(f'#!{sys.executable}\nimport os,sys,time\nfrom pathlib import Path\n'
                         f'if "--suggest-json" in sys.argv:\n    Path({str(registro)!r}).write_text(str(os.getpid())+"\\n"+sys.argv[-1])\n    time.sleep(20)\n'
                         f'else: os.execv({real!r}, [{real!r}, *sys.argv[1:]])\n')
        falso.chmod(0o755);self.window.configured_ccom=str(falso)
        self.window.solicitar_sugerencias()
        self.esperar(registro.exists)
        pid,snapshot=registro.read_text().splitlines()
        self.editor.insertPlainText('i')
        self.esperar(lambda:self.control.coordinador.task is None)
        self.assertFalse(self.control.popup.isVisible())
        self.assertFalse(Path(snapshot).exists())
        with self.assertRaises(ProcessLookupError): os.kill(int(pid),0)
