#!/usr/bin/env python3
"""D/E: comparación observable contra O0; solo cambia el adaptador de compilación."""
from pathlib import Path
import argparse
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
from sim_asm import Simulator, parse


def compile_o0(source, base):
    """Entrada: fuente/base. Salida: ensamblador O0. Efectos: ejecuta ccom actual."""
    result = subprocess.run([str(ROOT / 'ccom'), '-s', str(source), '-o', str(base.with_suffix('.bin'))],
                            capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise ValueError(result.stderr.strip())
    return base


def compile_new(binary, source, base):
    """Adaptador D: devuelve base y tramos observables. E lo sustituirá por su CLI."""
    result = subprocess.run([str(binary), str(source), str(base)],
                            capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise ValueError(result.stderr.strip() or f'auxiliar: salida {result.returncode}')
    regions = [tuple(map(int, line.split()[1:])) for line in result.stdout.splitlines()
               if line.startswith('observable ')]
    return base, regions


def execute(base, data):
    """Entrada: ASM/MEM y bytes externos. Salida: simulador terminado sin avisos.

    Efectos: instancia y ejecuta el simulador original con límite explícito.
    Restricciones: no replica instrucciones ni compara ciclos como equivalencia.
    """
    sim = Simulator(parse(base.with_suffix('.asm').read_text()))
    sim.load_data_image(base.with_suffix('.mem').read_text())
    sim.load_data(data)
    sim.run(2000000)
    if sim.reason != 'salto a sí mismo' or sim.warnings:
        raise ValueError(f'{sim.reason}; avisos={sim.warnings[:3]}')
    return sim


def observable(sim, regions):
    """Entrada: simulador/tramos. Salida: proyección comparable, sin memoria interna."""
    terminal = next((name for name in ('fin_main', '.L_arith_error')
                     if sim.program.labels.get(name) == sim.pc), None)
    if terminal is None:
        raise ValueError('terminal ajeno a fin/error aritmético')
    return (tuple(bytes(sim.memory[a:a + size]) for a, size in regions),
            bytes(sim.memory[0x1000:0x10000]), sim.auth, sim.kv_lock, sim.exc,
            sim.anchor, tuple(tuple(row) for row in sim.vault), terminal)


def compare(source, directory, compiler):
    """Entrada: fuente, carpeta y adaptador nuevo. Salida: bundles O1/O0 o error.

    Efectos: compila y ejecuta ambos caminos con los mismos datos externos.
    Restricciones: el comparador permanece igual cuando E cambie el adaptador.
    """
    old = compile_o0(source, directory / 'o0')
    new, regions = compiler(source, directory / 'o1')
    counts = None
    for data in (bytes(1024), bytes(range(256)) * 4):
        baseline = execute(old, data)
        optimized = execute(new, data)
        counts = len(optimized.program.bundles), len(baseline.program.bundles)
        if observable(baseline, regions) != observable(optimized, regions):
            raise ValueError('estado observable diferente')
    return counts


def main():
    """Entrada: auxiliar opcional. Salida: 1 si algún caso falla, tras listarlos todos."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', type=Path, default=ROOT / 'build/emitir_tac')
    args = parser.parse_args()
    binary = args.binario.resolve()
    failures = []
    count = 0
    print('programa\tresultado\tbundles O1\tbundles O0')
    sources = sorted([*ROOT.joinpath('pruebas').rglob('*.cls'),
                      *ROOT.joinpath('ejercicios').rglob('*.cls')])
    with tempfile.TemporaryDirectory(prefix='calser_emit_') as temporary:
        for source in sources:
            if source.parent == ROOT / 'pruebas' and re.fullmatch(r'e\d+_.*\.cls', source.name):
                continue
            name = str(source.relative_to(ROOT))
            count += 1
            try:
                o1, o0 = compare(source, Path(temporary),
                    lambda src, base: compile_new(binary, src, base))
                print(f'{name}\tOK\t{o1}\t{o0}')
            except (ValueError, OSError, subprocess.SubprocessError) as error:
                failures.append(f'{name}: {error}')
                print(f'{name}\tFALLA\t-\t-')
    print(f'Comparados: {count}; datos externos: cero y patrón de 1024 bytes')
    print('Programas que difieren o fallan:')
    print('\n'.join(failures) if failures else 'ninguno')
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
