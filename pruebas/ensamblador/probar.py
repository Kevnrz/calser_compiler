#!/usr/bin/env python3
"""Pruebas del ensamblador de ccom (-x y el ensamblado tras la Fase 4).

Vectores de isa.md §6.5, ejemplo de §11.1, formato del .bin
(fase4_contrato_comun.md §5), errores de §11.4 e ida y vuelta con el .asm
que genera el compilador. Sin dependencias externas.
"""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CCOM = ROOT / 'ccom'

# isa.md §6.5: instrucción, slot y palabra esperada.
VECTORS = [
    ('add r5, r3, r4', 0, '054D0000'),
    ('lw r7, 0(r6)', 3, '05D80000'),
    ('blt r3, r4, -2', 2, '0D0FFFFE'),
    ('feis r5, r3, r4, 0, 2', 1, '854D0800'),
    ('sanc r4', 1, '98100000'),
]

# isa.md §11.1: salida del ensamblador para el ejemplo.
EXAMPLE_HEX = [
    '00000000_00000000_49400003_49000000',
    '00000000_00000000_00000000_00000000',
    '00000000_0D500000_00000000_41100001',
]

# isa.md §11.4: un .asm por error, con parte del mensaje esperado.
ERRORS = [
    ('tres slots', '{ nopi | nopi | nopi }\n',
     'un bundle debe tener exactamente 4 slots (tiene 3)'),
    ('cinco slots', '{ nopi | nopi | nopi | nopi | nopi }\n',
     'un bundle debe tener exactamente 4 slots (tiene 5)'),
    ('slot equivocado', '{ lw r4, 0(r5) | nopi | nopi | nopi }\n',
     "'lw' no puede ir en el slot 0"),
    ('crypto fuera del slot 1', '{ feis r5, r3, r4, 0, 2 | nopi | nopi | nopi }\n',
     "'feis' no puede ir en el slot 0"),
    ('inmediato fuera de rango', '{ mov r4, 131072 | nopi | nopi | nopi }\n',
     'inmediato fuera de rango: 131072'),
    ('desplazamiento fuera de rango', '{ nopi | nopi | beq r4, r5, 131072 | nopi }\n',
     'destino fuera de rango: 131072'),
    ('destino fuera de rango', '{ nopi | nopi | jef -2097153 | nopi }\n',
     'destino fuera de rango: -2097153'),
    ('etiqueta no definida', '{ nopi | nopi | jef nada | nopi }\n',
     "etiqueta no definida: 'nada'"),
    ('etiqueta repetida', 'a:\n{ nopi | nopi | nopi | nopi }\na:\n{ nopi | nopi | nopi | nopi }\n',
     "etiqueta definida dos veces: 'a'"),
    ('mismo registro destino', '{ mov r4, 1 | mov r4, 2 | nopi | nopi }\n',
     'los slots 0 y 1 escriben el mismo registro r4'),
    ('mnemonico desconocido', '{ mul r4, r5, r6 | nopi | nopi | nopi }\n',
     "mnemonico desconocido: 'mul'"),
]

# Sección 1 del cambio: cuerpo de main y nombre del caso.
ARITHMETIC = [
    ('mul', 'Vm a : int = 7 ; Vm b : int = -3 ; Vm x : int = a * b ;'),
    ('div', 'Vm a : int = -7 ; Vm b : int = 2 ; Vm x : int = a / b ;'),
    ('mod', 'Vm a : int = -7 ; Vm b : int = 2 ; Vm x : int = a % b ;'),
    ('pow', 'Vm a : int = 2 ; Vm b : int = 10 ; Vm x : int = a ^ b ;'),
    ('div0', 'Vm a : int = 5 ; Vm b : int = 0 ; Vm x : int = a / b ;'),
]


def run(*args):
    return subprocess.run([str(CCOM), *map(str, args)],
                          capture_output=True, text=True)


def assemble(directory, name, text):
    """Ensambla text con -x. Devuelve (resultado, base de las salidas)."""
    source = directory / (name + '.asm')
    source.write_text(text)
    return run('-x', source), directory / name


def hex_lines(base):
    return base.with_suffix('.hex').read_text().splitlines()


