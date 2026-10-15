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
from sim_asm import Annotations, AsmError, Simulator, Tracer, auth_hash32, parse

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
        self.assertTrue(sim.exc)
        self.assertFalse(sim.auth)
        self.assertFalse(sim.kv_lock)
        self.assertEqual(sim.anchor, 0xFEFF95FF)
        self.assertEqual(sim.vault, [[0] * 4 for _ in range(4)])

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

    def test_auth_hash_external_vectors(self):
        # Valores de la referencia C ascon/ascon-c, conservados en
        # vliw-block-cipher/tb/reference; no se generan con auth_hash32.
        vectors = [(0x03020100, 0xEDC7E4D7), (0, 0x8F28B790),
                   (1, 0xEC267D58), (0x80000000, 0xF5C202DC),
                   (0x80000001, 0xFEFEC6C6), (0x12345678, 0x88035407),
                   (0xFFFFFFFF, 0x8A6AD37E), (0xA5A55A5A, 0xD020C92A),
                   (0xF47BD70B, 0xFEFF95FF), (0xA45326BA, 0xF47BD70B)]
        for token, expected in vectors:
            with self.subTest(token=hex(token)):
                self.assertEqual(auth_hash32(token), expected)
        for invalid in (-1, 1 << 32):
            with self.assertRaises(ValueError):
                auth_hash32(invalid)

    def test_auth_success_failure_and_exception_clear(self):
        for token, success in [(0xF47BD70B, True), (0, False), (1234567, False),
                               (999, False), (0xFFFFFFFF, False), (0xA45326BA, False)]:
            with self.subTest(token=token):
                sim = Simulator(parse('{ nopi | auth t0 | nopi | nopi }\n' + END))
                sim.regs[4] = token
                sim.auth = sim.exc = True
                sim.run()
                self.assertEqual(sim.auth, success)
                self.assertEqual(sim.exc, not success)
                self.assertEqual(sim.anchor, token if success else 0xFEFF95FF)
                self.assertEqual(sim.cycles, 16)  # auth 14 + fin 1 + descarte 1

    def test_lamport_consumption_logout_replay_and_next_token(self):
        sim = Simulator(parse('\n'.join([
            '{ nopi | auth r4 | nopi | nopi }',
            '{ nopi | logout | nopi | nopi }',
            '{ nopi | auth r4 | nopi | nopi }',
            '{ nopi | auth r5 | nopi | nopi }', END])))
        sim.regs[4:6] = [0xF47BD70B, 0xA45326BA]
        sim.step()
        self.assertEqual((sim.auth, sim.exc, sim.anchor), (True, False, 0xF47BD70B))
        sim.step()
        self.assertEqual((sim.auth, sim.anchor), (False, 0xF47BD70B))
        sim.step()
        self.assertEqual((sim.auth, sim.exc, sim.anchor), (False, True, 0xF47BD70B))
        sim.step()
        self.assertEqual((sim.auth, sim.exc, sim.anchor), (True, False, 0xA45326BA))
        self.assertFalse(sim.warnings)

    def test_sanc_renews_chain_and_999_has_no_special_treatment(self):
        # H(999) = 0xA6FB3D15, calculado por tools/lamport.py del hardware.
        sim = Simulator(parse('\n'.join([
            '{ nopi | auth r4 | nopi | nopi }',
            '{ nopi | sanc r5 | nopi | nopi }',
            '{ nopi | logout | nopi | nopi }',
            '{ nopi | auth r6 | nopi | nopi }', END])))
        sim.regs[4:7] = [0xF47BD70B, 0xA6FB3D15, 999]
        sim.run()
        self.assertEqual((sim.auth, sim.exc, sim.anchor), (True, False, 999))
        self.assertFalse(sim.warnings)

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
        sim.regs[4:6] = [10, 0xF47BD70B]
        sim.run()
        self.assertTrue(sim.kv_lock)
        self.assertTrue(sim.auth)
        self.assertTrue(sim.exc)
        self.assertEqual(sim.vault[0][0], 10)
        # Una nueva instancia representa arranque de fabrica, no reset normal.
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
        sim.regs[4] = 0xF47BD70B
        sim.run()
        self.assertTrue(sim.auth)
        self.assertEqual(sim.anchor, 0xF47BD70B)
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


