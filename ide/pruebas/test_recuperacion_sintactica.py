"""Diagnóstico LL(1) del primer error y corrección progresiva sin guardar."""
from pathlib import Path
import unittest
from ide.pruebas import test_comprobacion as fixtures
from ide.reparaciones_ccom import comprobar_sintaxis, proponer_reparaciones
from ide.nucleo.asistencia import EstadoAsistencia

DOS = '# café 😀\nF main [] : void ;\n    Vm x : int = ;\n    Vm y : int = ;\n    E ;\n.\n'


class RecuperacionEnIDE(unittest.TestCase):
    setUp = fixtures.ComprobacionAutomatica.setUp
    cleanup = fixtures.ComprobacionAutomatica.cleanup
    esperar = fixtures.ComprobacionAutomatica.esperar
    escribir = fixtures.ComprobacionAutomatica.escribir

    def test_primer_error_navegable_y_edicion_progresiva(self):
        path=self.folder/'dos errores.cls';path.write_text(DOS)
        editor=self.window.abrir(path)
        editor.analisis_timer.setInterval(10)
        editor.programar_analisis()
        self.esperar(lambda:len(self.window.auto_results.get(editor,[]))==1)
        self.assertEqual(self.window.problems.topLevelItemCount(),1)
        self.assertEqual([p.linea for p in self.window.auto_results[editor]],[3])
        item=self.window.problems.topLevelItem(0)
        self.window.ir_problema(item,0)
        self.assertEqual(editor.textCursor().selectedText(),';')
        self.escribir(DOS.replace('x : int = ;','x : int = 1 ;'))
        self.esperar(lambda:len(self.window.auto_results.get(editor,[]))==1 and
                     self.window.auto_results[editor][0].linea==4)
        self.assertEqual(self.window.auto_results[editor][0].linea,4)
        self.escribir(DOS.replace('int = ;','int = 1 ;'))
        self.esperar(lambda:editor in self.window.auto_results and not self.window.auto_results[editor])
        self.assertEqual(self.window.problems.topLevelItemCount(),0)
        self.assertEqual(path.read_text(),DOS)

    def test_f5_no_duplica_sintaxis_del_motor_automatico(self):
        path = self.folder / 'errores para F5.cls'
        path.write_text(DOS)
        editor = self.window.abrir(path)
        editor.analisis_timer.setInterval(10)
        editor.programar_analisis()
        self.esperar(lambda: len(self.window.auto_results.get(editor, [])) == 1)
        self.window.compilar()
        self.esperar(lambda: editor in self.window.results and self.window.process is None)
        manual = self.window.results[editor]
        self.assertGreaterEqual(len(manual), 2)
        self.window.actualizar_problemas()
        self.assertEqual(self.window.problems.topLevelItemCount(), len(manual))
        self.assertTrue(all(p.origen != 'ccom automático' for p in editor.problemas))

    def test_no_acepta_programa_recuperado_en_validacion_o_reparacion(self):
        self.assertEqual(comprobar_sintaxis(DOS),'error_sintactico')
        self.assertEqual(proponer_reparaciones(DOS).estado,EstadoAsistencia.SIN_OPCIONES)
        valido=DOS.replace('int = ;','int = 1 ;')
        self.assertEqual(comprobar_sintaxis(valido),'aceptado')


if __name__=='__main__':unittest.main()
