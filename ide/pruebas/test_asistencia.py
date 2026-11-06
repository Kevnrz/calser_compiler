"""Coordinación serial y UI de sugerencias con proveedores controlados de prueba."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
import threading
import time
import unittest
from unittest.mock import patch
from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide.nucleo.asistencia import (ContextoDocumento, EdicionTexto, EstadoAsistencia, Reparacion,
                                  ResultadoReparaciones, ResultadoSugerencias, Sugerencia)
from ide.interfaz.asistencia import CoordinadorAsistencia
from ide.pruebas import test_carpetas as fixtures

APP = fixtures.APP


def opciones_controladas(texto, cursor, *, cancelar=None):
    inicio = texto.rfind('in')
    return ResultadoSugerencias(EstadoAsistencia.OK, (
        Sugerencia('int', 'int', inicio, len(texto), 'tipo', 'Entero · opción controlada de prueba'),
        Sugerencia('float', 'float', inicio, len(texto), 'tipo', 'Real · opción controlada de prueba')))


def contexto(documento='doc', revision=0):
    return ContextoDocumento(documento, revision, 'in', 2, 2)


class Coordinacion(unittest.TestCase):
    esperar = fixtures.Carpetas.esperar

    def setUp(self):
        self.coordinador = CoordinadorAsistencia(plazo_ms=3000)
        self.respuestas = []
        self.coordinador.resultado.connect(self.respuestas.append)
        self.addCleanup(self.limpiar)

    def limpiar(self):
        self.assertTrue(self.coordinador.detener())
        APP.processEvents()
        self.coordinador.deleteLater(); APP.processEvents()

    def terminar(self):
        self.esperar(lambda: self.coordinador.task is None and not self.coordinador.pendientes)

    def test_motor_ausente_se_distingue_de_error(self):
        with patch('ide.interfaz.asistencia.cargar_motor', return_value=None):
            self.coordinador.solicitar('sugerencias', contexto()); self.terminar()
        self.assertEqual(self.respuestas[0].resultado.estado, EstadoAsistencia.BLOQUEADO)
        self.assertIn('todavía', self.respuestas[0].resultado.explicacion)

    def test_motor_se_ejecuta_fuera_del_hilo_grafico(self):
        hilos = []
        def motor(*args, **kwargs):
            hilos.append(threading.get_ident())
            return opciones_controladas(*args, **kwargs)
        self.coordinador.establecer_motor('sugerencias', motor)
        s = self.coordinador.solicitar('sugerencias', contexto()); self.terminar()
        self.assertNotEqual(hilos, [threading.get_ident()])
        self.assertEqual(self.respuestas[0].solicitud, s)
        self.assertEqual(self.respuestas[0].resultado.estado, EstadoAsistencia.OK)

    def test_ultima_pendiente_reemplaza_anterior_y_respuesta_vieja_se_descarta(self):
        iniciado, soltar = threading.Event(), threading.Event()
        llamadas = []
        def motor(texto, cursor, *, cancelar=None):
            llamadas.append(cursor)
            if len(llamadas) == 1:
                iniciado.set(); soltar.wait(2)
            return opciones_controladas(texto, cursor, cancelar=cancelar)
        self.coordinador.establecer_motor('sugerencias', motor)
        try:
            self.coordinador.solicitar('sugerencias', contexto())
            self.assertTrue(iniciado.wait(2))
            self.coordinador.solicitar('sugerencias', contexto(revision=1))
            ultima = self.coordinador.solicitar('sugerencias', contexto(revision=2))
            soltar.set(); self.terminar()
        finally: soltar.set()
        self.assertEqual(len(llamadas), 2)
        self.assertEqual([r.solicitud for r in self.respuestas], [ultima])

    def test_documentos_y_tipos_se_serializan(self):
        activas, maximas = [0], [0]
        def motor(*args, **kwargs):
            activas[0] += 1; maximas[0] = max(maximas[0], activas[0]); time.sleep(.02)
            activas[0] -= 1
            return opciones_controladas(*args, **kwargs)
        self.coordinador.establecer_motor('sugerencias', motor)
        self.coordinador.establecer_motor('reparaciones', lambda texto, **kw: ResultadoReparaciones(EstadoAsistencia.SIN_OPCIONES))
        self.coordinador.solicitar('sugerencias', contexto('uno'))
        self.coordinador.solicitar('sugerencias', contexto('dos'))
        self.coordinador.solicitar('reparaciones', contexto('dos'))
        self.terminar()
        self.assertEqual(len(self.respuestas), 3); self.assertEqual(maximas[0], 1)

    def test_invalidar_cancela_cooperativamente(self):
        iniciado, cancelado = threading.Event(), threading.Event()
        def motor(texto, cursor, *, cancelar=None):
            iniciado.set()
            while not cancelar(): time.sleep(.002)
            cancelado.set()
            return ResultadoSugerencias(EstadoAsistencia.CANCELADO)
        self.coordinador.establecer_motor('sugerencias', motor)
        self.coordinador.solicitar('sugerencias', contexto())
        self.assertTrue(iniciado.wait(2))
        self.coordinador.invalidar('doc'); self.terminar()
        self.assertTrue(cancelado.is_set()); self.assertFalse(self.respuestas)

    def test_excepcion_y_contrato_invalido_no_parecen_sin_opciones(self):
        def fallo(*args, **kw): raise RuntimeError('motor roto')
        for motor in (fallo, lambda *a, **k: {},
                      lambda *a, **k: ResultadoSugerencias(EstadoAsistencia.OK, (Sugerencia('int','int',5,5,'tipo'),))):
            self.coordinador.establecer_motor('sugerencias', motor)
            self.coordinador.solicitar('sugerencias', contexto()); self.terminar()
            self.assertEqual(self.respuestas[-1].resultado.estado, EstadoAsistencia.ERROR)
            self.assertTrue(self.respuestas[-1].resultado.explicacion)

    def test_dependencia_faltante_del_motor_no_se_oculta_como_ausencia(self):
        error = ModuleNotFoundError('dependencia faltante', name='dependencia_del_motor')
        with patch('ide.interfaz.asistencia.importlib.import_module', side_effect=error):
            self.coordinador.solicitar('sugerencias', contexto()); self.terminar()
        self.assertEqual(self.respuestas[0].resultado.estado, EstadoAsistencia.ERROR)

    def test_reparaciones_validadas_sin_aplicarlas(self):
        r = Reparacion('ejemplo', 'Reemplazar fragmento', (EdicionTexto(0,2,'int'),))
        self.coordinador.establecer_motor('reparaciones', lambda *a, **k: ResultadoReparaciones(EstadoAsistencia.OK, (r,)))
        self.coordinador.solicitar('reparaciones', contexto()); self.terminar()
        self.assertEqual(self.respuestas[-1].resultado.propuestas, (r,))

    def test_limites_de_texto_y_opciones(self):
        self.coordinador.establecer_motor('sugerencias', lambda *a, **k: ResultadoSugerencias(EstadoAsistencia.OK,
            tuple(Sugerencia(str(i),'int',0,2,'tipo') for i in range(65))))
        self.coordinador.solicitar('sugerencias', contexto()); self.terminar()
        self.assertEqual(self.respuestas[-1].resultado.estado, EstadoAsistencia.LIMITE)
        grande = ContextoDocumento('doc', 1, 'x' * 2_000_001, 0, 0)
        self.coordinador.solicitar('sugerencias', grande)
        self.assertEqual(self.respuestas[-1].resultado.estado, EstadoAsistencia.LIMITE)

    def test_timeout_no_arranca_otro_hilo_hasta_terminar_el_anterior(self):
        iniciado, soltar = threading.Event(), threading.Event()
        llamadas = []
        def motor(*args, **kw):
            llamadas.append(True)
            if len(llamadas) == 1:
                iniciado.set(); soltar.wait(2)
            return opciones_controladas(*args, **kw)
        self.coordinador.plazo_ms = 25
        self.coordinador.establecer_motor('sugerencias', motor)
        try:
            self.coordinador.solicitar('sugerencias', contexto()); self.assertTrue(iniciado.wait(2))
            self.esperar(lambda: bool(self.respuestas))
            self.assertEqual(self.respuestas[0].resultado.estado, EstadoAsistencia.LIMITE)
            self.coordinador.solicitar('sugerencias', contexto('dos'))
            APP.processEvents(); self.assertEqual(len(llamadas), 1)
            soltar.set(); self.terminar()
            self.assertEqual(len(llamadas), 2)
        finally: soltar.set()

    def test_cambio_proveedor_descarta_respuesta_anterior(self):
        iniciado, soltar = threading.Event(), threading.Event()
        def viejo(*args, **kw):
            iniciado.set(); soltar.wait(2); return opciones_controladas(*args, **kw)
        try:
            self.coordinador.establecer_motor('sugerencias', viejo)
            self.coordinador.solicitar('sugerencias', contexto()); self.assertTrue(iniciado.wait(2))
            self.coordinador.establecer_motor('sugerencias', opciones_controladas)
            nueva = self.coordinador.solicitar('sugerencias', contexto())
            soltar.set(); self.terminar()
            self.assertEqual([r.solicitud for r in self.respuestas], [nueva])
        finally: soltar.set()


class InterfazSugerencias(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def preparar(self, texto='Vm total : in'):
        self.window.resize(1000,700); self.window.show(); self.window.activateWindow()
        self.editor = self.window.editor(); self.editor.setPlainText(texto)
        self.editor.document().setModified(False)
        cursor = self.editor.textCursor(); cursor.movePosition(QTextCursor.MoveOperation.End); self.editor.setTextCursor(cursor)
        self.editor.setFocus(); QTest.qWait(20)
        self.control = self.window.asistencia
        self.control.establecer_motor('sugerencias', opciones_controladas)

    def pedir(self):
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda: self.control.popup.isVisible())

    def test_ctrl_espacio_muestra_lista_detalle_y_conserva_foco(self):
        self.preparar(); contexto = self.editor.contexto_asistencia(); self.pedir()
        self.assertEqual(self.control.popup.lista.count(), 2)
        self.assertIn('Entero', self.control.popup.detalle.text())
        self.assertTrue(self.editor.hasFocus())
        self.assertEqual(self.editor.contexto_asistencia(), contexto)
        self.assertFalse(self.editor.document().isModified())

    def test_flechas_enter_y_tab_aceptan_sin_insertar_teclas(self):
        self.preparar(); self.pedir()
        QTest.keyClick(self.editor, Qt.Key.Key_Down)
        self.assertIn('Real', self.control.popup.detalle.text())
        QTest.keyClick(self.editor, Qt.Key.Key_Return)
        self.assertEqual(self.editor.toPlainText(), 'Vm total : float')
        self.assertFalse(self.control.popup.isVisible())
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), 'Vm total : in')
        self.pedir(); QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'Vm total : int')
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), 'Vm total : in')

    def test_escape_no_modifica_texto_seleccion_o_historial(self):
        self.preparar(); ctx=self.editor.contexto_asistencia(); self.pedir()
        QTest.keyClick(self.editor, Qt.Key.Key_Escape)
        self.assertFalse(self.control.popup.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(),ctx)
        self.assertFalse(self.editor.document().isModified())
        self.assertFalse(self.editor.document().isUndoAvailable())

    def test_escribir_cierra_lista_y_la_tecla_llega_al_editor(self):
        self.preparar(); self.pedir()
        QTest.keyClicks(self.editor, 't')
        self.assertEqual(self.editor.toPlainText(), 'Vm total : int')
        self.assertFalse(self.control.popup.isVisible())

    def test_click_en_opcion_acepta(self):
        self.preparar(); self.pedir()
        lista = self.control.popup.lista
        QTest.mouseClick(lista.viewport(), Qt.MouseButton.LeftButton, pos=lista.visualItemRect(lista.item(1)).center())
        self.assertEqual(self.editor.toPlainText(), 'Vm total : float')
        self.assertFalse(self.control.popup.isVisible())

    def test_cursor_mover_y_click_externo_descartan_lista(self):
        self.preparar(); self.pedir()
        QTest.keyClick(self.editor, Qt.Key.Key_Left)
        self.assertFalse(self.control.popup.isVisible())
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        self.pedir()
        QTest.mouseClick(self.window.busqueda_proyecto.campo, Qt.MouseButton.LeftButton)
        self.assertFalse(self.control.popup.isVisible())
        self.assertEqual(self.editor.toPlainText(), 'Vm total : in')

    def test_motor_ausente_mensaje_sin_sugerencias_inventadas(self):
        self.preparar(); self.control.establecer_motor('sugerencias', None)
        with patch('ide.interfaz.asistencia.cargar_motor', return_value=None):
            self.window.solicitar_sugerencias()
            self.esperar(lambda: self.control.coordinador.task is None)
        self.assertFalse(self.control.popup.isVisible())
        self.assertIn('todavía no', self.window.statusBar().currentMessage())
        self.assertFalse(self.editor.document().isModified())

    def test_cambiar_pestana_descarta_respuesta_lenta(self):
        self.preparar(); iniciado, soltar = threading.Event(), threading.Event()
        def lento(*args, **kw):
            iniciado.set(); soltar.wait(2); return opciones_controladas(*args, **kw)
        try:
            self.control.establecer_motor('sugerencias', lento)
            self.window.solicitar_sugerencias(); self.assertTrue(iniciado.wait(2))
            otro = self.window.nuevo(); soltar.set()
            self.esperar(lambda: self.control.coordinador.task is None)
            self.assertFalse(self.control.popup.isVisible()); self.assertEqual(otro.toPlainText(),'')
        finally: soltar.set()

    def test_editar_durante_tarea_descarta_respuesta_y_ui_sigue_respondiendo(self):
        self.preparar(); iniciado, soltar = threading.Event(), threading.Event()
        def lento(*args, **kw):
            iniciado.set(); soltar.wait(2); return opciones_controladas(*args, **kw)
        try:
            self.control.establecer_motor('sugerencias', lento)
            self.window.solicitar_sugerencias(); self.assertTrue(iniciado.wait(2))
            latidos=[];QTimer.singleShot(0,lambda:latidos.append(True));APP.processEvents();self.assertTrue(latidos)
            self.editor.insertPlainText('t'); soltar.set()
            self.esperar(lambda: self.control.coordinador.task is None)
            self.assertFalse(self.control.popup.isVisible());self.assertEqual(self.editor.toPlainText(),'Vm total : int')
        finally: soltar.set()

    def test_unicode_cursor_medio_de_palabra_y_una_operacion_undo(self):
        self.preparar('# 😀\nVm total')
        def motor(texto,cursor,*,cancelar=None):
            return ResultadoSugerencias(EstadoAsistencia.OK,(Sugerencia('contador','contador',texto.index('total'),len(texto),'identificador'),))
        self.control.establecer_motor('sugerencias',motor)
        cursor=self.editor.textCursor();cursor.setPosition(len('# 😀\nVm to'.encode('utf-16-le'))//2);self.editor.setTextCursor(cursor)
        self.pedir();QTest.keyClick(self.editor,Qt.Key.Key_Return)
        self.assertEqual(self.editor.toPlainText(),'# 😀\nVm contador')
        self.assertEqual(self.editor.contexto_asistencia().cursor,len('# 😀\nVm contador'))
        self.editor.undo();self.assertEqual(self.editor.toPlainText(),'# 😀\nVm total')

    def test_geometria_dentro_de_pantalla(self):
        self.preparar();self.pedir()
        self.assertTrue(self.editor.screen().availableGeometry().contains(self.control.popup.frameGeometry()))

    def test_ctrl_espacio_fuera_del_editor_no_roba_foco(self):
        self.preparar()
        campo = self.window.busqueda_proyecto.campo
        campo.setFocus()
        QTest.keyClick(campo, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        APP.processEvents()
        self.assertIs(self.window.focusWidget(), campo)
        self.assertIsNone(self.control.coordinador.task)
        self.assertFalse(self.control.popup.isVisible())

    def test_cerrar_documento_descarta_respuesta_lenta(self):
        self.preparar(); iniciado, soltar = threading.Event(), threading.Event()
        def lento(*args, **kw):
            iniciado.set(); soltar.wait(2); return opciones_controladas(*args, **kw)
        try:
            self.control.establecer_motor('sugerencias', lento)
            self.window.solicitar_sugerencias(); self.assertTrue(iniciado.wait(2))
            doc = self.editor.documento_id
            self.window.cerrar_tab(self.window.tabs.currentIndex())
            soltar.set(); self.esperar(lambda: self.control.coordinador.task is None)
            self.assertNotIn(doc, self.control.editores)
            self.assertFalse(self.control.popup.isVisible())
        finally: soltar.set()

    def test_cierre_espera_cancelacion_sin_destruir_hilo_activo(self):
        self.preparar(); iniciado, soltar = threading.Event(), threading.Event()
        def lento(*args, **kw):
            iniciado.set(); soltar.wait(4); return opciones_controladas(*args, **kw)
        try:
            self.control.establecer_motor('sugerencias', lento)
            self.window.solicitar_sugerencias(); self.assertTrue(iniciado.wait(2))
            self.assertFalse(self.window.close())
            self.assertIn('cancelación', self.window.statusBar().currentMessage())
            soltar.set(); self.esperar(lambda: self.control.coordinador.task is None)
            self.assertFalse(self.control.popup.isVisible())
            # El cierre aplazado no deja el coordinador desactivado.
            self.control.establecer_motor('sugerencias', opciones_controladas)
            self.pedir()
        finally: soltar.set()

    def test_reparaciones_se_entregan_sin_aplicarse_automaticamente(self):
        self.preparar(); recibidas=[]
        self.control.reparacionesRecibidas.connect(recibidas.append)
        self.control.establecer_motor('reparaciones',lambda texto,**kw:ResultadoReparaciones(EstadoAsistencia.OK,
            (Reparacion('prueba','Cambiar texto',(EdicionTexto(0,2,'Vi'),)),)))
        self.control.solicitar('reparaciones')
        self.esperar(lambda:bool(recibidas))
        self.assertEqual(self.editor.toPlainText(),'Vm total : in')
        self.assertFalse(self.editor.document().isModified())