def annotated(entries, variables):
    """Arma un .asm con las anotaciones que escribe ccom -s (enunciado CE1108
    §6.2): ('label', nombre) o (bundle, línea). La línea es un entero, '-' o
    None (sin comentario de línea)."""
    lines = ['# ccom: demo.cls']
    index = 0
    for entry in entries:
        if entry[0] == 'label':
            lines.append(f'{entry[1]}:   # bundle {index}')
            continue
        text, line = entry
        note = f'# {index:04d}' + (f'  linea {line}' if line is not None else '')
        lines.append(f'  {text}   {note}')
        index += 1
    lines += ['', '# Variables (direccion de datos)', '#',
              '#   ===== SYMBOL TABLE =====', '#   Scope 0: GLOBAL', '#']
    lines += variables
    return '\n'.join(lines) + '\n'


# Un programa pequeño con su traza esperada exacta. Variables (ccom -s los
# escribe así): x en 0, ok en 4, v[3] en 8, r (retorno de f) en 20.
TRACE_VARIABLES = [
    '#   Scope 1: FUNCTION f',
    '#       r : int (return) width=4 addr=0x0014 line=9',
    '#',
    '#   Scope 2: FUNCTION main',
    '#       x : int width=4 addr=0x0000 line=1',
    '#       ok : bool width=4 addr=0x0004 line=2',
    '#       v : int[3] width=12 addr=0x0008 line=3',
    '#',
    '#   =======================',
]

TRACE_PROGRAM = [
    ('{ mov t0, 7 | nopi | nopi | nopi }', 1),                          # 0
    (GAP, None),                                                        # 1
    ('{ nopi | nopi | nopi | sw t0, 0(rz) }', None),                    # 2
    (GAP, None),                                                        # 3
    ('{ mov t1, 1 | nopi | nopi | nopi }', 2),                          # 4
    (GAP, None),                                                        # 5
    ('{ nopi | nopi | nopi | sw t1, 4(rz) }', None),                    # 6
    (GAP, None),                                                        # 7
    ('{ mov t2, 9 | nopi | nopi | nopi }', 3),                          # 8
    (GAP, None),                                                        # 9
    ('{ nopi | nopi | nopi | sw t2, 16(rz) }', None),                   # 10
    (GAP, None),                                                        # 11
    # Fuera de las variables (pila, casilla oculta del S): no se muestra.
    ('{ nopi | nopi | nopi | sw t2, 24(rz) }', None),                   # 12
    (GAP, None),                                                        # 13
    ('{ nopi | nopi | jas rl, .L_func_f | nopi }', 4),                  # 14
    (GAP, None),                                                        # 15
    # Salto hacia una rutina interna: no se muestra.
    ('{ nopi | nopi | jef __ccom_arith_0_end | nopi }', 5),             # 16
    (GAP, None),                                                        # 17
    ('label', '__ccom_arith_0_end'),
    ('{ mov t3, 2 | nopi | nopi | nopi }', None),                       # 18
    (GAP, None),                                                        # 19
    # Salto condicional no tomado: no se muestra.
    ('{ nopi | nopi | beq t3, t0, .L_never | nopi }', 6),               # 20
    (GAP, None),                                                        # 21
    ('{ nopi | nopi | bne t3, t0, .L_next | nopi }', 7),                 # 22
    (GAP, None),                                                        # 23
    ('label', '.L_never'),
    ('{ nopi | nopi | jef fin_main | nopi }', None),                    # 24
    (GAP, None),                                                        # 25
    ('label', '.L_next'),
    ('{ nopi | nopi | jef fin_main | nopi }', 8),                       # 26
    (GAP, None),                                                        # 27
    ('label', 'fin_main'),
    ('{ nopi | nopi | jef fin_main | nopi }', '-'),                     # 28
    ('label', '.L_func_f'),
    ('{ nopi | nopi | nopi | sw rz, 20(rz) }', 9),                      # 29
    (GAP, None),                                                        # 30
    ('{ nopi | nopi | jefR rz, rl | nopi }', 10),                       # 31
]

