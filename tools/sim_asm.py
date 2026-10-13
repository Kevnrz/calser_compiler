#!/usr/bin/env python3
"""Modelo de bundles con Feistel4 y bóveda; autenticación de prueba (sin Ascon).

Registros inicialmente cero y memoria little endian de 64 KiB. Los resultados
no son visibles al bundle inmediatamente siguiente. Un salto tomado introduce
un ciclo de descarte y continúa en su destino (no ejecuta el PC+1 prefetched).
No ensambla binarios. Según la tarea actualizada, auth acepta todos los tokens
salvo 999; esta regla no valida Ascon ni una cadena real de Lamport.
"""
import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys
from typing import Optional

MASK = 0xFFFFFFFF
MEMORY_SIZE = 65536
ALIASES = {"rz": 0, "rl": 1, "rx": 2, "rg": 3}
ALIASES.update({f"t{i}": i + 4 for i in range(12)})
ALU_R = {"add", "sub", "and", "or", "xor", "sll", "srl", "sra",
         "slt", "sle", "seq", "sne", "ifmov"}
BRANCHES = {"beq", "bne", "blt", "bge"}
MEMORY = {"lw", "sw", "lb", "sb"}
CRYPTO = {"feis", "auth", "logout", "kv_write", "kv_lock", "sanc"}
LABEL = r"[A-Za-z_.$][A-Za-z0-9_.$]*"


class AsmError(ValueError):
    """Entrada inválida o ejecución fuera del modelo de memoria."""


def signed(value):
    value &= MASK
    return value if value < 0x80000000 else value - (1 << 32)


def rotate_left(value, bits):
    value &= MASK
    return ((value << bits) | (value >> (32 - bits))) & MASK


def feistel_f(value, key):
    return ((rotate_left(value, 5) + key) & MASK) ^ rotate_left(value, 13)


def register(token):
    if token in ALIASES:
        return ALIASES[token]
    if re.fullmatch(r"r(?:[0-9]|1[0-5])", token):
        return int(token[1:])
    raise AsmError(f"registro inválido: {token}")


def integer(token, bits=None):
    try:
        value = int(token, 16) if re.fullmatch(r"[+-]?0[xX][0-9a-fA-F]+", token) else int(token, 10)
    except ValueError as error:
        raise AsmError(f"entero inválido: {token}") from error
    if bits and not -(1 << (bits - 1)) <= value < (1 << (bits - 1)):
        raise AsmError(f"inmediato fuera de {bits} bits con signo: {token}")
    return value


@dataclass(frozen=True)
class Instruction:
    mnemonic: str
    args: tuple
    reads: tuple = ()
    dest: Optional[int] = None


@dataclass(frozen=True)
class Bundle:
    slots: tuple
    line: int


@dataclass
class Program:
    bundles: list
    labels: dict


