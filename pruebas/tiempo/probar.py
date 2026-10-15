#!/usr/bin/env python3
"""Verifica --time sin depender de una duracion exacta ni de la carga del equipo."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PATTERN = re.compile(r'^ccom: tiempo de (compilacion|ensamblado|exportacion AST) '
                     r'\((completado|fallido)\): (\d+\.\d{3}) ms$', re.M)


def run(args):
    return subprocess.run([str(ROOT / 'ccom'), *map(str, args)],
                          capture_output=True, text=True, timeout=30)


def check(args, code, operation=None):
    result = run(args)
    assert result.returncode == code, (args, result.stderr)
    matches = list(PATTERN.finditer(result.stderr))
    if operation is None:
        assert not matches, result.stderr
    else:
        assert len(matches) == 1, result.stderr
        match = matches[0]
        assert match[1] == operation, result.stderr
        assert match[2] == ('completado' if code == 0 else 'fallido'), result.stderr
        assert result.stderr.rstrip().endswith(match[0]), result.stderr
    return result


def main():
    with tempfile.TemporaryDirectory(prefix='calser_tiempo_') as temporary:
        folder = Path(temporary)
        source = ROOT / 'pruebas/p02_factorial.cls'
        plain = folder / 'plain.bin'
        timed = folder / 'timed.bin'
        before = check(['-s', '-m', '-p', '-v', source, '-o', plain], 0)
        after = check(['-s', '-m', '-p', '-v', source, '--time', '-o', timed], 0, 'compilacion')
        assert before.stdout == after.stdout
        assert PATTERN.sub('', after.stderr).rstrip() == before.stderr.rstrip()
        for extension in ('.asm', '.bin', '.hex', '.mem'):
            assert plain.with_suffix(extension).read_bytes() == timed.with_suffix(extension).read_bytes()
        check(['--time', '--no-schedule', source, '-o', timed], 0, 'compilacion')
        check(['--time', '-x', plain.with_suffix('.asm'), '-o', timed], 0, 'ensamblado')
        invalid_asm = folder / 'invalid.asm'
        invalid_asm.write_text('{ nopi }\n')
        check(['--time', '-x', invalid_asm, '-o', timed], 1, 'ensamblado')
        for name in ('e52_caracter_invalido', 'e16_falta_punto_coma', 'e01_no_declarado'):
            check(['--time', ROOT / ('pruebas/' + name + '.cls'), '-o', timed], 1, 'compilacion')
        check(['--time', folder / 'missing.cls', '-o', timed], 1, 'compilacion')
        check(['--time', source, '-o', folder / 'missing/out.bin'], 1, 'compilacion')
        json = folder / 'ast.json'
        check(['--time', '--ast-json', json, source, '-o', timed], 0, 'exportacion AST')
        assert json.is_file()
        check(['--time', '--ast-json', json, '-s', source, '-o', timed], 0, 'compilacion')
        check(['--time'], 2)
        check(['--time', '--desconocida', source], 2)
    print('PASS --time: exito, errores, modos, stdout y archivos sin cambios')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
