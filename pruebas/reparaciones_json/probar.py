"""M4: casos de la entrega migrados al parser C y sus copias en memoria."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
VALIDO = 'F main [] : void ;\nE ;\n.\n'


def main():
    with tempfile.TemporaryDirectory(prefix='calser_repairs_') as tmp:
        carpeta = Path(tmp); fuente = carpeta/'fuente café 😀.cls'
        artefactos=[fuente.with_suffix(s) for s in ('.bin','.hex','.mem','.asm','.json')]
        for p in artefactos:p.write_bytes(b'preservar')
        def run(*args):
            return subprocess.run([str(ROOT/'ccom'),*map(str,args)],cwd=carpeta,capture_output=True,timeout=5)
        def reparar(texto):
            raw=texto.encode();fuente.write_bytes(raw)
            p=run('--repair-json',fuente); d=json.loads(p.stdout)
            assert p.returncode in (0,1) and d['exit_code']==p.returncode
            assert d['operation']=='repair' and d['offset_unit']=='utf8' and d['source']==str(fuente)
            assert bool(d['repairs'])==(d['status']=='ok') and len(d['repairs'])<=4
            assert fuente.read_bytes()==raw and all(p.read_bytes()==b'preservar' for p in artefactos)
            for r in d['repairs']:
                assert r['validation']['resolved'] and len(r['edits'])==1
                for c in r['preconditions']:
                    assert c['start']<c['end'] and raw[c['start']:c['end']].decode()==c['expected']
                e=r['edits'][0];assert e['start']==e['end'] and 0<=e['start']<=len(raw)
                raw[:e['start']].decode()
            return d
        def corregido(texto,r):
            raw=texto.encode(); e=r['edits'][0]
            return (raw[:e['start']]+e['text'].encode()+raw[e['end']:]).decode()
        assert reparar(VALIDO)['status']=='sin_opciones'
        # Sintaxis válida no implica semántica correcta: no inventar E ni operandos.
        assert reparar('F main [] : void;\n.')['status']=='sin_opciones'
        texto='F main [] : void ;\nVm x : int\nE ;\n.'
        d=reparar(texto); assert len(d['repairs'])==1
        r=d['repairs'][0]
        assert r['code']=='INSERCION_PUNTO_COMA' and r['automatic_safe'] and r['validation']['accepted']
        assert 'int;\nE' in corregido(texto,r)
        assert reparar(corregido(texto,r))['status']=='sin_opciones'
        # EOF incompleto: una reparación parcial puede ofrecerse solo manualmente.
        d=reparar('F main [] : void ;\nVm x : int = 3')
        assert d['repairs'][0]['code']=='INSERCION_PUNTO_COMA'
        assert not d['repairs'][0]['automatic_safe'] and not d['repairs'][0]['validation']['accepted']
        d=reparar('F main [] : void ;\nE ;')
        assert d['repairs'][0]['code']=='INSERCION_PUNTO_FINAL' and not d['repairs'][0]['automatic_safe']
        for texto,codigo in (
            ('F main [] : void ;\nC [verdadero ;\nE ;\n.','INSERCION_CORCHETE_CIERRE'),
            ('F main [] : void ;\nVm x : int = (3 + 2 ;\nE ;\n.','INSERCION_PARENTESIS_CIERRE')):
            d=reparar(texto)
            assert any(r['code']==codigo for r in d['repairs'])
            assert all(not r['automatic_safe'] for r in d['repairs'])
        assert reparar('F main [] : void ;\nx = 3 + ;\nE ;\n.')['status']=='sin_opciones'
        assert reparar('F main [] : void ;\nVm x : int @\nE ;\n.')['status']=='bloqueado'
        assert reparar('F main [] : void ; #[ abierto')['status']=='bloqueado'
        texto='#[ ñ 😀 ; ]#\r\nF main [] : void ;\r\nVm x : int # comentario ; aquí\r\nE ;\r\n.'
        d=reparar(texto); nuevo=corregido(texto,d['repairs'][0])
        assert 'int; # comentario ; aquí\r\nE' in nuevo and '#[ ñ 😀 ; ]#' in nuevo
        # Estado y buffers se reinician entre todos los candidatos de una consulta.
        base='F main [] : void ;\nVm x : int = (3 + 2 ;\nE ;\n.'
        assert reparar(base)==reparar(base)
        for texto,estado in ((VALIDO,'aceptado'),(VALIDO.replace('E ;','E'),'error_sintactico'),(VALIDO+'@','error_lexico')):
            fuente.write_text(texto)
            q=run('--syntax-json',fuente);d=json.loads(q.stdout)
            assert d['operation']=='syntax' and d['status']==estado
            assert q.returncode==(0 if estado=='aceptado' else 1)
        for flag in ('--repair-json','--syntax-json'):
            for otros in (['--check'],['--check-json'],['--tokens-json'],['--suggest-json','0'],['-x'],['-s'],['-t'],['-m'],['-p'],['--no-schedule'],['-o','x.bin'],['--ast-json','x.json']):
                assert run(flag,*otros,fuente).returncode==2
            assert run(flag,carpeta/'ausente.cls').returncode==1
            fuente.write_text(' '*200001)
            assert json.loads(run(flag,fuente).stdout)['status']=='limite'
            fuente.write_text(':'*50001)
            assert json.loads(run(flag,fuente).stdout)['status']=='limite'
            fuente.write_text(VALIDO)
            assert run(flag,fuente,'-v','--time').returncode==0
    print('PASS M4: cierres LAC, copias completas, precondiciones, EOF parcial, comentarios, Unicode y CLI')


if __name__=='__main__':main()
