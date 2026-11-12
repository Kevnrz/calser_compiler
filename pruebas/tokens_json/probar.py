"""Lexer Flex como servicio: rangos, comentarios, recuperación y CLI clásica."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix='calser_tokens_') as tmp:
        carpeta = Path(tmp)
        fuente = carpeta / 'fuente con café 😀.cls'
        for sufijo in ('.bin', '.hex', '.mem', '.asm', '.json'):
            fuente.with_suffix(sufijo).write_bytes(b'preservar')
        def run(*args):
            return subprocess.run([str(ROOT / 'ccom'), *map(str, args)], capture_output=True, cwd=carpeta, timeout=10)
        def tokens(texto, codigo=0):
            fuente.write_text(texto, encoding='utf-8', newline='')
            resultado = run('--tokens-json', fuente)
            assert resultado.returncode == codigo, resultado.stderr
            informe = json.loads(resultado.stdout)
            assert informe['operation'] == 'lex' and informe['offset_unit'] == 'utf8'
            assert informe['source'] == str(fuente) and informe['exit_code'] == codigo
            assert not informe['truncated'] and informe['status'] == ('errors' if codigo else 'ok')
            raw = texto.encode()
            previo = 0
            for token in informe['tokens']:
                assert previo <= token['start'] < token['end'] <= len(raw)
                raw[token['start']:token['end']].decode('utf-8')
                previo = token['end']
            assert fuente.read_bytes() == raw
            assert all(fuente.with_suffix(s).read_bytes() == b'preservar' for s in ('.bin', '.hex', '.mem', '.asm', '.json'))
            return informe
        texto = '# café 😀\r\n#[ bloque\n😀 ]#\tVm x : int = 42 ; verdadero cifrar entero Vmx != [] ()'
        d = tokens(texto)
        lexemas = [(texto.encode()[t['start']:t['end']].decode(), t['kind']) for t in d['tokens']]
        assert ('42', 'numero') in lexemas and ('int', 'tipo') in lexemas
        assert ('verdadero', 'booleano') in lexemas and ('cifrar', 'reservada') in lexemas
        assert ('entero', 'identificador') in lexemas and ('Vm', 'reservada') in lexemas
        assert len([t for t in d['tokens'] if t['kind'] == 'comentario']) == 2
        # No parser: incluso un programa incompleto/sin main tiene tokens hasta EOF.
        tokens('F main [ int int ; Vm despues : bool = falso ;')
        d = tokens('@ 😀 2147483649 ' + 'a' * 33 + ' ; int', 1)
        assert len(d['diagnostics']) == 7 and d['tokens'][-1]['kind'] == 'tipo'
        d = tokens('#[ sin cerrar\n😀', 1)
        assert d['fatal'] and d['tokens'] == [{'kind': 'comentario', 'start': 0, 'end': len('#[ sin cerrar\n😀'.encode())}]
        assert tokens('')['tokens'] == []
        fuente.write_text('int ' * 100001)
        limitado = run('--tokens-json', fuente)
        d = json.loads(limitado.stdout)
        assert limitado.returncode == 1 and d['status'] == 'limit' and d['truncated']
        assert len(d['tokens']) == 100000
        fuente.write_text('@' * 200)
        d = json.loads(run('--tokens-json', fuente).stdout)
        assert d['status'] == 'limit' and len(d['diagnostics']) == 128
        for flags in (['-s'], ['-x'], ['-p'], ['-t'], ['-m'], ['--no-schedule'],
                      ['--ast-json', 'salida.json'], ['-o', 'salida.bin'], ['--check-json'], ['--check']):
            assert run('--tokens-json', *flags, fuente).returncode == 2
        with fuente.open('wb') as demasiado:
            demasiado.truncate(8000001)
        grande = run('--tokens-json', fuente)
        assert grande.returncode == 1 and json.loads(grande.stdout)['status'] == 'error'
        ausente = run('--tokens-json', carpeta / 'ausente.cls')
        assert ausente.returncode == 1 and json.loads(ausente.stdout)['status'] == 'error'
        fuente.write_text('# comentario\nF main [] : void ; E ; .')
        progreso = run('--tokens-json', fuente, '-v', '--time')
        assert progreso.returncode == 0 and b'fase 1: analisis lexico ... ok' in progreso.stderr
        assert run('--check-json', fuente).returncode == 0
        assert run(fuente, '-s', '-o', carpeta / 'programa.bin').returncode == 0
    print('PASS --tokens-json: Flex, comentarios, UTF-8, EOF, limites, CLI y salidas intactas')


if __name__ == '__main__':
    main()
