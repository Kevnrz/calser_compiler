#!/usr/bin/env python3
"""Comprueba que la tabla de etiquetas de -m y el .asm coinciden.

La tabla se construye en la Fase 3 y la Fase 4 la consume. Para cada
programa:
  1. ninguna etiqueta se define dos veces en el .asm;
  2. cada etiqueta de la tabla está definida exactamente una vez en el .asm;
  3. cada etiqueta definida en el .asm está en la tabla.

Excepciones documentadas (include/label_table.h):
  - Las etiquetas internas de cada expansión no son nodos del AST y no están
    en la tabla. Se reconocen por su prefijo: __ccom_arith_ (rutinas
    aritméticas), __ccom_array_ (copia y puesta en cero de arreglos) y
    .L_crypto_ (plantillas de cifrado).
  - Un & o | dentro de un inicializador global constante va a la imagen de
    datos y no genera código: su etiqueta SC_END puede faltar en el .asm.
    Solo se permite si el nodo es AND/OR y está en la sección de globales,
    antes de la primera función.
Sin dependencias externas.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CCOM = ROOT / 'ccom'
HERE = Path(__file__).resolve().parent

INTERNAL_PREFIXES = ('__ccom_arith_', '__ccom_array_', '.L_crypto_')

LABEL = r'[A-Za-z_.][A-Za-z0-9_.]*'


class Failure(Exception):
    pass


def compile_program(source, directory, index):
    """Devuelve (tabla de etiquetas, etiquetas definidas en el .asm)."""
    base = directory / ('p%d' % index)
    result = subprocess.run(
        [str(CCOM), '-s', '-m', str(source), '-o', str(base.with_suffix('.bin'))],
        capture_output=True, text=True)
    if result.returncode != 0:
        raise Failure('ccom terminó con código %d' % result.returncode)

    # Filas de la tabla: nombre | rol | AST kind: tipo | Line: n
    table = []
    block = result.stdout.split('===== LABEL TABLE =====')[1]
    for line in block.splitlines():
        match = re.fullmatch(
            r'(%s)\s*\| (\w+)\s*\| AST kind: (\w+) \| Line: (\d+)' % LABEL, line)
        if match:
            table.append((match[1], match[2], match[3], int(match[4])))
    if not table:
        raise Failure('no se pudo leer la tabla de etiquetas')

    # Definiciones en el .asm: "nombre:" al inicio de la línea (isa.md §11.1).
    defined = []
    for line in base.with_suffix('.asm').read_text().splitlines():
        text = line.split('#')[0].strip()
        while True:
            match = re.match(r'(%s)\s*:' % LABEL, text)
            if not match:
                break
            defined.append(match[1])
            text = text[match.end():].strip()
    return table, defined


def check(table, defined):
    """Devuelve (etiquetas internas vistas, SC_END globales sin definir)."""
    counts = {}
    for name in defined:
        counts[name] = counts.get(name, 0) + 1
    repeated = sorted(name for name, count in counts.items() if count > 1)
    if repeated:
        raise Failure('definidas más de una vez en el .asm: %s' % repeated)

    names = [name for name, _, _, _ in table]
    if len(names) != len(set(names)):
        raise Failure('nombres repetidos en la tabla')
    if any(name.startswith(INTERNAL_PREFIXES) for name in names):
        raise Failure('una etiqueta interna está en la tabla')

    # Primera función: las globales van antes (gramática: globales funciones).
    first_function = min(line for _, role, _, line in table if role == 'FUNCTION')

    absent = []
    for name, role, kind, line in table:
        if name in counts:
            continue
        constant_global = (role == 'SC_END' and kind in ('AND', 'OR') and
                           0 < line < first_function)
        if not constant_global:
            raise Failure('%s (%s, línea %d) está en la tabla y no en el .asm'
                          % (name, role, line))
        absent.append((name, line))

    internal = []
    for name in counts:
        if name in names:
            continue
        if not name.startswith(INTERNAL_PREFIXES):
            raise Failure('%s está en el .asm y no en la tabla' % name)
        internal.append(name)
    return internal, absent


def main():
    programs = sorted(ROOT.glob('pruebas/p[0-9][0-9]_*.cls'))
    programs += sorted(ROOT.glob('pruebas/programas/*.cls'))
    special = HERE / 'globales_corto_circuito.cls'
    programs.append(special)

    failed = 0
    seen = {prefix: 0 for prefix in INTERNAL_PREFIXES}
    labels = 0

    with tempfile.TemporaryDirectory() as folder:
        for index, source in enumerate(programs):
            name = str(source.relative_to(ROOT))
            try:
                table, defined = compile_program(source, Path(folder), index)
                internal, absent = check(table, defined)
                labels += len(table)
                for label in internal:
                    for prefix in INTERNAL_PREFIXES:
                        if label.startswith(prefix):
                            seen[prefix] += 1
                if source == special:
                    # Línea 1: & constante (imagen de datos, sin código).
                    # Línea 3: & no constante (código al inicio de main).
                    # Línea 6: | dentro de main.
                    sc = sorted((line, label in defined) for label, role, _, line in table
                                if role == 'SC_END')
                    if sc != [(1, False), (3, True), (6, True)]:
                        raise Failure('SC_END por línea (línea, definida): %s' % sc)
                elif absent:
                    raise Failure('SC_END globales sin definir: %s' % absent)
            except Failure as error:
                failed += 1
                print('FAIL %s: %s' % (name, error))

    for prefix, count in seen.items():
        if count == 0:
            failed += 1
            print('FAIL ninguna etiqueta interna %s* apareció: la excepción no se probó' % prefix)

    if failed:
        return 1

    print('PASS %d programas, %d etiquetas de tabla: cada una definida una vez en el .asm '
          'y ninguna del .asm fuera de la tabla' % (len(programs), labels))
    print('PASS excepciones por prefijo: %s'
          % ', '.join('%s* %d' % item for item in seen.items()))
    print('PASS inicializadores globales: & constante sin etiqueta en el .asm; '
          '& no constante con su etiqueta al inicio de main')
    return 0


if __name__ == '__main__':
    sys.exit(main())
