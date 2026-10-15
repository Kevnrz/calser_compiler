#!/usr/bin/env python3
"""Ejecutor opcional de pruebas Calser; Make solo prepara los binarios necesarios."""
import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable
sys.dont_write_bytecode = True


@dataclass(frozen=True)
class Suite:
    description: str
    targets: tuple
    commands: tuple


def python(script, *args):
    return (PYTHON, str(ROOT / script), *map(str, args))


SUITES = {
    'aritmetica': Suite('Multiplicacion, division, modulo y potencia',
        ('build/emitir_aritmetica',),
        (python('pruebas/aritmetica/probar.py', ROOT / 'build/emitir_aritmetica'),)),
    'arreglos': Suite('Indices, memoria, copia y paso de arreglos',
        ('build/emitir_arreglos',),
        (python('pruebas/arreglos/probar.py', ROOT / 'build/emitir_arreglos'),)),
    'crypto': Suite('Rutinas crypto e integracion con el compilador',
        ('build/emitir_crypto', 'ccom'),
        (python('pruebas/crypto/probar.py', ROOT / 'build/emitir_crypto'),
         python('pruebas/crypto/probar_integracion.py'))),
    'ast': Suite('Exportacion y estructura del AST', ('ccom',),
        (python('pruebas/ast/probar.py'),)),
    'ensamblador': Suite('Ensamblado, formatos y diagnosticos', ('ccom',),
        (python('pruebas/ensamblador/probar.py'),)),
    'programas': Suite('Valores finales de programas en el simulador', ('ccom',),
        (python('pruebas/programas/probar.py'),)),
    'ayuda': Suite('Banderas -h y --help', ('ccom',),
        (python('pruebas/ayuda/probar.py'),)),
    'tiempo': Suite('Medicion con --time', ('ccom',),
        (python('pruebas/tiempo/probar.py'),)),
    'matrices': Suite('Suma, resta y productos matriciales en ambos modos', ('ccom',),
        (python('pruebas/matrices/probar.py'),)),
    'etiquetas': Suite('Resolucion de etiquetas y direcciones', ('ccom',),
        (python('pruebas/etiquetas/probar.py'),)),
    'simulador': Suite('Modelo de software de la ISA', ('ccom',),
        (python('pruebas/simulador/probar.py'),)),
    'bundles': Suite('IR, dependencias y organizacion de bundles',
        ('ccom', 'build/probar_ir', 'build/revisar_ir',
         'build/emitir_organizador', 'build/probar_dependencias'),
        ((str(ROOT / 'build/probar_ir'),),
         (str(ROOT / 'build/probar_dependencias'),),
         python('pruebas/bundles/probar.py'),
         python('pruebas/bundles/probar_organizador.py'))),
    'salidas': Suite('Coherencia de ASM, BIN, HEX y MEM', ('ccom',),
        (python('pruebas/bundles/probar_salidas.py'),)),
    'validar-bundles': Suite('Aceptacion de paralelismo y reporte de factorial', ('ccom',),
        (python('tools/validar_bundles.py'),)),
    'ejecutor': Suite('Seleccion, fallos y resumen del ejecutor', (),
        (python('pruebas/ejecutor/probar.py'),)),
}


def execute(command, options):
    """Ejecuta sin shell desde la raiz; informa fallos y conserva los logs utiles."""
    environment = os.environ.copy()
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    # Las suites existentes usan assert para verificar resultados.
    environment.pop('PYTHONOPTIMIZE', None)
    try:
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                capture_output=True, text=True, timeout=options.timeout)
    except (OSError, subprocess.TimeoutExpired) as error:
        print('ERROR: %s' % error, file=sys.stderr)
        return None
    if options.detalle or result.returncode:
        if result.stdout:
            print(result.stdout, end='' if result.stdout.endswith('\n') else '\n')
        if result.stderr:
            print(result.stderr, file=sys.stderr,
                  end='' if result.stderr.endswith('\n') else '\n')
    return result.returncode


