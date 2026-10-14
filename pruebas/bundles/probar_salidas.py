#!/usr/bin/env python3
"""Direcciones finales y coherencia ASM/BIN/HEX/MEM tras organizar bundles."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
from sim_asm import parse

FUNCT = {'beq':1, 'bne':2, 'blt':3, 'bge':4, 'jefR':5, 'jef':16, 'jas':17}
LABEL = r'[A-Za-z_.][A-Za-z0-9_.]*'

def run(*args):
    return subprocess.run([str(ROOT/'ccom'), *map(str,args)],capture_output=True,text=True)

def signed(value, bits):
    return value-(1<<bits) if value&(1<<(bits-1)) else value

def inspect(base, output, directory, suffix):
    text = base.with_suffix('.asm').read_text()
    program = parse(text)
    count = len(program.bundles)
    terminal = program.bundles[-1].slots[2]
    assert terminal.mnemonic == 'jef' and terminal.args[-1] == '.L_arith_error'
    assert program.labels['.L_arith_error'] == count-1,(base,'bundle vacío después del final terminal')
    indices = [int(m[1]) for line in text.splitlines()
               if (m := re.match(r'^\s*\{.*\}\s*#\s*(\d+)\b',line))]
    assert indices == list(range(count)), (base,'direcciones de bundles',indices)
    label_comments = {m[1]:int(m[2]) for line in text.splitlines()
                      if (m := re.match(rf'^\s*({LABEL}):\s*# bundle (\d+)\s*$',line))}
    table_comments = {m[1]:int(m[2]) for line in text.splitlines()
                      if (m := re.match(rf'^#\s+({LABEL})\s+(\d+)(?:\s+\(interna\))?\s*$',line))}
    assert label_comments == program.labels == table_comments,(base,'etiquetas del listado')
    table = output.split('===== LABEL TABLE =====')[1]
    semantic_labels = {m[1]:None if m[2]=='-' else int(m[2]) for line in table.splitlines()
                       if (m := re.match(rf'^({LABEL})\s*\|.*\| Address: (\d+|-)\s*$',line))}
    assert semantic_labels, base
    for name,address in semantic_labels.items():
        assert address == program.labels.get(name),(base,name,address,program.labels.get(name))

    raw = base.with_suffix('.bin').read_bytes()
    header = struct.unpack_from('<4I',raw)
    assert header[0]==count and header[1]==0 and header[3]==0 and header[2]%4==0,(base,header)
    assert len(raw)==16+count*16+header[2],(base,'tamaño del binario')
    words = [struct.unpack_from('<4I',raw,16+pc*16) for pc in range(count)]
    expected_hex = ['_'.join(f'{w:08X}' for w in row[::-1]) for row in words]
    assert base.with_suffix('.hex').read_text().splitlines()==expected_hex,(base,'orden de slots en hex')
    printed = [m[1] for line in output.splitlines()
               if (m := re.match(r'^\d+\s+\{.*\}\s+([0-9A-F]{8}(?:_[0-9A-F]{8}){3})\s*$',line))]
    assert printed == expected_hex,(base,'salida -p')
    mem = base.with_suffix('.mem').read_text().split()
    assert mem and mem[0]=='@0' and len(mem)-1==header[2]//4
    expected_data = b''.join(int(word,16).to_bytes(4,'little') for word in mem[1:])
    assert raw[16+count*16:]==expected_data,(base,'imagen de datos')

    branches = 0
    for pc,bundle in enumerate(program.bundles):
        op = bundle.slots[2]
        if op.mnemonic == 'nopi':
            assert words[pc][2]==0
            continue
        word = words[pc][2]
        assert word>>31==0 and (word>>26)&31==FUNCT[op.mnemonic],(base,pc,'opcode BRU')
        if op.mnemonic == 'jefR':
            rd,src = op.args
            assert (word>>22)&15==rd and (word>>18)&15==src and word&0x3FFF==0,(base,pc,'jefR')
        else:
            bits = 22 if op.mnemonic in {'jef','jas'} else 18
            target = op.args[-1]
            expected_target = program.labels[target] if target in program.labels else pc+int(target,0)
            encoded_target = pc+signed(word&((1<<bits)-1),bits)
            assert encoded_target==expected_target,(base,pc,target,encoded_target,expected_target)
            if op.mnemonic in {'beq','bne','blt','bge'}:
                assert (word>>18)&15==op.args[0] and (word>>22)&15==op.args[1],(base,pc,'operandos BRU')
            else:
                rd = op.args[0] if op.mnemonic=='jas' else 0
                assert (word>>22)&15==rd,(base,pc,'enlace')
        branches += 1
    again = directory/(suffix+'_reensamblado')
    result = run('-x',base.with_suffix('.asm'),'-o',again.with_suffix('.bin'))
    assert result.returncode==0,result.stderr
    rebuilt=again.with_suffix('.bin').read_bytes()
    assert struct.unpack_from('<4I',rebuilt)==(count,0,0,0)
    assert rebuilt[16:]==raw[16:16+count*16],(base,'ida y vuelta binario')
    assert again.with_suffix('.hex').read_bytes()==base.with_suffix('.hex').read_bytes()
    return program.labels,count,branches

def main():
    programs = 0
    total_branches = 0
    factorial = None
    with tempfile.TemporaryDirectory() as temporary:
        directory=Path(temporary)
        for source in sorted((ROOT/'pruebas').rglob('*.cls')):
            cases=[]
            for scalar in [False,True]:
                suffix=f'programa_{programs}_'+('escalar' if scalar else 'organizado')
                base=directory/suffix
                args=['-s','-m','-p',source,'-o',base.with_suffix('.bin')]
                if scalar:args.append('--no-schedule')
                result=run(*args)
                cases.append((result,base,suffix))
            assert cases[0][0].returncode==cases[1][0].returncode,(source,'modos distintos')
            if cases[0][0].returncode:continue
            modes=[]
            for result,base,suffix in cases:
                labels,count,branches=inspect(base,result.stdout,directory,suffix)
                modes.append((labels,count));total_branches+=branches
            if source.name=='p02_factorial.cls':factorial=modes
            programs+=1
    assert factorial is not None
    (new,new_count),(old,old_count)=factorial
    assert new_count<old_count and new['.L_func_factorial']!=old['.L_func_factorial']
    print(f'PASS salidas: {programs} programas × 2 modos; ASM, BIN, HEX, MEM, -m, -p y reensamblado coherentes')
    print(f'PASS BRU: {total_branches} instrucciones codificadas con destinos/registros finales correctos')
    print(f'PASS reubicación factorial: .L_func_factorial {old[".L_func_factorial"]} → {new[".L_func_factorial"]}; bundles {old_count} → {new_count}')

if __name__=='__main__':main()