TRACE_EXPECTED = [
    '[bundle 0002 · línea 1] x = 7',
    '[bundle 0006 · línea 2] ok = verdadero',
    '[bundle 0010 · línea 3] v[2] = 9',
    '[bundle 0014 · línea 4] llamada → .L_func_f (bundle 29)',
    '[bundle 0029 · línea 9] r = 0  (prólogo)',
    '[bundle 0031 · línea 10] retorno → bundle 15',
    '[bundle 0022 · línea 7] bne → .L_next (bundle 26)',
    '[bundle 0026 · línea 8] jef → fin_main (bundle 28)',
    '[bundle 0028] jef → fin_main (bundle 28)',
]


def traced(text, limit=5000):
    program = parse(text)
    sim = Simulator(program)
    sim.tracer = Tracer(Annotations(text, len(program.bundles)), program.labels, limit)
    sim.run()
    return sim, sim.tracer.text(sim)


class TraceTests(unittest.TestCase):
    def test_exact_trace_of_a_small_program(self):
        text = annotated(TRACE_PROGRAM, TRACE_VARIABLES)
        sim, trace = traced(text)
        self.assertEqual(sim.warnings, [])
        events = [line for line in trace.splitlines()
                  if line.startswith('[bundle')]
        self.assertEqual(events, TRACE_EXPECTED)
        self.assertIn('\nEstado final de las variables:\n'
                      '  x = 7\n  ok = verdadero\n  v = [0, 0, 9]\n  r = 0\n', trace)
        # La cabecera documenta las dos reglas de lo que no se muestra.
        self.assertIn('la pila de llamadas (rx)', trace)
        self.assertIn('hacia las rutinas internas', trace)

    def test_trace_does_not_change_the_execution(self):
        text = annotated(TRACE_PROGRAM, TRACE_VARIABLES)
        plain = Simulator(parse(text)).run()
        sim, _ = traced(text)
        self.assertEqual((plain.steps, plain.cycles, plain.regs, plain.memory),
                         (sim.steps, sim.cycles, sim.regs, sim.memory))

    def test_internal_jumps_are_not_traced(self):
        text = annotated(TRACE_PROGRAM, TRACE_VARIABLES)
        _, trace = traced(text)
        events = [line for line in trace.splitlines() if line.startswith('[bundle')]
        self.assertFalse([line for line in events if '__ccom_' in line or '.L_crypto_' in line])
        self.assertFalse([line for line in events if 'beq' in line])  # no tomado

    def test_limit_truncates_but_keeps_the_final_state(self):
        text = annotated(TRACE_PROGRAM, TRACE_VARIABLES)
        _, trace = traced(text, limit=3)
        lines = trace.splitlines()
        events = [line for line in lines if line.startswith('[bundle')]
        self.assertEqual(events, TRACE_EXPECTED[:3])
        self.assertIn('… traza truncada en 3 eventos', lines)
        self.assertIn('Estado final de las variables:', lines)
        self.assertIn('  v = [0, 0, 9]', lines)

    def test_command_line(self):
        script = ROOT / 'tools' / 'sim_asm.py'
        with tempfile.TemporaryDirectory() as folder:
            asm = Path(folder) / 'demo.asm'
            asm.write_text(annotated(TRACE_PROGRAM, TRACE_VARIABLES))
            out = Path(folder) / 'demo.traza'
            result = subprocess.run([sys.executable, '-I', str(script), str(asm),
                                     '--traza', str(out), '--traza-max', '4'],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            lines = out.read_text(encoding='utf-8').splitlines()
            self.assertEqual([l for l in lines if l.startswith('[bundle')],
                             TRACE_EXPECTED[:4])
            self.assertIn('… traza truncada en 4 eventos', lines)
            # --traza-max sin --traza, o no positivo, o sobre el .asm: error.
            for arguments in (['--traza-max', '4'],
                              ['--traza', str(out), '--traza-max', '0'],
                              ['--traza', str(asm)]):
                result = subprocess.run([sys.executable, '-I', str(script), str(asm)] + arguments,
                                        capture_output=True)
                self.assertEqual(result.returncode, 2, arguments)

    def test_an_unannotated_asm_traces_only_the_flow(self):
        _, trace = traced('{ nopi | nopi | jef fin | nopi }\n' + GAP + '\n' + END)
        events = [line for line in trace.splitlines() if line.startswith('[bundle')]
        self.assertEqual(events, ['[bundle 0000] jef → fin (bundle 2)',
                                  '[bundle 0002] jef → fin (bundle 2)'])
        self.assertIn('(el .asm no trae la tabla de variables)', trace)

    def test_repeated_names_are_qualified(self):
        variables = ['#   Scope 1: FUNCTION f', '#       r : int width=4 addr=0x0000 line=1',
                     '#   Scope 2: FUNCTION g', '#       r : int width=4 addr=0x0004 line=5',
                     '#   Scope 3: S g', '#       r : int width=4 addr=0x0008 line=9']
        notes = Annotations(annotated([('{ nopi | nopi | nopi | nopi }', 1)], variables), 1)
        self.assertEqual([v['shown'] for v in notes.variables], ['f.r', 'g.r@5', 'g.r@9'])

    def test_compiled_programs(self):
        ccom = ROOT / 'ccom'
        if not ccom.exists():
            self.skipTest('falta compilar ccom')
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            for name in ('p02_factorial', 'p09_crypto'):
                base = directory / name
                result = subprocess.run([str(ccom), '-s', str(ROOT / 'pruebas' / (name + '.cls')),
                                         '-o', str(base.with_suffix('.bin'))],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                text = base.with_suffix('.asm').read_text()
                program = parse(text)
                sim = Simulator(program)
                sim.tracer = Tracer(Annotations(text, len(program.bundles)), program.labels, 5000)
                sim.load_data_image(base.with_suffix('.mem').read_text())
                if name == 'p09_crypto':
                    sim.load_data(bytes(range(256)) * 4)
                sim.run(100000)
                trace = sim.tracer.text(sim)
                self.assertEqual(sim.warnings, [])
                self.assertNotIn('__ccom_', ''.join(l for l in trace.splitlines() if l.startswith('[')))
                if name == 'p02_factorial':
                    self.assertIn('res = 0  (prólogo)', trace)
                    self.assertIn('res = 120', trace)
                    self.assertIn('  f = 120', trace)
                else:
                    for expected in ('auth', 'kv_write k=0 w=0', 'kv_lock', 'logout',
                                     'cifrar: 128 bloques de 0x1000 a 0x2000',
                                     'descifrar: 128 bloques de 0x2000 a 0x3000',
                                     'cifrar: 1 bloque de 0x1000 a 0x4000'):
                        self.assertTrue(any(expected in l for l in trace.splitlines()), expected)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        unittest.main()
    else:
        # Corrida completa (make check-simulador): una línea PASS y sin
        # tiempos, para que la evidencia de make check sea determinista.
        import io
        stream = io.StringIO()
        result = unittest.TextTestRunner(stream=stream, verbosity=0).run(
            unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
        if result.wasSuccessful():
            print(f'PASS simulador: {result.testsRun} casos (parser, pipeline, '
                  'cifrado, traza de ejecución)')
        else:
            print(stream.getvalue(), file=sys.stderr)
            sys.exit(1)
