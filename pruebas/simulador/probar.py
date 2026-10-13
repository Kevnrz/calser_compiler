#!/usr/bin/env python3
"""Pruebas del parser, los bundles y el pipeline; no son pruebas RTL."""
from pathlib import Path
import subprocess
import sys
import random
import tempfile
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

    def test_official_isa_example_conditional_self_branch(self):
        sim = Simulator(parse(Path(__file__).with_name('ejemplo_isa.asm').read_text())).run()
        self.assertEqual(sim.regs[4], 4)
        self.assertEqual(sim.reason, 'fin del archivo')
        self.assertEqual(sim.warnings, [])
        self.assertEqual((sim.steps, sim.cycles), (6, 9))

    def test_register_jump_uses_low_22_bits(self):
        sim = Simulator(parse('{ nopi | nopi | jefR rz, rl | nopi }\n' + END))
        sim.regs[1] = (1 << 22) | 1
        sim.run()
        self.assertEqual(sim.reason, 'salto a sí mismo')
        self.assertEqual(sim.warnings, [])

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

    def test_unauthorized_crypto_has_no_effect(self):
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
        self.assertEqual(len(sim.warnings), 0)
        self.assertFalse(sim.exc)

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


# Plantillas de docs/crypto_para_ccom.md §4; etiquetas distintas por expansión.
def crypto_template(origin, destination, blocks, key=0, decrypt=False, name='cifra'):
    order = range(3, -1, -1) if decrypt else range(4)
    lo, hi = (4, 0) if decrypt else (0, 4)
    lines = [f'{{ mov t4, 0 | mov t5, {blocks} | nopi | nopi }}',
             f'{{ mov t2, {origin} | mov t3, {destination} | nopi | nopi }}',
             f'{{ nopi | nopi | bge rz, t5, fin_{name} | nopi }}',
             f'{name}: {{ nopi | nopi | nopi | lw t0, {lo}(t2) }}',
             f'{{ nopi | nopi | nopi | lw t1, {hi}(t2) }}',
             '{ saddi t2, t2, 8 | nopi | nopi | nopi }']
    for i, word in enumerate(order):
        dest, other = ('t0', 't1') if i % 2 == 0 else ('t1', 't0')
        lines.append(f'{{ nopi | feis {dest}, {dest}, {other}, {key}, {word} | nopi | nopi }}')
        if i < 3:
            lines.append('{ saddi t4, t4, 1 | nopi | nopi | nopi }' if i == 0 else GAP)
    lines.extend([f'{{ nopi | nopi | nopi | sw t0, {lo}(t3) }}',
                  f'{{ saddi t3, t3, 8 | nopi | blt t4, t5, {name} | sw t1, {hi}(t3) }}',
                  f'fin_{name}:'])
    return '\n'.join(lines)


def reference_f(value, key):
    # Referencia independiente: rotación de la cadena de bits, no shifts.
    bits = f'{value:032b}'
    rot5 = int(bits[5:] + bits[:5], 2)
    rot13 = int(bits[13:] + bits[:13], 2)
    return ((rot5 + key) % (1 << 32)) ^ rot13


def reference_encrypt(data, key):
    result = bytearray()
    for address in range(0, len(data), 8):
        left = int.from_bytes(data[address:address + 4], 'little')
        right = int.from_bytes(data[address + 4:address + 8], 'little')
        for word in key:
            left, right = right, left ^ reference_f(right, word)
        result.extend(left.to_bytes(4, 'little') + right.to_bytes(4, 'little'))
    return result


