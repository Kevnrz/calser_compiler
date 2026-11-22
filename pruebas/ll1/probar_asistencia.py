"""Comprueba los servicios productivos con yyparse interceptado.

Python organiza las consultas y comprueba el contrato. La gramática y las
propuestas se calculan en C. Cualquier uso de Bison aborta el auxiliar nativo.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', required=True, type=Path)
    binary = parser.parse_args().binario.resolve()
    count = 0
    with tempfile.TemporaryDirectory(prefix='calser_asistencia_ll1_') as temporary:
        source = Path(temporary) / 'fuente ñ 😀.cls'
        artifacts = [source.with_suffix(s) for s in ('.asm', '.mem', '.bin', '.json')]
        for artifact in artifacts:
            artifact.write_bytes(b'conservar')

        def query(mode, text, cursor=None):
            nonlocal count
            raw = text.encode()
            source.write_bytes(raw)
            args = [] if cursor is None else [str(len(text[:cursor].encode()))]
            native = subprocess.run([str(binary), '--' + mode, *args, str(source)],
                                    capture_output=True, timeout=10)
            classic = subprocess.run([str(ROOT / 'ccom'), '--' + mode + '-json',
                                      *args, str(source)], capture_output=True, timeout=10)
            assert native.returncode in (0, 1), native.stderr
            assert native.returncode == classic.returncode
            result = json.loads(native.stdout)
            assert result == json.loads(classic.stdout)
            assert result['exit_code'] == native.returncode
            assert source.read_bytes() == raw
            assert all(a.read_bytes() == b'conservar' for a in artifacts)
            count += 1
            return result

        def suggestions(text, cursor=None):
            return query('suggest', text, len(text) if cursor is None else cursor)

        def preferred(text):
            return [s['text'] for s in suggestions(text)['suggestions'] if s['inline_preferred']]

        for name, text, relative in [('main', '[]: ', 4), ('suma', '[]', 1)]:
            option = suggestions('F ' + name)['suggestions'][0]
            assert option['text'] == text and option['cursor_relative'] == relative
        assert preferred('F main[]: ') == ['void']
        assert set(preferred('F suma[]: ')) == {'bool', 'int', 'void'}
        assert set(preferred('F main[]:void; Vm x: ')) == {'bool', 'int'}
        text = '# ñ 😀\r\nF main[]:void; E; .'
        result = suggestions(text, text.index('void') + 2)
        option = result['suggestions'][0]
        assert option['text'] == 'void'
        assert text.encode()[option['start']:option['end']] == b'void'
        assert '.' not in preferred('F main[]:void;')
        assert '.' not in preferred('F main[]:void; E;')
        assert preferred('F main[]:void; E;\n') == ['.']
        assert preferred('F main[]:void; E;\n.\n') == []
        assert preferred('F main[]:void;\n') == []
        assert suggestions('F main[]:void; C')['suggestions'][0]['text'] == '[]'
        for text in ('#[ abierto', '# comentario', '@ F main', 'F main[]:void; x=; '):
            assert suggestions(text)['status'] == 'bloqueado'
        text = 'F main[]:vo @'
        assert suggestions(text, text.index('vo') + 2)['suggestions'][0]['text'] == 'void'
        assert suggestions('F ')['status'] == 'sin_opciones'

        for text, code in [
            ('F main[]:void; Vm x:int\nE;.', 'INSERCION_PUNTO_COMA'),
            ('F main[]:void; E;', 'INSERCION_PUNTO_FINAL'),
            ('F main[]:void; C[verdadero; E;.', 'INSERCION_CORCHETE_CIERRE'),
            ('F main[]:void; Vm x:int=(3+2; E;.', 'INSERCION_PARENTESIS_CIERRE'),
        ]:
            repairs = query('repair', text)['repairs']
            proposal = next(r for r in repairs if r['code'] == code)
            edit = proposal['edits'][0]
            raw = text.encode()
            amended = (raw[:edit['start']] + edit['text'].encode() + raw[edit['end']:]).decode()
            checked = query('syntax', amended)
            assert proposal['validation']['accepted'] == (checked['status'] == 'aceptado')
            assert proposal['validation']['resolved']
            assert proposal['automatic_safe'] == (code == 'INSERCION_PUNTO_COMA')
        assert query('repair', 'F main[]:void; x=3+; E;.')['status'] == 'sin_opciones'
        assert query('repair', 'F main[]:void; @ E;.')['status'] == 'bloqueado'
        partial = query('repair', 'F main[]:void; Vm x:int=3')['repairs'][0]
        assert not partial['automatic_safe'] and not partial['validation']['accepted']
        assert query('syntax', 'F main[]:void; E;.')['status'] == 'aceptado'
    print(f'PASS {count} consultas de asistencia LL(1) sin Bison y con JSON productivo idéntico')


if __name__ == '__main__':
    main()
