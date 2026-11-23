"""Vigencia de la referencia LL(1) frente al parser que compila Calser."""
import copy
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from tools.verificar_gramatica_ll1 import reglas_bison, verificar, verificar_terminales

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which('bison'), 'Bison es necesario para extraer la gramática vigente')
class GramaticaVigente(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory(prefix='calser_gramatica_test_') as temporary:
            folder = Path(temporary)
            xml = folder / 'parser.xml'
            subprocess.run(['bison', '--xml=' + str(xml), '-o', str(folder / 'parser.c'),
                            str(ROOT / 'grammar/parser.y')], check=True,
                           capture_output=True, timeout=30)
            cls.reglas = reglas_bison(xml)

    def test_reglas_actuales_transformadas_coinciden_sin_conflictos(self):
        tabla = verificar(self.reglas)
        self.assertTrue(tabla.es_ll1)
        self.assertEqual(len(tabla.m), 341)
        self.assertEqual(sum(map(len, self.reglas.values())), 103)

    def test_extraccion_excluye_recuperacion_y_acciones_internas(self):
        self.assertNotIn('$accept', self.reglas)
        self.assertTrue(all(not cabeza.startswith('$') for cabeza in self.reglas))
        simbolos = {s for cuerpos in self.reglas.values() for c in cuerpos for s in c}
        self.assertNotIn('error', simbolos)
        self.assertNotIn('$@1', simbolos)
        self.assertNotIn('TOKEN_CURSOR', simbolos)
        self.assertIn('id', simbolos)
        self.assertIn('num', simbolos)
        self.assertIn('|', simbolos)

    def test_cambio_de_regla_factorizada_exige_revision(self):
        reglas = copy.deepcopy(self.reglas)
        reglas['dims'].append(('[', 'num', ']', '[', 'num', ']', '[', 'num', ']'))
        with self.assertRaisesRegex(ValueError, 'Cambió la regla dims'):
            verificar(reglas)

    def test_cambio_fuera_de_factorizaciones_tambien_se_detecta(self):
        reglas = copy.deepcopy(self.reglas)
        reglas['retorno'] = [('void',)]
        with self.assertRaisesRegex(ValueError, 'difiere en retorno'):
            verificar(reglas)

    def test_alfabeto_flex_y_nuevo_terminal_no_pasan_inadvertidos(self):
        lexer = (ROOT / 'grammar/lexer.l').read_text()
        self.assertEqual(len(verificar_terminales(lexer)), 49)
        with self.assertRaisesRegex(ValueError, 'terminales Flex y LL'):
            verificar_terminales(lexer + '\n"P" LEX_RETURN(TOKEN_P, "reservada");\n')

    def test_verificar_no_modifica_la_gramatica_extraida(self):
        antes = copy.deepcopy(self.reglas)
        verificar(self.reglas)
        self.assertEqual(self.reglas, antes)


if __name__ == '__main__':
    unittest.main()
