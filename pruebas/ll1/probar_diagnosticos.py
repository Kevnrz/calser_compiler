"""Diagnósticos del editor con LL(1) y semántica existente de ccom.

Comprueba posiciones, protocolos, límites y conservación de los archivos.
El auxiliar intercepta yyparse para demostrar que los errores sintácticos
se resuelven sin Bison. Los documentos válidos conservan la semántica clásica.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from ide import ccom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', required=True, type=Path)
    binary = parser.parse_args().binario.resolve()
    count = 0
    with tempfile.TemporaryDirectory(prefix='calser_check_ll1_') as temporary:
        folder = Path(temporary)
        source = folder / 'programa ñ 😀.cls'
        artifacts = [source.with_suffix(s) for s in ('.bin', '.mem', '.hex', '.asm', '.json')]
        for path in artifacts:
            path.write_bytes(b'preservar')

        def run(*args):
            return subprocess.run([str(ROOT / 'ccom'), *map(str, args)], cwd=folder,
                                  capture_output=True, timeout=10)

        def check(text, category=None, without_bison=False):
            nonlocal count
            source.write_bytes(text.encode())
            original = source.read_bytes()
            result = run('--check-ll1-json', source)
            data = json.loads(result.stdout)
            problems = ccom.diagnosticos_json(result.stdout, source, text, result.returncode,
                                              operacion='check_ll1')
            assert result.returncode == (1 if category else 0), result.stderr
            assert len(problems) == len(data['diagnostics'])
            if category:
                assert data['diagnostics'] and all(d['category'] == category for d in data['diagnostics'])
            else:
                assert not problems
            if without_bison:
                native = subprocess.run([str(binary), '--check-ll1', str(source)],
                                        capture_output=True, timeout=10)
                assert native.returncode == result.returncode, native.stderr
                assert json.loads(native.stdout) == data
            assert source.read_bytes() == original
            assert all(p.read_bytes() == b'preservar' for p in artifacts)
            count += 1
            return data

        valid = 'F main[]:void; Vm x:int=3; E;.\n'
        check(valid)
        check('F main[]:void; total=desconocida+1; E;.', 'semantic')
        missing = check('F suma[int a,int b]:int c; c=a+b; E;.', 'semantic')
        assert missing['diagnostics'][0]['range'] is None
        for text in ('F main[]:void; Vm x:int=; E;.',
                     '# café 😀\r\nF main[]:void;\r\n\tvs\r\nE;\r\n.',
                     'F main[]:void; E;', 'F main[]:void; E;\n',
                     'F main[]:void; E;\r\n', ''):
            data = check(text, 'syntax', True)
            assert len(data['diagnostics']) == 1
            assert data['diagnostics'][0]['range'] is not None
            assert 'Se esperaba' in data['diagnostics'][0]['message']
            if text.endswith(('E;', '\n')) or not text:
                position = data['diagnostics'][0]['range']
                assert position['start'] == position['end'] == len(text.encode())
        # La capa gráfica ya evita consultar documentos sin tokens de código.
        check('# comentario de prueba', 'syntax', True)
        for text in ('@\n' + valid, '#[ bloque sin cerrar', '😀\n' + valid):
            check(text, 'lexical', True)
        text = 'F main[]:void; Vm x:int=; Vm y:int=; E;.'
        check(text, 'syntax', True)
        source.write_text(text)
        classic = json.loads(run('--check-json', source).stdout)
        assert classic['operation'] == 'check' and len(classic['diagnostics']) > 1
        check(valid)
        for text in (' ' * 200001, 'F main[]:void; Vm x:int=' + '(' * 5000):
            source.write_text(text)
            result = run('--check-ll1-json', source)
            data = json.loads(result.stdout)
            assert result.returncode == 1 and data['status'] == 'limit' and data['truncated']
        for flags in (['--check-json'], ['--check'], ['--tokens-json'], ['--syntax-json'],
                      ['--repair-json'], ['--suggest-json', '0'], ['-s'], ['-O1'],
                      ['--ast-json', 'x.json'], ['-o', 'x.bin']):
            assert run('--check-ll1-json', *flags, source).returncode == 2
        assert json.loads(run('--check-ll1-json', folder / 'ausente.cls').stdout)['status'] == 'error'
        source.write_text(valid)
        timed = run('--check-ll1-json', '-v', '--time', source)
        assert timed.returncode == 0 and json.loads(timed.stdout)['status'] == 'ok'
    print(f'PASS {count} documentos del editor con LL(1), semántica C, rangos EOF y CLI aislada')


if __name__ == '__main__':
    main()
