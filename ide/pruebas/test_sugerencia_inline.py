"""Recorrido gráfico focalizado con ccom real: gris, Tab, cancelación y cierres."""
from pathlib import Path
import unittest
from PySide6.QtCore import Qt
from PySide6.QtGui import QTextCursor
from PySide6.QtTest import QTest
from ide.pruebas import test_carpetas as base
from ide.interfaz.preferencias_asistencia import PreferenciasAsistencia


class SugerenciaInline(unittest.TestCase):
    setUp = base.Carpetas.setUp
    limpiar = base.Carpetas.limpiar
    esperar = base.Carpetas.esperar
    terminar = base.Carpetas.terminar

    def preparar(self, texto, posicion=None):
        self.editor = self.window.editor()
        self.editor.setPlainText(texto)
        cursor = self.editor.textCursor()
        indice = len(texto) if posicion is None else posicion
        cursor.setPosition(len(texto[:indice].encode('utf-16-le')) // 2)
        self.editor.setTextCursor(cursor)
        self.editor.document().setModified(False)
        self.window.show(); self.window.activateWindow()
        self.editor.setFocus(); QTest.qWait(20)
        self.control = self.window.asistencia
        self.control.configurar({**self.control.configuracion, 'sugerencias_inline': True, 'pausa_ms': 300})

    def gris(self):
        self.esperar(lambda:self.editor.sugerencia_inline.isVisible())

    def test_completa_en_gris_tab_y_undo_sin_modificar_preview(self):
        self.preparar('# café 😀\nF main [] : v')
        QTest.keyClicks(self.editor, 'o')
        contexto = self.editor.contexto_asistencia()
        self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, 'id')
        QTest.keyClick(self.editor, Qt.Key.Key_BracketRight, Qt.KeyboardModifier.AltModifier)
        self.assertTrue(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(), contexto)
        self.assertFalse(self.control.popup.isVisible())
        self.window.grab().save('/tmp/calside-sugerencia-inline.png')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertTrue(self.editor.toPlainText().endswith('void'))
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Z, Qt.KeyboardModifier.ControlModifier)
        self.assertEqual(self.editor.toPlainText(), contexto.texto)

    def test_escape_enter_y_foco_descartan_sin_aceptar(self):
        self.preparar('F main [] : v')
        for tecla in (Qt.Key.Key_Escape, Qt.Key.Key_Return):
            self.editor.setPlainText('F main [] : v')
            cursor = self.editor.textCursor(); cursor.movePosition(QTextCursor.MoveOperation.End); self.editor.setTextCursor(cursor)
            QTest.keyClicks(self.editor, 'o'); self.gris()
            QTest.keyClick(self.editor, tecla)
            self.assertFalse(self.editor.sugerencia_inline.isVisible())
            self.assertNotIn('void', self.editor.toPlainText())
        self.editor.setPlainText('F main [] : v')
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        QTest.keyClicks(self.editor, 'o'); self.gris()
        self.window.busqueda_proyecto.campo.setFocus(); QTest.qWait(20)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())

    def test_comentario_y_cambio_de_pestana_no_insertan_propuestas(self):
        self.preparar('# nota')
        QTest.keyClicks(self.editor, ' vo')
        QTest.qWait(500)
        self.esperar(lambda:self.control.coordinador.task is None)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.toPlainText(), '# nota vo')
        self.editor.setPlainText('F main [] : v')
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        QTest.keyClicks(self.editor, 'o')
        self.window.nuevo(); QTest.qWait(500)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.window.editor().toPlainText(), '')

    def test_reparacion_punto_y_coma_inline_sin_panel_ni_guardado(self):
        texto = 'F main [] : void ;\n    Vm x : int = \n    E ;\n.\n'
        self.preparar(texto, texto.index('\n', texto.index('Vm')))
        QTest.keyClicks(self.editor, '3')
        antes = self.editor.toPlainText()
        self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, ';')
        self.assertEqual(self.editor.toPlainText(), antes)
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertIn('3;', self.editor.toPlainText())
        self.editor.undo(); self.assertEqual(self.editor.toPlainText(), antes)
        self.assertFalse(list(self.folder.rglob('*.cls')))

    def test_auto_enter_regla_real_con_una_activacion_y_deshacer(self):
        texto = 'F main [] : void ;\n    Vm x : int = 3\n    E ;\n.\n'
        self.preparar(texto, texto.index('\n', texto.index('Vm')))
        dialogo=PreferenciasAsistencia(self.control.configuracion, self.control.reglas, self.window)
        dialogo.automaticas.setChecked(True)
        datos=dialogo.datos(); self.assertEqual(datos['reglas_automaticas'], ['INSERCION_PUNTO_COMA'])
        self.control.configurar(datos);dialogo.deleteLater()
        QTest.keyClick(self.editor, Qt.Key.Key_Return)
        antes=self.editor.toPlainText()
        self.esperar(lambda:'3;' in self.editor.toPlainText())
        self.editor.undo();self.assertEqual(self.editor.toPlainText(), antes)
        QTest.qWait(450);self.assertEqual(self.editor.toPlainText(), antes)

    def test_manual_ctrl_espacio_conserva_lista_y_tab(self):
        self.preparar('F main [] : vo')
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda:self.control.popup.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(),'F main [] : void')

    def test_diagnostico_tardio_no_cancela_gris_y_tab_sigue_funcionando(self):
        self.preparar('F main [] : v')
        self.window.auto_action.setChecked(True)
        self.window.comprobador.demora = 800
        QTest.keyClicks(self.editor, 'o')
        self.gris()
        contexto = self.editor.contexto_asistencia()
        self.esperar(lambda:bool(self.window.auto_results.get(self.editor)))
        QTest.qWait(50)  # Procesar el cambio de tamaño al aparecer el aviso.
        self.assertTrue(self.window.diagnostic_message.isVisible())
        self.assertEqual(self.editor.contexto_asistencia(), contexto)
        self.assertTrue(self.editor.sugerencia_inline.isVisible())
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F main [] : void')

    def test_main_completo_propone_cabecera_compacta_y_cursor_fuera(self):
        self.preparar('F mai')
        QTest.keyClicks(self.editor, 'n'); self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto, '[]: ')
        self.assertEqual(self.editor.toPlainText(), 'F main')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F main[]: ')
        self.assertEqual(self.editor.textCursor().position(), len('F main[]: '))
        self.editor.undo();self.assertEqual(self.editor.toPlainText(), 'F main')

    def test_linea_vacia_no_inventa_C_y_prefijo_V_si_completa(self):
        self.preparar('F main[]: void;\n   ')
        QTest.keyClicks(self.editor, ' ')
        self.esperar(lambda:not self.control.timer_pausa.isActive())
        self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.toPlainText(), 'F main[]: void;\n    ')
        QTest.keyClick(self.editor, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
        self.esperar(lambda:self.control.popup.isVisible())
        self.assertIn('C', [self.control.popup.lista.item(i).data(Qt.ItemDataRole.UserRole).texto
                            for i in range(self.control.popup.lista.count())])
        QTest.keyClick(self.editor, Qt.Key.Key_Escape)
        QTest.keyClicks(self.editor, 'V')
        self.gris()
        self.assertIn(self.editor.sugerencia_inline.texto, ('i','m'))

    def test_tab_sale_del_par_y_continua_con_dos_puntos_y_punto_coma(self):
        self.preparar('F sum')
        QTest.keyClicks(self.editor, 'a'); self.gris()
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F suma[]')
        self.assertEqual(self.editor.textCursor().position(), len('F suma['))
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)  # Salir sin escribir tabulación.
        self.assertEqual(self.editor.toPlainText(), 'F suma[]')
        self.assertEqual(self.editor.textCursor().position(), len('F suma[]'))
        self.gris();self.assertEqual(self.editor.sugerencia_inline.texto, ':')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F suma[]:')
        QTest.keyClicks(self.editor, ' vo');self.gris()
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F suma[]: void')
        self.gris();self.assertEqual(self.editor.sugerencia_inline.texto, ';')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(), 'F suma[]: void;')
        self.editor.undo();self.assertEqual(self.editor.toPlainText(), 'F suma[]: void')

    def test_flecha_sale_del_par_y_editar_cancela_salto_especial(self):
        self.preparar('F sum')
        QTest.keyClicks(self.editor, 'a');self.gris()
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        QTest.keyClick(self.editor, Qt.Key.Key_Right)
        self.gris();self.assertEqual(self.editor.sugerencia_inline.texto, ':')
        QTest.keyClick(self.editor, Qt.Key.Key_Escape)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.editor.setPlainText('F sum')
        cursor=self.editor.textCursor();cursor.movePosition(QTextCursor.MoveOperation.End);self.editor.setTextCursor(cursor)
        QTest.keyClicks(self.editor, 'a');self.gris()
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        QTest.keyClicks(self.editor, 'int')
        self.assertIsNone(self.control.salida_plantilla)
        self.editor.undo()
        self.assertIsNone(self.control.salida_plantilla)

    def test_main_continua_void_y_se_detiene_tras_cabecera_y_tras_punto(self):
        self.preparar('F mai')
        QTest.keyClicks(self.editor, 'n');self.gris()
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.gris();self.assertEqual(self.editor.sugerencia_inline.texto, 'void')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.gris();self.assertEqual(self.editor.sugerencia_inline.texto, ';')
        QTest.keyClick(self.editor, Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(),'F main[]: void;')
        self.esperar(lambda:not self.control.timer_pausa.isActive())
        self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertFalse(self.control.opciones_inline)
        QTest.keyClicks(self.editor, '.')
        self.esperar(lambda:not self.control.timer_pausa.isActive())
        self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
        self.assertFalse(self.editor.sugerencia_inline.isVisible())
        self.assertEqual(self.editor.toPlainText(),'F main[]: void;.')

    def test_retorno_generico_muestra_tipos_y_permite_elegir(self):
        self.preparar('F suma[]:')
        QTest.keyClicks(self.editor, ' ');self.gris()
        self.assertEqual(self.editor.sugerencia_inline.texto,'bool')
        QTest.keyClick(self.editor,Qt.Key.Key_BracketRight,Qt.KeyboardModifier.AltModifier)
        self.assertEqual(self.editor.sugerencia_inline.texto,'int')
        QTest.keyClick(self.editor,Qt.Key.Key_Tab)
        self.assertEqual(self.editor.toPlainText(),'F suma[]: int')

    def test_contexto_nativo_main_no_se_filtra_al_cuerpo(self):
        from ide.sugerencias_ccom import sugerir
        for texto in ('F main[]: void; L', 'F main[]: void; C', 'F main[]: void; R main'):
            opciones = sugerir(texto, len(texto)).sugerencias
            self.assertEqual(opciones[0].texto,'[]')
            self.assertEqual(opciones[0].cursor_relativo,1)
        texto='F main[]: void; Vm x : '
        opciones=sugerir(texto,len(texto)).sugerencias
        self.assertEqual([o.texto for o in opciones if o.preferida_inline],['bool','int'])
        for texto in ('F main[]: void;', 'F main[]: void;.'):
            opciones=sugerir(texto,len(texto)).sugerencias
            self.assertTrue(opciones)
            self.assertFalse(any(o.preferida_inline for o in opciones))

    def test_enter_despues_de_E_punto_coma_propone_cierre_y_no_otro_F(self):
        texto=('F main[]: void;\n Vm resultado : int;\n resultado = suma[30,10];\n E;\n.\n'
               'F suma[int a, int b]: int c;\n c = a + b;\n E;')
        for automaticas in (False,True):
            with self.subTest(correccion_automatica=automaticas):
                self.preparar(texto[:-1])
                self.control.configurar({**self.control.configuracion,
                    'reparaciones_automaticas':automaticas,'reglas_automaticas':['INSERCION_PUNTO_COMA']})
                QTest.keyClicks(self.editor,';')
                self.esperar(lambda:not self.control.timer_pausa.isActive())
                self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
                self.assertFalse(self.editor.sugerencia_inline.isVisible())
                self.assertEqual(self.editor.toPlainText(),texto)
                QTest.keyClick(self.editor,Qt.Key.Key_Return)
                antes=self.editor.toPlainText()
                self.gris();self.assertEqual(self.editor.sugerencia_inline.texto,'.')
                self.assertEqual(self.editor.toPlainText(),antes)
                QTest.keyClick(self.editor,Qt.Key.Key_Tab)
                self.assertEqual(self.editor.toPlainText(),antes+'.')
                self.esperar(lambda:not self.control.timer_pausa.isActive())
                self.esperar(lambda:self.control.coordinador.task is None and not self.control.coordinador.pendientes)
                self.assertFalse(self.editor.sugerencia_inline.isVisible())
                self.editor.undo();self.assertEqual(self.editor.toPlainText(),antes)

    def test_parentesis_de_llamada_tiene_mensaje_y_posicion_correctos(self):
        from ide import ccom
        from ide.servicio_ccom import consultar
        texto=('F main[]: void;\n Vm resultado : int;\n resultado = suma(30,10);\n E;\n.\n'
               'F suma[int a, int b]: int c;\n c = a + b;\n E;\n.\n')
        salida,fuente,codigo=consultar(texto,['--check-json'])
        problemas=ccom.diagnosticos_json(salida,fuente,texto,codigo)
        self.assertIn("se esperaba '['",problemas[0].descripcion)
        self.assertIn('nombre(...)',problemas[0].descripcion)
        self.assertNotIn("se esperaba ';'",problemas[0].descripcion)
        self.assertEqual(problemas[0].linea,3)
        self.assertEqual(problemas[0].rango_utf8[0],texto.encode().index(b'(30'))
        correcto=texto.replace('suma(30,10)','suma[30,10]')
        salida,fuente,codigo=consultar(correcto,['--check-json'])
        self.assertEqual(codigo,0)
        self.assertFalse(ccom.diagnosticos_json(salida,fuente,correcto,codigo))
