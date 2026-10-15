#!/usr/bin/env python3
"""Comprueba operaciones matriciales Calser en ambos modos del organizador."""
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / 'pruebas/programas'))
import probar as programas

# Resultados calculados independientemente del codigo Calser, por filas.
# Por ejemplo AB[0][1] = 1*5 + (-2)*2 = 1; BA[0][1] = 0*(-2) + 5*4 = 20.
CASES = [
    ('cuadradas', {
        'a': [1, -2, 3, 4], 'b': [0, 5, -1, 2],
        'identidad': [1, 0, 0, 1], 'cero': [0, 0, 0, 0],
        'sumado': [1, 3, 2, 6], 'restado': [1, -7, 4, 2],
        'ab': [2, 1, -4, 23], 'ba': [15, 20, 5, 10],
        'ai': [1, -2, 3, 4], 'ia': [1, -2, 3, 4],
        'az': [0, 0, 0, 0], 'za': [0, 0, 0, 0],
    }),
    ('rectangulares', {
        'a': [1, -2, 3, 0, 4, -1], 'b': [2, 0, -1, 5, 3, -2],
        'ab': [13, -16, -7, 22],
        'ba': [2, -4, 6, -1, 22, -8, 3, -14, 11],
    }),
]


def main():
    simulator = programas.load_simulator()
    failed = 0
    with tempfile.TemporaryDirectory(prefix='calser_matrices_') as temporary:
        for name, expected in CASES:
            for conservative in (False, True):
                mode = 'sin organizador' if conservative else 'con organizador'
                base = Path(temporary) / (name + ('_escalar' if conservative else '_organizado'))
                command = [str(ROOT / 'ccom'), '-s', '-m']
                if conservative:
                    command.append('--no-schedule')
                command += [str(HERE / (name + '.cls')), '-o', str(base.with_suffix('.bin'))]
                try:
                    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
                    if result.returncode:
                        raise programas.Failure(result.stderr.strip())
                    programas.check_values(programas.simulate(simulator, base), result.stdout, expected)
                    print('PASS %s (%s): %d matrices verificadas' % (name, mode, len(expected)))
                except (programas.Failure, simulator.AsmError, OSError, subprocess.TimeoutExpired) as error:
                    failed += 1
                    print('FAIL %s (%s): %s' % (name, mode, error))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