def parse_instruction(text, slot):
    fields = text.strip().split(None, 1)
    if not fields:
        raise AsmError("slot vacío; use nopi")
    op = fields[0]
    args = tuple(x.strip() for x in fields[1].split(',')) if len(fields) == 2 else ()
    counts = {**dict.fromkeys(ALU_R, 3), **dict.fromkeys(BRANCHES, 3),
              **dict.fromkeys(MEMORY, 2), "nopi": 0, "mov": 2,
              "saddi": 3, "xori": 3, "jef": 1, "jas": 2, "jefR": 2,
              "feis": 5, "auth": 1, "logout": 0, "kv_write": 3,
              "kv_lock": 0, "sanc": 1}
    if op not in counts:
        raise AsmError(f"instrucción desconocida: {op}")
    if len(args) != counts[op] or any(not x for x in args):
        raise AsmError(f"{op} requiere {counts[op]} operandos")
    legal = (range(4) if op == "nopi" else (1,) if op in CRYPTO else
             (3,) if op in MEMORY else (2,) if op in BRANCHES | {"jef", "jas", "jefR"}
             else (0, 1))
    if slot not in legal:
        raise AsmError(f"{op} no pertenece al slot {slot}")
    if op == "nopi" or op in {"logout", "kv_lock"}:
        return Instruction(op, ())
    if op in ALU_R:
        rd, a, b = map(register, args)
        return Instruction(op, (rd, a, b), (a, b), rd)
    if op == "mov":
        rd, imm = register(args[0]), integer(args[1], 18)
        return Instruction(op, (rd, imm), (), rd)
    if op in {"saddi", "xori"}:
        rd, a, imm = register(args[0]), register(args[1]), integer(args[2], 18)
        return Instruction(op, (rd, a, imm), (a,), rd)
    if op in MEMORY:
        match = re.fullmatch(r"([^()\s]+)\s*\(\s*([^()\s]+)\s*\)", args[1])
        if not match:
            raise AsmError(f"dirección inválida: {args[1]}; use desplazamiento(registro)")
        rd, offset, base = register(args[0]), integer(match[1], 18), register(match[2])
        store = op in {"sw", "sb"}
        return Instruction(op, (rd, offset, base), (base, rd) if store else (base,),
                           None if store else rd)
    if op in BRANCHES:
        a, b = register(args[0]), register(args[1])
        return Instruction(op, (a, b, args[2]), (a, b))
    if op == "jef":
        return Instruction(op, args)
    if op in {"jas", "jefR"}:
        rd = register(args[0])
        target = register(args[1]) if op == "jefR" else args[1]
        return Instruction(op, (rd, target), (target,) if op == "jefR" else (), rd)
    if op in {"auth", "sanc"}:
        a = register(args[0])
        return Instruction(op, (a,), (a,))
    if op == "kv_write":
        a, k, w = register(args[0]), integer(args[1]), integer(args[2])
        if not (0 <= k <= 3 and 0 <= w <= 3):
            raise AsmError("k y w deben estar entre 0 y 3")
        return Instruction(op, (a, k, w), (a,))
    rd, a, b = map(register, args[:3])
    k, i = map(integer, args[3:])
    if not (0 <= k <= 3 and 0 <= i <= 3):
        raise AsmError("k e i deben estar entre 0 y 3")
    return Instruction(op, (rd, a, b, k, i), (a, b), rd)


def branch_target(token, pc, labels, bits):
    if token in labels:
        target = labels[token]
        integer(str(target - pc), bits)
        return target
    return pc + integer(token, bits)


def parse(text):
    labels, bundles = {}, []
    for line_number, raw in enumerate(text.splitlines(), 1):
        line = re.split(r"#|;|//", raw, maxsplit=1)[0].strip()
        if not line:
            continue
        try:
            while (match := re.match(rf"^({LABEL})\s*:\s*", line)):
                name = match[1]
                if name in labels:
                    raise AsmError(f"etiqueta duplicada: {name}")
                labels[name] = len(bundles)
                line = line[match.end():].strip()
            if not line:
                continue
            if not (line.startswith('{') and line.endswith('}')):
                raise AsmError("se esperaba { slot0 | slot1 | slot2 | slot3 }")
            slots = line[1:-1].split('|')
            if len(slots) != 4:
                raise AsmError("cada bundle debe tener exactamente 4 slots")
            instructions = tuple(parse_instruction(s, i) for i, s in enumerate(slots))
            destinations = [op.dest for op in instructions if op.dest not in (None, 0)]
            if len(destinations) != len(set(destinations)):
                raise AsmError("dos slots escriben el mismo registro destino")
            bundles.append(Bundle(instructions, line_number))
        except AsmError as error:
            raise AsmError(f"línea {line_number}: {error}") from error
    if not bundles:
        raise AsmError("el programa no contiene bundles")
    for pc, bundle in enumerate(bundles):
        for op in bundle.slots:
            if op.mnemonic in BRANCHES | {"jef", "jas"}:
                token = op.args[-1]
                try:
                    branch_target(token, pc, labels, 18 if op.mnemonic in BRANCHES else 22)
                except AsmError as error:
                    raise AsmError(f"línea {bundle.line}: destino inválido o etiqueta indefinida: {token}") from error
    return Program(bundles, labels)


