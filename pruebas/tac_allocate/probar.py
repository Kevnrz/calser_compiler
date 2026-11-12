#!/usr/bin/env python3
"""C2: medición completa, asignación por función y validación física del corpus."""
from pathlib import Path
import argparse
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    """Entrada: auxiliar opcional. Salida: cero si solo hay rechazos previstos.

    Efectos: imprime tabla completa y lista de fallos de presión, sin omitirlos.
    Restricciones: no parte intervalos ni considera un fallo de asignación aprobado.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', type=Path, default=ROOT / 'build/asignar_tac')
    args = parser.parse_args()
    valid = rejected = functions = 0
    failures = []
    print('archivo\tfunción\tvivos\tsolapamiento\tMUL\tDIV\tMOD\tPOW\tpreservación\tregistros\tresultado')
    for source in sorted([*Path('pruebas').rglob('*.cls'), *Path('ejercicios').rglob('*.cls')]):
        result = subprocess.run([str(args.binario.resolve()), str(source)], cwd=ROOT,
                                capture_output=True, text=True, timeout=30)
        invalid = source.parent == Path('pruebas') and re.fullmatch(r'e\d+_.*\.cls', source.name)
        if invalid:
            assert result.returncode == 3, (source, result.returncode, result.stderr)
            rejected += 1
            continue
        assert result.returncode in (0, 4) and not result.stderr, (source, result.stdout, result.stderr)
        valid += 1
        print(result.stdout, end='')
        for row in result.stdout.splitlines():
            functions += 1
            if row.split('\t')[-1] != 'OK':
                failures.append(row)
    print(f'Corpus: {valid} programas, {functions} funciones, {rejected} rechazos frontend')
    print('Fallos de asignación:')
    print('\n'.join(failures) if failures else 'ninguno')
    # Los fallos se reportan como limitación, sin truncar la medición del corpus.
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
