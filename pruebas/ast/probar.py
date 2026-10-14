#!/usr/bin/env python3
"""Verifica que el AST JSON conserve la estructura y sea independiente del codegen."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def run(*args):
    return subprocess.run([str(ROOT / 'ccom'), *map(str,args)], capture_output=True, text=True)

def walk(node):
    yield node
    for child in node['children']:
        yield from walk(child)

with tempfile.TemporaryDirectory() as folder:
    folder = Path(folder)
    target = folder / 'ast.json'
    source = ROOT / 'pruebas/p09_crypto.cls'
    result = run('--ast-json', target, '-t', source)
    assert result.returncode == 0, result.stderr
    doc = json.loads(target.read_text())
    nodes = list(walk(doc['root']))
    assert doc['schema'] == 'calser.ast' and doc['version'] == 1
    assert doc['source'] == str(source)
    assert [n['id'] for n in nodes] == list(range(1,len(nodes)+1))
    assert [line.strip().split()[0] for line in result.stdout.splitlines()] == [n['kind'] for n in nodes]
    crypto = [n for n in nodes if n['kind'] == 'CRYPTO' and n.get('text') == 'cifrar'][0]
    assert [c.get('text', c.get('value')) for c in crypto['children'][0]['children']] == ['origen','cifrado',128,0]
    assert crypto['line'] > 0
    # Exportar solo AST no requiere que llamadas/arreglos tengan codegen.
    result = run(ROOT / 'pruebas/p10_llamadas_anidadas.cls', '--ast-json', target)
    assert result.returncode == 0, result.stderr
    assert any(n['kind']=='CALL' for n in walk(json.loads(target.read_text())['root']))
    # Exportación sintáctica: un error semántico no impide explorar el árbol.
    assert run('--ast-json', target, ROOT / 'pruebas/e58_crypto_k_rango.cls').returncode == 0
    unusual = folder / 'fuente"\\\n.cls'
    unusual.write_text(source.read_text())
    assert run('--ast-json', target, unusual).returncode == 0
    assert json.loads(target.read_text())['source'] == str(unusual)
    before = unusual.read_bytes()
    assert run('--ast-json', unusual, unusual).returncode == 1
    assert unusual.read_bytes() == before
    alias = folder / 'alias.cls'
    alias.symlink_to(unusual)
    assert run('--ast-json', alias, unusual).returncode == 1
    assert unusual.read_bytes() == before
    assert run('--ast-json', folder/'no-existe/out.json', source).returncode == 1
    broken = folder / 'invalido.cls'
    broken.write_text('F main [ ;')
    missing = folder/'invalido.json'
    assert run('--ast-json', missing, broken).returncode == 1 and not missing.exists()
    assert run('--ast-json').returncode == 2
    result = run('--ast-json', target, '-s', source, '-o', folder/'combinado.bin')
    assert result.returncode == 0, result.stderr
    assert (folder/'combinado.asm').exists() and (folder/'combinado.mem').exists()
print('PASS AST JSON: estructura, orden, IDs, ubicación, escapes, modo sintáctico, errores y combinación con -s')
