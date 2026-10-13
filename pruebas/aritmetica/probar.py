#!/usr/bin/env python3
"""Software ISA model for emitted K-4 code; not RTL or full ccom execution."""
from pathlib import Path
import random
import re
import subprocess
import sys
MASK=(1<<32)-1
MIN=-(1<<31)
MAX=(1<<31)-1

def signed(x):
    x &= MASK
    return x if x<=MAX else x-(1<<32)

def parse(text):
    labels,code={},[]
    for line in text.splitlines():
        if line.endswith(':'):
            label=line[:-1]
            assert label not in labels, ('duplicate label',label)
            labels[label]=len(code)
            continue
        assert line.startswith('{') and line.endswith('}')
        slots=[s.strip() for s in line[1:-1].split('|')]
        assert len(slots)==4
        ops=[(i,re.split(r'[ ,]+',s)) for i,s in enumerate(slots) if s!='nopi']
        assert len(ops)<=1
        if ops:
            slot,fields=ops[0]
            assert slot==(2 if fields[0] in {'beq','bne','blt','bge','jef'} else 0)
        code.append(ops[0][1] if ops else ['nopi'])
    for op in code:
        if op[0] in {'beq','bne','blt','bge','jef'}:
            assert op[-1] in labels or op[-1]=='error_aritmetico'
    return code,labels

def run(program,a,b):
    code,labels=program
    regs=[0]*16
    regs[1:4]=[0x13579,0x24680,0x98765]
    regs[4],regs[5],regs[15]=a&MASK,b&MASK,0xABCDEF01
    last=[-100]*16
    pc=cycle=steps=0
    def read(token):
        r=int(token[1:])
        assert cycle-last[r]>=2, ('RAW hazard',pc,token)
        return regs[r]
    while pc<len(code):
        steps+=1
        assert steps<100000,'routine did not terminate'
        mn,*args=code[pc]
        target=None
        if mn=='nopi': pass
        elif mn=='jef': target=args[0]
        elif mn in {'beq','bne','blt','bge'}:
            x,y=read(args[0]),read(args[1])
            take={'beq':x==y,'bne':x!=y,'blt':signed(x)<signed(y),'bge':signed(x)>=signed(y)}[mn]
            if take: target=args[2]
        else:
            dest=int(args[0][1:])
            if mn=='mov':
                value=int(args[1]); assert -131072<=value<=131071
            elif mn=='saddi':
                imm=int(args[2]); assert -131072<=imm<=131071
                value=read(args[1])+imm
            else:
                x,y=read(args[1]),read(args[2])
                if mn=='add': value=x+y
                elif mn=='sub': value=x-y
                elif mn=='and': value=x&y
                elif mn=='or': value=x|y
                elif mn=='xor': value=x^y
                elif mn=='sll': value=x<<(y&31)
                elif mn=='srl': value=x>>(y&31)
                elif mn=='slt': value=int(signed(x)<signed(y))
                else: raise AssertionError(mn)
            assert 4<=dest<=14,('unexpected clobber',dest)
            regs[dest]=value&MASK
            last[dest]=cycle
        if target=='error_aritmetico': return 'error'
        if target is not None:
            pc=labels[target]; cycle+=2 # Taken branch discards fetched bundle.
        else: pc+=1; cycle+=1
    assert regs[4:6]==[a&MASK,b&MASK]
    assert regs[1:4]==[0x13579,0x24680,0x98765]
    assert regs[15]==0xABCDEF01
    return signed(regs[6])

def expected(kind,a,b):
    if kind=='mul': return signed(a*b)
    if kind=='pow': return 'error' if b<0 else signed(pow(a,b,1<<32))
    if b==0: return 'error'
    q=abs(a)//abs(b)
    if (a<0)!=(b<0): q=-q
    return signed(q if kind=='div' else a-q*b)

def main():
    binary=str(Path(sys.argv[1]).resolve())
    subprocess.run([binary,'--contract'],check=True)
    rng=random.Random(43011108)
    edge=[MIN,MIN+1,-65536,-7,-2,-1,0,1,2,7,65536,MAX]
    total=0
    for kind in ['mul','div','mod','pow']:
        program=parse(subprocess.check_output([binary,kind],text=True))
        cases=[(a,b) for a in edge for b in ([-1,0,1,2,3,10,31] if kind=='pow' else edge)]
        cases += [(rng.randint(MIN,MAX),rng.randint(-1,31) if kind=='pow' else rng.randint(MIN,MAX)) for _ in range(300)]
        if kind=='pow': cases += [(a,b) for a in [-1,0,1] for b in [1<<30,MAX]]
        cases += [(2,10),(-7,2),(7,-2),(-7,-2),(0,0),(5,5)]
        for a,b in cases:
            got,want=run(program,a,b),expected(kind,a,b)
            assert got==want,(kind,a,b,got,want)
        total+=len(cases)
        print(f'PASS {kind}: {len(cases)} casos, dos expansiones por caso')
    print(f'PASS total: {total} casos; slots, etiquetas, RAW y registros conservados')
if __name__=='__main__': main()
