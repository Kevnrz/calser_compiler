"""M3: casos migrados de la entrega y consultas reales Bison/LAC por cursor."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def main():
    with tempfile.TemporaryDirectory(prefix='calser_suggest_') as tmp:
        carpeta = Path(tmp)
        fuente = carpeta / 'fuente café 😀 con espacios.cls'
        artefactos = [fuente.with_suffix(s) for s in ('.bin', '.hex', '.mem', '.asm', '.json')]
        for p in artefactos: p.write_bytes(b'preservar')
        def run(*args):
            return subprocess.run([str(ROOT / 'ccom'), *map(str, args)], capture_output=True, cwd=carpeta, timeout=5)
        def consultar(texto, cursor=None):
            fuente.write_text(texto, encoding='utf-8', newline='')
            cursor = len(texto) if cursor is None else cursor
            pos = len(texto[:cursor].encode())
            salida = run('--suggest-json', pos, fuente)
            assert salida.returncode in (0, 1), salida.stderr
            datos = json.loads(salida.stdout)
            assert datos['schema_version'] == 1 and datos['operation'] == 'suggest' and datos['offset_unit'] == 'utf8'
            assert datos['source'] == str(fuente) and datos['cursor'] == pos
            assert datos['exit_code'] == salida.returncode
            assert bool(datos['suggestions']) == (datos['status'] == 'ok')
            assert len(datos['suggestions']) <= 64
            assert fuente.read_text() == texto.replace('\r\n', '\n')
            assert all(p.read_bytes() == b'preservar' for p in artefactos)
            for s in datos['suggestions']:
                a,b = s['start'],s['end']
                assert 0 <= a <= pos <= b <= len(texto.encode())
                texto.encode()[:a].decode(); texto.encode()[:b].decode()
                assert s['text'] not in ('id', 'num', '$', 'ε', 'identificador', 'numero', 'TOKEN_CURSOR')
            return datos
        def opciones(texto, cursor=None):
            return [s['text'] for s in consultar(texto, cursor)['suggestions']]
        assert opciones('') == ['F', 'Vi', 'Vm']
        assert consultar('F ')['status'] == 'sin_opciones'
        assert opciones('F main [] : vo') == ['void']
        medio = 'F main [] : void ; E ; .'
        q = consultar(medio, medio.index('void') + 2)
        assert [s['text'] for s in q['suggestions']] == ['void']
        assert medio.encode()[q['suggestions'][0]['start']:q['suggestions'][0]['end']] == b'void'
        assert opciones('V') == ['Vi', 'Vm']
        assert opciones('Vm dato : ') == ['bool', 'int']
        assert 'void' not in opciones('F funcion [ ')
        assert '.' in opciones('F main [] : void ; E ;\n')
        assert 'F' in opciones('F main [] : void ; E ; .\n')
        assert '==' in opciones('F main [] : void ; Vm x : int = 1 ; x = x ')
        # Un error anterior bloquea; un error posterior no invalida el prefijo.
        for t in ('@ F ', 'F main [] : void ; Vm x : int = ; '):
            assert consultar(t)['status'] == 'bloqueado'
        sufijo = 'F main [] : vo @ ;'
        assert opciones(sufijo, sufijo.index('vo') + 2) == ['void']
        for t in ('# comentario', '#[ sin cerrar', 'F main [] : void ; #[ bloque ]#'):
            cursor = len(t) if not t.endswith(']#') else t.index('bloque') + 2
            assert consultar(t, cursor)['status'] == 'bloqueado'
        assert opciones('#[ cerrado ]# ') == ['F', 'Vi', 'Vm']
        unicode = '# á😀\r\n\tF main [] : vo'
        assert opciones(unicode) == ['void']
        a=consultar('F main [] : void ;\n'); b=consultar('F main [] : void ;\n')
        assert a == b
        # Verificar contexto hasta el cursor en muchas fronteras, sin accidentes
        # del estado Flex/Bison ni dependencia de que la fuente esté completa.
        texto = (ROOT / 'pruebas/p02_factorial.cls').read_text()
        for posicion in range(0, len(texto) + 1, 7):
            consultar(texto, posicion)
        fuente.write_text(':' * 50001)
        tokens = run('--suggest-json', 0, fuente)
        assert tokens.returncode == 1 and json.loads(tokens.stdout)['status'] == 'limite'
        fuente.write_text(' ' * 200001)
        r=run('--suggest-json', 0, fuente)
        assert r.returncode == 1 and json.loads(r.stdout)['status'] == 'limite'
        fuente.write_text('😀')
        r=run('--suggest-json', 1, fuente)
        assert r.returncode == 1 and json.loads(r.stdout)['status'] == 'error'
        for n in ('-1', '+1', ' 1', 'x', '184467440737095516160000'):
            assert run('--suggest-json', n, fuente).returncode == 2
        for flags in (['--check'], ['--check-json'], ['--tokens-json'], ['-s'], ['-t'], ['-m'], ['-p'],
                      ['-x'], ['--no-schedule'], ['-o','salida.bin'], ['--ast-json','salida.json']):
            assert run('--suggest-json', 0, *flags, fuente).returncode == 2
        assert run('--suggest-json', 0, carpeta/'ausente.cls').returncode == 1
        assert run('--suggest-json', 0, fuente, '-v', '--time').returncode == 0
    print('PASS --suggest-json: prefijos LAC, fragmentos, comentarios, Unicode, errores, limites y CLI')


if __name__ == '__main__': main()