def prepare(targets, options):
    if options.sin_compilar or not targets:
        return True
    print('Preparando: %s' % ', '.join(targets), flush=True)
    return execute(('make', '--no-print-directory', *targets), options) == 0


def summary(passed, failed, skipped, start):
    print('Resumen: %d aprobadas, %d fallidas, %d omitidas (%.3f s)' %
          (passed, failed, skipped, time.monotonic() - start), flush=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, epilog=
        'Sin suites se ejecutan todas. Ejemplo: python3 tools/probar.py matrices tiempo ayuda')
    parser.add_argument('suites', nargs='*', metavar='SUITE', help='nombres de --listar; todas por defecto')
    parser.add_argument('--listar', action='store_true', help='lista las suites sin construir ni ejecutar')
    parser.add_argument('--sin-compilar', action='store_true', help='usa los binarios existentes sin llamar a Make')
    parser.add_argument('--detalle', action='store_true', help='muestra tambien la salida de comandos exitosos')
    parser.add_argument('--detener-al-fallar', action='store_true', help='omite las suites restantes tras el primer fallo')
    parser.add_argument('--timeout', type=float, default=300, metavar='SEGUNDOS', help='limite por comando, incluida la construccion (300 por defecto)')
    parser.add_argument('--caso', type=Path, metavar='ARCHIVO.cls', help='compila un caso individual; ruta relativa al directorio actual')
    parser.add_argument('--codigo-esperado', type=int, choices=(0, 1), help='codigo esperado de --caso (0 por defecto; 1 para un programa invalido)')
    options = parser.parse_args(argv)
    if not 0 < options.timeout < float('inf'):
        parser.error('--timeout debe ser un numero positivo y finito')
    if options.caso and options.suites:
        parser.error('--caso no se combina con suites')
    if options.codigo_esperado is not None and not options.caso:
        parser.error('--codigo-esperado requiere --caso')
    unknown = [name for name in options.suites if name not in SUITES]
    if unknown:
        parser.error('suites desconocidas: %s; consultar --listar' % ', '.join(unknown))
    if options.listar:
        for name, suite in SUITES.items():
            print('%-17s %s' % (name, suite.description))
        return 0
    source = options.caso.resolve() if options.caso else None
    if source and (source.suffix != '.cls' or not source.is_file()):
        parser.error('--caso requiere un archivo .cls existente')

    start = time.monotonic()
    if source:
        if not prepare(('ccom',), options):
            print('FAIL preparacion: no se ejecuto el caso')
            return 1
        with tempfile.TemporaryDirectory(prefix='calser_caso_') as temporary:
            command = (str(ROOT / 'ccom'), '-s', '-m', str(source),
                       '-o', str(Path(temporary) / 'salida.bin'))
            code = execute(command, options)
        expected = options.codigo_esperado if options.codigo_esperado is not None else 0
        passed = code == expected
        print('%s %s: codigo esperado %d, obtenido %s' %
              ('PASS' if passed else 'FAIL', source.name, expected, code))
        summary(int(passed), int(not passed), 0, start)
        return 0 if passed else 1

    selected = list(dict.fromkeys(options.suites or SUITES))
    targets = list(dict.fromkeys(target for name in selected for target in SUITES[name].targets))
    if not prepare(targets, options):
        print('FAIL preparacion: no se ejecutaron las %d suites seleccionadas' % len(selected))
        return 1
    passed = failed = 0
    for name in selected:
        print('Ejecutando %s...' % name, flush=True)
        ok = True
        for command in SUITES[name].commands:
            if execute(command, options) != 0:
                ok = False
                break
        passed += int(ok)
        failed += int(not ok)
        print('%s %s' % ('PASS' if ok else 'FAIL', name), flush=True)
        if not ok and options.detener_al_fallar:
            break
    summary(passed, failed, len(selected) - passed - failed, start)
    return 1 if failed else 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print('\nPruebas interrumpidas.', file=sys.stderr)
        raise SystemExit(130)
