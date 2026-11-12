"""Confirmaciones por stderr sin contaminar AST/bundles ni anunciar salidas fallidas."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SUCCESS = ('Compilación correcta.', 'Ensamblado correcto.', 'AST exportado correctamente.', 'Análisis correcto: sin errores.')


def main():
    with tempfile.TemporaryDirectory(prefix='calser_exito_') as folder:
        folder = Path(folder)
        source = folder/'suma con espacios.cls'
        source.write_text('F main [] : void ; Vm resultado : int = 2 + 3 ; E ; .\n')
        def run(*args):
            return subprocess.run([str(ROOT/'ccom'), *map(str,args)], cwd=folder, capture_output=True, text=True, timeout=10)
        for asm in (False, True):
            output = folder/('con asm.bin' if asm else 'sin asm.bin')
            result = run(source, '-o', output, *(['-s'] if asm else []))
            assert result.returncode == 0 and result.stdout == '', result
            assert 'Compilación correcta.' in result.stderr
            for ext in ('.bin', '.hex', '.mem', *(['.asm'] if asm else [])):
                path = output.with_suffix(ext)
                assert path.is_file() and f'  {path}\n' in result.stderr, result.stderr
            assert (str(output.with_suffix('.asm')) in result.stderr) == asm
        result = run('--check', source)
        assert result.returncode == 0 and result.stdout == ''
        assert result.stderr == 'ccom: Análisis correcto: sin errores.\n'
        assert 'generados' not in result.stderr
        json_path = folder/'arbol.json'
        semantic_error = folder/'sin main.cls'
        semantic_error.write_text('F suma [] : void ; E ; .')
        result = run('--ast-json', json_path, '-t', semantic_error)
        assert result.returncode == 0 and result.stdout
        assert 'AST exportado correctamente.' in result.stderr and str(json_path) in result.stderr
        assert 'Compilación correcta.' not in result.stderr and '.bin\n' not in result.stderr
        result = run('-s', '--ast-json', json_path, source, '-o', folder/'combinado.bin')
        assert result.returncode == 0 and 'Compilación correcta.' in result.stderr
        assert f'  {json_path}\n' in result.stderr
        for flags in (['-i'], ['-O1'], ['-O1', '-i']):
            output = folder / 'ast y compilacion.bin'
            result = run(*flags, '--ast-json', json_path, source, '-o', output)
            assert result.returncode == 0 and 'Compilación correcta.' in result.stderr, result
            assert 'AST exportado correctamente.' not in result.stderr
            for extension in ('.bin', '.hex', '.mem', *(['.tac'] if '-i' in flags else [])):
                path = output.with_suffix(extension)
                assert path.is_file() and f'  {path}\n' in result.stderr, result.stderr
            assert f'  {json_path}\n' in result.stderr
        result = run('-x', folder/'con asm.asm', '-s', '-o', folder/'ensamblado.bin')
        assert result.returncode == 0 and 'Ensamblado correcto.' in result.stderr
        assert str(folder/'ensamblado.asm') not in result.stderr
        assert 'se ignora' in result.stderr
        for args in ((source, '-o', folder/'no existe'/'falla.bin'),
                     ('--check', semantic_error), ('--ast-json', folder/'no existe'/'arbol.json', source),
                     ('--check', '-s', source)):
            result = run(*args)
            assert result.returncode != 0 and not any(m in result.stderr for m in SUCCESS), result
        result = run('--help')
        assert result.returncode == 0 and not result.stderr
        for args in (('-t',), ('-p',), ('-m',)):
            result = run(source, *args, '-o', folder/'formatos.bin')
            assert result.returncode == 0 and result.stdout, result
            assert not any(m in result.stdout for m in SUCCESS), result.stdout
    print('PASS confirmación: análisis, compilación, ensamblado, AST, salidas y errores')


if __name__ == '__main__':
    main()
