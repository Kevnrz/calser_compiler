"""C6: consultas excluyentes, rechazos sin artefactos y compilación O1 vigente."""
from pathlib import Path
import itertools
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
QUERIES = [('--check',), ('--check-json',), ('--tokens-json',),
           ('--syntax-json',), ('--suggest-json', '0'), ('--repair-json',),
           ('--clones-json', '5')]


def main():
    rejected = 0
    with tempfile.TemporaryDirectory(prefix='calser_c6_') as directory:
        folder = Path(directory)
        source = folder / 'fuente con espacios.cls'
        source.write_text('F main [] : void ; E ; .\n')
        metrics = folder / 'metricas.json'
        for suffix in ('.bin', '.hex', '.mem', '.asm', '.tac', '.json'):
            (folder / ('fuente con espacios' + suffix)).write_bytes(b'preservar')
        metrics.write_bytes(b'preservar')
        before = {p: p.read_bytes() for p in folder.iterdir()}

        def run(*arguments):
            return subprocess.run([str(ROOT / 'ccom'), *map(str, arguments), str(source)],
                                  cwd=folder, capture_output=True, timeout=10)

        incompatible = [('-i',), ('--tac',), ('-O0',), ('-O1',), ('-O2',),
                        ('--opt=rename',), ('--opt=unroll', '--unroll=2'),
                        ('--unroll=2',), ('--metrics', metrics),
                        ('-O1', '--metrics', metrics)]
        for query, flags in itertools.product(QUERIES, incompatible):
            for arguments in ((*query, *flags), (*flags, *query)):
                result = run(*arguments)
                assert result.returncode == 2 and not result.stdout and result.stderr, (arguments, result)
                rejected += 1
        for first, second in itertools.permutations(QUERIES, 2):
            result = run(*first, *second)
            assert result.returncode == 2 and not result.stdout, (first, second, result)
            rejected += 1
        assert {p: p.read_bytes() for p in folder.iterdir()} == before

        # Las opciones de generación mantienen su efecto sin consulta.
        output = folder / 'compilado.bin'
        result = run('--opt=unroll', '--unroll=2', '-i', '--metrics', metrics, '-o', output)
        assert result.returncode == 0, result.stderr
        assert json.loads(metrics.read_bytes())['pasadas'] == ['unroll']
        assert all(output.with_suffix(suffix).is_file() for suffix in ('.bin', '.hex', '.mem', '.tac'))
        for flag in ('--check', '--check-json'):
            result = run(flag, '-v', '--time')
            assert result.returncode == 0 and b'tiempo de analisis' in result.stderr, result
    print(f'PASS C6: {rejected} combinaciones rechazadas con codigo 2, archivos intactos y O1 vigente')


if __name__ == '__main__':
    main()
