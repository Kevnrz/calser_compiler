#!/usr/bin/env python3
"""validar_bundles.py — Herramienta de aceptación: comparación del factorial por bundles.

Mide emisión conservadora y organizada en el simulador. Comprueba
resultado, ausencia de avisos y uso de dos ALU con menos ciclos y bundles.

Usa: ccom, sim_asm.py y pruebas/p02_factorial.cls.
Lo usa: make validar-bundles y make check.
Referencias: isa.md §3, §9; docs/organizacion_bundles.md.
"""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT/'tools'))
from sim_asm import ALU_R, Annotations, Simulator, Tracer, parse, signed

ALU = ALU_R | {'mov', 'saddi', 'xori'}
SOURCE = ROOT/'pruebas/p02_factorial.cls'
LIMIT = 1_000_000

class AcceptanceError(Exception):
    pass

class MeasuredSimulator(Simulator):
    def __init__(self, program):
        """__init__ — inicializa simulador y contador de visitas.

        Entrada:
            self  instancia nueva.
            program  Program decodificado.
        Salida:
            None. Efectos: inicializa estado de Simulator y visits.
        Restricciones:
            ninguna.
        """
        super().__init__(program)
        self.visits = Counter()
    def step(self):
        """step — cuenta visitas y ejecuta un bundle.

        Entrada:
            self  instancia del simulador.
        Salida:
            bool devuelto por Simulator.step. Efectos: incrementa visits y modifica estado del simulador.
        Restricciones:
            PC válido; propaga AsmError.
        """
        self.visits[self.pc] += 1
        return super().step()

# ===========================================================================
# CRITERIOS Y MEDICIONES
# ===========================================================================

def require(condition, message):
    """require — exige un criterio de aceptación.

    Entrada:
        condition  condición booleana.
        message  mensaje de fallo.
    Salida:
        None si cumple. Efectos: ninguno.
    Restricciones:
        lanza AcceptanceError si condition es falsa.
    """
    if not condition:
        raise AcceptanceError(message)

def run_mode(source, output, mode, expected, artifacts=False):
    """run_mode — compila y mide factorial en un modo.

    Entrada:
        source  ruta del fuente.
        output  carpeta de salida existente.
        mode  escalar para --no-schedule, otro valor organiza.
        expected  resultado entero esperado.
        artifacts  True para guardar evidencia textual.
    Salida:
        diccionario de métricas. Efectos: ejecuta ccom, crea salidas y simula; escribe evidencias si artifacts.
    Restricciones:
        ccom construido; exige fin terminal, cero avisos y main.f esperado;
        AcceptanceError si falla.
    """
    base = output/('factorial_'+mode)
    try:
        source_arg = str(source.relative_to(ROOT))
    except ValueError:
        source_arg = str(source)
    command = ['./ccom', '-s', '-m', '-v', source_arg, '-o', str(base.with_suffix('.bin'))]
    if mode == 'escalar': command.append('--no-schedule')
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if artifacts:
        base.with_suffix('.compilacion.txt').write_text(
            'Comando: '+' '.join(command)+'\n\n'+result.stderr+'\n'+result.stdout)
    require(result.returncode == 0, f'Compilación {mode}: {result.stderr}')
    text = base.with_suffix('.asm').read_text()
    program = parse(text)
    notes = Annotations(text, len(program.bundles))
    sim = MeasuredSimulator(program)
    sim.tracer = Tracer(notes, program.labels, 10000)
    sim.load_data_image(base.with_suffix('.mem').read_text())
    sim.run(LIMIT)
    if artifacts:
        base.with_suffix('.ejecucion.txt').write_text(sim.report()+'\n')
        base.with_suffix('.traza.txt').write_text(sim.tracer.text(sim))
    require(sim.reason == 'salto a sí mismo', f'{mode}: fin inesperado {sim.reason}')
    require(not sim.warnings, f'{mode}: avisos de separación {sim.warnings}')
    variables = [v for v in notes.variables if v['owner']=='main' and v['name']=='f']
    require(len(variables)==1, f'{mode}: no se encontró main.f')
    address = variables[0]['address']
    value = signed(int.from_bytes(sim.memory[address:address+4], 'little'))
    require(value == expected, f'{mode}: f={value}, esperado {expected}')
    occupancy = [sum(op.mnemonic!='nopi' for op in b.slots) for b in program.bundles]
    dual = [pc for pc,b in enumerate(program.bundles)
            if all(op.mnemonic in ALU for op in b.slots[:2])]
    lines = text.splitlines()
    examples = [{'bundle':pc, 'veces_ejecutado':sim.visits[pc],
                 'asm':lines[program.bundles[pc].line-1].split('#',1)[0].strip()}
                for pc in dual]
    metrics = {
        'resultado':value,
        'bundles_estaticos':len(program.bundles),
        'bundles_vacios':occupancy.count(0),
        'bundles_ejecutados':sim.steps,
        'ciclos':sim.cycles,
        'avisos':len(sim.warnings),
        'bundles_dos_alu_estaticos':len(dual),
        'bundles_dos_alu_ejecutados':sum(sim.visits[pc] for pc in dual),
        'ocupacion_estatica_porcentaje':round(sum(occupancy)*100/(4*len(occupancy)),2),
        'instrucciones_ejecutadas':sum(sim.visits[pc]*used for pc,used in enumerate(occupancy)),
        'ejemplos_dos_alu':examples,
    }
    return metrics

# ===========================================================================
# ACEPTACIÓN Y REPORTE
# ===========================================================================

