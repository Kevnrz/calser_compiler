#!/usr/bin/env python3
"""Prueba el módulo C real, con evaluador auxiliar y el simulador de bundles."""
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from sim_asm import Simulator, parse

KEY = [0x0123ABCD, 0x89ABCDEF, 0x13579BDF, 0x2468ACE0]


def reference(data):
    def f(x, key):
        # Rotación mediante cadena de bits para independizarla del simulador.
        bits = f'{x:032b}'
        return ((int(bits[5:] + bits[:5], 2) + key) % (1 << 32)) ^ int(bits[13:] + bits[:13], 2)
    result = bytearray()
    for index in range(0, len(data), 8):
        left = int.from_bytes(data[index:index + 4], 'little')
        right = int.from_bytes(data[index + 4:index + 8], 'little')
        for key in KEY:
            left, right = right, left ^ f(right, key)
        result.extend(left.to_bytes(4, 'little') + right.to_bytes(4, 'little'))
    return result


def check(binary, mode, blocks, key, mask, dynamic, rng):
    command = [binary, mode, str(blocks), str(key), hex(mask)]
    if dynamic:
        command.append('dynamic')
    code = subprocess.check_output(command, text=True)
    program = parse(code)
    sim = Simulator(program)
    data = bytes(rng.getrandbits(8) for _ in range(max(1, blocks) * 8))
    sim.load_data(data)
    sim.run()
    tag = (mode, blocks, key, hex(mask), dynamic)
    assert sim.reason == 'salto a sí mismo' and not sim.warnings, (tag, sim.reason, sim.warnings)
    assert sim.regs[1:4] == [1001, 1002, 1003], (tag, 'rl/rx/rg alterados')
    for reg in range(4, 16):
        if mask & (1 << reg):
            assert sim.regs[reg] == 1000 + reg, (tag, 'temporal vivo alterado', reg, sim.regs[reg])
            saved = int.from_bytes(sim.memory[64 + 4 * (reg - 4):68 + 4 * (reg - 4)], 'little')
            assert saved == 1000 + reg, (tag, 'casilla de temporal incorrecta', reg)
    assert sim.vault[key] == KEY, (tag, 'llave de 32 bits incorrecta')
    assert sim.kv_lock and sim.anchor == 5
    for other in range(4):
        if other != key:
            assert sim.vault[other] == [0] * 4
    if blocks > 0:
        if mode != 'inplace':
            assert sim.memory[0x2000:0x2000 + len(data)] == reference(data), (tag, 'cifrado incorrecto')
        if mode == 'demo':
            assert sim.memory[0x3000:0x3000 + len(data)] == data, (tag, 'descifrado incorrecto')
        elif mode == 'inplace':
            assert sim.memory[0x1000:0x1000 + len(data)] == data, (tag, 'in-place incorrecto')
    else:
        assert sim.memory[0x2000:0x2008] == bytes(8)
        assert sim.memory[0x3000:0x3008] == bytes(8)
    if mode != 'single':
        assert sim.memory[0x4000:0x4008] == data[:8], (tag, 'operación sin sesión alteró datos')
        assert not sim.auth and sim.exc
        names = [name for name in program.labels if name.startswith('.L_crypto_')]
        assert len(names) == 6, (tag, 'faltan etiquetas de expansiones')
    else:
        assert sim.auth and not sim.exc
    return code


def main():
    binary = str(Path(sys.argv[1]).resolve())
    subprocess.run([binary, '--contract'], check=True)
    rng = random.Random(430101)
    count = 0
    for blocks in [-1, 0, 1, 3, 8, 128, 512]:
        for key in range(4):
            for mask in [0, 0x5550, 0xFFF0]:
                for dynamic in [False, True]:
                    check(binary, 'demo', blocks, key, mask, dynamic, rng)
                    count += 1
    for key in range(4):
        for dynamic in [False, True]:
            check(binary, 'single', 8, key, 0xFFF0, dynamic, rng)
            check(binary, 'inplace', 8, key, 0xFFF0, dynamic, rng)
            count += 2
    print(f'PASS {count} casos crypto: referencia Feistel, descifrado, in-place, argumentos dinámicos,')
    print('     constantes de 32 bits, 12 temporales vivos, etiquetas únicas y cero avisos RAW')


if __name__ == '__main__':
    main()
