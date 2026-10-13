#!/usr/bin/env python3
"""Pruebas del parser, los bundles y el pipeline; no son pruebas RTL."""
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from sim_asm import AsmError, Simulator, parse

GAP = '{ nopi | nopi | nopi | nopi }'
END = 'fin: { nopi | nopi | jef fin | nopi }'


def run(*lines, limit=100):
    return Simulator(parse('\n'.join(lines))).run(limit)


class SimulatorTests(unittest.TestCase):
    def test_example(self):
        sim = Simulator(parse(Path(__file__).with_name('ejemplo_r4.asm').read_text())).run()
        self.assertEqual(sim.regs[4], 4)
        self.assertEqual(sim.memory[:4], b'\x04\x00\x00\x00')
        self.assertEqual(sim.warnings, [])
        self.assertEqual(sim.reason, 'salto a sí mismo')

    def test_missing_gap_reads_old_value(self):
        sim = run('{ mov r4, 7 | nopi | nopi | nopi }',
                  '{ add r5, r4, r4 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[4:6], [7, 0])
        self.assertEqual(len(sim.warnings), 1)

    def test_gap_makes_result_visible(self):
        sim = run('{ mov r4, 7 | nopi | nopi | nopi }', GAP,
                  '{ add r5, r4, r4 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[5], 14)
        self.assertEqual(sim.warnings, [])

    def test_slots_read_simultaneously(self):
        sim = run('{ mov r4, 3 | nopi | nopi | nopi }', GAP,
                  '{ saddi r4, r4, 1 | add r5, r4, r4 | nopi | sw r4, 0(rz) }', END)
        self.assertEqual(sim.regs[4:6], [4, 6])
        self.assertEqual(sim.memory[0], 3)
        self.assertEqual(sim.warnings, [])

    def test_taken_jump_completes_bundle_and_discards_next(self):
        sim = run('{ mov r4, 9 | nopi | jef destino | sw rz, 0(rz) }',
                  '{ mov r4, 99 | nopi | nopi | sw r4, 0(rz) }',
                  'destino: { add r5, r4, r4 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[4:6], [9, 18])
        self.assertEqual(sim.memory[0], 0)
        self.assertEqual(sim.warnings, [])

    def test_not_taken_jump_executes_next(self):
        sim = run('{ mov r4, 1 | nopi | nopi | nopi }', GAP,
                  '{ nopi | nopi | beq r4, rz, fin | nopi }',
                  '{ mov r5, 8 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[5], 8)

    def test_call_and_return_link_pc_plus_one(self):
        sim = run('{ nopi | nopi | jas rl, funcion | nopi }',
                  '{ mov r6, 12 | nopi | nopi | nopi }',
                  '{ nopi | nopi | jef fin | nopi }', GAP,
                  'funcion: { mov r4, 5 | nopi | nopi | nopi }',
                  '{ nopi | nopi | jefR rx, rl | nopi }', END)
        self.assertEqual(sim.regs[1], 1)
        self.assertEqual(sim.regs[2], 6)
        self.assertEqual(sim.regs[4], 5)
        self.assertEqual(sim.regs[6], 12)
        self.assertEqual(sim.warnings, [])

    def test_branch_hazard_uses_old_value(self):
        sim = run('{ mov r4, 1 | nopi | nopi | nopi }',
                  '{ nopi | nopi | beq r4, rz, fin | nopi }',
                  '{ mov r5, 99 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[5], 0)
        self.assertEqual(len(sim.warnings), 1)

    def test_relative_numeric_jump(self):
        sim = run('{ nopi | nopi | jef 2 | nopi }',
                  '{ mov r4, 99 | nopi | nopi | nopi }',
                  '{ mov r4, 4 | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[4], 4)

    def test_self_jump_completes_other_slots(self):
        sim = run('fin: { mov r4, 4 | nopi | jef fin | sw rz, 0(rz) }')
        self.assertEqual(sim.regs[4], 4)
        self.assertEqual(sim.steps, 1)

    def test_store_then_load_needs_no_gap(self):
        sim = run('{ mov r4, -1 | nopi | nopi | nopi }', GAP,
                  '{ nopi | nopi | nopi | sw r4, 0(rz) }',
                  '{ nopi | nopi | nopi | lw r5, 0(rz) }',
                  '{ add r6, r5, rz | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[5], 0xFFFFFFFF)
        self.assertEqual(sim.regs[6], 0)
        self.assertEqual(len(sim.warnings), 1)

    def test_little_endian_bytes_sign_extension_and_word_alignment(self):
        sim = run('{ mov r4, 128 | nopi | nopi | nopi }', GAP,
                  '{ nopi | nopi | nopi | sb r4, 1(rz) }',
                  '{ nopi | nopi | nopi | lb r5, 1(rz) }',
                  '{ nopi | nopi | nopi | lw r6, 3(rz) }', END)
        self.assertEqual(sim.regs[5], 0xFFFFFF80)
        self.assertEqual(sim.regs[6], 0x8000)
        self.assertEqual(sim.memory[:4], b'\x00\x80\x00\x00')

    def test_zero_register_discards_writes(self):
        sim = run('{ mov rz, 123 | nopi | nopi | nopi }',
                  '{ add t0, rz, rz | nopi | nopi | nopi }', END)
        self.assertEqual(sim.regs[0], 0)
        self.assertEqual(sim.regs[4], 0)
        self.assertEqual(sim.warnings, [])

    def test_signed_comparisons_and_arithmetic(self):
        sim = run('{ mov r4, -1 | mov r5, 1 | nopi | nopi }', GAP,
                  '{ slt r6, r4, r5 | sle r7, r4, r5 | blt r4, r5, destino | nopi }',
                  '{ mov r6, 99 | nopi | nopi | nopi }',
                  'destino: { add r8, r4, r5 | sub r9, rz, r5 | nopi | nopi }', END)
        self.assertEqual(sim.regs[6:10], [1, 1, 0, 0xFFFFFFFF])

    def test_alu_operations(self):
        cases = {'and': 0, 'or': 0xFFFFFFFF, 'xor': 0xFFFFFFFF,
                 'seq': 0, 'sne': 1, 'srl': 0x40000000,
                 'sra': 0xC0000000, 'sll': 0, 'ifmov': 0x80000000}
        for op, expected in cases.items():
            with self.subTest(op=op):
                sim = run('{ mov r4, 65536 | mov r5, 1 | nopi | nopi }', GAP,
                          '{ mov r6, 15 | nopi | nopi | nopi }', GAP,
                          '{ sll r4, r4, r6 | nopi | nopi | nopi }', GAP,
                          '{ saddi r7, r4, -1 | nopi | nopi | nopi }', GAP,
                          f'{{ {op} r8, r4, {"r5" if op in {"srl", "sra", "sll", "ifmov"} else "r7"} | nopi | nopi | nopi }}', END)
                self.assertEqual(sim.regs[8], expected)
                self.assertEqual(sim.warnings, [])
        sim = run('{ mov r4, 7 | nopi | nopi | nopi }', GAP,
                  '{ ifmov r4, rz, rz | xori r5, rz, -1 | nopi | nopi }', END)
        self.assertEqual(sim.regs[4:6], [7, 0xFFFFFFFF])

    def test_crypto_no_effect_but_checks_dependencies(self):
        sim = run('{ mov t0, 9 | nopi | nopi | nopi }', GAP,
                  '{ nopi | feis t0, t0, rz, 0, 3 | nopi | nopi }',
                  '{ add t1, t0, rz | auth t0 | nopi | nopi }',
                  '{ nopi | kv_write t0, 0, 2 | nopi | nopi }',
                  '{ nopi | kv_lock | nopi | nopi }',
                  '{ nopi | sanc t0 | nopi | nopi }',
                  '{ nopi | logout | nopi | nopi }', END)
        self.assertEqual(sim.regs[4], 9)
        self.assertEqual(sim.regs[5], 9)
        self.assertEqual(sim.crypto_count, 6)
        self.assertEqual(len(sim.warnings), 2)

    def test_limit_and_eof_flush_results(self):
        sim = run('{ mov r4, 7 | nopi | nopi | nopi }')
        self.assertEqual(sim.regs[4], 7)
        self.assertEqual(sim.reason, 'fin del archivo')
        sim = run('inicio: ' + GAP, '{ nopi | nopi | jef inicio | nopi }', limit=5)
        self.assertEqual(sim.steps, 5)
        self.assertEqual(sim.reason, 'límite de bundles')

    def test_labels_comments_and_aliases(self):
        program = parse('# comentario\na: b: ' + GAP + ' // comentario\n' + END)
        self.assertEqual(program.labels['a'], program.labels['b'])
        sim = run('{ mov t11, 0x7 | mov rg, -0x1 | nopi | nopi }', END)
        self.assertEqual(sim.regs[15], 7)
        self.assertEqual(sim.regs[3], 0xFFFFFFFF)

    def test_rejects_malformed_input(self):
        cases = ['', '{ nopi | nopi }', '{ | nopi | nopi | nopi }',
                 '{ lw r4, 0(rz) | nopi | nopi | nopi }',
                 '{ mov r4, 1 | mov r4, 2 | nopi | nopi }',
                 '{ mov r16, 1 | nopi | nopi | nopi }',
                 '{ mov r4, 131072 | nopi | nopi | nopi }',
                 '{ nopi | feis r4, r5, r6, 4, 0 | nopi | nopi }',
                 '{ nopi | kv_write r4, 0, -1 | nopi | nopi }',
                 '{ nopi | nopi | jef desconocida | nopi }',
                 '{ nopi | nopi | jef 2097152 | nopi }',
                 '{ nopi | nopi | nopi | lw r4, 0(r16) }',
                 '{ wat r4, 0 | nopi | nopi | nopi }',
                 'a: ' + GAP + '\na: ' + GAP]
        for text in cases:
            with self.subTest(text=text), self.assertRaises(AsmError):
                parse(text)

    def test_out_of_range_memory_and_jump(self):
        with self.assertRaises(AsmError):
            run('{ nopi | nopi | nopi | lw r4, -4(rz) }')
        with self.assertRaises(AsmError):
            run('{ nopi | nopi | jef 3 | nopi }')
        with self.assertRaises(AsmError):
            run(GAP, limit=0)

    def test_cli_example_and_error_exit_codes(self):
        script = ROOT / 'tools/sim_asm.py'
        example = Path(__file__).with_name('ejemplo_r4.asm')
        result = subprocess.run([sys.executable, str(script), str(example)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('r4 = 4', result.stdout)
        self.assertIn('Avisos de separación: 0', result.stdout)
        result = subprocess.run([sys.executable, str(script), str(example), '--max-bundles', '1'], capture_output=True)
        self.assertEqual(result.returncode, 1)
        result = subprocess.run([sys.executable, str(script), str(example), '--max-bundles', '0'], capture_output=True)
        self.assertEqual(result.returncode, 2)


if __name__ == '__main__':
    unittest.main()
