#!/usr/bin/env python3
"""sim_asm.py — Herramienta de ejecución: modelo de bundles y trazas de la ISA.

Decodifica ensamblador y modela registros, memoria y separación RAW.
Simula Feistel y bóveda con autenticación Lamport: Ascon-Hash256 de cuatro
bytes little-endian, truncado a 32 bits, contra el ancla activa.

Usa: biblioteca estándar de Python; listado .asm e imagen .mem.
Lo usa: ejercicios.py, validar_bundles.py, pruebas y línea de comandos.
Referencias: isa.md §3, §5, §6, §9, §10, §11; enunciado §8.4.
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
# Etiquetas internas de una expansión (no son de la tabla de la Fase 3):
# rutinas aritméticas, copia de arreglos y plantillas de cifrado.
INTERNAL_PREFIXES = ("__ccom_arith_", "__ccom_array_", ".L_crypto_")


class AsmError(ValueError):
    """Entrada inválida o ejecución fuera del modelo de memoria."""


# ===========================================================================
# PALABRAS Y OPERANDOS
# ===========================================================================


# Ancla de fábrica: x100 de la cadena con semilla 0xCAFEDECE (isa.md §7.2).
LAMPORT_INITIAL_ANCHOR = 0xFEFF95FF


def auth_hash32(token):
    """H de la ISA: primeros cuatro bytes de Ascon-Hash256, little-endian.

    Entrada: palabra sin signo de 32 bits. Salida: palabra sin signo de 32 bits.
    Modelo de tools/lamport.py del proyecto de hardware vliw-block-cipher;
    contrastado con vectores de la referencia C ascon/ascon-c en las pruebas.
    La entrada fija de cuatro bytes requiere una permutación de 12 rondas
    tras la inicialización precalculada y el relleno de NIST SP 800-232.
    """
    if not 0 <= token <= MASK:
        raise ValueError("el token debe ser una palabra sin signo de 32 bits")
    mask64 = (1 << 64) - 1
    state = [0x9B1E5494E934D681, 0x4BC3A01E333751D2, 0xAE65396C6B34B81A,
             0x3C7FD4A4D56A4DB3, 0x1A5C464906C5976D]
    state[0] ^= token | (1 << 32)  # Cuatro bytes LE y relleno 01 00 00 00.
    rotations = ((19, 28), (61, 39), (1, 6), (10, 17), (7, 41))
    for constant in (0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5,
                     0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B):
        state[2] ^= constant
        state[0] ^= state[4]
        state[4] ^= state[3]
        state[2] ^= state[1]
        temp = [state[j] ^ ((~state[(j + 1) % 5] & mask64) & state[(j + 2) % 5])
                for j in range(5)]
        temp[1] ^= temp[0]
        temp[0] ^= temp[4]
        temp[3] ^= temp[2]
        temp[2] = ~temp[2] & mask64
        state = [word ^ (((word >> r1) | (word << (64 - r1))) & mask64)
                      ^ (((word >> r2) | (word << (64 - r2))) & mask64)
                 for word, (r1, r2) in zip(temp, rotations)]
    return state[0] & MASK


def signed(value):
    """signed — interpreta una palabra como entero de 32 bits con signo.

    Entrada:
        value  entero.
    Salida:
        entero entre -2147483648 y 2147483647. Efectos: ninguno.
    Restricciones:
        ninguna.
    """
    value &= MASK
    return value if value < 0x80000000 else value - (1 << 32)


def rotate_left(value, bits):
    """rotate_left — rota una palabra de 32 bits a la izquierda.

    Entrada:
        value  entero.
        bits  cantidad de bits.
    Salida:
        palabra de 32 bits. Efectos: ninguno.
    Restricciones:
        bits entre 0 y 32; aquí se usa 5 o 13.
    """
    value &= MASK
    return ((value << bits) | (value >> (32 - bits))) & MASK


def feistel_f(value, key):
    """feistel_f — calcula la función de ronda Feistel del modelo.

    Entrada:
        value  palabra de datos.
        key  palabra de subllave.
    Salida:
        palabra de 32 bits. Efectos: ninguno.
    Restricciones:
        ninguna.
    """
    return ((rotate_left(value, 5) + key) & MASK) ^ rotate_left(value, 13)


def register(token):
    """register — decodifica un registro o alias.

    Entrada:
        token  cadena del operando.
    Salida:
        número ISA 0–15. Efectos: ninguno.
    Restricciones:
        AsmError si no reconoce token.
    """
    if token in ALIASES:
        return ALIASES[token]
    if re.fullmatch(r"r(?:[0-9]|1[0-5])", token):
        return int(token[1:])
    raise AsmError(f"registro inválido: {token}")


def integer(token, bits=None):
    """integer — convierte un entero y verifica ancho opcional.

    Entrada:
        token  cadena decimal o hexadecimal.
        bits  ancho con signo o None.
    Salida:
        entero. Efectos: ninguno.
    Restricciones:
        AsmError por formato/rango; bits positivo si se especifica.
    """
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


# ===========================================================================
# DECODIFICACIÓN DE ENSAMBLADOR
# ===========================================================================

def parse_instruction(text, slot):
    """parse_instruction — decodifica un slot de ensamblador.

    Entrada:
        text  texto de instrucción.
        slot  slot 0–3.
    Salida:
        Instruction con operandos, lecturas y destino. Efectos: ninguno.
    Restricciones:
        AsmError por formato, unidad o rango; etiquetas se resuelven después.
    """
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
    """branch_target — resuelve un destino simbólico o relativo.

    Entrada:
        token  etiqueta o desplazamiento.
        pc  índice de bundle actual.
        labels  diccionario de etiquetas.
        bits  ancho con signo, 18 o 22.
    Salida:
        índice de bundle destino. Efectos: ninguno.
    Restricciones:
        AsmError por rango/formato; no comprueba aquí límites del programa.
    """
    if token in labels:
        target = labels[token]
        integer(str(target - pc), bits)
        return target
    return pc + integer(token, bits)


def parse(text):
    """parse — lee etiquetas y bundles de un programa ensamblador.

    Entrada:
        text  texto completo del .asm.
    Salida:
        Program con bundles y etiquetas. Efectos: ninguno.
    Restricciones:
        AsmError por formato, duplicados o destinos; necesita al menos un
        bundle.
    """
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


# ===========================================================================
# ANOTACIONES DEL LISTADO
# ===========================================================================

class Annotations:
    """Lo que el .asm anotado de ccom -s dice en sus comentarios (enunciado
    CE1108 §6.2): la línea del .cls de cada bundle y las variables con su
    dirección de datos. Sin anotaciones, las posiciones quedan en cero y la
    lista de variables queda vacía."""

    SCOPE = re.compile(r"^#   Scope (\d+): (\w+)(?: (\S+))?$")
    VARIABLE = re.compile(
        r"^#       (\w+) : (int|bool)((?:\[\d+\])*)(?: \((return|parameter)\))?"
        r" width=(\d+) addr=0x([0-9A-Fa-f]+) line=(\d+)$")

    def __init__(self, text, bundle_count):
        # Línea del fuente de cada bundle. El listado la muestra solo cuando
        # cambia, así que cada bundle hereda la última que se mostró.
        """__init__ — lee posiciones y variables del listado anotado.

        Entrada:
            self  instancia nueva.
            text  texto del .asm.
            bundle_count  cantidad de bundles.
        Salida:
            None. Efectos: inicializa líneas, líneas por slot y variables; califica nombres.
        Restricciones:
            cantidad no negativa; anotaciones ausentes dejan posiciones cero
            y variables vacías.
        """
        self.lines = [0] * bundle_count
        self.slot_lines = [[0] * 4 for _ in range(bundle_count)]
        shown = 0
        for raw in text.splitlines():
            match = re.match(r"^\s*\{.*\}\s*#\s*(\d+)(?:\s+linea\s+(\d+|-))?(?:\s+slots\s+([0-3]=\d+(?:,[0-3]=\d+)*))?\s*$", raw)
            if not match:
                continue
            index = int(match[1])
            if match[2]:
                shown = 0 if match[2] == "-" else int(match[2])
            if index < bundle_count:
                self.lines[index] = shown
                self.slot_lines[index] = [shown] * 4
                if match[3]:
                    for item in match[3].split(','):
                        slot, line = map(int, item.split('='))
                        self.slot_lines[index][slot] = line

        # Variables: [{name, owner, line, kind, type, dims, width, address}]
        self.variables = []
        if "# Variables (direccion de datos)" in text:
            owner = None
            for raw in text.split("# Variables (direccion de datos)", 1)[1].splitlines():
                scope = self.SCOPE.match(raw)
                if scope:
                    owner = scope[3]
                    continue
                match = self.VARIABLE.match(raw)
                if match:
                    dims = [int(x) for x in re.findall(r"\d+", match[3])]
                    self.variables.append({
                        "name": match[1], "owner": owner, "type": match[2],
                        "dims": dims, "kind": match[4] or "variable",
                        "width": int(match[5]), "address": int(match[6], 16),
                        "line": int(match[7])})
        self._qualify()

    def _qualify(self):
        """_qualify — desambigua nombres mostrados de variables.

        Entrada:
            self  instancia con variables.
        Salida:
            None. Efectos: agrega shown a cada variable.
        Restricciones:
            usa dueño y línea cuando se repiten nombres.
        """
        by_name, by_owner = {}, {}
        for v in self.variables:
            by_name[v["name"]] = by_name.get(v["name"], 0) + 1
            key = (v["owner"], v["name"])
            by_owner[key] = by_owner.get(key, 0) + 1
        for v in self.variables:
            shown = v["name"]
            if by_name[v["name"]] > 1:
                shown = f"{v['owner'] or 'global'}.{shown}"
                if by_owner[(v["owner"], v["name"])] > 1:
                    shown += f"@{v['line']}"
            v["shown"] = shown

    def locate(self, address):
        """locate — localiza la variable que contiene una dirección.

        Entrada:
            self  instancia con variables.
            address  dirección de byte.
        Salida:
            tupla (variable, índice de palabra) o None. Efectos: ninguno.
        Restricciones:
            ninguna.
        """
        for v in self.variables:
            if v["address"] <= address < v["address"] + v["width"]:
                return v, (address - v["address"]) // 4
        return None


# ===========================================================================
# PRESENTACIÓN DE VARIABLES
# ===========================================================================

def format_value(variable, value):
    """format_value — representa una palabra según el tipo de variable.

    Entrada:
        variable  diccionario de variable del listado.
        value  palabra de 32 bits.
    Salida:
        cadena bool, entero con signo o dirección de parámetro arreglo. Efectos: ninguno.
    Restricciones:
        ninguna.
    """
    if variable["kind"] == "parameter" and variable["dims"]:
        return f"@0x{value & MASK:04X}"
    if variable["type"] == "bool":
        return "verdadero" if value & MASK else "falso"
    return str(signed(value))


def element_name(variable, index):
    """element_name — nombra un elemento de arreglo por filas.

    Entrada:
        variable  diccionario con dims/kind/shown.
        index  índice lineal desde 0.
    Salida:
        nombre mostrado o nombre con índices. Efectos: ninguno.
    Restricciones:
        dimensiones positivas para arreglos; parámetros muestran su nombre
        sin índices.
    """
    dims = variable["dims"]
    if not dims or (variable["kind"] == "parameter"):
        return variable["shown"]
    if len(dims) == 1:
        return f"{variable['shown']}[{index}]"
    return f"{variable['shown']}[{index // dims[1]}][{index % dims[1]}]"


# ===========================================================================
# TRAZA DE EJECUCIÓN
# ===========================================================================

class Tracer:
    """Traza de ejecución (enunciado CE1108 §8.4): valores de variables y
    cambios de flujo, un evento por línea, a partir del .asm anotado.

    Reglas, también escritas en la cabecera del archivo:
      - no se muestran las escrituras fuera de las variables del listado: la
        pila de llamadas (rx), las casillas ocultas del S y los temporales
        que se salvan en las llamadas;
      - no se muestran los saltos hacia las rutinas internas
        (__ccom_arith_*, __ccom_array_*, .L_crypto_*): una multiplicación
        tiene decenas y solo meten ruido. Sí se muestra el salto a
        .L_arith_error.
    """

    HEADER = [
        "# Traza de ejecución (enunciado CE1108 §8.4)",
        "# Cada línea: [bundle N · línea L del .cls] evento.",
        "# No se muestran las escrituras fuera de las variables del listado:",
        "# la pila de llamadas (rx), las casillas ocultas del S y los temporales",
        "# que se salvan en las llamadas.",
        "# No se muestran los saltos hacia las rutinas internas",
        "# (__ccom_arith_*, __ccom_array_*, .L_crypto_*); sí el salto a",
        "# .L_arith_error. Los saltos condicionales no tomados tampoco.",
    ]

    CRYPTO_FIELD = re.compile(r"^\.L_crypto_(fin_)?(cifra|descifra)_(\d+)$")

    def __init__(self, annotations, labels, limit):
        """__init__ — inicializa la traza y localiza plantillas de cifrado.

        Entrada:
            self  instancia nueva.
            annotations  Annotations.
            labels  diccionario de etiquetas.
            limit  máximo de eventos.
        Salida:
            None. Efectos: inicializa eventos, prólogos y mapas de cifrado.
        Restricciones:
            limit positivo lo exige la interfaz de comandos.
        """
        self.notes = annotations
        self.labels = labels
        self.limit = limit
        self.events = []
        self.dropped = 0
        self.prologues = {labels[name] for name in labels
                          if name.startswith(".L_func_")}
        # Plantillas de cifrado: el bundle que precede a la etiqueta de inicio
        # es el bge que salta si no hay bloques, con origen, destino y
        # bloques ya en t2, t3 y t5; la etiqueta de fin marca el final.
        self.cipher_start, self.cipher_end = {}, {}
        for name, address in labels.items():
            match = self.CRYPTO_FIELD.match(name)
            if match:
                key = (match[2], match[3])
                if match[1]:
                    self.cipher_end[address] = key
                else:
                    self.cipher_start[address - 1] = key
        self.cipher_args = {}

    def _prefix(self, pc, slot=None):
        """_prefix — construye posición de un evento.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            slot  slot opcional 0–3.
        Salida:
            cadena con bundle y línea disponible. Efectos: ninguno.
        Restricciones:
            pc no negativo.
        """
        line = self.notes.lines[pc] if pc < len(self.notes.lines) else 0
        if slot is not None and pc < len(self.notes.slot_lines):
            line = self.notes.slot_lines[pc][slot]
        if line:
            return f"[bundle {pc:04d} · línea {line}]"
        return f"[bundle {pc:04d}]"

    def _emit(self, pc, text, slot=None):
        """_emit — agrega un evento sujeto al límite de traza.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            text  texto del evento.
            slot  slot opcional 0–3.
        Salida:
            None. Efectos: agrega evento o incrementa dropped si llegó al límite.
        Restricciones:
            ninguna.
        """
        if len(self.events) >= self.limit:
            self.dropped += 1
            return
        self.events.append(f"{self._prefix(pc, slot)} {text}")

    def visit(self, pc, snapshot):
        """visit — observa entrada y salida de una plantilla de cifrado.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            snapshot  16 registros antes de ejecutar.
        Salida:
            None. Efectos: guarda argumentos y agrega resumen al llegar al fin del cifrado.
        Restricciones:
            t2/t3/t5 contienen origen/destino/bloques.
        """
        key = self.cipher_start.get(pc)
        if key:
            # origen = t2 (r6), destino = t3 (r7), bloques = t5 (r9)
            self.cipher_args[key] = (snapshot[6], snapshot[7], signed(snapshot[9]))
        key = self.cipher_end.get(pc)
        if key and key in self.cipher_args:
            origin, destination, blocks = self.cipher_args.pop(key)
            blocks = max(blocks, 0)
            word = "bloque" if blocks == 1 else "bloques"
            name = "cifrar" if key[0] == "cifra" else "descifrar"
            self._emit(pc, f"{name}: {blocks} {word} de 0x{origin:04X} "
                           f"a 0x{destination:04X}")

    def store(self, pc, address, value, size):
        """store — registra una escritura a una variable del listado.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            address  dirección de byte.
            value  valor escrito.
            size  tamaño de escritura, no utilizado.
        Salida:
            None. Efectos: agrega evento si address pertenece a una variable.
        Restricciones:
            no muestra pila, temporales ni casillas ocultas sin símbolo.
        """
        found = self.notes.locate(address)
        if found is None:
            return
        variable, index = found
        suffix = "  (prólogo)" if pc in self.prologues else ""
        self._emit(pc, f"{element_name(variable, index)} = "
                       f"{format_value(variable, value)}{suffix}", slot=3)

    def jump(self, pc, op, target):
        """jump — registra un salto tomado o retorno.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            op  Instruction BRU.
            target  destino absoluto.
        Salida:
            None. Efectos: agrega evento de flujo salvo etiqueta interna de expansión.
        Restricciones:
            solo se invoca para saltos tomados.
        """
        mn = op.mnemonic
        if mn == "jefR":
            self._emit(pc, f"retorno → bundle {target}", slot=2)
            return
        token = op.args[-1]
        if token.startswith(INTERNAL_PREFIXES):
            return
        where = f"{token} (bundle {target})" if token in self.labels else f"bundle {target}"
        self._emit(pc, f"{'llamada' if mn == 'jas' else mn} → {where}", slot=2)

    def crypto(self, pc, op):
        """crypto — registra una operación de estado CRYPTO.

        Entrada:
            self  instancia de traza.
            pc  índice de bundle.
            op  Instruction de cifrado.
        Salida:
            None. Efectos: agrega evento de operación o de selector kv_write.
        Restricciones:
            feis no se registra individualmente desde Simulator.step.
        """
        if op.mnemonic == "kv_write":
            _, key, word = op.args
            self._emit(pc, f"kv_write k={key} w={word}", slot=1)
        else:
            self._emit(pc, op.mnemonic, slot=1)

    def values(self, simulator):
        """values — lee los valores finales de variables por dirección.

        Entrada:
            self  instancia de traza.
            simulator  simulador con memoria.
        Salida:
            lista de pares (variable, palabras). Efectos: ninguno.
        Restricciones:
            palabras little endian de las direcciones del listado.
        """
        result = []
        for v in sorted(self.notes.variables, key=lambda v: v["address"]):
            words = [int.from_bytes(simulator.memory[v["address"] + 4 * i:v["address"] + 4 * i + 4],
                                    "little") for i in range(max(v["width"] // 4, 1))]
            result.append((v, words))
        return result

    def final_state(self, simulator):
        """final_state — formatea el estado final de variables.

        Entrada:
            self  instancia de traza.
            simulator  simulador ejecutado.
        Salida:
            lista de líneas de estado. Efectos: ninguno.
        Restricciones:
            ninguna.
        """
        lines = []
        for v, words in self.values(simulator):
            if v["dims"] and v["kind"] != "parameter":
                value = "[" + ", ".join(format_value(v, w) for w in words) + "]"
            else:
                value = format_value(v, words[0])
            lines.append(f"  {v['shown']} = {value}")
        if not self.notes.variables:
            lines.append("  (el .asm no trae la tabla de variables)")
        return lines

    def text(self, simulator):
        """text — construye la traza completa y estado final.

        Entrada:
            self  instancia de traza.
            simulator  simulador ejecutado.
        Salida:
            cadena con salto final y aviso si se truncó. Efectos: ninguno.
        Restricciones:
            ninguna.
        """
        out = list(self.HEADER) + self.events
        if self.dropped:
            out.append(f"… traza truncada en {self.limit} eventos")
        out.append("")
        out.append("Estado final de las variables:")
        out.extend(self.final_state(simulator))
        return "\n".join(out) + "\n"


# ===========================================================================
# MODELO DEL PIPELINE
# ===========================================================================

class Simulator:
    def __init__(self, program, warn=None):
        """__init__ — inicializa registros, memoria y estado del modelo.

        Entrada:
            self  instancia nueva.
            program  Program decodificado.
            warn  callback opcional para avisos.
        Salida:
            None. Efectos: inicializa 16 registros, memoria de 64 KiB y estado CRYPTO.
        Restricciones:
            no carga imagen de datos automáticamente.
        """
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
        self.anchor = LAMPORT_INITIAL_ANCHOR  # Nueva instancia: arranque de fábrica.
        self.vault = [[0] * 4 for _ in range(4)]
        self.tracer = None  # Tracer opcional: no cambia la ejecución.

    def load_data(self, data, address=0x1000):
        """load_data — carga bytes en la zona de archivos.

        Entrada:
            self  instancia del simulador.
            data  bytes a cargar.
            address  dirección, 0x1000 por defecto.
        Salida:
            None. Efectos: escribe memoria sin extenderla.
        Restricciones:
            debe caber entre 0x1000 y 0xFFFF; AsmError si no cabe.
        """
        if not 0x1000 <= address < len(self.memory) or address + len(data) > len(self.memory):
            raise AsmError("el archivo debe caber en la zona 0x1000–0xFFFF")
        self.memory[address:address + len(data)] = data

    def load_data_image(self, text):
        """load_data_image — carga la imagen estática .mem.

        Entrada:
            self  instancia del simulador.
            text  texto @0 y palabras hexadecimales.
        Salida:
            None. Efectos: escribe palabras little endian desde dirección cero.
        Restricciones:
            máximo 1024 palabras; AsmError por formato/rango.
        """
        lines = [re.split(r"#|//", raw, maxsplit=1)[0].strip() for raw in text.splitlines()]
        lines = [line for line in lines if line]
        if not lines or lines[0] != '@0':
            raise AsmError("la imagen .mem debe comenzar con @0")
        if len(lines) - 1 > 1024:
            raise AsmError("la imagen .mem excede 0x0FFF")
        data = bytearray()
        for line in lines[1:]:
            if not re.fullmatch(r'[0-9a-fA-F]{8}', line):
                raise AsmError(f"palabra inválida en la imagen .mem: {line}")
            data.extend(int(line, 16).to_bytes(4, 'little'))
        self.memory[:len(data)] = data

    def execute_crypto(self, op, snapshot):
        """execute_crypto — ejecuta una operación del modelo CRYPTO.

        Entrada:
            self  instancia del simulador.
            op  Instruction CRYPTO.
            snapshot  instantánea de 16 registros.
        Salida:
            palabra de feis si autentica; None si no escribe registro. Efectos: modifica contador, bóveda, banderas, ancla y ciclos.
        Restricciones:
            cada auth compara H(token) con el ancla activa; un token
            aceptado se consume al actualizar el ancla.
        """
        self.crypto_count += 1
        mn, args = op.mnemonic, op.args
        if mn == "auth":
            token = snapshot[args[0]]
            self.auth = auth_hash32(token) == self.anchor
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
        """commit — hace visibles escrituras de registros pendientes.

        Entrada:
            self  instancia del simulador.
        Salida:
            None. Efectos: actualiza registros, mantiene rz=0 y vacía pending.
        Restricciones:
            ninguna.
        """
        for rd, value in self.pending.items():
            if rd != 0:
                self.regs[rd] = value & MASK
        self.regs[0] = 0
        self.pending = {}

    def address(self, base, offset, size):
        """address — calcula la dirección efectiva de un acceso.

        Entrada:
            self  instancia del simulador.
            base  valor del registro base.
            offset  desplazamiento con signo.
            size  tamaño de acceso, 1 o 4.
        Salida:
            dirección efectiva dentro de memoria. Efectos: ninguno.
        Restricciones:
            lw/sw ignoran bits bajos de dirección; AsmError si acceso sale de
            memoria.
        """
        address = (base + offset) & MASK
        if size == 4:
            address &= ~3  # El hardware ignora addr[1:0] para lw/sw (§5.4).
        if address + size > len(self.memory):
            raise AsmError(f"acceso fuera de memoria: 0x{address:08X}")
        return address

    def step(self):
        """step — ejecuta un bundle con la lectura del pipeline.

        Entrada:
            self  instancia del simulador.
        Salida:
            True si continúa; False ante jef a sí mismo. Efectos: actualiza registros pendientes, memoria, PC, ciclos, avisos, CRYPTO y traza.
        Restricciones:
            PC dentro del programa; lee antes de commit; AsmError por
            destino/acceso fuera de rango.
        """
        bundle = self.program.bundles[self.pc]
        # Leer antes de hacer visibles los resultados del bundle anterior.
        snapshot = self.regs.copy()
        if self.tracer:
            self.tracer.visit(self.pc, snapshot)
        for slot, op in enumerate(bundle.slots):
            for reg in sorted((set(op.reads) & self.pending.keys()) - {0}):
                message = (f"AVISO separación: bundle {self.pc}, línea {bundle.line}, "
                           f"slot {slot}: {op.mnemonic} lee r{reg} escrito en el bundle anterior")
                self.warnings.append(message)
                if self.warn:
                    self.warn(message)
        self.commit()
        writes, target, taken = {}, None, None
        for op in bundle.slots:
            mn, args = op.mnemonic, op.args
            value = None
            if mn == "nopi":
                continue
            if mn in CRYPTO:
                value = self.execute_crypto(op, snapshot)
                if self.tracer and mn != "feis":
                    self.tracer.crypto(self.pc, op)
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
                    if self.tracer:
                        self.tracer.store(self.pc, address, data, size)
                else:
                    value = int.from_bytes(self.memory[address:address + size], 'little',
                                           signed=(mn == "lb"))
            elif mn in BRANCHES:
                a, b = snapshot[args[0]], snapshot[args[1]]
                take = {"beq": a == b, "bne": a != b,
                        "blt": signed(a) < signed(b), "bge": signed(a) >= signed(b)}[mn]
                if take:
                    target = branch_target(args[2], self.pc, self.program.labels, 18)
                    taken = op
            elif mn in {"jef", "jas"}:
                target = branch_target(args[-1], self.pc, self.program.labels, 22)
                taken = op
                if mn == "jas": value = self.pc + 1
            elif mn == "jefR":
                target = snapshot[args[1]] & ((1 << 22) - 1)
                taken = op
                value = self.pc + 1
            if value is not None and op.dest not in (None, 0):
                writes[op.dest] = value & MASK
        self.pending = writes
        self.steps += 1
        self.cycles += 1
        if target is not None:
            if self.tracer:
                self.tracer.jump(self.pc, taken, target)
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
        """run — ejecuta hasta detenerse o alcanzar el límite.

        Entrada:
            self  instancia del simulador.
            max_bundles  máximo de bundles ejecutados, 100000 por defecto.
        Salida:
            la misma instancia. Efectos: ejecuta pasos, fija reason y drena escrituras pendientes.
        Restricciones:
            límite positivo; AsmError añade contexto del bundle.
        """
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
        """report — representa el estado final del modelo.

        Entrada:
            self  instancia del simulador.
        Salida:
            cadena con contadores, CRYPTO, registros y primeras 256 posiciones de memoria. Efectos: ninguno.
        Restricciones:
            ninguna.
        """
        lines = [f"Fin: {self.reason}; bundles ejecutados: {self.steps}; ciclos: {self.cycles}",
                 f"Avisos de separación: {len(self.warnings)}"]
        if self.crypto_count:
            lines.append(f"CRYPTO: {self.crypto_count} instrucciones; autenticación Lamport "
                         "(Ascon-Hash256 truncado a 32 bits)")
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


# ===========================================================================
# LÍNEA DE COMANDOS
# ===========================================================================

def address_argument(token):
    """address_argument — convierte una dirección de línea de comandos.

    Entrada:
        token  cadena decimal o hexadecimal.
    Salida:
        entero. Efectos: ninguno.
    Restricciones:
        transforma AsmError en argparse.ArgumentTypeError.
    """
    try:
        return integer(token)
    except AsmError as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def main(argv=None):
    """main — ejecuta el simulador con archivos y opciones.

    Entrada:
        argv  argumentos de comandos o None para sys.argv.
    Salida:
        0 si termina sin avisos; 1 por avisos/límite; 2 por errores capturados. Efectos: lee .asm/.mem/datos; escribe traza/volcado opcional y reporte.
    Restricciones:
        argparse puede terminar el proceso por uso incorrecto; no sobrescribe
        entradas con traza/volcado.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('asm', type=Path, help='archivo con etiquetas y bundles de cuatro slots')
    parser.add_argument('--max-bundles', type=int, default=100000, help='límite de bundles ejecutados')
    parser.add_argument('--data-image', type=Path, help='imagen estática .mem de ccom, con @0')
    parser.add_argument('--load-data', type=Path, help='archivo binario a inyectar antes de ejecutar')
    parser.add_argument('--data-address', type=address_argument, default=0x1000,
                        help='dirección de carga decimal o hexadecimal (por defecto 0x1000)')
    parser.add_argument('--dump-memory', type=Path, help='guardar los 64 KiB finales como archivo binario')
    parser.add_argument('--traza', type=Path,
                        help='escribir la traza de ejecución (valores de variables y '
                             'cambios de flujo, enunciado CE1108 §8.4); usa las anotaciones del .asm de ccom -s')
    parser.add_argument('--traza-max', type=int, default=5000,
                        help='máximo de eventos de la traza (por defecto 5000)')
    options = parser.parse_args(argv)
    try:
        asm_text = options.asm.read_text()
        program = parse(asm_text)
        simulator = Simulator(program, warn=lambda message: print(message, file=sys.stderr))
        if options.traza_max != 5000 and not options.traza:
            raise AsmError("--traza-max requiere --traza")
        if options.traza:
            if options.traza_max <= 0:
                raise AsmError("--traza-max debe ser positivo")
            if options.traza.resolve() == options.asm.resolve():
                raise AsmError("la traza no debe sobrescribir el .asm de entrada")
            simulator.tracer = Tracer(Annotations(asm_text, len(program.bundles)),
                                      program.labels, options.traza_max)
        if options.data_image:
            simulator.load_data_image(options.data_image.read_text())
        if options.load_data:
            simulator.load_data(options.load_data.read_bytes(), options.data_address)
        elif options.data_address != 0x1000:
            raise AsmError("--data-address requiere --load-data")
        try:
            simulator.run(options.max_bundles)
        finally:
            if simulator.tracer:
                options.traza.write_text(simulator.tracer.text(simulator), encoding='utf-8')
        if options.dump_memory:
            simulator_path = options.asm.resolve()
            inputs = {simulator_path}
            if options.load_data:
                inputs.add(options.load_data.resolve())
            if options.data_image:
                inputs.add(options.data_image.resolve())
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
