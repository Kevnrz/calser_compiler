#!/usr/bin/env python3
"""C1: análisis HIGH/ISA de todo el corpus; casos dirigidos en fuentes temporales."""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
NESTED = '''F main [] : void ;
    Vm x : int = 0 ;
    S [ i, 0, 3, 1 ] ;
        L [ x < 2 ] ;
            x = x + 1 ;
        .
    .
    E ;
.
'''
NO_BACKEDGE = '''F main [] : void ;
    L [ verdadero ] ;
        E ;
    .
    E ;
.
'''
NO_BACKEDGE_S = '''F main [] : void ;
    S [ i, 0, 3, 1 ] ;
        E ;
    .
    E ;
.
'''


def run(binary, mode, source):
    """Entrada: auxiliar/modo/fuente. Salida: proceso. Efectos: analiza sin emitir archivos."""
    return subprocess.run([str(binary), mode, str(source)], capture_output=True,
                          text=True, timeout=30)


def main():
    """Entrada: modo y auxiliar opcional. Salida: 0 si pasa. Efectos: crea fuentes temporales."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('live', 'loops'))
    parser.add_argument('--binario', type=Path, default=ROOT / 'build/analizar_tac')
    args = parser.parse_args()
    binary = args.binario.resolve()
    valid = rejected = 0
    for source in sorted([*ROOT.joinpath('pruebas').rglob('*.cls'),
                          *ROOT.joinpath('ejercicios').rglob('*.cls')]):
        result = run(binary, args.mode, source)
        invalid = source.parent == ROOT / 'pruebas' and re.fullmatch(r'e\d+_.*\.cls', source.name)
        if invalid:
            assert result.returncode == 3, (source, result.returncode, result.stderr)
            rejected += 1
        else:
            assert result.returncode == 0 and not result.stderr, (source, result.stdout, result.stderr)
            valid += 1
    with tempfile.TemporaryDirectory(prefix='calser_c1_') as temporary:
        for name, code in [('anidados', NESTED), ('sin_retroceso_L', NO_BACKEDGE),
                           ('sin_retroceso_S', NO_BACKEDGE_S)]:
            source = Path(temporary) / (name + '.cls')
            source.write_text(code)
            result = run(binary, args.mode, source)
            assert result.returncode == 0 and not result.stderr, (name, result.stdout, result.stderr)
            if args.mode == 'loops':
                if name == 'anidados':
                    assert 'HIGH main loops=2 marks=2 without=0 depth=2' in result.stdout
                    assert 'ISA main loops=2 marks=0 without=0 depth=2' in result.stdout
                else:
                    assert 'HIGH main loops=0 marks=1 without=1 depth=0' in result.stdout
    print(f'PASS {args.mode}: {valid} programas HIGH/ISA virtual, 3 temporales; '
          f'{rejected} rechazos; representación e imagen intactas')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