def validate(output):
    """validate — compara factorial conservador y organizado.

    Entrada:
        output  carpeta de artefactos.
    Salida:
        diccionario de reporte si todos los criterios cumplen. Efectos: crea salidas, sustituye reportes previos y prueba entradas en temporal; imprime PASS.
    Restricciones:
        exige fuente con inicializador esperado, dos ALU ejecutadas y
        reducción de ciclos/bundles; puede lanzar AcceptanceError.
    """
    output.mkdir(parents=True, exist_ok=True)
    # Invalidar reportes anteriores antes de iniciar una nueva aceptación.
    json_path = output/'reporte.json'
    md_path = output/'reporte.md'
    for path in (json_path, md_path):
        if path.exists(): path.unlink()
    scalar = run_mode(SOURCE, output, 'escalar', 120, artifacts=True)
    parallel = run_mode(SOURCE, output, 'paralelo', 120, artifacts=True)
    require(parallel['bundles_dos_alu_ejecutados']>0, 'Las dos ALU no se ejecutaron simultáneamente')
    for metric in ['ciclos','bundles_estaticos','bundles_vacios']:
        require(parallel[metric]<scalar[metric], f'No se redujo {metric}')
    checks = []
    original = SOURCE.read_text()
    require(original.count('Vi x : int = 5 ;')==1, 'Cambió la entrada de p02; revisar la prueba auxiliar')
    with tempfile.TemporaryDirectory(prefix='calser-factorial-') as temporary:
        directory = Path(temporary)
        for n in [-1,0,1,2,10,12]:
            source = directory/f'factorial_{n}.cls'
            source.write_text(original.replace('Vi x : int = 5 ;',f'Vi x : int = {n} ;'))
            expected = math.factorial(n) if n>=0 else 0
            modes = [run_mode(source,directory,mode,expected) for mode in ['escalar','paralelo']]
            checks.append({'entrada':n,'esperado':expected,'escalar':modes[0]['resultado'],
                           'paralelo':modes[1]['resultado'],'avisos':0})
    reduction = round((scalar['ciclos']-parallel['ciclos'])*100/scalar['ciclos'],2)
    report = {'fuente':'pruebas/p02_factorial.cls','aceptacion':'OK',
              'escalar':scalar,'paralelo':parallel,'reduccion_ciclos_porcentaje':reduction,
              'entradas_adicionales':checks}
    json_path.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    labels = [('Resultado','resultado'),('Bundles estáticos','bundles_estaticos'),
              ('Bundles vacíos','bundles_vacios'),('Bundles ejecutados','bundles_ejecutados'),
              ('Ciclos','ciclos'),('Bundles con dos ALU (estáticos)','bundles_dos_alu_estaticos'),
              ('Bundles con dos ALU (ejecutados)','bundles_dos_alu_ejecutados'),
              ('Ocupación estática (%)','ocupacion_estatica_porcentaje'),('Avisos','avisos')]
    rows = ['# Aceptación de la organización de bundles','',
            'Fuente: `pruebas/p02_factorial.cls`. Resultado: **OK**.','',
            '| Medida | Escalar | Paralelo |','|---|---:|---:|']
    rows += [f'| {label} | {scalar[key]} | {parallel[key]} |' for label,key in labels]
    rows += ['',f'Reducción de ciclos: **{reduction}%**.','',
             '## Bundles con ambas ALU','',
             'Cada contador corresponde a visitas durante la ejecución, no solo a presencia en el archivo.','']
    for example in parallel['ejemplos_dos_alu']:
        rows += [f'Bundle {example["bundle"]}, ejecutado {example["veces_ejecutado"]} veces:',
                 '', '```asm',example['asm'],'```','']
    rows += ['## Entradas adicionales','', '| n | Esperado | Escalar | Paralelo |',
             '|---:|---:|---:|---:|']
    rows += [f'| {c["entrada"]} | {c["esperado"]} | {c["escalar"]} | {c["paralelo"]} |' for c in checks]
    rows += ['', 'Todas terminaron sin avisos. La entrada negativa valida el retorno 0 definido por p02.','',
             '## Archivos para inspeccionar','',
             '- [Ensamblador paralelo](factorial_paralelo.asm)',
             '- [Ensamblador escalar](factorial_escalar.asm)',
             '- [Traza paralela](factorial_paralelo.traza.txt)',
             '- [Traza escalar](factorial_escalar.traza.txt)',
             '- [Métricas JSON](reporte.json)','',
             'Los archivos `.bin`, `.hex`, `.mem`, `.compilacion.txt` y `.ejecucion.txt` se conservan para ambos modos.','',
             'Esta prueba usa el simulador de la ISA; no mide ejecución en RTL ni en hardware físico.','']
    md_path.write_text('\n'.join(rows))
    print(f'PASS factorial: 120; ciclos {scalar["ciclos"]} → {parallel["ciclos"]} ({reduction}% menos)')
    print(f'PASS paralelismo: ambas ALU ejecutadas {parallel["bundles_dos_alu_ejecutados"]} veces; cero avisos')
    print('PASS entradas adicionales: -1, 0, 1, 2, 10 y 12, en ambos modos')
    print(f'Reporte: {md_path}')
    return report

def main(argv=None):
    """main — ejecuta la aceptación desde la línea de comandos.

    Entrada:
        argv  argumentos o None para sys.argv.
    Salida:
        0 si valida; 1 por AcceptanceError, OSError o ValueError. Efectos: crea artefactos de validación y escribe mensajes.
    Restricciones:
        argparse puede terminar por uso incorrecto.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'build/validacion_bundles')
    options = parser.parse_args(argv)
    try:
        validate(options.output.resolve())
        return 0
    except (AcceptanceError,OSError,ValueError) as error:
        print(f'validar_bundles: {error}',file=sys.stderr)
        return 1

if __name__=='__main__':sys.exit(main())
