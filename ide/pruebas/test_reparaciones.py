"""Panel manual de reparaciones: previa, contexto y aplicación explícita."""
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
import threading
import unittest
from unittest.mock import patch
from PySide6.QtCore import Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide.diagnosticos import Problema
from ide.nucleo.asistencia import (EdicionTexto, EstadoAsistencia, PrecondicionTexto,
                                  Reparacion, ResultadoReparaciones)
from ide.interfaz.reparaciones import MAX_CAMBIO_PREVIA, vista_previa
from ide.pruebas import test_carpetas as fixtures

APP=fixtures.APP
TEXTO='F main [] : void ;\n    Vm x : int = 3\n    E ;\n.\n'


def propuestas_controladas(texto,*,cancelar=None):
    fin=texto.index('\n',texto.index('Vm'))
    nombre=texto.index('x :')
    return ResultadoReparaciones(EstadoAsistencia.OK,(
        Reparacion('terminador-prueba','Insertar ; al terminar la declaración',
                   (EdicionTexto(fin,fin,' ;'),),precondiciones=(PrecondicionTexto(fin-1,fin,'3'),),
                   validacion='Proveedor controlado de prueba; el motor real deberá reanalizar el candidato.'),
        Reparacion('varios-prueba','Dos cambios de prueba',
                   (EdicionTexto(fin,fin,' ;'),EdicionTexto(nombre,nombre+1,'total')),
                   automatica_segura=True,razones=('Marca controlada para probar que no se aplica automáticamente.',),
                   validacion='Dato de prueba, sin autorización para automatización.')))


class Previa(unittest.TestCase):
    def test_presenta_todos_los_cambios_y_posiciones_unicode(self):
        texto='# 😀\nE\n.'
        r=Reparacion('prueba','Insertar ;',(EdicionTexto(5,5,' ;'),))
        previa=vista_previa(texto,r)
        self.assertIn('línea 2, columna 2',previa)
        self.assertIn('- E',previa);self.assertIn('+ E ;',previa)

    def test_limita_cambios_grandes_sin_diff_costoso(self):
        texto='x'*(MAX_CAMBIO_PREVIA+1)
        r=Reparacion('prueba','Eliminar',(EdicionTexto(0,len(texto),''),))
        with self.assertRaises(ValueError):vista_previa(texto,r)


