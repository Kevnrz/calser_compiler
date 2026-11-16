"""Contrato nativo --check-json: datos del front-end, CLI y archivos intactos."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
VALIDO = 'F main [] : void ;\n    E ;\n.\n'


def main():
    with tempfile.TemporaryDirectory(prefix='calser_json_') as tmp:
        folder = Path(tmp)
        source = folder / 'fuente "café 😀" con espacios.cls'
        artifacts = {}
        for extension in ('.bin', '.hex', '.mem', '.asm', '.json'):
            p = source.with_suffix(extension); p.write_bytes(b'conservar')
            artifacts[p] = p.read_bytes()

        def run(*args):
            return subprocess.run([str(ROOT / 'ccom'), *map(str, args)], cwd=folder,
                                  capture_output=True, text=True, encoding='utf-8', timeout=10)

        def check(texto, code, categoria=None, cantidad=0):
            source.write_text(texto, encoding='utf-8', newline='')
            original = source.read_bytes()
            result = run('--check-json', source)
            assert result.returncode == code, result.stderr
            data = json.loads(result.stdout)
            assert data['schema_version'] == 1 and data['operation'] == 'check'
            assert data['source'] == str(source) and data['offset_unit'] == 'utf8'
            assert data['exit_code'] == code and not data['truncated']
            assert data['status'] == ('errors' if code else 'ok')
            assert data['error_count'] == len(data['diagnostics']) == cantidad
            for d in data['diagnostics']:
                assert d['category'] == categoria and d['severity'] == 'error'
                assert d['message'] and d['code'].startswith('CCOM_')
                if d['range'] is not None:
                    r = d['range']; a, b = r['start'], r['end']
                    assert 0 <= a <= b <= len(original)
                    assert r['line'] == original.count(b'\n', 0, a) + 1
                    assert r['column'] == a - original.rfind(b'\n', 0, a)
            assert source.read_bytes() == original
            assert set(folder.iterdir()) == set(artifacts) | {source}
            assert all(p.read_bytes() == old for p, old in artifacts.items())
            return data

        check(VALIDO, 0)
        check('F main [] : void ;\n a = 1 ;\n b = 2 ;\n E ;\n.\n', 1, 'semantic', 2)
        check('@ @\n' + VALIDO, 1, 'lexical', 2)
        # M7: ambos errores independientes se obtienen del parser C en una consulta.
        check('F main [] : void ;\n Vm a : int = ;\n Vm b : int = ;\n E ;\n.\n', 1, 'syntax', 2)
        general = check('F suma [] : void ; E ; .\n', 1, 'semantic', 1)
        assert general['diagnostics'][0]['range'] is None
        texto = '# café 😀\r\nF main [] : void ;\r\n\tno_declarada = 1 ;\r\n E ;\r\n.\r\n'
        unicode = check(texto, 1, 'semantic', 1)
        r = unicode['diagnostics'][0]['range']
        assert texto.encode()[r['start']:r['end']] == b'no_declarada'
        for caracter in ('á', '😀'):
            d = check(caracter + '\n' + VALIDO, 1, 'lexical', len(caracter.encode()))
            assert [x['range']['start'] for x in d['diagnostics']] == list(range(len(caracter.encode())))
        punto = check('F main [] : void ;\n Vm a : int\n E ;\n.\n', 1, 'syntax', 1)
        assert punto['diagnostics'][0]['range']['start'] == punto['diagnostics'][0]['range']['end']

        source.write_text('@ ' * 200 + '\n' + VALIDO)
        limite = run('--check-json', source)
        data = json.loads(limite.stdout)
        assert limite.returncode == 1 and data['status'] == 'limit' and data['truncated']
        assert len(data['diagnostics']) == 128 and data['error_count'] == 200
        assert data['message'] and len(limite.stdout.encode()) < 1_000_000

        ausente = run('--check-json', folder / 'inexistente.cls')
        data = json.loads(ausente.stdout)
        assert ausente.returncode == 1 and data['status'] == 'error' and data['message']
        assert data['diagnostics'] == []

        source.write_text(VALIDO)
        for flags in (['-s'], ['-x'], ['-p'], ['-m'], ['-t'], ['--no-schedule'],
                      ['-o', source], ['--ast-json', source]):
            bad = run('--check-json', *flags, source)
            assert bad.returncode == 2 and not bad.stdout
            assert source.read_text() == VALIDO
        combinado = run(source, '--check-json', '--check', '-v', '--time')
        assert combinado.returncode == 2 and not combinado.stdout
        combinado = run(source, '--check-json', '-v', '--time')
        assert combinado.returncode == 0 and json.loads(combinado.stdout)['status'] == 'ok'
        assert 'analisis semantico ... ok' in combinado.stderr and 'tiempo de analisis' in combinado.stderr
        assert run('--check-json').returncode == 2
        assert run('--check-json', '--help').returncode == 0
        assert '--check-json' in run('--help').stdout
        assert all(p.read_bytes() == old for p, old in artifacts.items())
    print('PASS --check-json: protocolo, multiples diagnosticos, Unicode, limites, CLI y cero artefactos')


if __name__ == '__main__':
    main()
