#!/usr/bin/env python3
"""§9.11: compara el resultado interpretado con cada pasada HIGH; Aho §9.1."""
from pathlib import Path
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def run(source, *options):
    """Ejecuta el auxiliar común, conserva el protocolo y exige éxito del proceso."""
    result = subprocess.run([str(ROOT / 'build/interpretar_tac'), str(source), *options],
                            capture_output=True, timeout=30)
    assert result.returncode == 0 and not result.stderr, (source, options, result.stderr)
    return result.stdout


def main():
    """Compara cada programa sin CRYPTO; el intérprete clasifica las exclusiones."""
    count = omitted = 0
    sources = sorted([*ROOT.joinpath('pruebas').rglob('*.cls'),
                      *ROOT.joinpath('ejercicios').rglob('*.cls')])
    for source in sources:
        if source.parent == ROOT / 'pruebas' and re.fullmatch(r'e\d+_.*\.cls', source.name):
            continue
        before = run(source)
        if json.loads(before)['status'] == 'unsupported_crypto':
            omitted += 1
            continue
        for name in ('unroll', 'rename', 'dce'):
            after = run(source, f'--opt={name}')
            assert before == after, (source, name, before, after)
            count += 1
        # El factor explícito usa el mismo contrato que --unroll de ccom.
        assert before == run(source, '--opt=unroll', '--unroll=2'), source
    print(f'PASS pasadas HIGH: {count} comparaciones exactas; '
          f'{count // 3} factores explícitos; {omitted} programas CRYPTO omitidos')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