def read_bin(base):
    """Devuelve (encabezado, bundles como en el .hex, datos) del .bin."""
    raw = base.with_suffix('.bin').read_bytes()
    count, entry, data_size, data_address = struct.unpack_from('<4I', raw, 0)
    assert len(raw) == 16 + 16 * count + data_size, (len(raw), count, data_size)
    bundles = []
    for i in range(count):
        s0, s1, s2, s3 = struct.unpack_from('<4I', raw, 16 + 16 * i)
        bundles.append('%08X_%08X_%08X_%08X' % (s3, s2, s1, s0))
    data = list(struct.unpack_from('<%dI' % (data_size // 4), raw, 16 + 16 * count))
    return (count, entry, data_size, data_address), bundles, data


def bundle_count(asm):
    return sum(1 for line in asm.read_text().splitlines()
               if line.split('#')[0].strip().endswith('}'))


def test_vectors(directory):
    for index, (instruction, slot, expected) in enumerate(VECTORS):
        slots = ['nopi'] * 4
        slots[slot] = instruction
        result, base = assemble(directory, 'vector%d' % index,
                                '{ ' + ' | '.join(slots) + ' }\n')
        assert result.returncode == 0, (instruction, result.stderr)
        lines = hex_lines(base)
        assert len(lines) == 1, lines
        # En el .hex el slot 3 va a la izquierda.
        words = lines[0].split('_')[::-1]
        assert words[slot] == expected, (instruction, words)
        assert all(word == '00000000' for i, word in enumerate(words) if i != slot)
    print('PASS vectores: %d codificaciones de isa.md §6.5' % len(VECTORS))


def test_example(directory):
    source = ROOT / 'pruebas/simulador/ejemplo_isa.asm'
    base = directory / 'ejemplo'
    result = run('-x', source, '-o', base.with_suffix('.bin'))
    assert result.returncode == 0, result.stderr
    assert hex_lines(base) == EXAMPLE_HEX, hex_lines(base)
    print('PASS ejemplo de isa.md §11.1: .hex exacto')

    header, bundles, data = read_bin(base)
    assert header == (3, 0, 0, 0), header
    assert base.with_suffix('.bin').stat().st_size == 16 + 3 * 16
    assert bundles == EXAMPLE_HEX, bundles
    assert data == []
    assert base.with_suffix('.mem').read_text() == '@0\n'
    print('PASS .bin del ejemplo: encabezado 3, 0, 0, 0; 64 bytes; bundles iguales al .hex')

    # -p: índice, bundle en ensamblador y la línea del .hex.
    result = run('-x', '-p', source, '-o', base.with_suffix('.bin'))
    lines = result.stdout.splitlines()
    assert result.returncode == 0 and len(lines) == 3, result
    for index, (line, expected) in enumerate(zip(lines, EXAMPLE_HEX)):
        assert line.startswith('%04d  { ' % index) and line.endswith('  ' + expected), line

    # -v y el aviso de -s con -x.
    result = run('-x', '-v', '-s', source, '-o', base.with_suffix('.bin'))
    assert result.returncode == 0
    assert 'ccom: aviso: -s se ignora con -x' in result.stderr, result.stderr
    assert 'ccom: ensamblado ... ok' in result.stderr, result.stderr
    assert not base.with_suffix('.asm').exists()
    print('PASS banderas con -x: -p, -v y aviso de -s')


def test_errors(directory):
    for index, (name, text, message) in enumerate(ERRORS):
        result, base = assemble(directory, 'error%d' % index, text)
        assert result.returncode == 1, (name, result.returncode, result.stderr)
        source = base.with_suffix('.asm')
        assert ('ccom: %s:' % source) in result.stderr, (name, result.stderr)
        assert 'error de ensamblado: ' + message in result.stderr, (name, result.stderr)
        for suffix in ('.bin', '.hex', '.mem'):
            assert not base.with_suffix(suffix).exists(), (name, suffix)
    # El mismo registro rz en dos slots sí se permite (isa.md §11.4).
    result, _ = assemble(directory, 'rz', '{ mov rz, 1 | mov rz, 2 | nopi | nopi }\n')
    assert result.returncode == 0, result.stderr
    # Con -v, un ensamblado fallido termina en "con errores".
    source = directory / 'error0.asm'
    result = run('-x', '-v', source)
    assert 'ccom: ensamblado ... con errores' in result.stderr, result.stderr
    print('PASS errores: %d casos de isa.md §11.4 con código 1 y su mensaje' % len(ERRORS))


def test_syntax(directory):
    text = ('# comentario\n'
            '\n'
            'a: b:   # dos etiquetas en una linea\n'
            'c: { nopi | nopi | jef c | nopi }   # etiqueta antes del bundle\n'
            '{ mov t0, 0x1F | mov rx, -5 | bne rl, rg, a | sw t11, -4(rz) }\n'
            '{ | | jas rl, 2 | }\n'
            '{ nopi | kv_write t6, 1, 3 | jefR rz, rl | lb r4, 0x10(r5) }\n'
            'fin:\n')
    result, base = assemble(directory, 'sintaxis', text)
    assert result.returncode == 0, result.stderr
    assert hex_lines(base) == [
        '00000000_40000000_00000000_00000000',   # jef 0
        '0BC3FFFC_08C7FFFF_4883FFFB_4900001F',
        '00000000_44400002_00000000_00000000',   # slots vacíos = nopi
        '0D140010_14040000_90281C00_00000000',
    ], hex_lines(base)
    print('PASS sintaxis: comentarios, etiquetas, alias, hexadecimal y slots vacíos')


def test_round_trip(directory):
    programs = [(name, (ROOT / 'pruebas' / source).read_text())
                for name, source in [('p02', 'p02_factorial.cls'),
                                     ('p08', 'p08_comentarios.cls'),
                                     ('p09', 'p09_crypto.cls'),
                                     ('p11', 'p11_ejercicio3.cls'),
                                     ('p12', 'p12_ejercicio2.cls')]]
    programs += [(name, 'F main [] : void ;\n    %s\n    E ;\n.\n' % body)
                 for name, body in ARITHMETIC]
    for name, text in programs:
        source = directory / (name + '.cls')
        source.write_text(text)
        base = directory / (name + '_c')
        result = run('-s', '-v', source, '-o', base.with_suffix('.bin'))
        assert result.returncode == 0, (name, result.stderr)
        assert 'ccom: ensamblado ... ok' in result.stderr, (name, result.stderr)
        count = bundle_count(base.with_suffix('.asm'))
        lines = hex_lines(base)
        assert count > 0 and len(lines) == count, (name, count, len(lines))
        header, bundles, data = read_bin(base)
        assert header[0] == count and bundles == lines, name
        # El .asm de -s va anotado (enunciado CE1108 §6.2), todo en
        # comentarios: dirección de cada bundle, tabla de etiquetas con su
        # dirección y variables.
        asm = base.with_suffix('.asm').read_text()
        assert asm.startswith('# ccom: '), name
        assert '# 0000' in asm, name
        assert '# Tabla de etiquetas (direccion = indice de bundle)' in asm, name
        assert '# Variables (direccion de datos)' in asm, name
        # Ensamblar el .asm anotado con -x da los mismos bundles que el
        # compilador.
        again = directory / (name + '_x')
        result = run('-x', base.with_suffix('.asm'), '-o', again.with_suffix('.bin'))
        assert result.returncode == 0, (name, result.stderr)
        assert hex_lines(again) == lines, name
        if name == 'p09':
            assert header == (count, 0, 16, 0), header
            assert data == [0x1000, 0x2000, 0x3000, 0x4000], data
            assert base.with_suffix('.mem').read_text().split() == [
                '@0', '00001000', '00002000', '00003000', '00004000']
    # Sin -s no se escribe el .asm, pero sí .bin, .hex y .mem.
    base = directory / 'sin_s'
    result = run(directory / 'p08.cls', '-o', base.with_suffix('.bin'))
    assert result.returncode == 0 and result.stdout == '' and result.stderr == '', result
    assert not base.with_suffix('.asm').exists()
    assert all(base.with_suffix(suffix).exists() for suffix in ('.bin', '.hex', '.mem'))
    print('PASS ida y vuelta: %d programas; .asm anotado; -x da el mismo .hex; '
          'datos de p09 en el .bin' % len(programs))


def main():
    with tempfile.TemporaryDirectory() as folder:
        directory = Path(folder)
        test_vectors(directory)
        test_example(directory)
        test_errors(directory)
        test_syntax(directory)
        test_round_trip(directory)


if __name__ == '__main__':
    try:
        main()
    except AssertionError as error:
        print('FAIL', error, file=sys.stderr)
        raise
