#!/usr/bin/env python3
"""Genera los entregables de los tres ejercicios (enunciado CE1108 §10.4 y
§10.5): para cada uno, el fuente, el ensamblador y el binario, la salida de
la compilación, la traza de ejecución (§8.4) y el resumen de la ejecución
contra los valores esperados; más la evidencia de `make check`.

Uso: `make ejercicios` (desde la raíz del repositorio).

Es determinista: no escribe fechas ni rutas absolutas, así que correrlo dos
veces seguidas no cambia ningún archivo. Termina con código 1 si algún valor
esperado no se cumple. Sin dependencias externas.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import sim_asm  # noqa: E402

CCOM = ROOT / 'ccom'
OUTPUT = ROOT / 'ejercicios'
MAX_BUNDLES = 2_000_000
TRACE_LIMIT = 5000

# (carpeta, nombre, fuente única en pruebas/, valores esperados). Los valores
# esperados son (variable, valor): un entero, o una lista para un arreglo;
# un bool se escribe 1 (verdadero) o 0 (falso).
EXERCISES = [
    ('ejercicio1_factorial', 'ejercicio1', 'pruebas/p02_factorial.cls',
     [('f', 120)]),
    ('ejercicio2_matriz', 'ejercicio2', 'pruebas/p12_ejercicio2.cls',
     [('ok', 1), ('p', [1, 1])]),
    ('ejercicio3_cifrado', 'ejercicio3', 'pruebas/p11_ejercicio3.cls',
     [('n1', 8), ('n2', 8)]),
]

# Zona de los archivos de load_file (isa.md §7): el texto se carga en 0x1000.
DATA_ADDRESS = 0x1000
DATA_SIZE = 1024


def relative(path):
    return path.relative_to(ROOT).as_posix()


def hexdump(memory, address, count=64):
    """Los bytes como hexadecimal y como texto (lo no imprimible, como '.')."""
    rows = []
    for offset in range(0, count, 16):
        chunk = memory[address + offset:address + offset + 16]
        hexadecimal = ' '.join(f'{byte:02x}' for byte in chunk)
        text = ''.join(chr(byte) if 32 <= byte < 127 else '.' for byte in chunk)
        rows.append(f'  0x{address + offset:04X}  {hexadecimal}  |{text}|')
    return rows


def compile_exercise(folder, name, source):
    """Copia el fuente y lo compila con -v -m -s -p. Devuelve (código, líneas
    de compilacion.txt)."""
    cls = folder / (name + '.cls')
    shutil.copyfile(ROOT / source, cls)
    command = ['./ccom', '-v', '-m', '-s', '-p', relative(cls),
               '-o', relative(folder / (name + '.bin'))]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    lines = ['$ ' + ' '.join(command),
             f'codigo de salida: {result.returncode}', '',
             '===== Fases (stderr, -v) =====']
    lines += result.stderr.splitlines()
    lines += ['', '===== Salida estandar: tablas (-m) y paquetes (-p) =====']
    lines += result.stdout.splitlines()
    return result.returncode, lines


def check_value(tracer, simulator, name, expected):
    """Compara una variable con lo esperado. Devuelve (esperado, obtenido,
    cumple), los dos primeros como texto (verdadero/falso en un bool)."""
    for variable, words in tracer.values(simulator):
        if variable['shown'] != name:
            continue
        if isinstance(expected, list):
            got = [sim_asm.signed(w) for w in words]
            return (str(expected), str(got), got == expected)
        shown = sim_asm.format_value(variable, expected)
        return (shown, sim_asm.format_value(variable, words[0]),
                sim_asm.signed(words[0]) == expected)
    return (str(expected), '(no está en la tabla)', False)


def run_exercise(folder, name, expected):
    """Simula el .asm. Devuelve (simulador, tracer, líneas de ejecucion.txt,
    cumple)."""
    asm = folder / (name + '.asm')
    text = asm.read_text()
    program = sim_asm.parse(text)
    simulator = sim_asm.Simulator(program)
    tracer = sim_asm.Tracer(sim_asm.Annotations(text, len(program.bundles)),
                            program.labels, TRACE_LIMIT)
    simulator.tracer = tracer
    simulator.load_data_image((folder / (name + '.mem')).read_text())

    data = None
    if (folder / 'datos.txt').exists():
        data = (folder / 'datos.txt').read_bytes()
        if len(data) != DATA_SIZE:
            raise SystemExit(f'datos.txt debe tener {DATA_SIZE} bytes, tiene {len(data)}')
        simulator.load_data(data, DATA_ADDRESS)

    simulator.run(MAX_BUNDLES)
    ok = simulator.reason == 'salto a sí mismo' and not simulator.warnings

    report = simulator.report().splitlines()
    lines = [f'Ejecucion de {name} con tools/sim_asm.py',
             f'Programa: {relative(asm)} ({len(program.bundles)} bundles)']
    lines += report[:report.index('Memoria 0x0000–0x00FF (palabras no nulas):')]
    lines += ['', 'Valores finales de las variables:']
    lines += tracer.final_state(simulator)

    lines += ['', 'Resultado esperado contra obtenido:']
    for variable, value in expected:
        wanted, got, matches = check_value(tracer, simulator, variable, value)
        ok = ok and matches
        lines.append(f'  {variable}: esperado {wanted}, obtenido {got}  '
                     f'{"OK" if matches else "FALLA"}')

    if data is not None:
        memory = simulator.memory
        original = bytes(data)
        decrypted_equal = memory[0x3000:0x3400] == memory[0x1000:0x1400]
        encrypted_differs = memory[0x2000:0x2400] != memory[0x1000:0x1400]
        intact = memory[0x1000:0x1400] == original
        ok = ok and decrypted_equal and encrypted_differs and intact
        lines += ['',
                  'Archivo de entrada: datos.txt (1024 bytes) cargado en 0x1000.',
                  'Primeros 64 bytes de cada region (hexadecimal y texto):',
                  '',
                  '0x1000, original:']
        lines += hexdump(memory, 0x1000)
        lines += ['', '0x2000, cifrado:'] + hexdump(memory, 0x2000)
        lines += ['', '0x3000, descifrado:'] + hexdump(memory, 0x3000)
        lines += ['',
                  'Comprobaciones:',
                  f'  0x3000-0x33FF == 0x1000-0x13FF: {"si" if decrypted_equal else "NO"}',
                  f'  0x2000-0x23FF distinto del original: {"si" if encrypted_differs else "NO"}',
                  f'  0x1000-0x13FF igual a datos.txt (no se altero): {"si" if intact else "NO"}']

    lines += ['', f'Veredicto: {"OK" if ok else "FALLA"}']
    return simulator, tracer, lines, ok


def run_make_check():
    """La evidencia de `make check` (enunciado CE1108 §10.5)."""
    environment = {key: value for key, value in os.environ.items()
                   if key not in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL', 'MAKEOVERRIDES')}
    result = subprocess.run(['make', '-s', 'check'], cwd=ROOT, env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    lines = ['$ make -s check'] + result.stdout.splitlines()
    lines.append(f'codigo de salida: {result.returncode}')
    return result.returncode == 0, lines


def write(path, lines):
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main():
    if not CCOM.exists():
        raise SystemExit('falta compilar ccom (make)')

    failed = False
    for directory, name, source, expected in EXERCISES:
        folder = OUTPUT / directory
        folder.mkdir(parents=True, exist_ok=True)

        code, lines = compile_exercise(folder, name, source)
        write(folder / 'compilacion.txt', lines)
        if code != 0:
            print(f'FALLA {name}: ccom terminó con código {code}')
            failed = True
            continue

        simulator, tracer, lines, ok = run_exercise(folder, name, expected)
        (folder / 'traza.txt').write_text(tracer.text(simulator), encoding='utf-8')
        write(folder / 'ejecucion.txt', lines)

        trace_lines = len((folder / 'traza.txt').read_text(encoding='utf-8').splitlines())
        print(f'{"OK   " if ok else "FALLA"} {name}: {simulator.steps} bundles, '
              f'traza de {trace_lines} líneas'
              f'{" (TRUNCADA)" if tracer.dropped else ""}')
        failed = failed or not ok

    ok, lines = run_make_check()
    write(OUTPUT / 'make_check.txt', lines)
    print(f'{"OK   " if ok else "FALLA"} make check')
    failed = failed or not ok

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
