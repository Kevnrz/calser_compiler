#!/usr/bin/env python3
"""P15: E -> id: E.place = id.place (Aho §6.4, diseño §9.18).

Comprueba capturas ante llamadas, lectura directa y la asignación simultánea R.
Reutiliza el intérprete C y el comparador O1/O0; no amplía el corpus de 39 fuentes.
"""
from pathlib import Path
import importlib.util
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def load(name, path):
    """Importa auxiliares existentes sin ejecutar sus suites."""
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


VALUES = load('valores', 'pruebas/representacion/tac_build/probar_valores.py')
COMPARE = load('comparador', 'pruebas/generador_o1/tac_emit/comparar.py')
HEADER = '''Vm g : int = 1 ;
F toca [] : int r ; g = 2 ; r = 0 ; E ; .
F combina [ int a , int b , int c ] : int r ; r = a * 100 + b * 10 + c ; E ; .
'''
CASES = [
    ('binaria', 'Vm x : int = g + toca [] ;', {'g': 2, 'x': 1}),
    ('binaria_profunda', 'Vm x : int = g + (3 * (toca [] + 1)) ;', {'g': 2, 'x': 4}),
    ('argumentos', 'Vm x : int = combina [g, g, toca []] ;', {'g': 2, 'x': 110}),
    ('argumentos_profunda', 'Vm x : int = combina [g, 0, 3 * (toca [] + 1)] ;', {'g': 2, 'x': 103}),
    ('matriz', 'Vm m : int [3][1] = [[3], [7], [9]] ; Vm x : int = m[g, toca []] ;',
     {'g': 2, 'x': 7}),
    ('asignacion_indice', 'Vm vector : int [1] ; vector[toca []] = g ;', {'g': 2, 'vector': [1]}),
    ('asignacion_matriz', 'Vm vector : int [3][1] ; vector[g, toca []] = g ;',
     {'g': 2, 'vector': [0, 1, 0]}),
    ('limites_s', 'Vm x : int = 0 ; S [ i, g, toca [] + 2, 1 ] ; x = x + 1 ; .',
     {'g': 2, 'x': 1}),
    ('s_fin_mutable', 'Vm x : int = 3 ; Vm vueltas : int = 0 ; S [ i, 0, x, 1 ] ; x = 0 ; vueltas = vueltas + 1 ; .',
     {'x': 0, 'vueltas': 3}),
    ('s_inicio_variable', 'Vm inicio : int = 1 ; Vm vueltas : int = 0 ; S [ i, inicio, 4, 1 ] ; inicio = 9 ; vueltas = vueltas + 1 ; .',
     {'inicio': 9, 'vueltas': 3}),
    ('s_anidados', 'Vm fin : int = 3 ; Vm vueltas : int = 0 ; S [ i, 0, 2, 1 ] ; S [ j, 0, fin, 1 ] ; fin = 1 ; vueltas = vueltas + 1 ; . .',
     {'fin': 1, 'vueltas': 4}),
    ('condicion_c', 'Vm x : int = 0 ; C [ g == toca [] + 1 ] ; x = 7 ; .',
     {'g': 2, 'x': 7}),
    ('condicion_l', 'Vm x : int = 0 ; L [ g == toca [] + 1 ] ; x = x + 1 ; .',
     {'g': 2, 'x': 1}),
    ('corto_circuito', 'Vm x : bool = (g == toca [] + 1) & (g == 2) ;',
     {'g': 2, 'x': 1}),
    ('corto_circuito_omitido', 'Vm x : bool = (g == 0) & (g == toca [] + 1) ;',
     {'g': 1, 'x': 0}),
    ('sin_llamada', '''Vm h : int = 3 ; Vm x : int ; Vm y : int ;
        Vm m : int [3][4] ; x = g + h ; y = combina [g, h, 0] ;
        m[g, h] = x ; x = m[g, h] ;''', {'g': 1, 'y': 130, 'x': 4}),
]


def compile_high(source, folder):
    """Captura HIGH y métricas por CLI; todos los destinos son temporales."""
    base = folder / 'high'
    result = subprocess.run([str(ROOT / 'ccom'), '-O1', '-i', '--metrics',
        str(base.with_suffix('.json')), str(source), '-o', str(base.with_suffix('.bin'))],
        capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stderr
    metrics = json.loads(base.with_suffix('.json').read_text())
    text = base.with_suffix('.tac').read_text()
    count = sum(line.startswith('  ') for line in text[text.index('function '):].splitlines())
    assert metrics['alto_nivel']['inicial'] == count, (metrics, count)
    return text


def compare(source, folder):
    """El comparador original verifica memoria y estado CRYPTO con dos imágenes."""
    COMPARE.compare(source, folder,
        lambda src, base: COMPARE.compile_new(ROOT / 'ccom', src, base))


def main():
    with tempfile.TemporaryDirectory(prefix='calser_lecturas_') as temporary:
        folder = Path(temporary)
        source = folder / 'caso.cls'
        for name, body, expected in CASES:
            source.write_text(HEADER + 'F main [] : void ; ' + body + ' E ; .\n')
            VALUES.check_values(VALUES.run(ROOT / 'build/interpretar_tac', source), expected)
            high = compile_high(source, folder)
            compare(source, folder)
            if name == 'sin_llamada':
                main_high = high.split('function main ')[1]
                assert re.search(r'  t\d+ = g \+ h\n  x = t\d+\n', main_high), high
                assert '  param g, 0\n  param h, 1\n' in main_high, high
                assert re.search(r'  t\d+ = g \* 4\n  t\d+ = t\d+ \+ h\n', main_high), high
                assert not re.search(r'  t\d+ = (g|h|x)\n', main_high), high
            print(f'PASS P15 {name}: intérprete, métricas y equivalencia O0')
        source.write_text('''F intercambia [ int a , int b , int n ] : int r ;
C [ n == 0 ] ; r = a * 10 + b ; E ; .
R intercambia [ b, a, n - 1 ] ; .
F main [] : void ; Vm x : int = intercambia [1, 2, 1] ; E ; .
''')
        VALUES.check_values(VALUES.run(ROOT / 'build/interpretar_tac', source), {'x': 21})
        compare(source, folder)
        print('PASS P15 R: parámetros simultáneos, intérprete y equivalencia O0')
        # El intérprete C rechaza CRYPTO por contrato; su orden se verifica con
        # el comparador completo y la captura explícita en HIGH (diseño §9.18).
        for operation in ('cifrar', 'descifrar'):
            source.write_text('''Vm origen : int = 4096 ; Vm destino : int = 8192 ;
F direccion [] : int r ; origen = 12288 ; destino = 16384 ; r = 8192 ; E ; .
F bloques [] : int r ; origen = 20480 ; destino = 24576 ; r = 1 ; E ; .
F main [] : void ; autenticar [-193210613] ; cargar_llave [0, 0, 7] ; sellar [] ;
''' + operation + ' [origen, direccion [], bloques [], 0] ;\n' +
                operation + ' [4096, destino, bloques [], 0] ; E ; .\n')
            result = VALUES.run(ROOT / 'build/interpretar_tac', source)
            assert result['status'] == 'unsupported_crypto', result
            high = compile_high(source, folder)
            assert re.search(r'  t\d+ = origen\n  t\d+ = call direccion', high), high
            assert re.search(r'  t\d+ = destino\n  t\d+ = call bloques', high), high
            compare(source, folder)
            print(f'PASS P15 {operation}: capturas HIGH y equivalencia O0; intérprete rechaza CRYPTO')
    print(f'PASS P15: {len(CASES) + 1} casos con intérprete y equivalencia; 2 controles CRYPTO por comparador')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
