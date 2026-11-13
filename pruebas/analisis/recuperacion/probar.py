"""M7: recuperación C, precisión, progreso y prohibición de salidas parciales."""
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[3]
CCOM = os.environ.get('CCOM_PRUEBA_RECUPERACION', str(ROOT/'ccom'))
DOS = '# café 😀\r\nF main [] : void ;\r\n    Vm x : int = ;\r\n    Vm y : int = ;\r\n    E ;\r\n.\r\n'


def main():
    with tempfile.TemporaryDirectory(prefix='calser_recovery_') as tmp:
        folder=Path(tmp);source=folder/'fuente con espacios.cls'
        def run(*flags):
            return subprocess.run([CCOM,*map(str,flags),str(source)],capture_output=True,cwd=folder,timeout=10)
        def check(text, lines=None, status='errors'):
            source.write_text(text,encoding='utf-8',newline='')
            before=source.read_bytes()
            p=run('--check-json');assert p.returncode==1,p.stderr
            d=json.loads(p.stdout);assert d['status']==status,d
            syntax=[v for v in d['diagnostics'] if v['category']=='syntax']
            assert not any(v['category']=='semantic' for v in d['diagnostics']),d
            if lines is not None:
                assert [v['range']['line'] for v in syntax]==lines,d
            for v in syntax:
                a=v['range']; assert a and 0<=a['start']<=a['end']<=len(before)
                assert a['line']==before[:a['start']].count(b'\n')+1
            assert source.read_bytes()==before
            return d
        d=check(DOS,[3,4]); assert d['error_count']==2 and not d['truncated']
        # Follow-up diagnostics only change after actual edits; ccom never repairs.
        check(DOS.replace('x : int = ;','x : int = 1 ;'),[4])
        source.write_text(DOS.replace('int = ;','int = 1 ;'))
        assert run('--check-json').returncode==0
        check('Vm x:int=;\nVm y:int=;\nF main[]:void; E;.',[1,2])
        check('F mala [:void; .\nF main[]:void; Vm x:int=; E;.',[1,2])
        check('F main[]:void;\n C[verdadero];\n Vm x:int=;\n .\n Vm y:int=;\n E; .',[3,5])
        check('F main[]:void;\n C[verdadero];\n Vm x:int= A[falso];\n Vm y:int=;\n . E; .',[3,4])
        check('F main[]:void;\n L[verdadero];\n Vm x:int=;\n .\n Vm y:int=;\n E; .',[3,5])
        check('F main[]:void;\n Vm x:int=(;\n Vm y:int=;\n E; .',[2,3])
        check('F main[]:void;\n Vm x:int= .\nF otra[]:void;\n Vm y:int=; E; .',[2,4])
        d=check('F main[]:void; C[verdadero]; A; A; . E;.')
        claves=[(v['range']['start'],v['message']) for v in d['diagnostics']]
        assert len(claves)==len(set(claves)) and len(claves)<5,d
        # Missing separators conservatively skip to the next anchor, not invent it.
        check('F main[]:void;\n Vm x:int=1\n Vm y:int=2;\n Vm z:int=; E; .',[2,4])
        d=check('F main[]:void; Vm x:int=; #[ abierto')
        assert len([v for v in d['diagnostics'] if v['category']=='lexical'])==1
        assert len([v for v in d['diagnostics'] if v['category']=='syntax'])==1
        d=check('F main[]:void; Vm x:int=;')
        assert len(d['diagnostics'])<=2
        # Invalid closure before a new F cannot be guessed. Stop without cascading.
        d=check('F main[]:void; Vm x:int= F otra[]:void; Vm y:int=; E;.')
        assert len(d['diagnostics'])==1
        d=check('F main[]:void; '+('Vm x:int=; '*50)+'E;.',status='limit')
        assert d['truncated'] and len(d['diagnostics'])==32
        d=check('F main[]:void; Vm x:int= '+('nombre '*4100)+'; E;.',status='limit')
        assert d['truncated'] and '4096' in d['diagnostics'][-1]['message']
        # No semántica, AST JSON, listings, code generation or artifact truncation.
        source.write_text(DOS)
        targets=[folder/('preservar'+ext) for ext in ('.bin','.hex','.mem','.asm','.json')]
        for p in targets:p.write_bytes(b'preservar')
        for args in (['--check'],['-t'],['--ast-json',targets[-1]],['-s','-o',targets[0]],['-m','-p','-o',targets[0]]):
            p=run(*args); assert p.returncode==1 and not p.stdout,(args,p.stdout,p.stderr)
            assert b'error semantico' not in p.stderr and b'correcta' not in p.stderr
            assert all(p.read_bytes()==b'preservar' for p in targets)
        assert not source.with_suffix('.bin').exists()
        # M3/M4 still use only the first real error, never a recovered acceptance.
        source.write_text(DOS)
        p=run('--syntax-json');assert p.returncode==1 and json.loads(p.stdout)['status']=='error_sintactico'
        p=run('--repair-json');assert not json.loads(p.stdout)['repairs']
        source.write_text('F main[]:void; Vm x:int=3\n E;.')
        d=json.loads(run('--repair-json').stdout)
        assert d['repairs'][0]['validation']['accepted']
        source.write_text('F main[]:void; Vm x:int=; Vm y:int=3\n E;.')
        d=json.loads(run('--suggest-json',str(len(source.read_bytes()))).stdout)
        assert d['status']=='bloqueado'
        # Deterministic mutation corpus checks progress, parser cleanup and bounds.
        rand=random.Random(711)
        base='F main[]:void; Vm x:int=3; C[verdadero]; x=x+1; . E; .'.split()
        alphabet=[';', '.', '[', ']', '(', ')', 'Vm', 'C', 'A', 'E', 'F', 'nombre', '=', '1', '']
        for i in range(120):
            parts=base.copy()
            for _ in range(rand.randrange(1,6)):
                parts.insert(rand.randrange(len(parts)+1),rand.choice(alphabet))
            source.write_text(' '.join(parts))
            p=run('--check-json');assert p.returncode in (0,1),p.stderr
            d=json.loads(p.stdout);assert len(d['diagnostics'])<=32,d
            claves=[(v['range']['start'] if v['range'] else None,v['message']) for v in d['diagnostics']]
            assert len(claves)==len(set(claves)),d
            assert not (d['status']=='ok' and d['diagnostics'])
    print('PASS M7: errores multiples, cierres/ramas, Unicode, limites, consultas y artefactos; 120 mutaciones sin bloqueo.')


if __name__=='__main__': main()
