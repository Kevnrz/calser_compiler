"""Recorrido gráfico M6 reproducible, con ccom/simulador reales y varios buffers.

Usa Qt offscreen por defecto; no abre ni altera proyectos del usuario. Todos los
archivos viven en un directorio temporal. Bloquea la importación de analizadores
LL(1) para comprobar que ningún recorrido productivo los necesita. No usa dobles
de lenguaje. --captura guarda la ventana final antes de cerrar para revisión.
"""
import argparse
import importlib.abc
import json
import os
from pathlib import Path
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
LEGADO = frozenset('ide.nucleo.' + n for n in ('lexico', 'gramatica', 'tabla', 'parser'))


class SinAnalizadoresPython(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        if fullname in LEGADO:
            raise AssertionError('El recorrido productivo intentó importar ' + fullname)
        return None


def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument('--captura', type=Path)
    opciones = args.parse_args()
    assert not LEGADO.intersection(sys.modules)
    sys.meta_path.insert(0, SinAnalizadoresPython())
    os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QTextCursor
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    from ide.interfaz.ventana import Ventana
    app = QApplication.instance() or QApplication([])
    pasos = []
    with tempfile.TemporaryDirectory(prefix='calside_recorrido_m6_') as carpeta:
        carpeta = Path(carpeta)
        texto = '# café 😀\nF main [] : void ;\n    Vm resultado : int = 2 + 3 ;\n    E ;\n.\n'
        a, b = carpeta/'suma con espacios.cls', carpeta/'otra.cls'
        a.write_text(texto, encoding='utf-8')
        b.write_text(texto.replace('resultado', 'total'), encoding='utf-8')
        originales = {p: p.read_bytes() for p in (a, b)}
        ventana = Ventana(ROOT, preferencias=False)
        ventana.resize(1200, 850)
        ventana.show(); ventana.activateWindow(); app.processEvents()
        ventana.monitor.timer.stop()
        ventana.comprobador.demora = 10

        def esperar(condicion, explicacion, plazo=10):
            fin = time.monotonic() + plazo
            while not condicion() and time.monotonic() < fin:
                app.processEvents(); time.sleep(.005)
            assert condicion(), explicacion

        try:
            ventana.cargar_carpeta(carpeta)
            primero, segundo = ventana.abrir(a), ventana.abrir(b)
            for editor in (primero, segundo):
                editor.analisis_timer.setInterval(10)
                editor.programar_analisis()
            esperar(lambda: all(e.analisis_revision == e.revision and e in ventana.auto_results
                                for e in (primero, segundo)), 'Análisis nativo de ambas pestañas')
            assert not ventana.auto_results[primero] and not ventana.auto_results[segundo]
            pasos.append('M1/M2: analisis y colores de dos documentos, con Unicode')

            ventana.clones_panel.minimum.setValue(10)
            ventana.analizar_clones()
            esperar(lambda: ventana.clones_panel.task is None, 'Clones C completos')
            assert any(p.tipo == 'II' for p in ventana.clones_panel.pairs)
            ventana.clones_panel.list.setCurrentItem(ventana.clones_panel.list.topLevelItem(0))
            assert 'main' in ventana.clones_panel.previews[0].toPlainText()
            ventana.clones_panel.abrir(0)
            assert ventana.editor().path == ventana.clones_panel.pairs[0].primero.archivo
            ventana.tabs.setCurrentWidget(primero)
            pasos.append('M5: clones reales, comparacion y navegacion entre documentos')

            sin_cierre = texto.replace('2 + 3 ;', '2 + 3')
            primero.selectAll(); primero.insertPlainText(sin_cierre)
            primero.setFocus(); app.processEvents()
            esperar(lambda: primero in ventana.auto_results and bool(ventana.auto_results[primero]),
                    'Error sintáctico sin guardar')
            assert ventana.auto_results[primero][0].categoria == 'Sintáctico'
            assert not ventana.clones_panel.pairs
            QTest.keyClick(primero, Qt.Key.Key_Return, Qt.KeyboardModifier.AltModifier)
            panel = ventana.reparaciones_panel
            esperar(lambda: not panel.ocupado and panel.lista.count() > 0, 'Panel real de reparaciones')
            assert panel.aplicar_boton.isEnabled()
            panel.aplicar_boton.click()
            assert '2 + 3;' in primero.toPlainText()
            primero.undo(); assert primero.toPlainText() == sin_cierre
            primero.redo(); assert '2 + 3;' in primero.toPlainText()
            assert all(p.read_bytes() == raw for p, raw in originales.items())
            pasos.append('M4: reparar buffer, deshacer/rehacer, disco intacto')

            tercero = ventana.nuevo()
            tercero.selectAll(); tercero.insertPlainText('F main [] : vo')
            tercero.setFocus(); app.processEvents()
            QTest.keyClick(tercero, Qt.Key.Key_Space, Qt.KeyboardModifier.ControlModifier)
            popup = ventana.asistencia.popup
            esperar(lambda: popup.isVisible() and popup.lista.count() > 0, 'Sugerencias C en documento sin nombre')
            assert popup.lista.currentItem().text().startswith('void')
            QTest.keyClick(tercero, Qt.Key.Key_Return)
            assert tercero.toPlainText() == 'F main [] : void'
            assert tercero.path is None
            ventana.tabs.setCurrentWidget(segundo)
            assert not popup.isVisible()
            assert not ventana.auto_results[segundo]
            pasos.append('M3: completar sin guardar y cambiar de pestana sin mezclar resultados')

            ventana.tabs.setCurrentWidget(primero)
            ventana.guardar()
            ventana.compilar()
            esperar(lambda: ventana.process is None, 'Compilación F5', 15)
            assert primero in ventana.compilados, ventana.output.toPlainText()
            assert ventana.ejecutar()
            esperar(lambda: ventana.run_process is None, 'Simulación F6', 15)
            assert 'resultado = 5' in ventana.run_output.toPlainText()
            assert 'Ejecución terminada correctamente' in ventana.run_output.toPlainText()
            esperar(lambda: primero.analisis_revision == primero.revision and primero in ventana.auto_results,
                    'Análisis actualizado tras F5/F6')
            assert not ventana.auto_results[primero]
            assert b.read_bytes() == originales[b]
            assert not LEGADO.intersection(sys.modules)
            pasos.append('F5/F6: compilacion y simulacion reales, resultado = 5')
            if opciones.captura:
                app.processEvents()
                assert ventana.grab().save(str(opciones.captura)), 'No se pudo guardar captura'
        finally:
            if ventana.process or ventana.run_process:
                ventana.cancelar()
                esperar(lambda: ventana.process is None and ventana.run_process is None, 'Cerrar procesos')
            for i in range(ventana.tabs.count()):
                ventana.tabs.widget(i).document().setModified(False)
            assert ventana.close(), 'La ventana debe cerrar sus procesos/trabajadores'
            ventana.deleteLater(); app.processEvents()
    print(json.dumps({'status': 'ok', 'recorridos': pasos, 'analizadores_python_cargados': []}, ensure_ascii=False))


if __name__ == '__main__':
    main()
