"""Protocolo CCL1/JSON nativo: equivalencia, rangos, límites y no artefactos."""
import json
from pathlib import Path
import struct
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[3]


def paquete(documentos):
    partes = [b'CCL1', struct.pack('<I', len(documentos))]
    for nombre, texto in documentos:
        nombre = nombre.encode(); raw = texto.encode()
        partes += [struct.pack('<II', len(nombre), len(raw)), nombre, raw]
    return b''.join(partes)


def main():
    with tempfile.TemporaryDirectory(prefix='calser_clones_') as tmp:
        carpeta = Path(tmp)
        entrada = carpeta / 'consulta.ccl'
        for sufijo in ('.bin', '.hex', '.mem', '.asm', '.json'):
            entrada.with_suffix(sufijo).write_bytes(b'preservar')
        def run(*args):
            return subprocess.run([str(ROOT/'ccom'), *map(str,args)], capture_output=True, cwd=carpeta, timeout=15)
        def consulta(documentos=(), minimo=5, raw=None, estado='ok', extras=()):
            entrada.write_bytes(paquete(documentos) if raw is None else raw)
            resultado = run('--clones-json', minimo, *extras, entrada)
            assert resultado.returncode == (estado != 'ok'), resultado.stderr
            d = json.loads(resultado.stdout)
            assert d['schema_version'] == 1 and d['operation'] == 'clones'
            assert d['source'] == str(entrada) and d['minimum'] == minimo and d['offset_unit'] == 'utf8'
            assert d['status'] == estado and d['exit_code'] == resultado.returncode, d
            assert estado == 'ok' or (not d['pairs'] and d['explanation'])
            if estado == 'ok':
                assert d['files'] == [{'id': i, 'path': name, 'bytes': len(text.encode())} for i,(name,text) in enumerate(documentos)]
                for par in d['pairs']:
                    assert par['tokens'] >= minimo and par['a']['file'] < par['b']['file']
                    for f in (par['a'],par['b']):
                        text = documentos[f['file']][1].encode()
                        assert 0 <= f['start'] < f['end'] <= len(text)
                        text[:f['start']].decode(); text[:f['end']].decode()
                        assert f['line_start'] == text[:f['start']].count(b'\n')+1
                        assert f['line_end'] == text[:f['end']].count(b'\n')+1
            assert all(entrada.with_suffix(s).read_bytes() == b'preservar' for s in ('.bin','.hex','.mem','.asm','.json'))
            return d
        a='# café 😀\r\nVm x : int = 5 ;\r\nx = x + 1 ;'
        b='#[ bloque\n 😀 ]#\nVm x:int=5; x=x+1;'
        d=consulta([('/no/existe/a.cls',a),('/no/existe/b.cls',b)],10)
        assert len(d['pairs']) == 1 and d['pairs'][0]['type'] == 'I'
        assert d['pairs'][0]['a']['line_start'] == 2 and d['pairs'][0]['b']['line_start'] == 3
        assert consulta([('/a',a),('/b','Vm contador:int=10; contador=contador+2;')],10)['pairs'][0]['type']=='II'
        for x,y,n in [('x=x+y;', 'a=b+b;',6),('x=x+1;', 'y=y-2;',6),
                    ('cifrar[x,1];','descifrar[y,2];',7),('Vm x:int=1;','Vm y:bool=verdadero;',7),
                    ('x=y+x;','a=b+b;',6)]:
            assert not consulta([('/a',x),('/b',y)],n)['pairs'], (x,y)
        d=consulta([('/a','Vm x:bool=verdadero;x=falso;'),('/b','Vm y:bool=falso;y=verdadero;')],10)
        assert [p['type'] for p in d['pairs']]==['II']
        assert not consulta([('/a','x=1;'*1000)],10)['pairs']
        assert not consulta([('/a','E;.'),('/b','E;.')])['pairs']
        assert not consulta([])['pairs']
        assert not consulta([('/vacío 😀',''),('/b','')])['pairs']
        # Real Flex errors must not be normalized or compared after recovery.
        for invalid in ('@', 'ABC', '"cadena"', '2147483649', 'a'*33, '#[ abierto'):
            consulta([('/a','x=x+1;'+invalid),('/b','y=y+2;')],estado='lexical_error')
        consulta([('/a','x=x+1;')]*2,estado='error')
        for raw in (b'', b'CCL0'+struct.pack('<I',0), b'CCL1', paquete([])+b'extra',
                    b'CCL1'+struct.pack('<III',1,3,10)+b'/a',
                    b'CCL1'+struct.pack('<III',1,2,0)+b'/\0'):
            consulta(raw=raw,estado='error')
        consulta(raw=b'CCL1'+struct.pack('<I',201),estado='limit')
        consulta(raw=b'CCL1'+struct.pack('<III',1,2,2000001),estado='limit')
        consulta(raw=b'CCL1'+struct.pack('<III',1,16385,0),estado='limit')
        consulta([('/a','x;'*50001)],estado='limit')
        consulta([('/a',' '*2000000)]*0 + [(f'/{i}',' '*2000000) for i in range(11)],estado='limit')
        # Repeated seeds have an explicit bound; partial pairs are never success.
        consulta([('/a','x=1;'*2000),('/b','y=2;'*2000)],estado='limit')
        consulta([('/a','x;'*25000)],minimo=100000)
        consulta([('/a','x;'*10000)],minimo=10000,estado='limit')
        d=consulta([('/a',a),('/b',b)],10,extras=('-v','--time'))
        assert d == consulta([('/a',a),('/b',b)],10)
        # Query options and artifact-producing options are exclusive.
        for flags in (['-s'],['-x'],['-o','destino.bin'],['--check'],['--tokens-json'],
                      ['--repair-json'],['--syntax-json'],['--suggest-json','0'],['--check-json'],
                      ['--ast-json','ast.json'],['-p'],['-t'],['-m'],['--no-schedule']):
            assert run('--clones-json','5',*flags,entrada).returncode==2
        for minimo in ('4','-5','100001','abc','5x','+5','18446744073709551616'):
            assert run('--clones-json',minimo,entrada).returncode==2
        entrada.unlink()
        consulta(raw=b'CCL1'+struct.pack('<I',0))
        entrada.unlink()
        absent=run('--clones-json',5,entrada)
        assert absent.returncode==1 and json.loads(absent.stdout)['status']=='error'
    print('PASS clones C: tipos I/II, Flex, Unicode/CRLF, paquete, limites, CLI y artefactos.')


if __name__ == '__main__':
    main()
