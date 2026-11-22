"""Exporta las producciones LL(1) como datos C, sin calcular el parser ni su tabla.

Solo se ejecuta durante el desarrollo. El reconocedor C calcula PRIMERO,
SIGUIENTE y la tabla predictiva y no necesita Python al ejecutarse.
"""
from pathlib import Path
import argparse
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from ide.nucleo.gramatica import CALSER, FIN

DESTINO = ROOT / 'src/servicios_ide/ll1_grammar.inc'


def generar():
    """Asigna índices estables y conserva los códigos de token declarados en Bison."""
    aliases = dict((alias, token) for token, alias in re.findall(
        r'^%token\s+(?:<\w+>\s+)?(TOKEN_\w+)\s+"([^"]+)"',
        (ROOT / 'grammar/parser.y').read_text(), re.M))
    aliases['id'] = aliases['identificador']
    aliases['num'] = aliases['numero']
    terminales = [FIN] + CALSER.terminales
    simbolos = terminales + CALSER.no_terminales
    indices = {s: i for i, s in enumerate(simbolos)}
    width = max(len(p.cuerpo) for p in CALSER.producciones)
    lines = ['/* Datos generados por tools/generar_datos_ll1.py desde la referencia auditada.',
             ' * No editar a mano. PRIMERO, SIGUIENTE y la tabla se calculan en C. */',
             f'#define LL1_TERMINALS {len(terminales)}',
             f'#define LL1_NONTERMINALS {len(CALSER.no_terminales)}',
             f'#define LL1_PRODUCTIONS {len(CALSER.producciones)}',
             f'#define LL1_RHS_MAX {width}',
             f'#define LL1_START {indices[CALSER.inicial]}',
             'typedef struct { unsigned short head, count, rhs[LL1_RHS_MAX]; } Production;',
             'static const char *const names[] = {']
    lines += ['    ' + json.dumps(s) + ',' for s in simbolos]
    lines += ['};', 'static const int flex_tokens[LL1_TERMINALS] = {', '    0,']
    lines += ['    ' + aliases[s] + ',' for s in CALSER.terminales]
    lines += ['};', 'static const Production productions[LL1_PRODUCTIONS] = {']
    for p in CALSER.producciones:
        rhs = ', '.join(str(indices[s]) for s in p.cuerpo) or '0'
        lines.append('    {%d, %d, {%s}}, /* %d %s */' %
                     (indices[p.cabeza], len(p.cuerpo), rhs, p.numero, p.cabeza))
    lines += ['};', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Comprueba el archivo sin modificarlo')
    args = parser.parse_args()
    generated = generar()
    if args.check:
        if not DESTINO.exists() or DESTINO.read_text() != generated:
            parser.exit(1, 'Los datos C no coinciden con la referencia. Ejecutar tools/generar_datos_ll1.py\n')
        print('Datos C de la gramática LL(1) vigentes')
    else:
        DESTINO.write_text(generated)
        print(DESTINO)


if __name__ == '__main__':
    main()
