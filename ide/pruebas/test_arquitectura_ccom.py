"""Contratos M6 y recorrido aislado: ningún analizador LL(1) entra a producción."""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
from ide import ccom
from ide.clones import empaquetar
from ide.servicio_ccom import consultar

ROOT = Path(__file__).resolve().parents[2]
TEXTO = '# café 😀\r\nF main [] : void ; E ; .\n'


class ContratosNativos(unittest.TestCase):
    def respuestas(self):
        paquete, _ = empaquetar({Path('/tmp/a.cls'): TEXTO, Path('/tmp/b.cls'): TEXTO})
        consultas = [('check', TEXTO, ['--check-json']), ('lex', TEXTO, ['--tokens-json']),
                     ('suggest', 'F main [] : vo', ['--suggest-json', '14']),
                     ('repair', 'F main [] : void ; E ;', ['--repair-json']),
                     ('syntax', TEXTO, ['--syntax-json']), ('clones', paquete, ['--clones-json', '5'])]
        for operacion, texto, argumentos in consultas:
            salida, fuente, codigo = consultar(texto, argumentos, root=ROOT)
            yield operacion, salida, fuente, codigo

    def test_los_seis_servicios_reales_comparten_cabecera(self):
        for operacion, salida, fuente, codigo in self.respuestas():
            with self.subTest(operacion=operacion):
                d = ccom.informe_json(salida, fuente, codigo, operacion)
                self.assertEqual(d['operation'], operacion)
                self.assertEqual(codigo, 0)
                self.assertFalse(fuente.exists(), 'La instantánea debe estar eliminada')
                self.assertEqual(d['offset_unit'], 'utf8')

    def test_todos_rechazan_cruces_de_consulta_y_encodings_ajenos(self):
        for operacion, salida, fuente, codigo in self.respuestas():
            d = json.loads(salida)
            for campo, valor in [('schema_version', True), ('operation', 'otra'),
                                  ('source', '/tmp/otra'), ('exit_code', True), ('offset_unit', 'utf16')]:
                cambiado = copy.deepcopy(d); cambiado[campo] = valor
                with self.subTest(operacion=operacion, campo=campo), self.assertRaises(ccom.ErrorProtocolo):
                    ccom.informe_json(json.dumps(cambiado), fuente, codigo, operacion)
            for raw in (salida.decode().encode('utf-16'), b'{"x":NaN}', b'{"x":1,"x":2}'):
                with self.subTest(operacion=operacion, raw=raw[:20]), self.assertRaises(ccom.ErrorProtocolo):
                    ccom.informe_json(raw, fuente, codigo, operacion)

    def test_recorrido_real_en_proceso_que_bloquea_analizadores_python(self):
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', PYTHONDONTWRITEBYTECODE='1')
        resultado = subprocess.run([sys.executable, str(ROOT/'tools/probar_recorrido_ide.py')],
                                   cwd=ROOT, env=env, capture_output=True, timeout=40)
        self.assertEqual(resultado.returncode, 0, resultado.stderr.decode(errors='replace'))
        d = json.loads(resultado.stdout)
        self.assertEqual(d['status'], 'ok')
        self.assertEqual(len(d['recorridos']), 5)
        self.assertEqual(d['analizadores_python_cargados'], [])


if __name__ == '__main__':
    unittest.main()