class Panel(unittest.TestCase):
    setUp=fixtures.Carpetas.setUp
    limpiar=fixtures.Carpetas.limpiar
    esperar=fixtures.Carpetas.esperar
    terminar=fixtures.Carpetas.terminar

    def preparar(self,texto=TEXTO):
        self.path=self.folder/'programa.cls';self.path.write_text(texto)
        self.editor=self.window.abrir(self.path)
        self.window.resize(1000,700);self.window.show();self.window.activateWindow()
        self.editor.setFocus();QTest.qWait(20)
        self.panel=self.window.reparaciones_panel
        self.control=self.window.asistencia
        self.control.establecer_motor('reparaciones',propuestas_controladas)

    def pedir(self):
        QTest.keyClick(self.editor,Qt.Key.Key_Return,Qt.KeyboardModifier.AltModifier)
        self.esperar(lambda:not self.panel.ocupado and self.panel.lista.count()==2)

    def test_alt_enter_abre_panel_y_previa_sin_modificar(self):
        self.preparar();antes=self.editor.contexto_asistencia();self.pedir()
        self.assertIs(self.window.panels.currentWidget(),self.panel)
        self.assertTrue(self.window.results_action.isChecked())
        self.assertIn('Vm x : int = 3 ;',self.panel.previa.toPlainText())
        self.assertIn('Proveedor controlado',self.panel.explicacion.toPlainText())
        self.assertTrue(self.panel.aplicar_boton.isEnabled())
        self.assertEqual(self.editor.contexto_asistencia(),antes)
        self.assertFalse(self.editor.document().isModified())
        self.assertEqual(self.path.read_text(),TEXTO)

    def test_aplicar_varias_ediciones_un_undo_y_diagnosticos_se_actualizan(self):
        self.preparar()
        self.window.results[self.editor]=[Problema(self.path,2,20,'error: falta ;')]
        self.window.actualizar_problemas();self.pedir()
        self.panel.lista.setCurrentRow(1)
        self.assertIn(self.editor,self.window.results)
        self.assertEqual(self.editor.toPlainText(),TEXTO)
        QTest.mouseClick(self.panel.aplicar_boton,Qt.MouseButton.LeftButton)
        esperado=TEXTO.replace('x :','total :').replace('3\n','3 ;\n')
        self.assertEqual(self.editor.toPlainText(),esperado)
        self.assertTrue(self.editor.document().isModified())
        self.assertNotIn(self.editor,self.window.results)
        self.assertIn('aplicada',self.panel.estado.text())
        self.assertFalse(self.panel.aplicar_boton.isEnabled())
        self.assertEqual(self.path.read_text(),TEXTO)
        QTest.keyClick(self.editor,Qt.Key.Key_Z,Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(self.editor.toPlainText(),TEXTO)
        self.editor.redo();self.assertEqual(self.editor.toPlainText(),esperado)

    def test_cancelar_boton_o_escape_conserva_historial(self):
        self.preparar();self.pedir()
        QTest.mouseClick(self.panel.cancelar_boton,Qt.MouseButton.LeftButton)
        self.assertIsNone(self.panel.contexto);self.assertEqual(self.panel.lista.count(),0)
        self.assertFalse(self.editor.document().isUndoAvailable())
        self.editor.setFocus();self.pedir()
        self.panel.previa.setFocus();QTest.keyClick(self.panel.previa,Qt.Key.Key_Escape)
        self.assertIsNone(self.panel.contexto)
        self.assertEqual(self.editor.toPlainText(),TEXTO)

    def test_foco_panel_y_seleccionar_opcion_no_invalidan_documento(self):
        self.preparar();self.pedir()
        self.panel.lista.setFocus()
        QTest.keyClick(self.panel.lista,Qt.Key.Key_Down)
        self.assertTrue(self.panel.vigente)
        self.assertTrue(self.panel.aplicar_boton.isEnabled())
        self.panel.previa.setFocus()
        self.assertTrue(self.panel.vigente)
        self.assertFalse(self.editor.document().isModified())

    def test_editar_o_mover_cursor_desactiva_aplicar(self):
        self.preparar();self.pedir()
        self.editor.setFocus();self.editor.insertPlainText('# editado\n')
        self.assertFalse(self.panel.vigente);self.assertFalse(self.panel.aplicar_boton.isEnabled())
        self.assertIn('desactualizada',self.panel.estado.text())
        previo=self.editor.toPlainText();self.panel.elegir()
        self.assertEqual(self.editor.toPlainText(),previo)
        self.editor.setPlainText(TEXTO);self.editor.document().setModified(False)
        self.pedir()
        c=self.editor.textCursor();c.setPosition(1);self.editor.setTextCursor(c)
        self.assertFalse(self.panel.vigente)
        self.assertFalse(self.panel.aplicar_boton.isEnabled())

    def test_cambiar_o_cerrar_pestana_no_aplica_en_otro_documento(self):
        self.preparar();self.pedir()
        ctx=self.panel.contexto;r=self.panel.lista.currentItem().data(Qt.ItemDataRole.UserRole)
        otro=self.window.nuevo()
        self.assertFalse(self.panel.vigente)
        self.window.aplicar_reparacion_propuesta(ctx,r)
        self.assertEqual(otro.toPlainText(),'')
        self.assertEqual(self.editor.toPlainText(),TEXTO)
        self.window.tabs.setCurrentWidget(self.editor);self.pedir()
        self.window.cerrar_tab(self.window.tabs.currentIndex())
        self.assertFalse(self.panel.vigente)

    def test_motor_ausente_no_inventa_reparaciones(self):
        self.preparar();self.control.establecer_motor('reparaciones',None)
        with patch('ide.interfaz.asistencia.cargar_motor',return_value=None):
            self.window.solicitar_reparaciones()
            self.esperar(lambda:self.control.coordinador.task is None)
        self.assertIn('todavía no',self.panel.estado.text())
        self.assertEqual(self.panel.lista.count(),0)
        self.assertFalse(self.panel.aplicar_boton.isEnabled())
        self.assertEqual(self.path.read_text(),TEXTO)

    def test_sin_opciones_bloqueo_y_error_se_distinguen(self):
        self.preparar()
        for estado,mensaje in ((EstadoAsistencia.SIN_OPCIONES,''),(EstadoAsistencia.BLOQUEADO,'Comentario abierto'),
                               (EstadoAsistencia.ERROR,'Fallo del motor')):
            self.control.establecer_motor('reparaciones',lambda texto,*,cancelar=None,e=estado,m=mensaje:ResultadoReparaciones(e,explicacion=m))
            self.window.solicitar_reparaciones()
            self.esperar(lambda:self.control.coordinador.task is None)
            self.assertIn(mensaje or 'No hay reparaciones',self.panel.estado.text())
            self.assertFalse(self.panel.aplicar_boton.isEnabled())

    def test_cancelar_tarea_pendiente_descarta_resultado(self):
        self.preparar();iniciado,soltar=threading.Event(),threading.Event()
        def lento(texto,*,cancelar=None):
            iniciado.set();soltar.wait(2);return propuestas_controladas(texto,cancelar=cancelar)
        try:
            self.control.establecer_motor('reparaciones',lento)
            self.window.solicitar_reparaciones();self.assertTrue(iniciado.wait(2))
            self.window.cancelar_reparaciones();soltar.set()
            self.esperar(lambda:self.control.coordinador.task is None)
            self.assertEqual(self.panel.lista.count(),0)
            self.assertIn('cancelada',self.panel.estado.text())
        finally:soltar.set()

    def test_foco_en_panel_durante_tarea_no_la_cancela(self):
        self.preparar();iniciado,soltar=threading.Event(),threading.Event()
        def lento(texto,*,cancelar=None):
            iniciado.set();soltar.wait(2);return propuestas_controladas(texto,cancelar=cancelar)
        try:
            self.control.establecer_motor('reparaciones',lento)
            solicitud=self.window.solicitar_reparaciones();self.assertTrue(iniciado.wait(2))
            self.panel.lista.setFocus();APP.processEvents()
            self.assertTrue(self.control.coordinador.vigente(solicitud))
            soltar.set();self.esperar(lambda:self.panel.lista.count()==2)
            self.assertTrue(self.panel.aplicar_boton.isEnabled())
        finally:soltar.set()

    def test_previa_grande_deshabilita_aplicar_sin_ocultar_cambios(self):
        self.preparar('x'*(MAX_CAMBIO_PREVIA+1))
        self.control.establecer_motor('reparaciones',lambda texto,**kw:ResultadoReparaciones(EstadoAsistencia.OK,
            (Reparacion('prueba','Eliminar',(EdicionTexto(0,len(texto),''),)),)))
        self.window.solicitar_reparaciones();self.esperar(lambda:self.panel.lista.count()==1)
        self.assertIn('12000',self.panel.previa.toPlainText())
        self.assertFalse(self.panel.aplicar_boton.isEnabled())
        self.assertEqual(len(self.editor.toPlainText()),MAX_CAMBIO_PREVIA+1)

    def test_panel_se_adapta_al_lado_derecho(self):
        self.preparar();self.pedir()
        self.window.mover_panel();APP.processEvents()
        self.assertEqual(self.panel.splitter.orientation(),Qt.Orientation.Vertical)
        self.assertTrue(self.panel.aplicar_boton.isEnabled())
        self.assertIn('Vm x',self.panel.previa.toPlainText())

    def test_alt_enter_fuera_del_editor_no_roba_foco(self):
        self.preparar();campo=self.window.busqueda_proyecto.campo;campo.setFocus()
        QTest.keyClick(campo,Qt.Key.Key_Return,Qt.KeyboardModifier.AltModifier);APP.processEvents()
        self.assertIs(self.window.focusWidget(),campo)
        self.assertIsNone(self.control.coordinador.task)
        self.assertIsNone(self.panel.contexto)
