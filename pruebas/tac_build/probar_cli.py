#!/usr/bin/env python3
"""C1: construcción en memoria, esperados, bandera y artefactos sin ejecutar TAC.

Usa: ccom, probar_tac_build y biblioteca estándar. Lo usa make check-tac-build.
Referencias: Aho §6.4, §6.6, §8.4.1; enunciado §4.1. El intérprete es C2.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CASES = {
    'factorial_l': 'pruebas/p02_factorial.cls',
    'factorial_r': 'pruebas/programas/factorial_r.cls',
    'corto_circuito': 'pruebas/programas/corto_circuito.cls',
    'expresiones': 'pruebas/tac_build/casos/expresiones.cls',
    'condicionales': 'pruebas/tac_build/casos/condicionales.cls',
    'inicializadores': 'pruebas/tac_build/casos/inicializadores.cls',
    's_positivo': 'pruebas/programas/s_positivo.cls',
    's_negativo': 'pruebas/programas/s_negativo.cls',
    's_paso3': 'pruebas/programas/s_paso3.cls',
    's_paso_grande': 'pruebas/programas/s_paso_grande.cls',
    's_anidados': 'pruebas/programas/s_anidados.cls',
    'e_en_s': 'pruebas/programas/e_en_s.cls',
    'arreglos': 'pruebas/p05_arreglos_llamadas.cls',
    'lectura_1d': 'pruebas/programas/lectura_1d.cls',
    'copia_arreglos': 'pruebas/programas/copia_arreglos.cls',
    'llamadas_anidadas': 'pruebas/p10_llamadas_anidadas.cls',
    'retorno_cero': 'pruebas/programas/retorno_cero.cls',
    'crypto': 'pruebas/p09_crypto.cls',
    'r_arreglos': 'pruebas/programas/r_arreglos.cls',
    'r_arreglos_local': 'pruebas/programas/r_arreglos_local.cls',
    'r_arreglos_2d': 'pruebas/programas/r_arreglos_2d.cls',
    'r_arreglos_parametro': 'pruebas/programas/r_arreglos_parametro.cls',
}


def run(*args, cwd=None, code=0):
    """Entrada: argumentos y código esperado. Salida: CompletedProcess.

    Efectos: ejecuta ccom con timeout; captura ambos flujos.
    Restricciones: todas las salidas deben dirigirse a la carpeta temporal.
    """
    result = subprocess.run([str(ROOT / 'ccom'), *map(str, args)], cwd=cwd,
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == code, (args, result.returncode, result.stderr)
    return result


def consecutive_blocks(text):
    """Comprueba números de presentación consecutivos por función, sin leer operandos."""
    number = None
    for line in text.splitlines():
        if line.startswith('function '):
            assert line.endswith('(entry B0)'), line
            number = 0
        match = re.fullmatch(r'B(\d+): successors \[.*\]', line)
        if match:
            assert number is not None and int(match[1]) == number, line
            number += 1


def cases(folder):
    """Compara estructura en memoria y CLI con los esperados de cada fuente.

    Entrada: carpeta temporal. Salida: ninguna.
    Efectos: ejecuta pruebas C y compila con/sin -i y organización.
    Restricciones: no interpreta TAC ni modifica los esperados.
    """
    for name, path in CASES.items():
        source = ROOT / path
        expected = (HERE / 'esperados' / (name + '.tac')).read_text()
        memory = subprocess.run([str(ROOT / 'build/probar_tac_build'), str(source)],
                                capture_output=True, text=True, timeout=30)
        assert memory.returncode == 0, (name, memory.stderr)
        assert memory.stdout == expected, name
        consecutive_blocks(memory.stdout)
        assert memory.stderr == '', memory.stderr
        for mode in ([], ['--no-schedule']):
            plain = folder / 'plain.bin'
            tac = folder / 'with_tac.bin'
            before = run('-s', *mode, source, '-o', plain)
            after = run('-s', '-i', *mode, source, '-o', tac)
            assert before.stdout == after.stdout and before.stderr == after.stderr
            for extension in ('.asm', '.bin', '.hex', '.mem'):
                assert plain.with_suffix(extension).read_bytes() == tac.with_suffix(extension).read_bytes(), (name, extension)
            assert tac.with_suffix('.tac').read_text() == expected, (name, mode)
            consecutive_blocks(tac.with_suffix('.tac').read_text())


def combinations(folder):
    """Entrada: carpeta temporal. Salida: ninguna.

    Efectos: comprueba nombres y combinaciones, incluido JSON temprano.
    Restricciones: las variantes sin -i conservan su contrato previo.
    """
    source = folder / 'programa.cls'
    source.write_text('F main [] : void ; Vm x : int = 3 ; E ; .\n')
    run('--tac', source, cwd=folder)
    assert source.with_suffix('.tac').is_file()
    custom = folder / 'salida.sin_bin'
    run('-i', source, '-o', custom)
    assert custom.with_suffix('.tac').is_file()
    json = folder / 'arbol.json'
    result = run('--ast-json', json, '-i', '--time', source, '-o', custom)
    assert 'tiempo de compilacion (completado)' in result.stderr
    assert json.is_file() and custom.with_suffix('.tac').is_file()
    plain = run('-s', '-t', '-m', '-p', source, '-o', custom)
    with_tac = run('-s', '-t', '-m', '-p', '-i', source, '-o', custom)
    assert (plain.stdout, plain.stderr) == (with_tac.stdout, with_tac.stderr)
    asm = custom.with_suffix('.asm')
    assembly = folder / 'ensamblado.bin'
    result = run('-x', '-i', asm, '-o', assembly)
    assert result.stderr == 'ccom: aviso: -i se ignora con -x\n'
    assert not assembly.with_suffix('.tac').exists()


def failures(folder):
    """Entrada: carpeta temporal. Salida: ninguna.

    Efectos: comprueba errores de fases y escritura, y conserva el fuente.
    Restricciones: /dev/full se usa solo donde existe; se elimina su enlace temporal.
    """
    for name in ('e52_caracter_invalido', 'e16_falta_punto_coma', 'e01_no_declarado'):
        target = folder / (name + '.bin')
        run('-i', ROOT / 'pruebas' / (name + '.cls'), '-o', target, code=1)
        assert not target.with_suffix('.tac').exists()
    target = folder / 'invalido.bin'
    json = folder / 'invalido.json'
    run('--ast-json', json, '-i', ROOT / 'pruebas/e58_crypto_k_rango.cls',
        '-o', target, code=1)
    assert json.is_file() and not target.with_suffix('.tac').exists()
    expression = 'x'
    for _ in range(12):
        expression = 'x + (' + expression + ')'
    source = folder / 'sin_registros.cls'
    source.write_text('F main [] : void ; Vm x : int = 1 ; Vm r : int ; r = '
                      + expression + ' ; E ; .\n')
    result = run('-i', source, '-o', target, code=1)
    assert 'error de generacion' in result.stderr
    assert not target.with_suffix('.tac').exists()
    good = folder / 'protegido.tac'
    text = 'F main [] : void ; E ; .\n'
    good.write_text(text)
    result = run('-i', good, '-o', good.with_suffix('.bin'), code=1)
    assert 'no puede sobrescribir el fuente' in result.stderr
    assert good.read_text() == text
    if Path('/dev/full').exists():
        target = folder / 'parcial.bin'
        partial = target.with_suffix('.tac')
        partial.symlink_to('/dev/full')
        result = run('-i', good, '-o', target, code=1)
        assert 'error al escribir' in result.stderr
        assert not partial.exists() and not partial.is_symlink()
    target = folder / 'directorio.bin'
    target.with_suffix('.tac').mkdir()
    run('-i', good, '-o', target, code=1)
    assert target.with_suffix('.tac').is_dir()


def main():
    """Entrada: ninguna. Salida: 0 con todas las aserciones satisfechas.

    Efectos: ejecuta las pruebas y elimina sus salidas temporales.
    Restricciones: ccom y probar_tac_build deben estar actualizados.
    """
    with tempfile.TemporaryDirectory(prefix='calser_tac_build_') as temporary:
        folder = Path(temporary)
        cases(folder)
        combinations(folder)
        failures(folder)
    print(f'PASS construccion TAC: {len(CASES)} esperados, memoria, CLI y artefactos identicos')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
