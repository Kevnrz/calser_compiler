#!/usr/bin/env python3
"""Recorrido real .cls -> ccom -> .asm/.mem -> simulador, sin evaluador auxiliar."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from sim_asm import Simulator, parse


def compile_run(source, data, directory, name):
    source_file = directory / (name + '.cls')
    source_file.write_text(source)
    output = directory / (name + '.bin')
    result = subprocess.run([str(ROOT / 'ccom'), '-s', '-m', str(source_file), '-o', str(output)],
                            capture_output=True, text=True)
    assert result.returncode == 0, (name, result.stderr)
    sim = Simulator(parse(output.with_suffix('.asm').read_text()))
    sim.load_data_image(output.with_suffix('.mem').read_text())
    sim.load_data(data)
    sim.run()
    assert sim.reason == 'salto a sí mismo' and not sim.warnings, (name, sim.reason, sim.warnings)
    return sim, result.stdout, output


def main():
    original_program = (ROOT / 'pruebas/p09_crypto.cls').read_text()
    data = bytes(range(256)) * 4
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        for name, source, blocks in [
            ('p09', original_program, 128),
            ('expresiones', original_program.replace('128', '4 * 2').replace(
                'cifrar [ origen , cifrado', 'cifrar [ origen + 0 , cifrado + 0'), 8),
            ('sin_bloques', original_program.replace('128', '0'), 0),
            ('bloques_negativos', original_program.replace('128', '-1'), 0),
            ('global_dinamica', original_program.replace(
                'Vi origen: int = 4096 ;', 'Vm base: int = 4096 ;\nVm origen: int = base + 0 ;'), 128),
        ]:
            sim, _, output = compile_run(source, data, directory, name)
            size = blocks * 8
            if blocks:
                assert sim.memory[0x2000:0x2000 + size] != data[:size], name
                assert sim.memory[0x3000:0x3000 + size] == data[:size], name
            else:
                assert not any(sim.memory[0x2000:0x2400]), name
                assert not any(sim.memory[0x3000:0x3400]), name
            assert sim.memory[0x4000:0x4008] == data[:8], name
            assert not sim.auth and sim.kv_lock and sim.exc
            if name == 'p09':
                payload = directory / 'datos.bin'
                payload.write_bytes(data)
                dump = directory / 'final.bin'
                result = subprocess.run([sys.executable, str(ROOT / 'tools/sim_asm.py'),
                    str(output.with_suffix('.asm')), '--data-image', str(output.with_suffix('.mem')),
                    '--load-data', str(payload), '--dump-memory', str(dump)], capture_output=True, text=True)
                assert result.returncode == 0, result.stderr
                assert dump.read_bytes() == sim.memory
            print(f'PASS integración {name}: .cls, .asm, .mem, datos y cero avisos RAW')
        source = '''Vi minimo: int = -2147483648 ;
Vi maximo: int = 2147483647 ;
Vi lista: int [2][2] = [[1, 2], [3, 4]] ;
F main [] : void ;
    Vm a: int = minimo ;
    Vm b: int = maximo ;
    Vm c: int = -1985229329 ;
    Vm d: int ;
    E ;
.
'''
        sim, tables, _ = compile_run(source, data, directory, 'constantes')
        for name, expected in [('minimo', 0x80000000), ('maximo', 0x7FFFFFFF),
                               ('a', 0x80000000), ('b', 0x7FFFFFFF), ('c', 0x89ABCDEF), ('d', 0)]:
            match = re.search(rf'^    {name} : int.*addr=0x([0-9A-F]+)', tables, re.M)
            assert match, (name, tables)
            address = int(match[1], 16)
            assert int.from_bytes(sim.memory[address:address + 4], 'little') == expected, name
        match = re.search(r'^    lista : int\[2\]\[2\].*addr=0x([0-9A-F]+)', tables, re.M)
        address = int(match[1], 16)
        assert sim.memory[address:address + 16] == b''.join(i.to_bytes(4, 'little') for i in [1,2,3,4])
        print('PASS integración constantes: INT32_MIN/MAX, inicializadores locales, cero y matriz global')
        # Un error de escritura debe propagarse como fallo de ccom.
        src = directory / 'p09.cls'
        result = subprocess.run([str(ROOT / 'ccom'), '-s', str(src), '-o', str(directory / 'no-existe/out.bin')],
                                capture_output=True)
        assert result.returncode == 1
        print('PASS error de salida: no informa éxito si no puede escribir .asm/.mem')


if __name__ == '__main__':
    main()
