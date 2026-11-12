"""M4: reparaciones C reales, transporte y aplicación manual/automática optativa."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
from PySide6.QtCore import Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide import ccom
from ide.reparaciones_ccom import proponer_reparaciones, interpretar, comprobar_sintaxis
from ide.ediciones import aplicar_ediciones
from ide.nucleo.asistencia import EstadoAsistencia as E
from ide.pruebas import test_carpetas as fixtures

ROOT=fixtures.ROOT;APP=fixtures.APP
TEXTO='F main [] : void ;\n    Vm x : int = 3\n    E ;\n.\n'


def nativo(texto=TEXTO):
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp)/'fuente.cls';p.write_text(texto)
        import subprocess
        r=subprocess.run([str(ROOT/'ccom'),'--repair-json',str(p)],capture_output=True,timeout=5)
        return json.loads(r.stdout),p


class MotorC(unittest.TestCase):
    def test_cierres_y_precondiciones_reales(self):
        textos=(TEXTO,'F main [] : void ; E ;','F main [] : void ; Vm x : int = (2 + 3 ; E ; .','F main [] : void ; C [verdadero ; E ; .')
        codigos=('INSERCION_PUNTO_COMA','INSERCION_PUNTO_FINAL','INSERCION_PARENTESIS_CIERRE','INSERCION_CORCHETE_CIERRE')
        for texto,codigo in zip(textos,codigos):
            a=proponer_reparaciones(texto)
            self.assertEqual(a.estado,E.OK)
            r=next(r for r in a.propuestas if r.codigo==codigo)
            self.assertTrue(r.precondiciones)
            self.assertTrue(all(p.texto_esperado==texto[p.inicio:p.fin] and p.inicio<p.fin for p in r.precondiciones))
            corregido=aplicar_ediciones(texto,r.ediciones,r.precondiciones)
            self.assertNotEqual(texto,corregido)

    def test_no_consulta_parser_python_y_verifica_antes_despues_en_c(self):
        with patch('ide.nucleo.parser.analizar_sintaxis',side_effect=AssertionError('No usar LL1')):
            a=proponer_reparaciones(TEXTO)
            self.assertEqual(comprobar_sintaxis(TEXTO),'error_sintactico')
            self.assertEqual(comprobar_sintaxis(aplicar_ediciones(TEXTO,a.propuestas[0].ediciones)),'aceptado')

    def test_unicode_comentario_y_reemplazo_antes_del_salto(self):
        texto='#[ ñ 😀 ]#\r\n'+TEXTO.replace('3\n','3 # comentario\n')
        r=proponer_reparaciones(texto).propuestas[0]
        self.assertEqual(r.ediciones[0].inicio,texto.index('3 #')+1)
        nuevo=aplicar_ediciones(texto,r.ediciones,r.precondiciones)
        self.assertIn('3; # comentario',nuevo)
        self.assertIn('#[ ñ 😀 ]#\r\n',nuevo)

    def test_eof_parcial_lexico_y_operandos_no_inventados(self):
        a=proponer_reparaciones('F main [] : void ; Vm x : int = 3')
        self.assertEqual(a.estado,E.OK);self.assertFalse(a.propuestas[0].automatica_segura)
        self.assertIn('parcial',a.propuestas[0].validacion)
        self.assertEqual(proponer_reparaciones(TEXTO+'@').estado,E.BLOQUEADO)
        self.assertEqual(proponer_reparaciones(TEXTO.replace('3','3 + ;')).estado,E.SIN_OPCIONES)
        self.assertEqual(proponer_reparaciones('F main [] : void; .').estado,E.SIN_OPCIONES)

    def test_cancelacion_limite_timeout_y_ejecutable_incompatible(self):
        self.assertEqual(proponer_reparaciones(TEXTO,cancelar=lambda:True).estado,E.CANCELADO)
        self.assertEqual(proponer_reparaciones(' '*200001).estado,E.LIMITE)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'ccom falso';p.write_text(f'#!{sys.executable}\nprint("sin JSON")\n');p.chmod(0o755)
            self.assertEqual(proponer_reparaciones(TEXTO,configurado=str(p)).estado,E.ERROR)
            p.write_text(f'#!{sys.executable}\nimport time\ntime.sleep(20)\n')
            with patch('ide.servicio_ccom.time.monotonic',side_effect=[0,4]):
                self.assertEqual(proponer_reparaciones(TEXTO,configurado=str(p)).estado,E.LIMITE)

    def test_rechaza_metadatos_fuente_version_estado_y_codigo(self):
        original,fuente=nativo()
        for cambio in ({'source':'/otra.cls'},{'schema_version':True},{'operation':'suggest'},{'exit_code':1},
                       {'status':'sin_opciones'},{'repairs':[]},{'explanation':None}):
            d={**original,**cambio}
            with self.subTest(cambio=cambio),self.assertRaises(ccom.ErrorProtocolo):interpretar(json.dumps(d),fuente,TEXTO,0)

    def test_rechaza_precondiciones_narrativas_vacias_y_rangos_invalidos(self):
        original,fuente=nativo()
        for cambio in ({'preconditions':[]},{'preconditions':['verificar revisión']},
                       {'preconditions':[{'start':0,'end':0,'expected':''}]},
                       {'preconditions':[{'start':0,'end':1,'expected':'X'}]},
                       {'edits':[{'start':True,'end':True,'text':';'}]},
                       {'edits':[{'start':0,'end':1,'text':';'}]}, {'automatic_safe':'true'}):
            d=json.loads(json.dumps(original));d['repairs'][0].update(cambio)
            with self.subTest(cambio=cambio),self.assertRaises(ccom.ErrorProtocolo):interpretar(json.dumps(d),fuente,TEXTO,0)
        d=json.loads(json.dumps(original));d['repairs'][0]['validation']['resolved']=False
        with self.assertRaises(ccom.ErrorProtocolo):interpretar(json.dumps(d),fuente,TEXTO,0)


class InterfazReal(unittest.TestCase):
    setUp=fixtures.Carpetas.setUp
    limpiar=fixtures.Carpetas.limpiar
    esperar=fixtures.Carpetas.esperar
    terminar=fixtures.Carpetas.terminar

    def preparar(self):
        self.path=self.folder/'prueba.cls';self.path.write_text(TEXTO)
        self.editor=self.window.abrir(self.path)
        self.window.show();self.window.activateWindow();self.editor.setFocus();APP.processEvents()
        self.control=self.window.asistencia;self.panel=self.window.reparaciones_panel

    def pedir(self):
        QTest.keyClick(self.editor,Qt.Key.Key_Return,Qt.KeyboardModifier.AltModifier)
        self.esperar(lambda:not self.panel.ocupado)
        self.assertEqual(self.panel.lista.count(),1,self.panel.estado.text())

    def test_panel_real_vista_previa_aplicar_undo_sin_guardar(self):
        self.preparar();self.pedir()
        self.assertEqual(self.editor.toPlainText(),TEXTO)
        self.assertIn('3;',self.panel.previa.toPlainText())
        self.assertTrue(self.panel.aplicar_boton.isEnabled())
        self.panel.aplicar_boton.click()
        self.assertIn('3;',self.editor.toPlainText())
        self.assertEqual(self.path.read_text(),TEXTO)
        self.editor.undo();self.assertEqual(self.editor.toPlainText(),TEXTO)

    def test_catalogo_optativo_y_auto_nativa_sin_cadena(self):
        self.preparar()
        self.assertFalse(self.control.configuracion['reparaciones_automaticas'])
        self.control.configurar({**self.control.configuracion,'reparaciones_automaticas':True,
            'reglas_automaticas':['INSERCION_PUNTO_COMA'],'pausa_ms':300})
        pos=TEXTO.index('\n',TEXTO.index('Vm'))
        cursor=self.editor.textCursor();cursor.setPosition(pos);self.editor.setTextCursor(cursor)
        QTest.keyClick(self.editor,Qt.Key.Key_Return)
        antes=self.editor.toPlainText()
        self.esperar(lambda:'3;' in self.editor.toPlainText())
        self.assertEqual(self.path.read_text(),TEXTO)
        self.editor.undo();self.assertEqual(self.editor.toPlainText(),antes)
        QTest.qWait(400);self.assertEqual(self.editor.toPlainText(),antes)

    def test_automatico_requiere_regla_seleccionada(self):
        self.preparar()
        self.control.configurar({**self.control.configuracion,'reparaciones_automaticas':True,'pausa_ms':300})
        cursor=self.editor.textCursor();cursor.setPosition(TEXTO.index('\n',TEXTO.index('Vm')));self.editor.setTextCursor(cursor)
        QTest.keyClick(self.editor,Qt.Key.Key_Return)
        antes=self.editor.toPlainText();QTest.qWait(500)
        self.assertEqual(self.editor.toPlainText(),antes)

    def test_edicion_invalida_previa_real_y_asm_no_queda_ocupado(self):
        self.preparar();self.pedir()
        self.editor.insertPlainText('# cambio')
        self.assertFalse(self.panel.aplicar_boton.isEnabled())
        p=self.folder/'entrada.asm';p.write_text('nop')
        self.window.abrir(p);self.window.solicitar_reparaciones()
        self.assertFalse(self.panel.ocupado)
        self.assertIn('.asm',self.panel.estado.text())

    def test_cancelar_hijo_real_elimina_snapshot(self):
        self.preparar();registro=self.folder/'registro';falso=self.folder/'ccom delegado';real=str(ROOT/'ccom')
        falso.write_text(f'#!{sys.executable}\nimport os,sys,time\nfrom pathlib import Path\n'
            f'if "--repair-json" in sys.argv:\n    Path({str(registro)!r}).write_text(str(os.getpid())+"\\n"+sys.argv[-1])\n    time.sleep(20)\n'
            f'else: os.execv({real!r},[{real!r},*sys.argv[1:]])\n')
        falso.chmod(0o755);self.window.configured_ccom=str(falso)
        self.window.solicitar_reparaciones();self.esperar(registro.exists)
        pid,snapshot=registro.read_text().splitlines();self.window.cancelar_reparaciones()
        self.esperar(lambda:self.control.coordinador.task is None)
        self.assertFalse(Path(snapshot).exists())
        self.assertFalse(self.panel.aplicar_boton.isEnabled())
        with self.assertRaises(ProcessLookupError):os.kill(int(pid),0)
