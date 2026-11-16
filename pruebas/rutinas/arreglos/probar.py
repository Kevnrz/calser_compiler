#!/usr/bin/env python3
"""K-4b emitted ISA model with word memory, slots and dynamic RAW checks."""
from pathlib import Path
import random
import re
import subprocess
import sys
MASK=(1<<32)-1

def parse(text):
    labels,code={},[]
    for line in text.splitlines():
        if line.endswith(':'):
            assert line[:-1] not in labels
            labels[line[:-1]]=len(code)
            continue
        assert line.startswith('{') and line.endswith('}')
        slots=[s.strip() for s in line[1:-1].split('|')]
        assert len(slots)==4
        ops=[(i,s) for i,s in enumerate(slots) if s!='nopi']
        assert len(ops)<=1
        if not ops: code.append(('nopi',[])); continue
        slot,s=ops[0]; parts=re.split(r'[ ,()]+',s)
        mn=parts[0]; args=[x for x in parts[1:] if x]
        assert slot==({'lw':3,'sw':3,'bne':2}.get(mn,0))
        code.append((mn,args))
    for mn,args in code:
        if mn=='bne': assert args[-1] in labels
    return code,labels

def run(program,regs,mem):
    code,labels=program
    regs=regs.copy(); mem=mem.copy(); last=[-100]*16
    pc=cycle=steps=0
    def read(token):
        r=int(token[1:]); assert cycle-last[r]>=2,('RAW',pc,token)
        return regs[r]
    while pc<len(code):
        steps+=1; assert steps<100000
        mn,args=code[pc]; value=dest=target=None
        if mn=='nopi': pass
        elif mn=='bne':
            if read(args[0])!=read(args[1]): target=labels[args[2]]
        elif mn in {'lw','sw'}:
            offset=int(args[1]); assert -131072<=offset<=131071
            address=(read(args[2])+offset)&MASK
            assert address%4==0 and 0<=address<=65532
            if mn=='lw': dest=int(args[0][1:]); value=mem.get(address,0)
            else: mem[address]=read(args[0])
        else:
            dest=int(args[0][1:])
            if mn=='mov':
                value=int(args[1]); assert -131072<=value<=131071
            elif mn=='saddi': value=read(args[1])+int(args[2])
            elif mn=='add': value=read(args[1])+read(args[2])
            elif mn=='sll': value=read(args[1])<<(read(args[2])&31)
            else: raise AssertionError(mn)
        if dest is not None:
            assert 7<=dest<=12,('unexpected write',dest)
            regs[dest]=value&MASK; last[dest]=cycle
        if target is not None: pc=target; cycle+=2
        else: pc+=1; cycle+=1
    return regs,mem

def check(binary,kind,dims,rows,cols,param,i,j):
    n=rows*(cols if dims==2 else 1)
    src=256
    dst=src+4*n if src+8*n<=4096 else src # Disjoint arrays or exact alias.
    regs=[0]*16
    regs[1:4]=[12345,23456,34567]
    regs[4:7]=[i,j,0x87654321]
    regs[13:16]=[45678,56789,67890]
    mem={src+4*k:(k*17+0x12345678)&MASK for k in range(n)}
    if dst!=src: mem.update({dst+4*k:0xFFFFFFFF for k in range(n)})
    mem[64]=src; mem[128]=dst
    program=parse(subprocess.check_output([binary,kind,str(dims),str(rows),str(cols),str(param)],text=True))
    r,m=run(program,regs,mem)
    assert r[:7]==regs[:7] and r[13:]==regs[13:]
    index=i*(cols if dims==2 else 1)+(i if kind=='sameindex' and dims==2 else j if dims==2 else 0)
    addr=src+4*index
    expected=mem.copy()
    if kind in {'address','sameindex'}: assert r[7]==addr
    elif kind=='load': assert r[7]==addr and r[8]==mem[addr]
    elif kind=='store':
        expected[addr]=regs[6]; assert r[7]==addr and r[8]==regs[6]
    elif kind in {'pass','forward'}:
        expected[128]=src
        if kind=='forward': assert r[7]==addr and r[8]==mem[addr]
    elif kind=='copy':
        for k in range(n): expected[dst+4*k]=mem[src+4*k]
    elif kind=='zero':
        for k in range(n): expected[src+4*k]=0
    assert m==expected,(kind,dims,rows,cols,param,i,j)
    # Concrete 3x4 example requested by the plan.
    if dims==2 and rows==3 and cols==4 and i==1 and j==1 and kind=='address':
        assert r[7]==276 # base 256 + (1*4+1)*4

def main():
    binary=str(Path(sys.argv[1]).resolve())
    subprocess.run([binary,'--contract'],check=True)
    count=0
    # Every cell in the 3x4 matrix and the 1D vector; static and pointer bases.
    for dims,rows,cols in [(2,3,4),(1,12,1),(2,1,1),(2,2,7),(2,3,5)]:
        for param in [0,1]:
            for i in range(rows):
                for j in range(cols if dims==2 else 1):
                    for kind in ['address','load','store','pass','forward']:
                        check(binary,kind,dims,rows,cols,param,i,j); count+=1
            for kind in ['copy','zero']:
                check(binary,kind,dims,rows,cols,param,0,0); count+=1
            for i in range(min(rows,cols)):
                check(binary,'sameindex',dims,rows,cols,param,i,0); count+=1
    rng=random.Random(43011108)
    for _ in range(100):
        dims=rng.choice([1,2]); rows=rng.randint(1,12); cols=rng.randint(1,15) if dims==2 else 1
        i,j=rng.randrange(rows),rng.randrange(cols)
        param=rng.randrange(2)
        for kind in ['address','load','store','forward','copy','zero']:
            check(binary,kind,dims,rows,cols,param,i,j); count+=1
    # Largest compiler-owned static array at address 256: 960 words.
    for param in [0,1]:
        for kind in ['address','load','store','copy','zero']:
            check(binary,kind,2,30,32,param,29,31); count+=1
    print(f'PASS total: {count} casos; direccion, lectura, escritura, referencia, copia y cero')
    print('PASS matriz 3x4: direccion (1,1) = base + 20; slots y separacion RAW')
if __name__=='__main__': main()
