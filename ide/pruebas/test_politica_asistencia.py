"""Paso 6: permisos, pausa y automatización conservadora con proveedores de prueba."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from dataclasses import replace
import threading
import unittest
from unittest.mock import patch
from PySide6.QtCore import QSettings, Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide import preferencias
from ide.ediciones import aplicar_ediciones
from ide.nucleo.asistencia import (ContextoDocumento, EdicionTexto, EstadoAsistencia, PrecondicionTexto,
                                  Reparacion, ResultadoReparaciones, ResultadoSugerencias, Sugerencia)
from ide.politica_asistencia import (MAX_TEXTO_AUTOMATICO, REGLAS_AUTOMATICAS, ReglaAutomatica,
                                    evaluar_automatizacion, validar_preferencias)
from ide.interfaz.preferencias_asistencia import PreferenciasAsistencia
from ide.pruebas import test_carpetas as fixtures

APP = fixtures.APP
TEXTO = 'F main [] : void ;\n    Vm x : int = 3\n    E ;\n.\n'
REGLA = ReglaAutomatica('terminador-prueba', 'Terminador controlado de prueba', ';')


def reparaciones(texto, *, cancelar=None):
    fin = texto.index('\n', texto.index('Vm'))
    if texto[fin - 1] != '3':
        return ResultadoReparaciones(EstadoAsistencia.SIN_OPCIONES)
    r = Reparacion(REGLA.codigo, 'Insertar ; de prueba', (EdicionTexto(fin, fin, ' ;'),),
        automatica_segura=True, razones=('Justificación controlada de prueba.',),
        precondiciones=(PrecondicionTexto(fin - 1, fin, '3'),), validacion='El IDE comprobará el parser real.')
    return ResultadoReparaciones(EstadoAsistencia.OK, (r,))


def contexto(texto=TEXTO):
    salto = texto.index('\n', texto.index('Vm'))
    return ContextoDocumento('prueba', 1, texto, salto + 1, salto + 1)


class Politica(unittest.TestCase):
    def test_preferencias_validan_tipos_limites_y_por_defecto_solo_manual(self):
        d = validar_preferencias(None)
        self.assertTrue(d['habilitada'])
        self.assertFalse(d['sugerencias_pausa']); self.assertFalse(d['reparaciones_automaticas'])
        for datos in ({'pausa_ms': True}, {'pausa_ms': 299}, {'pausa_ms': 3001}, {'habilitada': 'false'},
                      {'reglas_automaticas': ['x'] * 65}):
            with self.subTest(datos=datos): self.assertEqual(validar_preferencias(datos), d)
        self.assertEqual(validar_preferencias({'reglas_automaticas': ['x', '', 'x', 1]})['reglas_automaticas'], ['x'])
        d['reglas_automaticas'].append('x')
        self.assertEqual(validar_preferencias({})['reglas_automaticas'], [])

    def test_catalogo_real_revisado_y_descriptores_validados(self):
        self.assertEqual([r.codigo for r in REGLAS_AUTOMATICAS], ['INSERCION_PUNTO_COMA'])
        for insercion in ('texto', '\n', '', ';;'):
            with self.assertRaises(ValueError): ReglaAutomatica('prueba', 'Prueba', insercion)

    def test_una_insercion_aprobada_requiere_aceptacion_del_parser_real(self):
        c = contexto(); resultado = reparaciones(c.texto)
        r, motivo = evaluar_automatizacion(c, resultado, (REGLA,))
        self.assertIs(r, resultado.propuestas[0]); self.assertIn('parser', motivo)
        self.assertIn('3 ;', aplicar_ediciones(c.texto, r.ediciones, r.precondiciones))

    def test_bandera_no_autoriza_reglas_desconocidas_desactivadas_o_ambiguas(self):
        c = contexto(); resultado = reparaciones(c.texto)
        self.assertIsNone(evaluar_automatizacion(c, resultado, ())[0])
        self.assertIsNone(evaluar_automatizacion(c, replace(resultado, propuestas=resultado.propuestas * 2), (REGLA,))[0])
        self.assertIsNone(evaluar_automatizacion(c, replace(resultado, propuestas=(replace(resultado.propuestas[0], automatica_segura=False),)), (REGLA,))[0])

    def test_rechaza_borrado_multiples_ediciones_salto_de_cursor_y_precondicion_ausente(self):
        c = contexto(); resultado = reparaciones(c.texto); r = resultado.propuestas[0]
        for cambios in ({'precondiciones': ()}, {'cursor_final': 0},
                        {'ediciones': (EdicionTexto(0, 1, ''),)},
                        {'ediciones': (EdicionTexto(0, 0, '#'),) + r.ediciones},
                        {'ediciones': (EdicionTexto(r.ediciones[0].inicio, r.ediciones[0].fin, 'otro ;'),)}):
            rr = replace(resultado, propuestas=(replace(r, **cambios),))
            with self.subTest(cambios=cambios): self.assertIsNone(evaluar_automatizacion(c, rr, (REGLA,))[0])

    def test_rechaza_cursor_fuera_de_linea_confirmada_seleccion_y_documento_grande(self):
        c = contexto(); resultado = reparaciones(c.texto)
        for cc in (replace(c, cursor=0, ancla=0), replace(c, ancla=0),
                   replace(c, texto=c.texto + ' ' * MAX_TEXTO_AUTOMATICO)):
            self.assertIsNone(evaluar_automatizacion(cc, resultado, (REGLA,))[0])

    def test_programa_incompleto_o_con_error_lexico_no_se_corrige_automaticamente(self):
        for texto in (TEXTO.replace('E ;', 'E'), TEXTO + '@', TEXTO.replace('Vm x', '# Vm x')):
            c = contexto(texto)
            self.assertIsNone(evaluar_automatizacion(c, reparaciones(texto), (REGLA,))[0])

    def test_cancelacion_se_consulta(self):
        c = contexto()
        self.assertIsNone(evaluar_automatizacion(c, reparaciones(c.texto), (REGLA,), cancelar=lambda: True)[0])


class Interaccion(unittest.TestCase):
    setUp = fixtures.Carpetas.setUp
    limpiar = fixtures.Carpetas.limpiar
    esperar = fixtures.Carpetas.esperar
    terminar = fixtures.Carpetas.terminar

    def preparar(self, texto=TEXTO):
        self.path = self.folder / 'programa.cls'; self.path.write_text(texto)
        self.editor = self.window.abrir(self.path)
        self.window.resize(1000, 700); self.window.show(); self.window.activateWindow()
        self.editor.setFocus(); QTest.qWait(20)
        self.control = self.window.asistencia
        self.llamadas = []
        self.control.establecer_motor('reparaciones', self.reparar)
        self.control.establecer_motor('sugerencias', self.sugerir)

    def sugerir(self, texto, cursor, *, cancelar=None):
        self.llamadas.append(('sugerencias', texto, cursor))
        return ResultadoSugerencias(EstadoAsistencia.OK,
            (Sugerencia('int', 'int', cursor, cursor, 'prueba'),))

    def reparar(self, texto, *, cancelar=None):
        self.llamadas.append(('reparaciones', texto, threading.get_ident()))
        return reparaciones(texto, cancelar=cancelar)

    def configurar(self, **datos):
        d = {'pausa_ms': 300}; d.update(datos)
        self.control.configurar(d)

    def confirmar_linea(self):
        fin = self.editor.toPlainText().index('\n', self.editor.toPlainText().index('Vm'))
        c = self.editor.textCursor(); c.setPosition(fin); self.editor.setTextCursor(c)
        self.editor.setFocus(); QTest.keyClick(self.editor, Qt.Key.Key_Return)

    def habilitar_auto(self):
        self.control.establecer_reglas((REGLA,))
        self.configurar(reparaciones_automaticas=True, reglas_automaticas=[REGLA.codigo])

    def test_pausa_se_reinicia_y_consulta_solo_el_ultimo_texto_sin_aceptar(self):
        self.preparar('Vm'); self.configurar(sugerencias_pausa=True)
        c = self.editor.textCursor(); c.movePosition(QTextCursor.MoveOperation.End); self.editor.setTextCursor(c)
        QTest.keyClicks(self.editor, ' x'); QTest.qWait(150)
        self.assertEqual(self.llamadas, [])
        QTest.keyClicks(self.editor, ' : in')
        self.esperar(lambda: self.control.popup.isVisible())
        self.assertEqual(len(self.llamadas), 1)
        self.assertEqual(self.llamadas[0][1], 'Vm x : in')
        self.assertEqual(self.editor.toPlainText(), 'Vm x : in')

    def test_movimiento_foco_escape_y_cambio_pestana_cancelan_pausa(self):
        self.preparar('Vm'); self.configurar(sugerencias_pausa=True)
        for accion in ('cursor', 'foco', 'escape', 'pestana'):
            self.window.tabs.setCurrentWidget(self.editor); self.editor.setFocus()
            QTest.keyClicks(self.editor, 'x')
            self.assertTrue(self.control.timer_pausa.isActive())
            if accion == 'cursor': QTest.keyClick(self.editor, Qt.Key.Key_Left)
            elif accion == 'foco': self.window.busqueda_proyecto.campo.setFocus()
            elif accion == 'escape': QTest.keyClick(self.editor, Qt.Key.Key_Escape)
            else: self.window.nuevo()
            QTest.qWait(350)
            self.assertEqual(self.llamadas, [])

    def test_carga_pegar_undo_y_ediciones_programaticas_no_disparan(self):
        self.preparar(''); self.configurar(sugerencias_pausa=True)
        self.editor.setPlainText('Vm'); self.editor.insertPlainText(' x')
        APP.clipboard().setText(' : int'); self.editor.paste(); self.editor.undo(); self.editor.redo()
        QTest.qWait(350)
        self.assertEqual(self.llamadas, [])
        self.assertFalse(self.control.timer_pausa.isActive())

    def test_interrupcion_general_bloquea_manual_y_automatico_sin_tocar_ccom(self):
        self.preparar(); self.configurar(habilitada=False, sugerencias_pausa=True)
        QTest.keyClicks(self.editor, 'x'); self.window.solicitar_sugerencias(); self.window.solicitar_reparaciones()
        QTest.qWait(350)
        self.assertEqual(self.llamadas, []); self.assertFalse(self.window.sugerencias_action.isEnabled())
        self.assertIn('desactivada', self.window.reparaciones_panel.estado.text())
        self.assertFalse(self.window.auto_action.isChecked())

    def test_sin_catalogo_no_solicita_reparaciones_aunque_configuracion_insista(self):
        self.preparar(); self.configurar(reparaciones_automaticas=True, reglas_automaticas=[REGLA.codigo])
        self.confirmar_linea(); QTest.qWait(350)
        self.assertEqual(self.llamadas, [])
        dialogo = PreferenciasAsistencia(self.control.configuracion, (), self.window)
        self.assertFalse(dialogo.automaticas.isEnabled())
        self.assertFalse(dialogo.datos()['reparaciones_automaticas']); dialogo.deleteLater()

    def test_auto_aplica_en_hilo_validado_y_un_undo_sin_guardar_ni_bucle(self):
        self.preparar(); self.habilitar_auto()
        self.confirmar_linea(); antes = self.editor.toPlainText()
        self.esperar(lambda: '3 ;' in self.editor.toPlainText())
        self.assertEqual(self.path.read_text(), TEXTO)
        self.assertNotEqual(self.llamadas[0][2], threading.get_ident())
        self.assertIn('automática aplicada', self.window.reparaciones_panel.estado.text())
        self.assertIn('3 ;', self.window.reparaciones_panel.previa.toPlainText())
        self.assertFalse(self.window.reparaciones_panel.aplicar_boton.isEnabled())
        QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(self.editor.toPlainText(), antes)
        QTest.qWait(350); self.assertEqual(len(self.llamadas), 1)
        self.editor.redo(); self.assertIn('3 ;', self.editor.toPlainText())
        QTest.qWait(350); self.assertEqual(len(self.llamadas), 1)

    def test_regla_debe_estar_seleccionada_ademas_de_permitida(self):
        self.preparar(); self.control.establecer_reglas((REGLA,))
        self.configurar(reparaciones_automaticas=True)
        self.confirmar_linea(); QTest.qWait(350)
        self.assertEqual(self.llamadas, [])

    def test_manual_no_aplica_por_bandera_auto(self):
        self.preparar(); self.habilitar_auto()
        self.window.solicitar_reparaciones()
        self.esperar(lambda: self.window.reparaciones_panel.lista.count() == 1)
        self.assertEqual(self.editor.toPlainText(), TEXTO)
        self.assertTrue(self.window.reparaciones_panel.aplicar_boton.isEnabled())

    def test_respuesta_automatica_ambigua_queda_para_revision(self):
        self.preparar(); self.habilitar_auto()
        self.control.establecer_motor('reparaciones', lambda texto, **kw: replace(reparaciones(texto),
            propuestas=reparaciones(texto).propuestas * 2))
        self.confirmar_linea(); antes = self.editor.toPlainText()
        self.esperar(lambda: 'revisión manual' in self.window.statusBar().currentMessage())
        self.assertEqual(self.editor.toPlainText(), antes)

    def test_desactivar_o_cambiar_foco_descarta_respuesta_tardia(self):
        self.preparar(); self.habilitar_auto()
        for accion in ('desactivar', 'foco'):
            self.editor.setPlainText(TEXTO); self.editor.document().setModified(False)
            self.habilitar_auto()
            iniciado, soltar = threading.Event(), threading.Event()
            def lento(texto, *, cancelar=None):
                iniciado.set(); soltar.wait(2); return reparaciones(texto)
            self.control.establecer_motor('reparaciones', lento)
            try:
                self.confirmar_linea(); self.esperar(iniciado.is_set)
                antes = self.editor.toPlainText()
                if accion == 'desactivar': self.configurar(habilitada=False)
                else: self.window.busqueda_proyecto.campo.setFocus()
                soltar.set(); self.esperar(lambda: self.control.coordinador.task is None)
                self.assertEqual(self.editor.toPlainText(), antes)
            finally: soltar.set()

    def test_no_autocorrige_enter_a_mitad_de_linea_o_con_seleccion(self):
        self.preparar(); self.habilitar_auto()
        c = self.editor.textCursor(); c.setPosition(2); self.editor.setTextCursor(c)
        QTest.keyClick(self.editor, Qt.Key.Key_Return); QTest.qWait(350)
        self.assertEqual(self.llamadas, [])
        self.editor.setPlainText(TEXTO); c = self.editor.textCursor()
        c.setPosition(TEXTO.index('3')); c.setPosition(c.position() + 1, QTextCursor.MoveMode.KeepAnchor)
        self.editor.setTextCursor(c); QTest.keyClick(self.editor, Qt.Key.Key_Return); QTest.qWait(350)
        self.assertEqual(self.llamadas, [])

    def test_configuracion_persistida_y_json_invalido_no_habilita_automatizacion(self):
        self.preparar(); self.habilitar_auto()
        settings = QSettings(str(self.base / 'preferencias.ini'), QSettings.Format.IniFormat)
        self.window.settings = settings; self.window.guardar_preferencias()
        d = preferencias.cargar(settings)
        self.assertEqual(d['asistencia'], self.control.configuracion)
        corrupto = preferencias.validar({'version': 1, 'asistencia': {'habilitada': 'si', 'reparaciones_automaticas': 1}})
        self.assertFalse(corrupto['asistencia']['reparaciones_automaticas'])

    def test_dialogo_cancelado_no_cambia_configuracion_y_no_consulta_motor(self):
        self.preparar(); antes = dict(self.control.configuracion)
        def cancelar(dialogo):
            dialogo.habilitada.setChecked(False)
            dialogo.pausa.setChecked(True)
            return dialogo.DialogCode.Rejected
        with patch.object(PreferenciasAsistencia, 'exec', cancelar): self.window.configurar_asistencia()
        self.assertEqual(self.control.configuracion, antes); self.assertEqual(self.llamadas, [])

    def test_dialogo_aplica_permisos_y_guarda_sin_reiniciar(self):
        self.preparar()
        def aceptar(dialogo):
            dialogo.habilitada.setChecked(False); dialogo.pausa.setChecked(True)
            dialogo.espera.setValue(900)
            return dialogo.DialogCode.Accepted
        with patch.object(PreferenciasAsistencia, 'exec', aceptar), patch.object(self.window, 'guardar_preferencias') as guardar:
            self.window.configurar_asistencia(); guardar.assert_called_once()
        self.assertFalse(self.control.configuracion['habilitada'])
        self.assertTrue(self.control.configuracion['sugerencias_pausa'])
        self.assertEqual(self.control.configuracion['pausa_ms'], 900)

    def test_cierre_con_pausa_pendiente_no_deja_consultas(self):
        self.preparar(); self.configurar(sugerencias_pausa=True)
        QTest.keyClicks(self.editor, 'x'); self.assertTrue(self.control.timer_pausa.isActive())
        self.assertTrue(self.control.detener()); QTest.qWait(350)
        self.assertEqual(self.llamadas, [])

    def test_conflicto_externo_y_compilacion_no_disparan_automatizacion(self):
        self.preparar(); self.habilitar_auto()
        with patch.object(self.window, 'tiene_cambio_externo', return_value=True):
            self.confirmar_linea(); QTest.qWait(350)
            self.assertEqual(self.llamadas, [])
        self.editor.setPlainText(TEXTO)
        with patch.object(self.window, 'process', object()):
            self.confirmar_linea(); QTest.qWait(350)
            self.assertEqual(self.llamadas, [])

    def test_timeout_y_retiro_de_regla_impiden_cambio_tardio(self):
        self.preparar(); self.habilitar_auto()
        for accion in ('timeout', 'regla'):
            self.editor.setPlainText(TEXTO); self.habilitar_auto()
            iniciado, soltar = threading.Event(), threading.Event()
            def lento(texto, *, cancelar=None):
                iniciado.set(); soltar.wait(2); return reparaciones(texto)
            self.control.establecer_motor('reparaciones', lento)
            try:
                self.confirmar_linea(); self.esperar(iniciado.is_set)
                antes = self.editor.toPlainText()
                if accion == 'timeout': self.control.coordinador.agotar()
                else: self.control.establecer_reglas(())
                soltar.set(); self.esperar(lambda: self.control.coordinador.task is None)
                self.assertEqual(self.editor.toPlainText(), antes)
            finally: soltar.set()

    def test_validacion_parser_se_ejecuta_fuera_del_hilo_grafico(self):
        self.preparar(); self.habilitar_auto()
        from ide.reparaciones_ccom import comprobar_sintaxis
        hilos = []
        def comprobar(*args, **kwargs):
            hilos.append(threading.get_ident())
            return comprobar_sintaxis(*args, **kwargs)
        with patch('ide.politica_asistencia.comprobar_sintaxis', comprobar):
            self.confirmar_linea(); self.esperar(lambda: '3 ;' in self.editor.toPlainText())
        self.assertEqual(len(hilos), 2)
        self.assertTrue(all(hilo != threading.get_ident() for hilo in hilos))
