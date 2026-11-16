#!/usr/bin/env python3
"""E: banderas, JSON y salidas CLI; el corpus cubre los valores y la ABI."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
BINARY = Path(os.environ.get('CCOM_TEST_BINARY', ROOT / 'ccom'))
SOURCE = ROOT / 'pruebas/p02_factorial.cls'
ORDER = ['unroll', 'rename', 'dce', 'reorder', 'spill']
KEYS = {'programa', 'nivel', 'pasadas', 'alto_nivel', 'isa_antes_expandir', 'emitidas',
        'bundles', 'nops', 'bytes', 'demanda', 'presion', 'mascara_registros',
        'tiempo_ms', 'contadores', 'ciclos_sim'}


def check_dce(folder):
    """C3.2: HIGH exacto, JSON y estado observable de §14.3 en el simulador."""
    sys.path.insert(0, str(ROOT / 'pruebas/generador_o1/tac_emit'))
    from comparar import compile_new, execute, observable
    cases = json.loads((ROOT / 'pruebas/optimizador/dce/esperados.json').read_text())
    for name, case in cases.items():
        removed, blocks = case['eliminadas'], case['bloques_eliminados']
        expected = case['valores']
        source = ROOT / 'pruebas/optimizador/dce' / (name + '.cls')
        before, after = folder / 'antes.tac', folder / 'despues.tac'
        subprocess.run([str(ROOT / 'build/probar_dce'), str(source), str(before),
                        str(after), str(removed), str(blocks)], check=True)
        assert before.read_bytes() == source.with_suffix('.antes.tac').read_bytes()
        assert after.read_bytes() == source.with_suffix('.despues.tac').read_bytes()
        metrics = folder / 'dce.json'
        base = folder / name
        result = subprocess.run([str(BINARY), '-O1', '--opt=dce', '-s', '-i',
            '--metrics', str(metrics), str(source), '-o', str(base) + '.bin'],
            capture_output=True, timeout=30)
        assert result.returncode == 0, result.stderr
        data = json.loads(metrics.read_text())
        assert data['contadores']['eliminadas'] == removed
        assert data['contadores']['bloques_eliminados'] == blocks
        # -i conserva el HIGH inicial; el auxiliar captura el resultado de la pasada.
        assert base.with_suffix('.tac').read_bytes() == before.read_bytes()
        baseline, regions = compile_new(BINARY, source, folder / 'sin_dce')
        for external in (bytes(1024), bytes(range(256)) * 4):
            sim = execute(base, external)
            assert observable(sim, regions) == observable(execute(baseline, external), regions)
            selected = False
            values = {}
            for line in base.with_suffix('.asm').read_text().splitlines():
                scope = re.match(r'#\s+Scope \d+: (\w+)(?: (\w+))?$', line)
                if scope:
                    selected = scope[1] == 'GLOBAL' or scope[2] == 'main'
                symbol = re.search(r'#\s+(\w+).* width=(\d+) addr=0x([0-9A-Fa-f]+)', line)
                if selected and symbol:
                    address = int(symbol[3], 16)
                    values[symbol[1]] = [int.from_bytes(sim.memory[a:a+4], 'little', signed=True)
                                        for a in range(address, address + int(symbol[2]), 4)]
            assert all(values.get(k) == v for k, v in expected.items()), (name, values)
            if 'auth' in case:
                assert sim.auth == case['auth'] and sim.exc == case['exc']
    print('PASS DCE CLI: d1=3/0 g=3 v=4; d2=0/1 g=2; d3=3/0 datos=[8,0]; 6 HIGH exactos')


def check_rename(folder):
    """C3.3a: HIGH exacto, JSON y estado observable de §14.2 en el simulador."""
    sys.path.insert(0, str(ROOT / 'pruebas/generador_o1/tac_emit'))
    from comparar import compile_new, execute, observable
    cases = json.loads((ROOT / 'pruebas/optimizador/rename/esperados.json').read_text())
    for name, case in cases.items():
        renamed = case['renombradas']
        expected = case['valores']
        source = ROOT / 'pruebas/optimizador/rename' / (name + '.cls')
        before, after = folder / 'antes.tac', folder / 'despues.tac'
        subprocess.run([str(ROOT / 'build/probar_rename'), str(source), str(before),
                        str(after), str(renamed)], check=True)
        assert before.read_bytes() == source.with_suffix('.antes.tac').read_bytes()
        assert after.read_bytes() == source.with_suffix('.despues.tac').read_bytes()
        metrics = folder / 'rename.json'
        base = folder / name
        result = subprocess.run([str(BINARY), '-O1', '--opt=rename', '-s', '-i',
            '--metrics', str(metrics), str(source), '-o', str(base) + '.bin'],
            capture_output=True, timeout=30)
        assert result.returncode == 0, result.stderr
        data = json.loads(metrics.read_text())
        assert data['contadores']['renombradas'] == renamed
        assert data['alto_nivel']['despues']['rename'] == data['alto_nivel']['inicial']
        # -i conserva el HIGH inicial; el auxiliar captura el resultado de la pasada.
        assert base.with_suffix('.tac').read_bytes() == before.read_bytes()
        baseline, regions = compile_new(BINARY, source, folder / 'sin_rename')
        for external in (bytes(1024), bytes(range(256)) * 4):
            sim = execute(base, external)
            assert observable(sim, regions) == observable(execute(baseline, external), regions)
            selected = False
            values = {}
            for line in base.with_suffix('.asm').read_text().splitlines():
                scope = re.match(r'#\s+Scope \d+: (\w+)(?: (\w+))?$', line)
                if scope:
                    selected = scope[1] == 'GLOBAL' or scope[2] == 'main'
                symbol = re.search(r'#\s+(\w+).* width=(\d+) addr=0x([0-9A-Fa-f]+)', line)
                if selected and symbol:
                    address = int(symbol[3], 16)
                    values[symbol[1]] = [int.from_bytes(sim.memory[a:a+4], 'little', signed=True)
                                        for a in range(address, address + int(symbol[2]), 4)]
            assert all(values.get(k) == v for k, v in expected.items()), (name, values)
            if 'auth' in case:
                assert sim.auth == case['auth'] and sim.exc == case['exc']
    print('PASS rename CLI: r1=2 salida=18; r2=2 g=20; r3=0 g=2 x=2 y=8; 6 HIGH exactos')


def main():
    with tempfile.TemporaryDirectory(prefix='calser_e_') as temporary:
        folder = Path(temporary)
        check_dce(folder)
        check_rename(folder)
        output = folder / 'salida.bin'
        metrics = folder / 'metricas.json'

        def run(*args, source=SOURCE, expected=0):
            result = subprocess.run([str(BINARY), *map(str, args), str(source),
                                     '-o', str(output)], capture_output=True, text=True, timeout=60)
            assert result.returncode == expected, (args, result.returncode, result.stderr)
            return result

        for args in [('-O2',), ('-O0', '-O1'), ('-O1', '-O0'),
                     ('--opt=',), ('--opt=dce,',), ('--opt=desconocida',),
                     ('--opt=dce', '-O0'), ('-O0', '--opt=dce'),
                     ('--unroll=2',), ('--opt=dce', '--unroll=2'),
                     ('--opt=unroll', '--unroll=1'), ('--opt=unroll', '--unroll=17'),
                     ('--opt=unroll', '--unroll=no'), ('--metrics', metrics),
                     ('-O0', '--metrics', metrics),
                     *[('-x', option) for option in ('-O1', '-O2', '--opt=dce', '--unroll=2')],
                     ('-x', '--metrics', metrics), ('-O9',)]:
            run(*args, expected=2)
            assert not metrics.exists()
        for factor in (2, 16):
            run('--opt=unroll', f'--unroll={factor}')
        missing = folder / 'ausente.cls'
        assert str(missing) in run('-O1', source=missing, expected=1).stderr
        run('-O1', '-O1')
        run('--opt=dce', '--metrics', metrics)
        assert json.loads(metrics.read_text())['pasadas'] == ['dce']
        run('-O1', '-s', '-i', '--metrics', metrics)
        initial = output.with_suffix('.tac').read_bytes()
        data = json.loads(metrics.read_text())
        assert data['pasadas'] == [] and data['alto_nivel']['despues'] == {}
        assert data['tiempo_ms']['pasadas'] == {}
        assert set(data) == KEYS and data['nivel'] == 'O1'
        assert data['programa'] == str(SOURCE) and data['ciclos_sim'] is None
        assert set(data['alto_nivel']) == {'inicial', 'despues'}
        assert set(data['tiempo_ms']) == {'total', 'pasadas'}
        assert data['tiempo_ms']['total'] >= 0
        assert data['nops'] == 4 * data['bundles'] - data['emitidas'] >= 0
        assert data['bytes'] == 16 * data['bundles']
        assert data['demanda'] < data['presion'] < int(data['mascara_registros'], 16).bit_count()
        for name in ('isa_antes_expandir', 'emitidas', 'bundles', 'nops', 'bytes', 'demanda', 'presion'):
            assert type(data[name]) is int and data[name] >= 0
        assert re.fullmatch(r'0x[0-9a-f]{4}', data['mascara_registros'])
        assert data['contadores'] == dict(desenrollados=[], renombradas=0, eliminadas=0,
            bloques_eliminados=0, reordenadas=0, derramados=0, cargas_derrame=0, guardados_derrame=0)
        options = ['--opt=spill,dce', '--opt=reorder,rename,unroll,dce']
        first = run(*options, '-s', '-i', '-v', '--metrics', metrics)
        full = json.loads(metrics.read_text())
        assert full['pasadas'] == ORDER
        assert list(full['alto_nivel']['despues']) == ORDER[:3]
        assert list(full['tiempo_ms']['pasadas']) == ORDER
        assert all(n == full['alto_nivel']['inicial'] for n in full['alto_nivel']['despues'].values())
        assert all(n >= 0 for n in full['tiempo_ms']['pasadas'].values())
        assert output.with_suffix('.tac').read_bytes() == initial
        for phase in ('construccion HIGH', 'bajada', 'asignacion', 'emision', *ORDER):
            assert phase in first.stderr
        previous = output.with_suffix('.asm').read_bytes()
        run('--opt=unroll,rename,dce,reorder,spill', '-s')
        assert output.with_suffix('.asm').read_bytes() == previous
        labels = run('-O1', '-m').stdout
        listing = output.with_suffix('.asm').read_text()
        for name, address in re.findall(r'^#\s+(\S+)\s+(\d+)(?:  \(interna\))?$', listing, re.M):
            assert f'  {name} = {address}' in labels
        assert '.L_o1_' in labels and 'fin_main = ' in labels
        run('-O1', '--no-schedule', '-s', '--metrics', metrics)
        conservative = json.loads(metrics.read_text())
        assert conservative['bundles'] >= data['bundles']
        assert conservative['emitidas'] == data['emitidas']
        assert conservative['bytes'] == conservative['bundles'] * 16
        # Mismo serializador de AST, incluido el escape de controles/ruta.
        escaped = folder / 'ruta"\\\n.cls'
        escaped.write_bytes(SOURCE.read_bytes())
        run('-O1', '--metrics', metrics, source=escaped)
        assert json.loads(metrics.read_text())['programa'] == str(escaped)
        ast = folder / 'ast.json'
        run('-O1', '--ast-json', ast)
        assert ast.exists() and output.exists()
        invalid = folder / 'invalido.cls'
        invalid.write_text('F main [] : void ; x = 1 ; E ; .\n')
        metrics.unlink()
        output.unlink()
        run('-O1', '--ast-json', ast, '--metrics', metrics, source=invalid, expected=1)
        assert ast.exists() and not output.exists() and not metrics.exists()
        # La CLI debe aplicar ROM al flujo conservador; no repite 4096/4097 de D.
        large = folder / 'rom.cls'
        large.write_text('F main [] : void ; E ; .\n' + ''.join(
            f'F f{k} [] : void ; Vm x : int = 0 ;\n' +
            'x = x + 1 ;\n' * 20 + 'E ; .\n' for k in range(50)))
        result = run('-O1', '--no-schedule', '--metrics', metrics, source=large, expected=1)
        assert str(large) in result.stderr and '4096' in result.stderr
        assert not metrics.exists()
        run('-h')
    print('PASS CLI E: opciones, union/orden, JSON, HIGH inicial, etiquetas y ROM conservadora')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