class CryptoTests(unittest.TestCase):
    def test_feistel_matches_independent_reference(self):
        rng = random.Random(4301)
        program = parse('\n'.join([
            '{ nopi | feis r6, r4, r5, 0, 0 | nopi | nopi }', GAP, END]))
        for _ in range(100):
            left, right, key = (rng.getrandbits(32) for _ in range(3))
            sim = Simulator(program)
            sim.auth = True
            sim.vault[0][0] = key
            sim.regs[4:6] = [left, right]
            sim.run()
            self.assertEqual(sim.regs[6], left ^ reference_f(right, key))
            self.assertEqual(sim.warnings, [])

    def test_authorized_feistel_keeps_raw_latency(self):
        sim = Simulator(parse('\n'.join([
            '{ nopi | feis r6, r4, r5, 0, 0 | nopi | nopi }',
            '{ add r7, r6, rz | nopi | nopi | nopi }', END])))
        sim.auth = True
        sim.regs[4:6] = [123, 456]
        sim.run()
        self.assertNotEqual(sim.regs[6], 0)
        self.assertEqual(sim.regs[7], 0)
        self.assertEqual(len(sim.warnings), 1)

    def test_auth_success_failure_and_exception_clear(self):
        for token, success in [(0, True), (1234567, True), (999, False), (0xFFFFFFFF, True)]:
            with self.subTest(token=token):
                sim = Simulator(parse('{ nopi | auth t0 | nopi | nopi }\n' + END))
                sim.regs[4] = token
                sim.exc = True
                sim.anchor = 42
                sim.run()
                self.assertEqual(sim.auth, success)
                self.assertEqual(sim.exc, not success)
                self.assertEqual(sim.anchor, token if success else 42)
                self.assertEqual(sim.cycles, 16)  # auth 14 + fin 1 + descarte 1

    def test_all_vault_words_write_and_are_available_next_bundle(self):
        for key in range(4):
            for word in range(4):
                with self.subTest(key=key, word=word):
                    sim = Simulator(parse('\n'.join([
                        f'{{ nopi | kv_write r4, {key}, {word} | nopi | nopi }}',
                        f'{{ nopi | feis r6, rz, rz, {key}, {word} | nopi | nopi }}', END])))
                    sim.auth = True
                    sim.regs[4] = 0x89ABCDEF
                    sim.run()
                    self.assertEqual(sim.vault[key][word], 0x89ABCDEF)
                    self.assertEqual(sim.regs[6], 0x89ABCDEF)
                    self.assertEqual(sim.warnings, [])

    def test_lock_survives_logout_and_reauthentication(self):
        sim = Simulator(parse('\n'.join([
            '{ nopi | kv_write r4, 0, 0 | nopi | nopi }',
            '{ nopi | kv_lock | nopi | nopi }',
            '{ nopi | logout | nopi | nopi }',
            '{ nopi | auth r5 | nopi | nopi }',
            '{ nopi | kv_write r5, 0, 0 | nopi | nopi }', END])))
        sim.auth = True
        sim.regs[4:6] = [10, 20]
        sim.run()
        self.assertTrue(sim.kv_lock)
        self.assertTrue(sim.auth)
        self.assertTrue(sim.exc)
        self.assertEqual(sim.vault[0][0], 10)
        # Un nuevo simulador representa reset y no conserva el sello o las llaves.
        reset = Simulator(sim.program)
        self.assertFalse(reset.kv_lock)
        self.assertEqual(reset.vault, [[0] * 4 for _ in range(4)])

    def test_unauthorized_operations_retain_state_and_set_exception(self):
        for operation in ['feis r4, r5, r6, 0, 0', 'kv_write r4, 0, 0', 'kv_lock', 'sanc r4']:
            with self.subTest(operation=operation):
                sim = Simulator(parse(f'{{ nopi | {operation} | nopi | nopi }}\n' + END))
                sim.regs[4:7] = [7, 8, 9]
                sim.anchor = 42
                sim.run()
                self.assertTrue(sim.exc)
                self.assertFalse(sim.kv_lock)
                self.assertEqual(sim.regs[4:7], [7, 8, 9])
                self.assertEqual(sim.anchor, 42)
                self.assertEqual(sim.vault, [[0] * 4 for _ in range(4)])

    def test_sanc_requires_auth_and_logout_does_not_clear_exception(self):
        sim = Simulator(parse('\n'.join([
            '{ nopi | sanc r4 | nopi | nopi }',
            '{ nopi | logout | nopi | nopi }', END])))
        sim.auth = sim.exc = True
        sim.regs[4] = 123
        sim.run()
        self.assertEqual(sim.anchor, 123)
        self.assertFalse(sim.auth)
        self.assertTrue(sim.exc)

    def test_same_bundle_reads_pre_crypto_registers(self):
        sim = Simulator(parse('{ mov r4, 99 | auth r4 | nopi | nopi }\n' + END))
        sim.regs[4] = 999
        sim.run()
        self.assertFalse(sim.auth)
        self.assertEqual(sim.regs[4], 99)

    def test_templates_encrypt_decrypt_and_denied_session(self):
        rng = random.Random(4301)
        keys = [0x0123ABCD, 0x89ABCDEF, 0x13579BDF, 0x2468ACE0]
        for blocks in [-1, 0, 1, 3, 8, 128, 512]:
            for key in range(4):
                with self.subTest(blocks=blocks, key=key):
                    count = max(1, blocks)
                    data = bytes(rng.getrandbits(8) for _ in range(count * 8))
                    code = '\n'.join([
                        crypto_template(0x1000, 0x2000, blocks, key, name='cifra_1'),
                        crypto_template(0x2000, 0x3000, blocks, key, True, 'descifra_2'),
                        '{ nopi | logout | nopi | nopi }',
                        crypto_template(0x1000, 0x4000, 1, key, name='cifra_3'), END])
                    sim = Simulator(parse(code))
                    sim.auth = True
                    sim.vault[key] = keys.copy()
                    sim.load_data(data)
                    sim.run()
                    if blocks > 0:
                        self.assertEqual(sim.memory[0x2000:0x2000 + len(data)], reference_encrypt(data, keys))
                        self.assertNotEqual(sim.memory[0x2000:0x2000 + len(data)], data)
                        self.assertEqual(sim.memory[0x3000:0x3000 + len(data)], data)
                    else:
                        self.assertEqual(sim.memory[0x2000:0x2008], bytes(8))
                        self.assertEqual(sim.memory[0x3000:0x3008], bytes(8))
                    self.assertEqual(sim.memory[0x4000:0x4008], data[:8])
                    self.assertTrue(sim.exc)
                    self.assertEqual(sim.warnings, [])

    def test_templates_in_place(self):
        data = bytes(range(64))
        sim = Simulator(parse('\n'.join([
            crypto_template(0x1000, 0x1000, 8, name='cifra_1'),
            crypto_template(0x1000, 0x1000, 8, decrypt=True, name='descifra_2'), END])))
        sim.auth = True
        sim.vault[0] = [1, 2, 3, 4]
        sim.load_data(data)
        sim.run()
        self.assertEqual(sim.memory[0x1000:0x1040], data)
        self.assertEqual(sim.warnings, [])

    def test_demo_fixture(self):
        sim = Simulator(parse(Path(__file__).with_name('ejemplo_crypto.asm').read_text()))
        data = bytes(range(256)) * 4
        sim.load_data(data)
        sim.run()
        self.assertEqual(sim.memory[0x2000:0x2400], reference_encrypt(data, sim.vault[0]))
        self.assertEqual(sim.memory[0x3000:0x3400], data)
        self.assertEqual(sim.memory[0x4000:0x4008], data[:8])
        self.assertEqual(sim.vault[0], [0x0123ABCD, 0x89ABCDEF, 0x13579BDF, 0x2468ACE0])
        self.assertTrue(sim.kv_lock)
        self.assertTrue(sim.exc)
        self.assertFalse(sim.auth)
        self.assertEqual(sim.warnings, [])

    def test_static_data_image(self):
        sim = Simulator(parse(END))
        sim.load_data_image('@0\n00001000\n89ABCDEF\n')
        self.assertEqual(sim.memory[:8], b'\x00\x10\x00\x00\xef\xcd\xab\x89')
        for text in ['', '@1\n00000000', '@0\nZZZZZZZZ', '@0\n@1', '@0\n1',
                     '@0\n' + '00000000\n' * 1025]:
            with self.subTest(text=text[:30]), self.assertRaises(AsmError):
                sim.load_data_image(text)

    def test_load_data_bounds(self):
        sim = Simulator(parse(END))
        sim.load_data(b'abc', 0xFFFF - 2)
        self.assertEqual(sim.memory[-3:], b'abc')
        for address, data in [(0, b'a'), (0x10000, b'a'), (0xFFFF, b'ab'), (-1, b'a')]:
            with self.subTest(address=address), self.assertRaises(AsmError):
                sim.load_data(data, address)
        self.assertEqual(len(sim.memory), 65536)

    def test_cli_load_dump_and_errors(self):
        script = ROOT / 'tools/sim_asm.py'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            asm, data, dump = root / 'test.asm', root / 'data.bin', root / 'mem.bin'
            asm.write_text(END)
            data.write_bytes(bytes(range(16)))
            command = [sys.executable, str(script), str(asm), '--load-data', str(data),
                       '--data-address', '0x1000', '--dump-memory', str(dump)]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(len(dump.read_bytes()), 65536)
            self.assertEqual(dump.read_bytes()[0x1000:0x1010], data.read_bytes())
            for arguments in [['--data-address', '0x2000'], ['--data-address', 'invalid'],
                              ['--load-data', str(data), '--data-address', '0xFFFF'],
                              ['--load-data', str(data), '--dump-memory', str(data)],
                              ['--dump-memory', str(asm)]]:
                result = subprocess.run([sys.executable, str(script), str(asm)] + arguments,
                                        capture_output=True)
                self.assertEqual(result.returncode, 2)
            self.assertEqual(data.read_bytes(), bytes(range(16)))
            self.assertEqual(asm.read_text(), END)


if __name__ == '__main__':
    unittest.main()