class Simulator:
    def __init__(self, program, warn=None):
        self.program = program
        self.regs = [0] * 16
        self.memory = bytearray(MEMORY_SIZE)
        self.pc = self.cycles = self.steps = self.crypto_count = 0
        self.pending = {}
        self.warnings = []
        self.warn = warn
        self.reason = ""
        self.auth = False
        self.kv_lock = False
        self.exc = False
        self.anchor = 0  # Solo observable desde Python, sin hash de autenticación.
        self.vault = [[0] * 4 for _ in range(4)]

    def load_data(self, data, address=0x1000):
        """Inyecta bytes en la zona de archivos, sin truncar ni extender memoria."""
        if not 0x1000 <= address < len(self.memory) or address + len(data) > len(self.memory):
            raise AsmError("el archivo debe caber en la zona 0x1000–0xFFFF")
        self.memory[address:address + len(data)] = data

    def execute_crypto(self, op, snapshot):
        """Devuelve el resultado de feis, o None cuando no escribe registro."""
        self.crypto_count += 1
        mn, args = op.mnemonic, op.args
        if mn == "auth":
            token = snapshot[args[0]]
            self.auth = token != 999
            self.exc = not self.auth
            if self.auth:
                self.anchor = token
            # auth ocupa 14 ciclos: uno ya contado en step y 13 de pausa.
            self.cycles += 13
        elif mn == "logout":
            self.auth = False
        elif not self.auth:
            self.exc = True
        elif mn == "feis":
            _, left, right, key, word = args
            return snapshot[left] ^ feistel_f(snapshot[right], self.vault[key][word])
        elif mn == "kv_write":
            if self.kv_lock:
                self.exc = True
            else:
                source, key, word = args
                self.vault[key][word] = snapshot[source]
        elif mn == "kv_lock":
            self.kv_lock = True
        elif mn == "sanc":
            self.anchor = snapshot[args[0]]
        return None

    def commit(self):
        for rd, value in self.pending.items():
            if rd != 0:
                self.regs[rd] = value & MASK
        self.regs[0] = 0
        self.pending = {}

    def address(self, base, offset, size):
        address = (base + offset) & MASK
        if size == 4:
            address &= ~3  # El hardware ignora addr[1:0] para lw/sw (§5.4).
        if address + size > len(self.memory):
            raise AsmError(f"acceso fuera de memoria: 0x{address:08X}")
        return address

    def step(self):
        bundle = self.program.bundles[self.pc]
        # Leer antes de hacer visibles los resultados del bundle anterior.
        snapshot = self.regs.copy()
        for slot, op in enumerate(bundle.slots):
            for reg in sorted((set(op.reads) & self.pending.keys()) - {0}):
                message = (f"AVISO separación: bundle {self.pc}, línea {bundle.line}, "
                           f"slot {slot}: {op.mnemonic} lee r{reg} escrito en el bundle anterior")
                self.warnings.append(message)
                if self.warn:
                    self.warn(message)
        self.commit()
        writes, target = {}, None
        for op in bundle.slots:
            mn, args = op.mnemonic, op.args
            value = None
            if mn == "nopi":
                continue
            if mn in CRYPTO:
                value = self.execute_crypto(op, snapshot)
            elif mn == "mov":
                value = args[1]
            elif mn in {"saddi", "xori"}:
                a, imm = snapshot[args[1]], args[2]
                value = a + imm if mn == "saddi" else a ^ (imm & MASK)
            elif mn in ALU_R:
                a, b = snapshot[args[1]], snapshot[args[2]]
                if mn == "add": value = a + b
                elif mn == "sub": value = a - b
                elif mn == "and": value = a & b
                elif mn == "or": value = a | b
                elif mn == "xor": value = a ^ b
                elif mn == "sll": value = a << (b & 31)
                elif mn == "srl": value = a >> (b & 31)
                elif mn == "sra": value = signed(a) >> (b & 31)
                elif mn == "slt": value = int(signed(a) < signed(b))
                elif mn == "sle": value = int(signed(a) <= signed(b))
                elif mn == "seq": value = int(a == b)
                elif mn == "sne": value = int(a != b)
                elif mn == "ifmov" and b != 0: value = a
            elif mn in MEMORY:
                rd, offset, base = args
                size = 4 if mn in {"lw", "sw"} else 1
                address = self.address(snapshot[base], offset, size)
                if mn in {"sw", "sb"}:
                    data = snapshot[rd] & ((1 << (8 * size)) - 1)
                    self.memory[address:address + size] = data.to_bytes(size, 'little')
                else:
                    value = int.from_bytes(self.memory[address:address + size], 'little',
                                           signed=(mn == "lb"))
            elif mn in BRANCHES:
                a, b = snapshot[args[0]], snapshot[args[1]]
                take = {"beq": a == b, "bne": a != b,
                        "blt": signed(a) < signed(b), "bge": signed(a) >= signed(b)}[mn]
                if take:
                    target = branch_target(args[2], self.pc, self.program.labels, 18)
            elif mn in {"jef", "jas"}:
                target = branch_target(args[-1], self.pc, self.program.labels, 22)
                if mn == "jas": value = self.pc + 1
            elif mn == "jefR":
                target = snapshot[args[1]] & ((1 << 22) - 1)
                value = self.pc + 1
            if value is not None and op.dest not in (None, 0):
                writes[op.dest] = value & MASK
        self.pending = writes
        self.steps += 1
        self.cycles += 1
        if target is not None:
            # El bundle del salto se completa; el prefetched se descarta.
            self.cycles += 1
            self.commit()
            if target == self.pc and bundle.slots[2].mnemonic == "jef":
                self.reason = "salto a sí mismo"
                return False
            if not 0 <= target < len(self.program.bundles):
                raise AsmError(f"destino fuera del programa: bundle {target}")
            self.pc = target
        else:
            self.pc += 1
        return True

    def run(self, max_bundles=100000):
        if max_bundles <= 0:
            raise AsmError("el límite de bundles debe ser positivo")
        while self.steps < max_bundles and self.pc < len(self.program.bundles):
            try:
                if not self.step():
                    break
            except AsmError as error:
                bundle = self.program.bundles[self.pc]
                raise AsmError(f"bundle {self.pc}, línea {bundle.line}: {error}") from error
        if not self.reason:
            self.reason = "límite de bundles" if self.steps >= max_bundles else "fin del archivo"
        self.commit()  # Drenar las escrituras pendientes para el reporte final.
        return self

    def report(self):
        lines = [f"Fin: {self.reason}; bundles ejecutados: {self.steps}; ciclos: {self.cycles}",
                 f"Avisos de separación: {len(self.warnings)}"]
        if self.crypto_count:
            lines.append(f"CRYPTO: {self.crypto_count} instrucciones; autenticación de prueba "
                         "(sin Ascon; token 999 falla)")
            lines.append(f"Estado: AUTH={int(self.auth)} KVLOCK={int(self.kv_lock)} EXC={int(self.exc)}")
        lines.append("Memoria 0x0000–0x00FF (palabras no nulas):")
        words = [(addr, int.from_bytes(self.memory[addr:addr + 4], 'little'))
                 for addr in range(0, 256, 4)]
        lines.extend(f"  0x{addr:04X}: 0x{word:08X} ({signed(word)})" for addr, word in words if word)
        if not any(word for _, word in words):
            lines.append("  (ninguna)")
        lines.append("Registros:")
        lines.extend(f"  r{i} = {signed(value)} (0x{value:08X})" for i, value in enumerate(self.regs))
        return '\n'.join(lines)


