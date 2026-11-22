#!/usr/bin/env python3
"""Check rápido de CLI, JSON, mediciones reales y HTML/SVG de medir.py."""
import copy
from html.parser import HTMLParser
import json
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools import medir


class Html(HTMLParser):
    def __init__(self, texto):
        super().__init__()
        self.svg = 0
        self.externos = []
        self.feed(texto)

    def handle_starttag(self, tag, attrs):
        self.svg += tag == 'svg'
        self.externos.extend(v for k, v in attrs if k in ('src', 'href') and v.startswith(('http:', 'https:', '//')))


class Medicion(unittest.TestCase):
    def ejecutar(self, salida, modo, fuente, codigo=0):
        r = subprocess.run([sys.executable, str(ROOT / 'tools/medir.py'),
                            '--modo', modo, '--salida', str(salida), str(ROOT / fuente)],
                           cwd=ROOT, capture_output=True, text=True, timeout=45)
        self.assertEqual(r.returncode, codigo, r.stdout + r.stderr)
        return r

    def esquema(self, r):
        self.assertEqual(r['schema_version'], 1)
        self.assertIs(r['equivalente'], True, r['errores'])
        self.assertEqual(r['errores'], [])
        self.assertIn(r['modo'], ('nivel', 'optimizacion'))
        self.assertTrue(r['programa'].endswith('.cls'))
        for etiqueta, m in r['compilaciones'].items():
            self.assertEqual(len(m['muestras_ms']), 5)
            self.assertTrue(all(isinstance(t, (int, float)) and t >= 0 for t in m['muestras_ms']))
            self.assertEqual(m['tiempo_ms'], statistics.median(m['muestras_ms']))
            self.assertIn('--time', m['comando'])
            if m['codigo']:
                self.assertTrue(m['error_esperado'])
                self.assertEqual(m['codigo'], 1)
                self.assertIn('sin registros temporales disponibles', m['error'])
                for campo in ('emitidas', 'bytes', 'bundles', 'ciclos', 'contadores'):
                    self.assertIsNone(m[campo])
                continue
            for campo in ('emitidas', 'bundles', 'bytes', 'ciclos'):
                self.assertIs(type(m[campo]), int)
                self.assertGreater(m[campo], 0)
            self.assertEqual(m['bytes'], 16 * m['bundles'])
            self.assertLessEqual(m['emitidas'], 4 * m['bundles'])
            self.assertGreaterEqual(m['archivo_bin_bytes'], m['bytes'] + 16)
            self.assertEqual(m['ciclos'], m['ciclos_por_datos']['cero'])
            self.assertIs(m['equivalente'], True)
            self.assertEqual(set(m['contadores']), set(medir.CONTADORES))
            self.assertIsInstance(m['contadores']['desenrollados'], list)
            for clave in medir.CONTADORES[1:]:
                self.assertIs(type(m['contadores'][clave]), int)
                self.assertGreaterEqual(m['contadores'][clave], 0)
            if m['nivel'] == 'O0':
                self.assertNotIn('--metrics', m['comando'])
                base = Path(m['comando'][-1]).with_suffix('.asm')
                programa = medir.parse(base.read_text())
                self.assertEqual(m['emitidas'], sum(op.mnemonic != 'nopi' for b in programa.bundles for op in b.slots))
            else:
                self.assertIn('--metrics', m['comando'])

    def test_cli_ambos_modos_esquema_y_html(self):
        with tempfile.TemporaryDirectory(prefix='calser-medir-') as tmp:
            salida = Path(tmp)
            self.ejecutar(salida, 'optimizacion', 'pruebas/optimizador/unroll/u1_constante.cls')
            self.ejecutar(salida, 'nivel', 'pruebas/rutinas/arreglos/matriz3x4.cls')
            indice = json.loads((salida / 'indice.json').read_text())
            self.assertEqual(indice['schema_version'], 1)
            self.assertEqual(len(indice['resultados']), 2)
            for r in indice['resultados']:
                self.esquema(r)
                self.assertEqual(json.loads((salida / r['archivo']).read_text()), r)
                etiquetas = ('antes', 'despues') if r['modo'] == 'optimizacion' else ('O0', 'O1', 'O2')
                self.assertEqual(tuple(r['compilaciones']), etiquetas)
            texto = (salida / 'reporte.html').read_text()
            html = Html(texto)
            self.assertEqual(html.svg, 8)
            self.assertEqual(html.externos, [])
            self.assertIn('@media print', texto)
            self.assertIn('vs antes:', texto)
            self.assertIn('O2 vs O0:', texto)
            self.assertIn('reordenadas', texto)
            self.assertNotIn('<script', texto)
            # Una discrepancia debe ser visible en rojo y conservar el error.
            alterado = copy.deepcopy(indice)
            alterado['resultados'][0]['equivalente'] = False
            alterado['resultados'][0]['errores'] = ['estado <distinto>']
            medir.escribir_html(alterado, salida)
            texto = (salida / 'reporte.html').read_text()
            self.assertIn('<article class="error">', texto)
            self.assertIn('estado &lt;distinto&gt;', texto)

    def test_spill_antes_no_compila_y_despues_valores_esperados(self):
        with tempfile.TemporaryDirectory(prefix='calser-medir-spill-') as tmp:
            salida = Path(tmp)
            esperados = json.loads((ROOT / 'pruebas/optimizador/spill/esperados.json').read_text())
            for fuente in sorted((ROOT / 'pruebas/optimizador/spill').glob('s[123]_*.cls')):
                with self.subTest(programa=fuente.stem):
                    self.ejecutar(salida, 'optimizacion', str(fuente.relative_to(ROOT)))
                    r = next(r for r in json.loads((salida / 'indice.json').read_text())['resultados']
                             if r['programa'] == str(fuente.relative_to(ROOT)))
                    self.esquema(r)
                    self.assertTrue(r['compilaciones']['antes']['error_esperado'])
                    self.assertEqual(r['compilaciones']['despues']['valores'], esperados[fuente.stem]['valores'])
            self.assertIn('no compila', (salida / 'reporte.html').read_text())
            self.assertIn('no disponible (no compila)', (salida / 'reporte.html').read_text())

    def test_listas_y_variacion(self):
        self.assertEqual(len(medir.predeterminados('optimizacion')), 15)
        self.assertEqual(len(medir.predeterminados('nivel')), 6)
        for pasada in medir.PASADAS:
            self.assertEqual(sum(p.parent.name == pasada for p in medir.predeterminados('optimizacion')), 3)
        self.assertEqual(medir.variacion(100, 75), '-25.0%')
        self.assertEqual(medir.variacion(0, 0), '0.0%')
        self.assertIn('base cero', medir.variacion(0, 1))
        self.assertEqual(medir.variacion(None, 1), 'no disponible')

    def test_error_de_uso(self):
        with tempfile.TemporaryDirectory(prefix='calser-medir-uso-') as tmp:
            self.ejecutar(Path(tmp), 'optimizacion', 'pruebas/p03_suma.cls', codigo=2)
            self.assertFalse((Path(tmp) / 'indice.json').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
