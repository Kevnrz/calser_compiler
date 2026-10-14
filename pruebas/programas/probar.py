#!/usr/bin/env python3
"""Prueba de valores: compila cada programa, lo corre en el simulador y
compara los valores finales de sus variables con los esperados.

Para cada caso: ccom -s -m, tools/sim_asm.py con la imagen de datos, y las
direcciones se leen de la tabla de símbolos de -m. Se exige fin por salto a
sí mismo y 0 avisos de separación. Sin dependencias externas.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CCOM = ROOT / 'ccom'
SIM = ROOT / 'tools/sim_asm.py'
HERE = Path(__file__).resolve().parent

# (nombre, fuente, {variable: valor o lista}). Solo se comparan variables
# cuyo nombre es único en la tabla de su programa.
CASES = [
    ('p02', ROOT / 'pruebas/p02_factorial.cls', {'f': 120}),
    ('p03', ROOT / 'pruebas/p03_suma.cls', {'r': 15}),
    # p05: r = suma_fila [ t , 1 ] = fila 1 de t = 4 + 5 + 6 = 15.
    #      poner [ w , 2 ] escribe w[2] = 7; w [ 0 ] = t [ 1 ] [ 2 ] = 6;
    #      w[1] no se toca (0). t no cambia.
    ('p05', ROOT / 'pruebas/p05_arreglos_llamadas.cls',
     {'r': 15, 'w': [6, 0, 7], 't': [1, 2, 3, 4, 5, 6]}),
    # p06: total suma los pares no negativos de [2, 5, 8] = 2 + 8 = 10;
    #      copia = datos; ok = t > 9 | ok = verdadero.
    ('p06', ROOT / 'pruebas/p06_tipos.cls',
     {'t': 10, 'copia': [2, 5, 8], 'ok': 1}),
    ('p10', ROOT / 'pruebas/p10_llamadas_anidadas.cls', {'sum': 30}),
    ('sumar', HERE / 'sumar.cls', {'x': 5}),
    ('factorial con R', HERE / 'factorial_r.cls', {'f': 6}),
    # La segunda llamada no asigna el retorno: debe valer 0, no el 9 anterior.
    ('retorno en 0', HERE / 'retorno_cero.cls', {'x': 9, 'y': 0}),
    ('lectura 1D', HERE / 'lectura_1d.cls', {'y': 7, 's': 10, 'z': 4, 'q': 6}),
    # Copia completa con parámetros arreglo reenviados por dos funciones.
    ('copia de arreglos', HERE / 'copia_arreglos.cls',
     {'t': 15, 'y': [4, 5, 6], 'x': [4, 5, 6]}),
    # Ciclo S (calser_diseno.md §5.8.2): fin exclusivo; el paso negativo
    # sigue mientras i > fin.
    ('S paso 1', HERE / 's_positivo.cls', {'x': 10}),          # 0+1+2+3+4
    ('S paso -1', HERE / 's_negativo.cls', {'x': 15}),         # 5+4+3+2+1
    ('S paso 3', HERE / 's_paso3.cls', {'x': 18}),             # 0+3+6+9
    # Paso que no cabe en un inmediato de 18 bits: 0 + 200000.
    ('S paso 200000', HERE / 's_paso_grande.cls', {'x': 200000}),
    # Suma de i*j, i de 0 a 2 y j de 0 a 3: (0+1+2)·(0+1+2+3).
    ('S anidados', HERE / 's_anidados.cls', {'x': 18}),
    # E dentro de un S: 15 está en la posición 2; 99 no está (-1); después
    # otro S suma a tres veces.
    ('E dentro de S', HERE / 'e_en_s.cls', {'a': 2, 'b': -1, 'c': 6}),
    # Ejercicio 2: 42 está en la fila 1, columna 1 de m.
    ('p12', ROOT / 'pruebas/p12_ejercicio2.cls', {'ok': 1, 'p': [1, 1]}),
]

# Programas que todavía no pasan por algo fuera de esta prueba: se informan
# y no cuentan como fallo. Cuando pasen, la prueba avisa para quitarlos.
PENDING = []


class Failure(Exception):
    pass


def load_simulator():
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(SIM.parent))
    import sim_asm
    return sim_asm


def compile_program(source, directory, name):
    """Devuelve (base de las salidas, tabla de -m)."""
    base = directory / re.sub(r'\W', '_', name)
    result = subprocess.run(
        [str(CCOM), '-s', '-m', str(source), '-o', str(base.with_suffix('.bin'))],
        capture_output=True, text=True)
    if result.returncode != 0:
        raise Failure('ccom terminó con código %d: %s'
                      % (result.returncode, result.stderr.strip().splitlines()[0]
                         if result.stderr.strip() else ''))
    return base, result.stdout


def simulate(sim_asm, base, data=None, address=0x1000):
    warnings = []
    simulator = sim_asm.Simulator(
        sim_asm.parse(base.with_suffix('.asm').read_text()), warn=warnings.append)
    simulator.load_data_image(base.with_suffix('.mem').read_text())
    if data is not None:
        simulator.load_data(data, address)
    simulator.run(2000000)
    if simulator.reason != 'salto a sí mismo':
        raise Failure('no terminó por salto a sí mismo: %s' % simulator.reason)
    if simulator.warnings:
        raise Failure('%d avisos de separación' % len(simulator.warnings))
    return simulator


def addresses(table):
    """{nombre: (dirección, palabras)} de las variables de la tabla de -m.
    Los nombres repetidos se descartan: no se pueden distinguir."""
    found = {}
    repeated = set()
    for name, width, address in re.findall(
            r'^    (\w+) : (?:int|bool)(?:\[\d+\])*(?: \(return\))? width=(\d+) addr=0x([0-9A-F]+)',
            table, re.M):
        if name in found:
            repeated.add(name)
        found[name] = (int(address, 16), int(width) // 4)
    for name in repeated:
        del found[name]
    return found


def check_values(simulator, table, expected):
    where = addresses(table)
    for name, value in expected.items():
        if name not in where:
            raise Failure("'%s' no está en la tabla (o está repetida)" % name)
        address, count = where[name]
        got = [int.from_bytes(simulator.memory[address + 4 * i:address + 4 * i + 4],
                              'little', signed=True) for i in range(count)]
        want = value if isinstance(value, list) else [value]
        if got != want:
            raise Failure('%s: esperado %s, obtenido %s'
                          % (name, value, got if isinstance(value, list) else got[0]))


def run_case(sim_asm, directory, name, source, expected):
    base, table = compile_program(source, directory, name)
    check_values(simulate(sim_asm, base), table, expected)


def run_p09(sim_asm, directory):
    """p09 con datos en 0x1000: descifrar devuelve el original, el cifrado es
    distinto, y cifrar sin sesión no cifra."""
    base, _ = compile_program(ROOT / 'pruebas/p09_crypto.cls', directory, 'p09')
    original = bytes(range(256)) * 4
    memory = simulate(sim_asm, base, original).memory
    if memory[0x1000:0x1400] != original:
        raise Failure('0x1000-0x13FF cambió')
    if memory[0x3000:0x3400] != original:
        raise Failure('0x3000-0x33FF distinto de 0x1000-0x13FF')
    if memory[0x2000:0x2400] == original:
        raise Failure('0x2000-0x23FF igual al original')
    if memory[0x4000:0x4008] != original[:8]:
        raise Failure('0x4000-0x4007 distinto de 0x1000-0x1007')


def run_p11(sim_asm, directory):
    """Ejercicio 3 con datos en 0x1000: cifra el archivo por tramos con dos
    llaves y lo descifra. 128 bloques en tramos de 16 son 8 tramos."""
    base, table = compile_program(ROOT / 'pruebas/p11_ejercicio3.cls', directory, 'p11')
    original = bytes(range(256)) * 4
    simulator = simulate(sim_asm, base, original)
    check_values(simulator, table, {'n1': 8, 'n2': 8})
    memory = simulator.memory
    if memory[0x1000:0x1400] != original:
        raise Failure('0x1000-0x13FF cambió')
    if memory[0x3000:0x3400] != original:
        raise Failure('0x3000-0x33FF distinto de 0x1000-0x13FF')
    if memory[0x2000:0x2400] == original:
        raise Failure('0x2000-0x23FF igual al original')


def describe(expected):
    return ', '.join('%s = %s' % item for item in expected.items())


def main():
    sim_asm = load_simulator()
    failed = 0
    with tempfile.TemporaryDirectory() as folder:
        directory = Path(folder)
        for name, source, expected in CASES:
            try:
                run_case(sim_asm, directory, name, source, expected)
                print('PASS %s: %s' % (name, describe(expected)))
            except (Failure, sim_asm.AsmError) as error:
                failed += 1
                print('FAIL %s: %s' % (name, error))
        try:
            run_p09(sim_asm, directory)
            print('PASS p09: 0x3000 == 0x1000, 0x2000 distinto, 0x4000-0x4007 == 0x1000-0x1007')
        except (Failure, sim_asm.AsmError) as error:
            failed += 1
            print('FAIL p09: %s' % error)
        try:
            run_p11(sim_asm, directory)
            print('PASS p11: n1 = 8, n2 = 8, 0x3000 == 0x1000, 0x2000 distinto')
        except (Failure, sim_asm.AsmError) as error:
            failed += 1
            print('FAIL p11: %s' % error)
        for name, source, expected, reason in PENDING:
            try:
                run_case(sim_asm, directory, name, source, expected)
                failed += 1
                print('FAIL %s: ya pasa (%s); quitarlo de PENDING' % (name, describe(expected)))
            except (Failure, sim_asm.AsmError) as error:
                print('PENDIENTE %s: %s. Hoy: %s' % (name, reason, error))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
