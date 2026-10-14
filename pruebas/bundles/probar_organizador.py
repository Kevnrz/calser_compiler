#!/usr/bin/env python3
"""Compara ejecución escalar y organizada: registros, variables y datos crypto."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
from sim_asm import Simulator, parse, ALU_R, Annotations, Tracer
ALU = ALU_R | {'mov', 'saddi', 'xori'}

class MeasuringSimulator(Simulator):
    def __init__(self, program):
        super().__init__(program)
        self.dual_executed = 0
    def step(self):
        if all(op.mnemonic in ALU for op in self.program.bundles[self.pc].slots[:2]):
            self.dual_executed += 1
        return super().step()

def simulate(asm, mem=None, data=None):
    program = parse(asm.read_text())
    sim = MeasuringSimulator(program)
    if mem: sim.load_data_image(mem.read_text())
    if data is not None: sim.load_data(data)
    sim.run(1000000)
    assert not sim.warnings, (asm, sim.warnings)
    assert sim.reason == 'salto a sí mismo', (asm, sim.reason)
    return sim

def compile(source, base, scalar=False):
    cmd = [str(ROOT/'ccom'), '-s', '-m', str(source), '-o', str(base.with_suffix('.bin'))]
    if scalar: cmd.append('--no-schedule')
    return subprocess.run(cmd, capture_output=True, text=True)

def slots_used(program):
    return [sum(op.mnemonic != 'nopi' for op in b.slots) for b in program.bundles]

rejected = subprocess.run([str(ROOT/'build/probar_dependencias'), '--reject'], capture_output=True, text=True)
assert rejected.returncode == 0 and not rejected.stdout and 'entrada invalida' in rejected.stderr
print('PASS calendario rechazado: diagnóstico y ningún callback de emisión ejecutado')

with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    subprocess.run([str(ROOT/'build/emitir_organizador'), str(directory)], check=True)
    count = 0
    for scalar in sorted(directory.glob('*.scalar.asm')):
        name = scalar.name.removesuffix('.scalar.asm')
        scheduled = directory/(name+'.scheduled.asm')
        if name in {'numeric_fallback', 'unknown_fallback'}:
            assert scheduled.read_bytes() == scalar.read_bytes(), name
            if name == 'unknown_fallback': continue
        organized = simulate(scheduled)
        if name in {'fixed_entry', 'fixed_state_entry'}:
            # Entrada tras un bundle fijo sin gap: se introduce separación.
            original = MeasuringSimulator(parse(scalar.read_text())); original.run()
            assert original.warnings
            if name == 'fixed_entry': assert organized.regs[5] == 6
            else: assert not organized.auth and organized.exc
            continue
        original = simulate(scalar)
        assert original.regs == organized.regs, name
        assert original.memory == organized.memory, name
        assert (original.auth, original.kv_lock, original.exc, original.crypto_count) == (
            organized.auth, organized.kv_lock, organized.exc, organized.crypto_count), name
        if name == 'long_listing': assert any(len(line)>512 for line in scheduled.read_text().splitlines())
        if name == 'four_slots': assert 4 in slots_used(organized.program)
        if name == 'fixed_bundle':
            assert any(tuple(op.mnemonic for op in b.slots) == ('mov','mov','nopi','nopi')
                       for b in organized.program.bundles)
        count += 1
    print(f'PASS organizador: {count} ejecuciones equivalentes, fallback y protección de entrada fija')
    branch_text = (directory/'branch_taken.scheduled.asm').read_text()
    branch_program = parse(branch_text)
    notes = Annotations(branch_text, len(branch_program.bundles))
    mixed = [i for i,b in enumerate(branch_program.bundles)
             if b.slots[2].mnemonic == 'bne' and b.slots[3].mnemonic == 'sw']
    assert mixed and notes.slot_lines[mixed[0]][2] == 13 and notes.slot_lines[mixed[0]][3] == 12
    branch_text += '\n# Variables (direccion de datos)\n#   Scope 1: FUNCTION main\n#       x : int width=4 addr=0x0000 line=12\n'
    traced = MeasuringSimulator(branch_program)
    traced.tracer = Tracer(Annotations(branch_text,len(branch_program.bundles)),branch_program.labels,1000)
    traced.run()
    assert any('línea 12]' in event and 'x = 1' in event for event in traced.tracer.events)
    assert any('línea 13]' in event and 'bne →' in event for event in traced.tracer.events)
    print('PASS anotaciones: almacenamiento y salto comparten bundle y conservan sus líneas individuales')

    # Todos los .cls compilables, incluidos los casos reales de llamadas,
    # arreglos, cortocircuito y crypto. No comparar casillas internas de enlaces:
    # sus valores son direcciones de código que cambian con el empaquetado.
    compared = 0
    data = bytes(range(256))*4
    for source in sorted((ROOT/'pruebas').rglob('*.cls')):
        base = directory/f'programa_{compared}'
        scalar_base = directory/f'programa_{compared}_scalar'
        old = compile(source, scalar_base, scalar=True)
        new = compile(source, base)
        assert old.returncode == new.returncode, (source,new.stderr)
        if old.returncode: continue
        a = simulate(scalar_base.with_suffix('.asm'),scalar_base.with_suffix('.mem'),data)
        b = simulate(base.with_suffix('.asm'),base.with_suffix('.mem'),data)
        assert scalar_base.with_suffix('.mem').read_bytes() == base.with_suffix('.mem').read_bytes()
        variables = re.findall(r'^    \w+ : (?:int|bool)(?:\[\d+\])*(?: \((?:return|parameter)\))? width=(\d+) addr=0x([0-9A-F]+)',old.stdout,re.M)
        for width,address in variables:
            start=int(address,16);end=start+int(width)
            assert a.memory[start:end] == b.memory[start:end], (source,start)
        assert a.memory[0x1000:] == b.memory[0x1000:],source
        assert (a.auth,a.kv_lock,a.exc,a.crypto_count) == (b.auth,b.kv_lock,b.exc,b.crypto_count),source
        if source.name == 'p02_factorial.cls':
            assert b.dual_executed>0 and b.cycles<a.cycles
            assert any(bu.slots[0].mnemonic=='sll' and bu.slots[1].mnemonic=='srl'
                       for bu in b.program.bundles)
            address=int(re.search(r'^    f : int.*addr=0x([0-9A-F]+)',new.stdout,re.M)[1],16)
            assert int.from_bytes(b.memory[address:address+4],'little') == 120
            print(f'PASS factorial: ciclos {a.cycles} → {b.cycles}; bundles {len(a.program.bundles)} → {len(b.program.bundles)}; dos ALU ejecutadas {b.dual_executed} veces; f=120')
        compared+=1
    print(f'PASS compilador: {compared} programas equivalentes con y sin organizador, cero avisos')