def address_argument(token):
    try:
        return integer(token)
    except AsmError as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('asm', type=Path, help='archivo con etiquetas y bundles de cuatro slots')
    parser.add_argument('--max-bundles', type=int, default=100000, help='límite de bundles ejecutados')
    parser.add_argument('--load-data', type=Path, help='archivo binario a inyectar antes de ejecutar')
    parser.add_argument('--data-address', type=address_argument, default=0x1000,
                        help='dirección de carga decimal o hexadecimal (por defecto 0x1000)')
    parser.add_argument('--dump-memory', type=Path, help='guardar los 64 KiB finales como archivo binario')
    options = parser.parse_args(argv)
    try:
        simulator = Simulator(parse(options.asm.read_text()), warn=lambda message: print(message, file=sys.stderr))
        if options.load_data:
            simulator.load_data(options.load_data.read_bytes(), options.data_address)
        elif options.data_address != 0x1000:
            raise AsmError("--data-address requiere --load-data")
        simulator.run(options.max_bundles)
        if options.dump_memory:
            simulator_path = options.asm.resolve()
            inputs = {simulator_path}
            if options.load_data:
                inputs.add(options.load_data.resolve())
            if options.dump_memory.resolve() in inputs:
                raise AsmError("el volcado de memoria no debe sobrescribir un archivo de entrada")
            options.dump_memory.write_bytes(simulator.memory)
        print(simulator.report())
        return 1 if simulator.warnings or simulator.reason == "límite de bundles" else 0
    except (OSError, UnicodeError, AsmError) as error:
        print(f"sim_asm: error: {error}", file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
