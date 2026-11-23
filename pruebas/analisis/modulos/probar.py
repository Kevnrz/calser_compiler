#!/usr/bin/env python3
"""Importaciones mínimas, diseño §12: compilación, ejecución y diagnósticos.

Los fuentes se crean en temporales para no incorporarlos a los corpus que
recorren todas las fuentes .cls como programas independientes.
"""
from pathlib import Path
import json
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from pruebas.generador_o1.tac_emit.comparar import execute, observable

A_GLOBAL = 'Vm base_a : int = 3 ;\n'
B_GLOBAL = 'Vm base_b : int = 4 ;\n'
A_FUNC = 'F sumar [ int x ] : int r ; r = x + base_a ; E ; .\n'
B_FUNC = 'F doblar [ int x ] : int r ; r = x * base_b ; E ; .\n'
MAIN_GLOBAL = 'Vm resultado : int = 0 ;\n'
MAIN_FUNC = 'F main [] : void ; resultado = sumar [ base_b ] + doblar [ base_a ] ; E ; .\n'
MAIN = MAIN_GLOBAL + MAIN_FUNC


class Modulos(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='calser_modulos_')
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.main = self.write('main.cls', 'P "a" ;\nP "b" ;\n' + MAIN)
        self.write('a.cls', A_GLOBAL + A_FUNC)
        self.write('b.cls', B_GLOBAL + B_FUNC)

    def write(self, name, text):
        path = self.folder / name
        path.write_text(text)
        return path

    def run_ccom(self, *args, source=None, code=0):
        result = subprocess.run([str(ROOT / 'ccom'), *map(str, args),
                                 str(source or self.main)], capture_output=True,
                                text=True, timeout=30)
        self.assertEqual(result.returncode, code, result.stderr + result.stdout)
        return result

    def test_dos_modulos_equivalentes_y_repetido(self):
        inline = self.write('inline.cls', A_GLOBAL + B_GLOBAL + MAIN_GLOBAL + A_FUNC + B_FUNC + MAIN_FUNC)
        for level in ('-O0', '-O1', '-O2'):
            for repeated in (False, True):
                self.main.write_text('P "a" ;\nP "b" ;\n' + ('P "a" ;\n' if repeated else '') + MAIN)
                left, right = self.folder / 'modulos', self.folder / 'inline_out'
                self.run_ccom(level, '-s', '-o', left.with_suffix('.bin'))
                self.run_ccom(level, '-s', '-o', right.with_suffix('.bin'), source=inline)
                # Mismo orden de declaraciones: también coincide el código.
                self.assertEqual(left.with_suffix('.bin').read_bytes(), right.with_suffix('.bin').read_bytes())
                for data in (bytes(1024), bytes(range(256)) * 4):
                    a, b = execute(left, data), execute(right, data)
                    self.assertEqual(observable(a, [(0, 0x1000)]), observable(b, [(0, 0x1000)]))
                    # La salida de main debe almacenar (4+3)+(3*4) = 19.
                    match = re.search(r'resultado.*?addr=0x([0-9a-fA-F]+)', left.with_suffix('.asm').read_text())
                    self.assertIsNotNone(match)
                    address = int(match[1], 16)
                    self.assertEqual(int.from_bytes(a.memory[address:address + 4], 'little', signed=True), 19)

    def test_no_encontrado_y_p_tarde(self):
        self.main.write_text('# principal\nP "nada" ;\n' + MAIN)
        result = self.run_ccom(code=1)
        self.assertIn(f'{self.main}:2:1:', result.stderr)
        self.assertIn(f"modulo no encontrado: 'nada' (se busco {self.folder}/nada.cls)", result.stderr)
        self.main.write_text(MAIN + 'P "a" ;\n')
        self.assertIn("'P' solo puede ir al inicio del archivo", self.run_ccom(code=1).stderr)

    def test_module_dir_y_consultas(self):
        elsewhere = self.folder / 'snapshot'
        elsewhere.mkdir()
        snapshot = elsewhere / 'documento.cls'
        snapshot.write_text(self.main.read_text())
        for mode in ('--check', '--check-json', '--check-ll1-json'):
            result = self.run_ccom(mode, '--module-dir', self.folder, source=snapshot)
            if mode != '--check':
                self.assertEqual(json.loads(result.stdout)['status'], 'ok')
        self.run_ccom('--module-dir', self.folder, '-o', self.folder / 'out.bin', source=snapshot)
        for query in (['--tokens-json'], ['--syntax-json'], ['--suggest-json', '0'],
                      ['--repair-json'], ['--clones-json', '5']):
            self.run_ccom(*query, '--module-dir', self.folder, source=snapshot, code=2)
        self.run_ccom('--module-dir', code=2)
        self.main.write_text('P "a" ;\n' + MAIN)
        tokens = json.loads(self.run_ccom('--tokens-json').stdout)['tokens']
        self.assertEqual([t['kind'] for t in tokens[:2]], ['reservada', 'cadena'])

    def test_errores_del_modulo_y_duplicados(self):
        for text, message, line in [
            ('# modulo\nP "b" ;\n' + A_GLOBAL + A_FUNC,
             'un modulo no puede importar otros modulos', 2),
            (A_GLOBAL + A_FUNC + 'P "b" ;\n',
             'un modulo no puede importar otros modulos', 3),
            ('# modulo\nP "sin cerrar\n' + A_FUNC, 'cadena sin cerrar', 2),
            ('# modulo\n@\n' + A_FUNC, 'caracter no reconocido', 2),
            ('# modulo\nF sumar [] : void ;\nVm x : int = ;\n.\n', 'error sintactico', 3),
        ]:
            with self.subTest(message=message, line=line):
                self.write('a.cls', text)
                result = self.run_ccom('--check', code=1)
                self.assertIn(message, result.stderr)
                self.assertIn(f'{self.folder / "a.cls"}:{line}:', result.stderr)
        self.write('a.cls', A_GLOBAL + A_FUNC)
        self.main.write_text('P "a" ;\n' + A_GLOBAL + MAIN)
        self.assertIn('ya fue declarado', self.run_ccom('--check', code=1).stderr)

    # §12: líneas distintas y UTF-8 antes del P permiten detectar rangos del
    # módulo filtrados por error al documento principal del protocolo v1.
    def semantic_inputs(self, mixed=False):
        module = '# modulo\nF sumar [] : void ;\n    desconocida = 1 ;\n    E ;\n.\n'
        main = '# principal á\n\n  P "a" ;\nF main [] : void ;\n'
        if mixed:
            main += '    ausente = 2 ;\n'
        main += '    E ;\n.\n'
        self.write('a.cls', module)
        self.main.write_text(main)
        return main, module

    def test_error_semantico_texto_con_fuente_del_modulo(self):
        _, module = self.semantic_inputs()
        for options in (('--check',), (), ('-O0',), ('-O1',), ('-O2',)):
            with self.subTest(options=options):
                result = self.run_ccom(*options, code=1)
                self.assertIn(f"{self.folder / 'a.cls'}:3:5: error semantico: 'desconocida' no ha sido declarado", result.stderr)
                self.assertIn('     3 | ' + module.splitlines()[2], result.stderr)
                self.assertNotIn(str(self.main) + ':', result.stderr)
                self.assertIn('Analisis semantico fallido: 1 error(es).', result.stderr)

    def test_error_semantico_json_en_el_p(self):
        main, _ = self.semantic_inputs()
        snapshot = self.folder / 'snapshot' / 'documento.cls'
        snapshot.parent.mkdir()
        snapshot.write_text(main)
        start = main.encode().index(b'P "a"')
        for mode in ('--check-json', '--check-ll1-json'):
            with self.subTest(mode=mode):
                result = self.run_ccom(mode, '--module-dir', self.folder, source=snapshot, code=1)
                data = json.loads(result.stdout)
                self.assertEqual(data['source'], str(snapshot))
                self.assertEqual(data['status'], 'errors')
                self.assertEqual(data['error_count'], 1)
                self.assertEqual(len(data['diagnostics']), 1)
                error = data['diagnostics'][0]
                self.assertEqual(error['code'], 'CCOM_SEMANTIC')
                self.assertEqual(error['category'], 'semantic')
                self.assertEqual(error['severity'], 'error')
                self.assertEqual(error['message'], "error en el modulo 'a': 'desconocida' no ha sido declarado (a.cls:3:5)")
                self.assertEqual(error['range'], {'start': start, 'end': start + len(b'P "a" ;'), 'line': 3, 'column': 3})
                self.assertNotIn('error semantico:', result.stderr)

    def test_error_semantico_principal_y_modulo_conservan_contador(self):
        main, _ = self.semantic_inputs(mixed=True)
        result = self.run_ccom('--check', code=1)
        self.assertIn(str(self.folder / 'a.cls') + ':3:5:', result.stderr)
        self.assertIn(str(self.main) + ':5:5:', result.stderr)
        self.assertIn('     3 |     desconocida = 1 ;', result.stderr)
        self.assertIn('     5 |     ausente = 2 ;', result.stderr)
        self.assertEqual(result.stderr.count('error semantico:'), 2)
        self.assertIn('Analisis semantico fallido: 2 error(es).', result.stderr)
        for mode in ('--check-json', '--check-ll1-json'):
            result = self.run_ccom(mode, '--module-dir', self.folder, code=1)
            data = json.loads(result.stdout)
            self.assertEqual(data['error_count'], 2)
            self.assertEqual(len(data['diagnostics']), 2)
            imported, own = data['diagnostics']
            start = main.encode().index(b'P "a"')
            self.assertEqual(imported['range'], {'start': start, 'end': start + len(b'P "a" ;'), 'line': 3, 'column': 3})
            self.assertIn("error en el modulo 'a'", imported['message'])
            self.assertEqual(own['range']['line'], 5)
            self.assertEqual(own['message'], "'ausente' no ha sido declarado")

    def test_errores_en_dos_modulos_y_p_repetido(self):
        # Cada nodo conserva su índice tras la unión; un P repetido mantiene
        # como ancla la primera importación, sin cargar otro archivo.
        self.write('a.cls', 'F sumar [] : void ; falta_a = 1 ; E ; .\n')
        self.write('b.cls', 'F doblar [] : void ; falta_b = 2 ; E ; .\n')
        self.main.write_text('P "a" ;\nP "b" ;\nP "a" ;\nF main [] : void ; E ; .\n')
        result = self.run_ccom('--check', code=1)
        self.assertIn(str(self.folder / 'a.cls') + ':1:21:', result.stderr)
        self.assertIn(str(self.folder / 'b.cls') + ':1:22:', result.stderr)
        self.assertIn('Analisis semantico fallido: 2 error(es).', result.stderr)
        for mode in ('--check-json', '--check-ll1-json'):
            data = json.loads(self.run_ccom(mode, '--module-dir', self.folder, code=1).stdout)
            self.assertEqual(data['error_count'], 2)
            self.assertEqual([d['range'] for d in data['diagnostics']], [
                {'start': 0, 'end': 7, 'line': 1, 'column': 1},
                {'start': 8, 'end': 15, 'line': 2, 'column': 1},
            ])
            self.assertIn("error en el modulo 'a'", data['diagnostics'][0]['message'])
            self.assertIn("error en el modulo 'b'", data['diagnostics'][1]['message'])

    def snapshot_import(self, text=None):
        main = text or '# principal á\n\n  P "a" ; # despues\nF main [] : void ; E ; .\n'
        snapshot = self.folder / 'snapshot' / 'documento.cls'
        snapshot.parent.mkdir(exist_ok=True)
        snapshot.write_bytes(main.encode())
        start = main.encode().index(b'P ')
        end = main.encode().index(b'; # despues', start) + 1
        return snapshot, main, {'start': start, 'end': end, 'line': 3, 'column': 3}

    def test_errores_lexicos_y_sintacticos_json_en_directiva(self):
        cases = [
            ('# modulo\nP "b" ;\n' + A_GLOBAL + A_FUNC,
             'un modulo no puede importar otros modulos', 2, 1, 'syntax', 'CCOM_SYNTAX'),
            ('# modulo\n@\n' + A_FUNC,
             "caracter no reconocido: '@'", 2, 1, 'lexical', 'CCOM_LEXICAL'),
            ('# modulo\nF sumar [] : void ;\nVm x : int = ;\n.\n',
             "token inesperado ';'", 3, 14, 'syntax', 'CCOM_SYNTAX'),
        ]
        snapshot, main, expected_range = self.snapshot_import()
        for text, message, line, column, category, code in cases:
            self.write('a.cls', text)
            for mode in ('--check-json', '--check-ll1-json'):
                with self.subTest(message=message, mode=mode):
                    result = self.run_ccom(mode, '--module-dir', self.folder, source=snapshot, code=1)
                    data = json.loads(result.stdout)
                    self.assertEqual(data['source'], str(snapshot))
                    self.assertEqual(data['status'], 'errors')
                    self.assertEqual(data['error_count'], 1)
                    self.assertEqual(len(data['diagnostics']), 1)
                    error = data['diagnostics'][0]
                    self.assertEqual(error['message'], f"error en el modulo 'a': {message} (a.cls:{line}:{column})")
                    self.assertEqual(error['range'], expected_range)
                    self.assertEqual(main.encode()[error['range']['start']:error['range']['end']], b'P "a" ;')
                    self.assertEqual(error['category'], category)
                    self.assertEqual(error['code'], code)
                    self.assertEqual(error['severity'], 'error')
                    self.assertNotIn('error lexico:', result.stderr)
                    self.assertNotIn('error sintactico:', result.stderr)

    def test_cascadas_existentes_del_modulo_json(self):
        # §12: estas mismas entradas textuales ya producen dos diagnósticos.
        # Se conserva su recuperación y cada diagnóstico usa la directiva del principal.
        cases = [
            ('# modulo\nP "sin cerrar\n' + A_FUNC, [
                ('lexical', 'CCOM_LEXICAL', 'cadena sin cerrar', 2, 3),
                ('syntax', 'CCOM_SYNTAX', "token inesperado 'F'; se esperaba 'cadena'", 3, 1),
            ]),
            (A_GLOBAL + A_FUNC + 'P "b" ;\n', [
                ('syntax', 'CCOM_SYNTAX', 'un modulo no puede importar otros modulos', 3, 1),
                ('syntax', 'CCOM_SYNTAX', 'fin de archivo inesperado', 3, 1),
            ]),
        ]
        snapshot, main, expected_range = self.snapshot_import()
        for text, expected in cases:
            self.write('a.cls', text)
            # Cantidad idéntica a texto, según la aclaración de §12: sin filtros.
            textual = self.run_ccom('--check', '--module-dir', self.folder, source=snapshot, code=1)
            text_count = textual.stderr.count('error lexico:') + textual.stderr.count('error sintactico:')
            self.assertEqual(text_count, len(expected))
            for mode in ('--check-json', '--check-ll1-json'):
                with self.subTest(text=text, mode=mode):
                    data = json.loads(self.run_ccom(mode, '--module-dir', self.folder, source=snapshot, code=1).stdout)
                    self.assertEqual(data['error_count'], text_count)
                    self.assertEqual(len(data['diagnostics']), len(expected))
                    for error, (category, code, message, line, column) in zip(data['diagnostics'], expected):
                        self.assertEqual(error['range'], expected_range)
                        self.assertEqual(error['message'], f"error en el modulo 'a': {message} (a.cls:{line}:{column})")
                        self.assertEqual(error['category'], category)
                        self.assertEqual(error['code'], code)
                        self.assertEqual(error['severity'], 'error')


    def test_modulo_no_encontrado_json_directiva_completa(self):
        snapshot, main, expected_range = self.snapshot_import('# principal á\n\n  P "nada"  ; # despues\nF main [] : void ; E ; .\n')
        for mode in ('--check-json', '--check-ll1-json'):
            data = json.loads(self.run_ccom(mode, '--module-dir', self.folder, source=snapshot, code=1).stdout)
            self.assertEqual(data['error_count'], 1)
            self.assertEqual(len(data['diagnostics']), 1)
            error = data['diagnostics'][0]
            self.assertEqual(error['range'], expected_range)
            self.assertEqual(main.encode()[expected_range['start']:expected_range['end']], b'P "nada"  ;')
            self.assertEqual(error['category'], 'compilation')
            self.assertEqual(error['code'], 'CCOM_COMPILATION')
            self.assertEqual(error['message'], f"modulo no encontrado: 'nada' (se busco {self.folder}/nada.cls)")

    def test_directiva_multilinea_y_crlf(self):
        # §12: el fin viene del token ;, no de buscar texto o limitarlo a la línea P.
        main = '# principal á\r\n\r\n  P # comentario\r\n "a"\r\n ; # despues\r\nF main [] : void ; E ; .\r\n'
        self.write('a.cls', 'F sumar [] : void ; desconocida = 1 ; E ; .\n')
        snapshot, _, expected_range = self.snapshot_import(main)
        for mode in ('--check-json', '--check-ll1-json'):
            data = json.loads(self.run_ccom(mode, '--module-dir', self.folder, source=snapshot, code=1).stdout)
            self.assertEqual(data['error_count'], 1)
            self.assertEqual(data['diagnostics'][0]['range'], expected_range)
            self.assertEqual(main.encode()[expected_range['start']:expected_range['end']], b'P # comentario\r\n "a"\r\n ;')

    def test_error_del_principal_no_se_contextualiza(self):
        self.main.write_text('P "a" ;\nF main [] : void ; propia = 1 ; E ; .\n')
        for mode in ('--check-json', '--check-ll1-json'):
            data = json.loads(self.run_ccom(mode, '--module-dir', self.folder, code=1).stdout)
            self.assertEqual(data['error_count'], 1)
            error = data['diagnostics'][0]
            self.assertEqual(error['message'], "'propia' no ha sido declarado")
            self.assertEqual(error['range']['line'], 2)
            self.assertEqual(error['range']['column'], 20)


if __name__ == '__main__':
    unittest.main()
