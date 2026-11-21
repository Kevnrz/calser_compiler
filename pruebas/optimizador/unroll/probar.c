/* C3.1: CLI, valores observables, JSON y capturas de la pasada real, §11.4.
 * Reutiliza el simulador/comparador; no replica Calser ni la ISA, Aho §9.1.
 */
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

/* Entrada: ninguna. Salida: código del verificador.
 * Efectos: Python sin shell ni script externo al binario de prueba.
 */
int main(void) {
    static const char script[] =
        "from pathlib import Path\n"
        "import json, subprocess, tempfile, sys, re\n"
        "sys.dont_write_bytecode = True\n"
        "sys.path.insert(0,str(Path('pruebas/generador_o1/tac_emit').resolve()))\n"
        "import comparar as c\n"
        "expected=json.loads(Path('pruebas/optimizador/unroll/esperados.json').read_text())\n"
        "with tempfile.TemporaryDirectory(prefix='calser_unroll_') as temporary:\n"
        " folder=Path(temporary)\n"
        " for name,e in expected.items():\n"
        "  src=e['fuente'];base=folder/name;factor=next((v.split('=')[1] for v in e['opciones'] if v.startswith('--unroll=')), '0')\n"
        "  a=base.with_suffix('.antes.tac');d=base.with_suffix('.despues.tac')\n"
        "  r=subprocess.run(['build/capturar_unroll',src,str(a),str(d),factor],capture_output=True,text=True,timeout=60)\n"
        "  assert not r.returncode,(name,r.stderr)\n"
        "  analysis=json.loads(r.stdout)\n"
        "  assert analysis['desenrollados']==e['desenrollados'],(name,analysis['desenrollados'],e['desenrollados'])\n"
        "  assert a.read_text()==(Path(e['antes']).read_text() if 'antes' in e else e['tac_antes']),name\n"
        "  assert d.read_text()==(Path(e['despues']).read_text() if 'despues' in e else e['tac_despues']),name\n"
        "  r=subprocess.run(['./ccom','-O1','--opt=unroll','-s',src,'-o',str(base.with_suffix('.bin')),'--metrics',str(base.with_suffix('.metrics.json')),*e['opciones']],capture_output=True,text=True,timeout=60)\n"
        "  assert not r.returncode,(name,r.stderr)\n"
        "  m=json.loads(base.with_suffix('.metrics.json').read_text())\n"
        "  assert m['contadores']['desenrollados']==e['desenrollados'],(name,m['contadores'])\n"
        "  assert m['alto_nivel']['despues']['unroll']==analysis['HIGH_despues']\n"
        "  variables={};regions=[];scope='';observable=False\n"
        "  for line in base.with_suffix('.asm').read_text().splitlines():\n"
        "   match=re.match(r'#\\s+Scope \\d+: (\\w+)(?: (\\w+))?$',line)\n"
        "   if match:\n"
        "    scope=match[2] or match[1];observable=match[1]=='GLOBAL' or match[2]=='main'\n"
        "   match=re.search(r'#\\s+(\\w+).* width=(\\d+) addr=0x([0-9A-Fa-f]+)',line)\n"
        "   if match:\n"
        "    addr=int(match[3],16);width=int(match[2]);variables[scope+'.'+match[1]]=addr\n"
        "    if observable:regions.append((addr,width))\n"
        "  old=c.compile_o0(Path(src),folder/'o0')\n"
        "  for data in (bytes(1024),bytes(range(256))*4):\n"
        "   before=c.execute(old,data);after=c.execute(base,data)\n"
        "   assert c.observable(before,regions)==c.observable(after,regions),name\n"
        "   for key,value in e['valores'].items():\n"
        "    values=value if isinstance(value,list) else [value];addr=variables[key]\n"
        "    actual=[int.from_bytes(after.memory[addr+4*k:addr+4*k+4],'little',signed=True) for k in range(len(values))]\n"
        "    assert actual==values,(name,key,actual,values)\n"
        "  print('PASS unroll',name,e['valores'],e['desenrollados'])\n"
        "print('PASS unroll:',len(expected),'escenarios; capturas HIGH, JSON y dos im\u00e1genes externas')\n"
;
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        execlp("python3", "python3", "-c", script, (char *)NULL);
        perror("python3");
        _exit(127);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
