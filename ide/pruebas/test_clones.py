"""Clones: tipos, límites, rangos y recorrido real del panel."""
import os
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from pathlib import Path
import tempfile
import time
import unittest

from ide.clones import Cancelado, LimiteAnalisis, detectar, tokenizar


class DetectorClones(unittest.TestCase):
    def test_tipo_i_ignora_espacios_comentarios_y_preserva_rangos(self):
        a = "# café 😀\nVm x : int = 5 ;\nx = x + 1 ;\n"
        b = "#[ comentario\nmultilínea ]#\nVm x:int=5; # comentario\nx=x+1;"
        pairs = detectar({Path('/tmp/a.cls'): a, Path('/tmp/b.cls'): b}, minimo=10)
        self.assertEqual(len(pairs), 1)
        pair = pairs[0]
        self.assertEqual(pair.tipo, 'I')
        self.assertEqual(pair.primero.linea_inicio, 2)
        self.assertEqual(pair.segundo.linea_inicio, 3)
        self.assertEqual(a[pair.primero.inicio:pair.primero.fin], "Vm x : int = 5 ;\nx = x + 1 ;")

    def test_tipo_ii_renombramiento_consistente_y_literales(self):
        a = "Vm x : int = 5 ; x = x + 1 ;"
        b = "Vm contador : int = 10 ; contador = contador + 2 ;"
        pairs = detectar({Path('/tmp/a.cls'): a, Path('/tmp/b.cls'): b}, 10)
        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0].tipo, 'II')
        self.assertEqual(pairs[0].tokens, len(tokenizar(a)))

    def test_no_colapsa_identificadores_distintos(self):
        sources = {Path('/tmp/a.cls'): "x = x + y ;", Path('/tmp/b.cls'): "a = b + b ;"}
        self.assertEqual(detectar(sources, 6), [])

    def test_operadores_y_reservadas_no_se_normalizan(self):
        for a,b in [("x = x + 1 ;", "y = y - 2 ;"), ("cifrar [ x , 1 ] ;", "descifrar [ y , 2 ] ;"), ("Vm x : int = 1 ;", "Vm y : bool = verdadero ;")]:
            with self.subTest(a=a,b=b):
                self.assertEqual(detectar({Path('/tmp/a.cls'):a,Path('/tmp/b.cls'):b},len(tokenizar(a))),[])

    def test_un_archivo_y_fragmentos_cortos_no_son_clones_entre_archivos(self):
        self.assertEqual(detectar({Path('/tmp/a.cls'):"x = 1 ; "*1000},10),[])
        self.assertEqual(detectar({Path('/tmp/a.cls'):"E ; .",Path('/tmp/b.cls'):"E ; ."},5),[])

    def test_cadenas_y_booleanos_conservan_tipo(self):
        a = 'Vm x : bool = verdadero ; x = falso ;'
        b = 'Vm y : bool = falso ; y = verdadero ;'
        pairs = detectar({Path('/tmp/a.cls'):a,Path('/tmp/b.cls'):b},len(tokenizar(a)))
        self.assertEqual([p.tipo for p in pairs],['II'])

    def test_cancelacion_y_limites_explicitos(self):
        with self.assertRaises(Cancelado):
            detectar({Path('/tmp/a.cls'):"x = 1 ;"},5,lambda:True)
        with self.assertRaises(LimiteAnalisis):
            detectar({Path(f'/tmp/{i}.cls'):'' for i in range(201)})
        with self.assertRaises(ValueError):
            detectar({},2)

    def test_ejemplos_tres_pares_maximos_sin_duplicados(self):
        folder=Path(__file__).resolve().parents[1]/'ejemplos/clones'
        pairs=detectar({p:p.read_text() for p in folder.glob('*.cls')})
        self.assertEqual(len(pairs),3)
        self.assertEqual(sorted(p.tipo for p in pairs),['I','II','II'])
        self.assertEqual({p.tokens for p in pairs},{44})


class IntegracionClones(unittest.TestCase):
    def setUp(self):
        from PySide6.QtWidgets import QApplication
        from ide.interfaz.ventana import Ventana
        self.app=QApplication.instance() or QApplication([])
        self.directory=tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder=Path(self.directory.name)
        self.window=Ventana(Path(__file__).resolve().parents[2], preferencias=False)
        self.addCleanup(self.cleanup)

    def cleanup(self):
        self.window.clones_panel.detener()
        self.app.processEvents()
        for i in range(self.window.tabs.count()):
            self.window.tabs.widget(i).document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        self.app.processEvents()

    def esperar(self):
        limit=time.monotonic()+5
        while self.window.clones_panel.task and time.monotonic()<limit:
            self.app.processEvents()
            time.sleep(.005)
        self.assertIsNone(self.window.clones_panel.task)

    def test_subcarpetas_buffers_navegacion_e_invalidacion(self):
        a=self.folder/'a.cls'
        nested=self.folder/'sub'; nested.mkdir()
        b=nested/'b.cls'
        text="# café 😀\nVm x : int = 5 ; x = x + 1 ;"
        a.write_text(text)
        b.write_text("F main [] : void ; E ; .")
        ignored=self.folder/'build'; ignored.mkdir()
        (ignored/'ignorado.cls').write_text(text)
        self.window.cargar_carpeta(self.folder)
        note=self.folder/'nota.txt';note.write_text(text)
        self.window.abrir(note)
        self.window.abrir(ignored/'ignorado.cls')
        editor=self.window.abrir(b)
        editor.selectAll();editor.insertPlainText(text)
        self.window.clones_panel.minimum.setValue(10)
        self.window.analizar_clones();self.esperar()
        panel=self.window.clones_panel
        self.assertEqual(len(panel.sources),2)
        self.assertEqual(len(panel.pairs),1)
        self.assertEqual(b.read_text(),"F main [] : void ; E ; .",'Analizar no debe guardar los buffers')
        panel.list.setCurrentItem(panel.list.topLevelItem(0))
        self.assertIn('Vm x',panel.previews[0].toPlainText())
        panel.abrir(0)
        self.assertEqual(self.window.editor().path,a)
        self.assertEqual(self.window.editor().textCursor().selectedText(),"Vm x : int = 5 ; x = x + 1 ;")
        editor.insertPlainText("# cambio\n")
        self.assertEqual(panel.list.topLevelItemCount(),0)
        self.assertTrue(panel.obsolete)

    def test_cambio_externo_impide_navegar_rango_antiguo(self):
        text='Vm x : int = 5 ; x = x + 1 ;'
        for name in ('a.cls','b.cls'):
            (self.folder/name).write_text(text)
        self.window.cargar_carpeta(self.folder)
        self.window.clones_panel.minimum.setValue(10)
        self.window.analizar_clones();self.esperar()
        panel=self.window.clones_panel
        panel.list.setCurrentItem(panel.list.topLevelItem(0))
        (self.folder/'a.cls').write_text('E ; .')
        panel.abrir(0)
        self.assertTrue(panel.obsolete)
        self.assertIsNone(self.window.editor().path)

    def test_cambiar_carpeta_durante_tarea_descarta_resultados(self):
        text='Vm x : int = 5 ; x = x + 1 ;'
        for name in ('a.cls','b.cls'):
            (self.folder/name).write_text(text)
        other=self.folder/'otra';other.mkdir()
        self.window.cargar_carpeta(self.folder)
        self.window.analizar_clones()
        self.window.cargar_carpeta(other)
        self.esperar()
        self.assertEqual(self.window.clones_panel.list.topLevelItemCount(),0)


if __name__=='__main__':
    unittest.main()
