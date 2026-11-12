#!/usr/bin/env python3
"""B: corpus y listados ISA virtual; sin asignar, emitir ni modificar HIGH.

Usa: desde_fuente, parser y semántico del proyecto; biblioteca estándar.
Lo usa check-tac-lower. Referencias: Aho §8.1–8.2 y §8.6.
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CASES = {
    'factorial_r': 'pruebas/programas/factorial_r.cls',
    'corto_circuito': 'pruebas/programas/corto_circuito.cls',
    's_negativo': 'pruebas/programas/s_negativo.cls',
    'lectura_1d': 'pruebas/programas/lectura_1d.cls',
    'r_arreglos': 'pruebas/programas/r_arreglos.cls',
    'r_arreglos_local': 'pruebas/programas/r_arreglos_local.cls',
    'r_arreglos_2d': 'pruebas/programas/r_arreglos_2d.cls',
    'r_arreglos_parametro': 'pruebas/programas/r_arreglos_parametro.cls',
    'crypto': 'pruebas/p09_crypto.cls',
    'inicializadores': 'pruebas/tac_build/casos/inicializadores.cls',
    'llamadas_anidadas': 'pruebas/p10_llamadas_anidadas.cls',
}
LARGE_CONSTANTS = '''F main [] : void ;
    Vm a : int = -2147483648 ;
    Vm b : int = 2147483647 ;
    Vm c : int = 200000 ;
    Vm d : int = -131073 ;
    E ;
.
'''


def run(binary, source):
    """Entrada: auxiliar y fuente. Salida: proceso con sus flujos capturados.

    Efectos: ejecuta frontend y bajada en memoria con timeout.
    Restricciones: el auxiliar distingue rechazo del frontend de error de selección.
    """
    return subprocess.run([str(binary), str(source)], capture_output=True,
                          text=True, timeout=30)


def main():
    """Entrada: --binario opcional. Salida: cero si pasan corpus y doce esperados.

    Efectos: ejecuta el auxiliar; crea y elimina solo un fuente temporal.
    Restricciones: no regenera esperados ni escribe salidas del compilador.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', type=Path, default=ROOT / 'build/bajar_tac')
    args = parser.parse_args()
    binary = args.binario.resolve()
    listings = {}
    rejected = 0
    for source in sorted([*ROOT.joinpath('pruebas').rglob('*.cls'),
                          *ROOT.joinpath('ejercicios').rglob('*.cls')]):
        result = run(binary, source)
        invalid = source.parent == ROOT / 'pruebas' and re.fullmatch(r'e\d+_.*\.cls', source.name)
        if invalid:
            assert result.returncode == 3, (source, result.returncode, result.stderr)
            rejected += 1
        else:
            assert result.returncode == 0 and not result.stderr, (source, result.returncode, result.stderr)
            listings[str(source.relative_to(ROOT))] = result.stdout
    for name, source in CASES.items():
        expected = (HERE / 'esperados' / (name + '.tac')).read_text()
        assert listings[source] == expected, name
    with tempfile.TemporaryDirectory(prefix='calser_tac_lower_') as directory:
        source = Path(directory) / 'constantes_grandes.cls'
        source.write_text(LARGE_CONSTANTS)
        result = run(binary, source)
        assert result.returncode == 0 and not result.stderr, result.stderr
        assert result.stdout == (HERE / 'esperados/constantes_grandes.tac').read_text()
    print(f'PASS bajada ISA virtual: {len(listings)} programas del corpus y 1 temporal; '
          f'{len(CASES) + 1} esperados; HIGH intacto; {rejected} rechazos del frontend')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
