#!/usr/bin/env python3
"""Contrato de ayuda: contenido, codigo de salida y ausencia de compilacion."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def run(folder, *args):
    return subprocess.run([str(ROOT / 'ccom'), *map(str, args)], cwd=folder,
                          capture_output=True, text=True, timeout=30)


def main():
    with tempfile.TemporaryDirectory(prefix='calser_ayuda_') as temporary:
        folder = Path(temporary)
        short = run(folder, '-h')
        long = run(folder, '--help')
        assert short.returncode == long.returncode == 0
        assert short.stderr == long.stderr == ''
        assert short.stdout == long.stdout
        for flag in ('-h, --help', '-t', '-m', '-v', '-o <salida>', '-s', '-p',
                     '-x', '--ast-json <json>', '--no-schedule', '--time'):
            assert any(line.strip().startswith(flag) for line in long.stdout.splitlines()), flag
        for text in ('Ejemplos:', 'Codigos de salida:', '.bin', '.hex', '.mem', '.asm'):
            assert text in long.stdout, text
        # La entrada es invalida y hay salidas preexistentes: la ayuda debe
        # terminar sin abrir el fuente, truncar archivos ni medir tiempos.
        source = folder / 'invalido.cls'
        source.write_text('esto no es Calser')
        files = {source: source.read_bytes()}
        for extension in ('.bin', '.hex', '.mem', '.asm', '.json'):
            path = folder / ('salida' + extension)
            path.write_bytes(b'conservar')
            files[path] = path.read_bytes()
        for args in (
            ['--time', '--help'],
            ['-o', folder / 'salida.bin', '--help', source],
            [source, '-s', '--ast-json', folder / 'salida.json', '-h'],
            ['-x', folder / 'inexistente.asm', '--help'],
        ):
            result = run(folder, *args)
            assert result.returncode == 0 and not result.stderr, result
            assert result.stdout == long.stdout
        assert set(folder.iterdir()) == set(files)
        assert all(path.read_bytes() == content for path, content in files.items())
        for args in ([], ['--desconocida'], ['-o'], ['--help=valor'],
                     ['--desconocida', '--help']):
            result = run(folder, *args)
            assert result.returncode == 2 and 'uso: ccom' in result.stderr, result
            assert not result.stdout
    print('PASS ayuda: -h/--help, opciones, ejemplos, errores y archivos intactos')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
