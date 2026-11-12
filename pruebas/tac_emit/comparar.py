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
    """Adaptador E: compila por CLI y lee tramos de la tabla pública del listado."""
    result = subprocess.run([str(binary), '-O1', '-s', str(source),
                             '-o', str(base.with_suffix('.bin'))],
                            capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise ValueError(result.stderr.strip() or f'ccom: salida {result.returncode}')
    regions = []
    selected = False
    for line in base.with_suffix('.asm').read_text().splitlines():
        scope = re.match(r'#\s+Scope \d+: (\w+)(?: (\w+))?$', line)
        if scope:
            selected = scope[1] == 'GLOBAL' or scope[2] == 'main'
        variable = re.search(r' width=(\d+) addr=0x([0-9A-Fa-f]+)', line)
        if selected and variable:
            regions.append((int(variable[2], 16), int(variable[1])))
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
    parser.add_argument('--binario', type=Path, default=ROOT / 'ccom')
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
