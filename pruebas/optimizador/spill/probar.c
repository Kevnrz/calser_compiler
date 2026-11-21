/* C3.4: CLI, métricas, ASM exacto y simulador existente, diseño §14.5.
 * El adaptador Python usa el comparador original; no replica la ISA.
 */
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

/* Entrada: ninguna. Salida: código del verificador.
 * Efectos: ejecuta Python sin shell ni archivos auxiliares persistentes.
 */
int main(void) {
    static const char script[] =
        "from pathlib import Path\n"
        "import sys, json, subprocess, tempfile, re\n"
        "sys.dont_write_bytecode = True\n"
        "sys.path.insert(0, str(Path('pruebas/generador_o1/tac_emit').resolve()))\n"
        "import comparar as c\n"
        "root = Path('pruebas/optimizador/spill')\n"
        "expected = json.loads((root / 'esperados.json').read_text())\n"
        "corpus = expected.pop('_corpus')\n"
        "with tempfile.TemporaryDirectory(prefix='calser_spill_') as t:\n"
        " for name, e in expected.items():\n"
        "  src = root / (name + '.cls')\n"
        "  base = Path(t) / name\n"
        "  args = ['./ccom', '-O1', '-s', str(src), '-o', str(base.with_suffix('.bin'))]\n"
        "  old = subprocess.run(args, capture_output=True, text=True, timeout=30)\n"
        "  assert old.returncode == e['codigo_o1'] and old.stderr == e['error_o1'], (name, old.stderr)\n"
        "  args_o0 = [value if value != '-O1' else '-O0' for value in args]\n"
        "  old_o0 = subprocess.run(args_o0, capture_output=True, text=True, timeout=30)\n"
        "  assert old_o0.returncode == e['codigo_o0'] and old_o0.stderr == e['error_o0'], (name, old_o0.stderr)\n"
        "  result = subprocess.run(args + ['--opt=spill', '--metrics', str(base.with_suffix('.metrics.json'))], capture_output=True, text=True, timeout=30)\n"
        "  assert result.returncode == 0, result.stderr\n"
        "  m = json.loads(base.with_suffix('.metrics.json').read_text())\n"
        "  for field in ('derramados', 'cargas_derrame', 'guardados_derrame'):\n"
        "   assert m['contadores'][field] == e[field], (name, field, m['contadores'])\n"
        "  assert base.with_suffix('.asm').read_bytes() == (root / (name + '.despues.asm')).read_bytes(), name\n"
        "  text = base.with_suffix('.asm').read_text()\n"
        "  variables = {}; scope = ''\n"
        "  for line in text.splitlines():\n"
        "   match = re.match(r'#\\s+Scope \\d+: (\\w+)(?: (\\w+))?$', line)\n"
        "   if match: scope = match[2] or match[1]\n"
        "   match = re.search(r'#\\s+(\\w+).* width=(\\d+) addr=0x([0-9A-Fa-f]+)', line)\n"
        "   if match: variables[scope + '.' + match[1]] = int(match[3], 16)\n"
        "  for data in (bytes(1024), bytes(range(256)) * 4):\n"
        "   sim = c.execute(base, data)\n"
        "   for key, value in e['valores'].items():\n"
        "    addr = variables[key]\n"
        "    measured = int.from_bytes(sim.memory[addr:addr+4], 'little', signed=True)\n"
        "    assert measured == value, (name, key, measured, value)\n"
        "  print('PASS spill', name, e['valores'], {k:e[k] for k in ('derramados','cargas_derrame','guardados_derrame')})\n"
        " for source in corpus:\n"
        "  c.compare(Path(source), Path(t), lambda src, base: c.compile_new(Path('./ccom').resolve(), src, base, 'spill'))\n"
        " print('PASS spill corpus:', len(corpus), 'programas; estado observable O0; dos imágenes externas')\n"

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
