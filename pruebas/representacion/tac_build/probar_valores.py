#!/usr/bin/env python3
"""C2: valores del TAC en memoria, reutilizando CASES del simulador sin copiarlos.

Usa: interpretar_tac y biblioteca estándar. Lo usa make check-tac-values.
No modifica el constructor ni genera/lee listados .tac.
"""
from pathlib import Path
import argparse
import importlib.util
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def load_cases():
    """Entrada: ninguna. Salida: módulo de la suite existente, sin ejecutar main.

    Efectos: importa su tabla CASES; no inicia el simulador ni compila programas.
    Restricciones: la tabla original sigue siendo la única fuente de esperados.
    """
    path = ROOT / 'pruebas/generador_o0/programas/probar.py'
    spec = importlib.util.spec_from_file_location('programas_existentes', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(binary, source, steps=1000000):
    """Entrada: binario, fuente y presupuesto. Salida: resultado JSON del auxiliar.

    Efectos: ejecuta el intérprete con timeout y captura ambos flujos.
    Restricciones: un fallo del proceso o del semántico no es un resultado TAC.
    """
    result = subprocess.run([str(binary), str(source), str(steps)],
                            capture_output=True, text=True, timeout=30)
    if result.returncode or result.stderr:
        raise AssertionError((source, result.returncode, result.stderr))
    return json.loads(result.stdout)


def check_values(result, expected):
    """Entrada: ejecución y esperados prestados de CASES. Salida: ninguna.

    Efectos: informa el primer valor diferente con obtenido y esperado.
    Restricciones: solo acepta finalización normal; homónimos excluidos por el auxiliar.
    """
    assert result['status'] == 'ok', (result['status'], result['diagnostic'])
    for name, value in expected.items():
        wanted = value if isinstance(value, list) else [value]
        actual = result['values'].get(name)
        assert actual == wanted, f'{name}: obtenido {actual}, esperado {wanted}'


def extra_cases(binary, folder):
    """Entrada: binario y carpeta temporal. Salida: número de controles aprobados.

    Efectos: crea fuentes para orden, aritmética, errores y rechazo CRYPTO.
    Restricciones: no incorpora fuentes al corpus de compilación versionado.
    """
    cases = [
        ('valor antes de indices', '''
Vm orden : int = 0 ;
F valor [] : int r ; orden = orden * 10 + 1 ; r = 9 ; E ; .
F indice [] : int r ; orden = orden * 10 + 2 ; r = 0 ; E ; .
F main [] : void ; Vm a : int [1] ; a [ indice [] ] = valor [] ; E ; .
''', 'ok', {'orden': 12, 'a': [9]}),
        ('argumentos izquierda a derecha', '''
Vm orden : int = 0 ;
F marca [ int n ] : int r ; orden = orden * 10 + n ; r = n ; E ; .
F combina [ int a , int b ] : int r ; r = a * 10 + b ; E ; .
F main [] : void ; Vm salida : int ; salida = combina [ marca [1], marca [2] ] ; E ; .
''', 'ok', {'orden': 12, 'salida': 12}),
        ('lectura antes de llamada', '''
Vm g : int = 3 ;
F toca [] : int r ; g = 7 ; r = 1 ; E ; .
F main [] : void ; Vm x : int = g + toca [] ; E ; .
''', 'ok', {'g': 7, 'x': 4}),
        ('32 bits y signos', '''
F main [] : void ;
    Vm minimo : int = -2147483648 ; Vm menos : int = -1 ;
    Vm d : int = minimo / menos ; Vm m : int = minimo % menos ;
    Vm n : int = -7 ; Vm tres : int = 3 ;
    Vm q : int = n / tres ; Vm resto : int = n % tres ;
    Vm wrap : int = 2147483647 ; wrap = wrap + 1 ;
    Vm potencia : int = 2 ; potencia = potencia ^ 32 ;
    Vm negado : int = - minimo ;
    Vm menor : bool = minimo < 0 ; E ; .
''', 'ok', {'d': -2147483648, 'm': 0, 'q': -2, 'resto': -1,
           'wrap': -2147483648, 'potencia': 0, 'negado': -2147483648, 'menor': 1}),
        ('corto circuito evita error', '''
F main [] : void ; Vm z : int = 0 ;
    Vm a : bool = falso & (1 / z > 0) ;
    Vm b : bool = verdadero | (1 / z > 0) ; E ; .
''', 'ok', {'a': 0, 'b': 1}),
        ('division por cero', '''
F main [] : void ; Vm z : int = 0 ; Vm r : int = 1 / z ; E ; .
''', 'division_by_zero', {}),
        ('modulo por cero', '''
F main [] : void ; Vm z : int = 0 ; Vm r : int = 1 % z ; E ; .
''', 'division_by_zero', {}),
        ('exponente negativo', '''
F main [] : void ; Vm n : int = -1 ; Vm r : int = 2 ^ n ; E ; .
''', 'negative_exponent', {}),
        ('CRYPTO rechazado', '''
F main [] : void ; autenticar [ 0 ] ; E ; .
''', 'unsupported_crypto', {}),
        ('limite de pasos', '''
F main [] : void ; L [ verdadero ] ; . E ; .
''', 'step_limit', {}),
    ]
    failed = 0
    for index, (name, text, status, expected) in enumerate(cases):
        source = folder / f'control_{index}.cls'
        source.write_text(text)
        try:
            result = run(binary, source, steps=1000)
            assert result['status'] == status, result
            if status == 'ok':
                check_values(result, expected)
            else:
                assert result['diagnostic'], result
                assert not result['values'], result
            if status == 'step_limit':
                assert result['steps'] == 1000, result
            print(f'PASS TAC control {name}: {status}')
        except (AssertionError, subprocess.TimeoutExpired) as error:
            failed += 1
            print(f'FAIL TAC control {name}: {error}')
    return len(cases) - failed, failed


def main():
    """Entrada: --binario opcional para builds estrictos/sanitizados. Salida: 0/1.

    Efectos: ejecuta CASES originales, lista CRYPTO omitidos y agrega controles.
    Restricciones: los omitidos no se suman a aprobados; una diferencia falla el target.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', type=Path, default=ROOT / 'build/interpretar_tac')
    options = parser.parse_args()
    suite = load_cases()
    passed = 0
    skipped = 0
    failed = 0
    for name, source, expected in suite.CASES:
        try:
            result = run(options.binario, source)
            if result['status'] == 'unsupported_crypto':
                skipped += 1
                print(f'OMITIDO TAC {name}: CRYPTO no soportado')
                continue
            check_values(result, expected)
            passed += 1
            print(f'PASS TAC {name}: {suite.describe(expected)}')
        except (AssertionError, subprocess.TimeoutExpired) as error:
            failed += 1
            print(f'FAIL TAC {name}: {error}')
    # Casos CRYPTO especiales de la suite original, fuera de su tabla CASES.
    for name, source in [('p09', ROOT / 'pruebas/p09_crypto.cls'),
                         ('p11', ROOT / 'pruebas/p11_ejercicio3.cls')]:
        try:
            result = run(options.binario, source)
            assert result['status'] == 'unsupported_crypto', result
            skipped += 1
            print(f'OMITIDO TAC {name}: CRYPTO no soportado')
        except (AssertionError, subprocess.TimeoutExpired) as error:
            failed += 1
            print(f'FAIL TAC rechazo {name}: {error}')
    with tempfile.TemporaryDirectory(prefix='calser_tac_valores_') as temporary:
        controls, control_failures = extra_cases(options.binario, Path(temporary))
    failed += control_failures
    print(f'TAC valores: {passed} programas aprobados, {skipped} omitidos, '
          f'{controls} controles aprobados, {failed} fallos')
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
