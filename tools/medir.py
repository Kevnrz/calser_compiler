#!/usr/bin/env python3
"""Mide optimizaciones o niveles y escribe JSON y un informe HTML/SVG sin red.

Ejemplos: python3 tools/medir.py --modo optimizacion
          python3 tools/medir.py --modo nivel --salida build/metricas programa.cls
El tiempo es la mediana de cinco --time; los ciclos corresponden a datos cero.
La equivalencia se verifica también con el patrón externo de comparar.py.
"""
import argparse
import hashlib
from html import escape
import json
from pathlib import Path
import re
import statistics
import struct
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from pruebas.generador_o1.tac_emit import comparar as comparador
from sim_asm import Annotations, parse

PASADAS = ('unroll', 'rename', 'dce', 'reorder', 'spill')
CONTADORES = ('desenrollados', 'renombradas', 'eliminadas', 'bloques_eliminados',
              'reordenadas', 'derramados', 'cargas_derrame', 'guardados_derrame')
DATOS = (bytes(1024), bytes(range(256)) * 4)
TIEMPO = re.compile(r'^ccom: tiempo de compilacion \((?:completado|fallido)\): (\d+\.\d+) ms\n?', re.M)
REPETICIONES = 5


def predeterminados(modo):
    if modo == 'optimizacion':
        return [p for pasada, prefijo in zip(PASADAS, 'urdos')
                for numero in range(1, 4)
                for p in sorted((ROOT / 'pruebas/optimizador' / pasada).glob(f'{prefijo}{numero}_*.cls'))]
    return [ROOT / nombre for nombre in (
        'ejercicios/ejercicio1_factorial/ejercicio1.cls',
        'ejercicios/ejercicio2_matriz/ejercicio2.cls',
        'ejercicios/ejercicio3_cifrado/ejercicio3.cls',
        'pruebas/generador_o0/matrices/cuadradas.cls',
        'pruebas/generador_o0/matrices/rectangulares.cls',
        'pruebas/rutinas/arreglos/matriz3x4.cls')]


def nombre_fuente(fuente):
    return str(fuente.relative_to(ROOT)) if fuente.is_relative_to(ROOT) else str(fuente)


def compilar(fuente, base, nivel, pasada=None):
    """Cinco compilaciones reales; no incluye simulación ni preflight en el tiempo."""
    base.parent.mkdir(parents=True, exist_ok=True)
    opciones = [f'-{nivel}', '-s', '--time']
    if pasada:
        opciones.append(f'--opt={pasada}')
    if nivel != 'O0':
        opciones += ['--metrics', str(base.with_suffix('.metrics.json'))]
    args = [str(ROOT / 'ccom'), *opciones, nombre_fuente(fuente),
            '-o', str(base.with_suffix('.bin'))]
    muestras, codigos, mensajes = [], [], []
    for _ in range(REPETICIONES):
        resultado = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=30)
        tiempos = TIEMPO.findall(resultado.stderr)
        if len(tiempos) != 1:
            raise ValueError(f'--time ausente o ambiguo: {resultado.stderr}')
        muestras.append(float(tiempos[0]))
        codigos.append(resultado.returncode)
        mensajes.append(TIEMPO.sub('', resultado.stderr))
    if len(set(codigos)) != 1:
        raise ValueError(f'compilación inestable: {codigos}')
    registro = dict(nivel=nivel, pasadas=[pasada] if pasada else list(PASADAS) if nivel == 'O2' else [],
                    comando=args, codigo=codigos[0], error=mensajes[-1] if codigos[0] else None,
                    tiempo_ms=statistics.median(muestras), muestras_ms=muestras,
                    emitidas=None, bundles=None, bytes=None, ciclos=None,
                    ciclos_por_datos=None, contadores=None, equivalente=False)
    if codigos[0]:
        if len(set(mensajes)) != 1:
            raise ValueError('diagnóstico inestable entre compilaciones')
        return registro
    programa = parse(base.with_suffix('.asm').read_text())
    binario = base.with_suffix('.bin').read_bytes()
    bundles, entrada, datos, direccion = struct.unpack_from('<4I', binario)
    tamano = 16 * bundles
    if len(binario) != 16 + tamano + datos or bundles != len(programa.bundles):
        raise ValueError('BIN y ASM discrepan en bundles')
    registro.update(bundles=bundles, bytes=tamano, archivo_bin_bytes=len(binario))
    if nivel == 'O0':
        # Mismo criterio de tac_metrics_collect: operaciones distintas de nopi.
        registro['emitidas'] = sum(op.mnemonic != 'nopi'
                                   for b in programa.bundles for op in b.slots)
        registro['contadores'] = {k: [] if k == 'desenrollados' else 0 for k in CONTADORES}
    else:
        metricas = json.loads(base.with_suffix('.metrics.json').read_text())
        if metricas['bundles'] != registro['bundles'] or metricas['bytes'] != tamano:
            raise ValueError('métricas de ccom y BIN discrepan')
        registro['emitidas'] = metricas['emitidas']
        registro['contadores'] = {k: metricas['contadores'][k] for k in CONTADORES}
    return registro


def valores_spill(base, simulador):
    """Lee variables anotadas igual que check-spill; no compara memoria interna."""
    notas = Annotations(base.with_suffix('.asm').read_text(), len(simulador.program.bundles))
    return {f"{v['owner'] or 'GLOBAL'}.{v['name']}": int.from_bytes(
        simulador.memory[v['address']:v['address'] + 4], 'little', signed=True)
        for v in notas.variables}


def medir(fuente, modo, pasada, carpeta):
    """Reutiliza compilación, regiones, simulación y proyección de comparar.py."""
    resultado = dict(schema_version=1, programa=nombre_fuente(fuente), modo=modo,
                     pasada=pasada, equivalente=False, errores=[], compilaciones={})
    esperados = json.loads((ROOT / 'pruebas/optimizador/spill/esperados.json').read_text())
    especial = esperados.get(fuente.stem) if fuente.parent == ROOT / 'pruebas/optimizador/spill' else None
    if especial and (modo != 'optimizacion' or pasada != 'spill'):
        raise ValueError('s1..s3 se miden en optimizacion con la pasada spill')
    referencia = carpeta / 'referencia'
    referencia.parent.mkdir(parents=True, exist_ok=True)
    if not especial:
        comparador.compile_o0(fuente, referencia)
    variantes = [('antes', 'O1', None), ('despues', 'O1', pasada)] if modo == 'optimizacion' else [
        (nivel, nivel, None) for nivel in ('O0', 'O1', 'O2')]
    for etiqueta, nivel, opt in variantes:
        base = carpeta / etiqueta / 'programa'
        base.parent.mkdir(parents=True, exist_ok=True)
        # Preflight sin medir: reutiliza el adaptador existente para obtener
        # los tramos observables, incluso en O2 (equivale a las cinco pasadas).
        regions = None
        if not (especial and etiqueta == 'antes') and nivel != 'O0':
            _, regions = comparador.compile_new(ROOT / 'ccom', fuente, base,
                                                ','.join(PASADAS) if nivel == 'O2' else opt)
        registro = compilar(fuente, base, nivel, opt)
        resultado['compilaciones'][etiqueta] = registro
        if especial and etiqueta == 'antes':
            registro['error_esperado'] = (registro['codigo'] == especial['codigo_o1']
                                         and registro['error'] == especial['error_o1'])
            if not registro['error_esperado']:
                resultado['errores'].append('El error O1 no coincide con esperados.json')
            continue
        if registro['codigo']:
            resultado['errores'].append(f'{etiqueta}: {registro["error"]}')
            continue
        estados, ciclos = [], []
        for datos in DATOS:
            sim = comparador.execute(base, datos)
            ciclos.append(sim.cycles)
            if especial:
                valores = valores_spill(base, sim)
                estados.append(all(valores.get(k) == v for k, v in especial['valores'].items()))
                registro['valores'] = {k: valores.get(k) for k in especial['valores']}
            elif nivel == 'O0':
                # Es el propio generador de referencia; execute verifica fin y avisos.
                estados.append(True)
            else:
                original = comparador.execute(referencia, datos)
                estados.append(comparador.observable(original, regions) == comparador.observable(sim, regions))
        registro.update(ciclos=ciclos[0], ciclos_por_datos=dict(cero=ciclos[0], patron=ciclos[1]),
                        equivalente=all(estados))
        if not registro['equivalente']:
            resultado['errores'].append(f'{etiqueta}: estado observable diferente')
    resultado['equivalente'] = not resultado['errores'] and all(
        r.get('error_esperado', False) if especial and k == 'antes' else r['equivalente']
        for k, r in resultado['compilaciones'].items())
    resultado['referencia'] = 'esperados.json de spill (dos imágenes)' if especial else 'O0 (dos imágenes)'
    return resultado


def numero(valor):
    return f'{valor:.3f}' if isinstance(valor, float) else str(valor)


def variacion(antes, despues):
    if antes is None or despues is None:
        return 'no disponible'
    if antes == 0:
        return '0.0%' if despues == 0 else 'no definida (base cero)'
    return f'{100 * (despues - antes) / antes:+.1f}%'


def grafica(compilaciones, campo, titulo):
    if not compilaciones:
        return f'<figure><figcaption>{titulo}</figcaption><p>Sin mediciones válidas</p></figure>'
    valores = [r[campo] for r in compilaciones.values() if r['codigo'] == 0 and r[campo] is not None]
    maximo = max(valores, default=0) or 1
    partes = [f'<figure><figcaption>{titulo}</figcaption>',
              f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 460 150" role="img" aria-label="{titulo}">']
    for i, (etiqueta, r) in enumerate(compilaciones.items()):
        y = 12 + i * 43
        partes.append(f'<text x="0" y="{y+17}">{escape(etiqueta)}</text>')
        if r['codigo'] or r[campo] is None:
            partes.append(f'<text x="92" y="{y+17}" class="error">no compila</text>')
        else:
            ancho = 240 * r[campo] / maximo
            partes.append(f'<rect x="92" y="{y}" width="{ancho:.2f}" height="25" fill="{("#526c9c", "#218069", "#936021")[i]}"/>')
            partes.append(f'<text x="455" y="{y+17}" text-anchor="end">{numero(r[campo])}</text>')
    partes.append('</svg>')
    etiquetas = list(compilaciones)
    inicial = compilaciones[etiquetas[0]]
    cambios = []
    for etiqueta in etiquetas[1:]:
        ultimo = compilaciones[etiqueta]
        delta = variacion(inicial[campo], ultimo[campo]) if not (inicial['codigo'] or ultimo['codigo']) else 'no disponible (no compila)'
        cambios.append(f'{etiqueta} vs {etiquetas[0]}: {delta}')
    partes.append('<p class="delta">' + escape(' · '.join(cambios)) + '</p></figure>')
    return ''.join(partes)


def escribir_html(indice, salida):
    partes = ['''<!doctype html><html lang="es"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Mediciones de ccom — CE1108</title><style>
*{box-sizing:border-box}body{font:15px system-ui,sans-serif;color:#192e3f;background:#f4f6f8;margin:0}
main{max-width:1100px;margin:auto;padding:32px}h1,h2,h3{color:#163f53}h1{margin-bottom:8px}
table{border-collapse:collapse;width:100%;margin:16px 0;font-size:13px}th,td{border:1px solid #b9c8d2;padding:7px;text-align:left;overflow-wrap:anywhere}th{background:#e3edf2}
article{background:white;border:1px solid #cad5dd;padding:20px;margin:22px 0;break-inside:avoid}
.graficas{display:grid;grid-template-columns:1fr 1fr;gap:12px}figure{margin:0;border:1px solid #d5dfe6;padding:12px}
figcaption{font-weight:bold}svg{width:100%;display:block}svg text{font:14px system-ui,sans-serif;fill:#192e3f}
.delta{font-size:12px;margin:0;color:#465c6b}.error,.error td{color:#a11521;background:#fff0f0}.error text,text.error{fill:#a11521}.ok{color:#14624d}code{overflow-wrap:anywhere}small{color:#465c6b}pre{white-space:pre-wrap;overflow-wrap:anywhere}
@media(max-width:650px){main{padding:12px}.graficas{grid-template-columns:1fr}}
@media print{@page{size:A4;margin:12mm}body{background:white;font-size:10px}main{padding:0;max-width:none}article{padding:10px;margin:12px 0}table{font-size:9px}th,td{padding:4px}h2{break-after:avoid}.graficas{grid-template-columns:1fr 1fr}*{print-color-adjust:exact}}
</style><main><h1>Mediciones de ccom</h1><p>CE1108 · Optimizaciones y niveles de compilación</p>
<p>Instrucciones: operaciones emitidas distintas de NOP. En O0 se cuentan los slots con mnemónico distinto de <code>nopi</code> mediante el parser del simulador; O1/O2 usan <code>--metrics</code>. Bytes: 16 por bundle, leídos de la cabecera del BIN; excluye la cabecera de 16 bytes y los datos iniciales. Ciclos: <code>Simulator.cycles</code> con datos cero. Tiempo: mediana de cinco compilaciones con <code>--time</code>, en ms, incluyendo escritura de salidas y métricas. Se valida equivalencia también con el patrón de 1024 bytes de comparar.py. Los tiempos dependen de la carga del equipo; una reducción da variación negativa.</p>
<h2>Resumen</h2><table><thead><tr><th>Modo / pasada</th><th>Programa</th><th>Equivalente</th><th>Instrucciones</th><th>Bytes</th><th>Ciclos</th><th>Tiempo (ms)</th></tr></thead><tbody>''']
    resultados = indice['resultados']
    for r in resultados:
        estado = 'Sí' if r['equivalente'] else 'NO'
        fila = [escape(r['modo'] + (' / ' + r['pasada'] if r['pasada'] else '')), escape(r['programa']), estado]
        for campo in ('emitidas', 'bytes', 'ciclos', 'tiempo_ms'):
            fila.append('<br>'.join(escape(f'{k}: ' + ('no compila' if v['codigo'] else numero(v[campo])))
                                     for k, v in r['compilaciones'].items()))
        partes.append(f'<tr class="{"ok" if r["equivalente"] else "error"}">' + ''.join(f'<td>{v}</td>' for v in fila) + '</tr>')
    partes.append('</tbody></table>')
    for modo in ('optimizacion', 'nivel'):
        grupos = PASADAS if modo == 'optimizacion' else (None,)
        for grupo in grupos:
            casos = [r for r in resultados if r['modo'] == modo and r['pasada'] == grupo]
            if not casos:
                continue
            partes.append(f'<h2>{"Optimización / " + grupo if grupo else "Niveles O0 / O1 / O2"}</h2>')
            for r in casos:
                partes.append(f'<article class="{"" if r["equivalente"] else "error"}"><h3>{escape(r["programa"])}</h3>')
                partes.append(f'<p>Equivalente: <strong>{"Sí" if r["equivalente"] else "NO"}</strong> · Referencia: {escape(r["referencia"])}</p>')
                for mensaje in r['errores']:
                    partes.append(f'<p class="error">{escape(mensaje)}</p>')
                partes.append('<div class="graficas">')
                for campo, titulo in (('emitidas', 'Instrucciones'), ('bytes', 'Bytes'), ('ciclos', 'Ciclos'), ('tiempo_ms', 'Tiempo (ms)')):
                    partes.append(grafica(r['compilaciones'], campo, titulo))
                partes.append('</div><table><tr><th>Contador</th>')
                seleccion = r['compilaciones'] if modo == 'optimizacion' else ({'O2': r['compilaciones']['O2']} if 'O2' in r['compilaciones'] else {})
                partes.extend(f'<th>{escape(k)}</th>' for k in seleccion)
                partes.append('</tr>')
                for campo in CONTADORES:
                    partes.append(f'<tr><td>{campo}</td>')
                    for v in seleccion.values():
                        valor = json.dumps(v['contadores'][campo], ensure_ascii=False) if v['contadores'] is not None else 'no compila'
                        partes.append(f'<td>{escape(valor)}</td>')
                    partes.append('</tr>')
                partes.append('</table>')
                for etiqueta, v in r['compilaciones'].items():
                    if v['codigo']:
                        partes.append(f'<p><b>{escape(etiqueta)}: no compila (código {v["codigo"]})</b></p><pre>{escape(v["error"])}</pre>')
                partes.append('</article>')
    partes.append('</main></html>')
    (salida / 'reporte.html').write_text(''.join(partes), encoding='utf-8')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--modo', choices=('optimizacion', 'nivel'), required=True)
    parser.add_argument('--pasada', choices=PASADAS, help='Para fuentes explícitas en modo optimizacion; por defecto se infiere de la carpeta')
    parser.add_argument('--salida', type=Path, default=ROOT / 'build/metricas')
    parser.add_argument('programas', nargs='*', type=Path)
    args = parser.parse_args(argv)
    if args.pasada and args.modo != 'optimizacion':
        parser.error('--pasada requiere --modo optimizacion')
    fuentes = list(dict.fromkeys(p.resolve() for p in (args.programas or predeterminados(args.modo))))
    tareas = []
    for fuente in fuentes:
        if not fuente.is_file() or fuente.suffix != '.cls':
            parser.error(f'fuente .cls no disponible: {fuente}')
        pasada = (args.pasada or fuente.parent.name) if args.modo == 'optimizacion' else None
        if args.modo == 'optimizacion' and pasada not in PASADAS:
            parser.error(f'no se puede inferir la pasada de {fuente}; usa --pasada')
        tareas.append((fuente, pasada))
    salida = args.salida.resolve()
    salida.mkdir(parents=True, exist_ok=True)
    indice_path = salida / 'indice.json'
    indice = json.loads(indice_path.read_text()) if indice_path.exists() else dict(schema_version=1, resultados=[])
    nuevos = []
    for fuente, pasada in tareas:
        identificador = f'{fuente.stem}.{args.modo}.' + hashlib.sha256(str(fuente).encode()).hexdigest()[:12]
        try:
            resultado = medir(fuente, args.modo, pasada, salida / 'artefactos' / identificador)
        except (ValueError, OSError, subprocess.SubprocessError) as error:
            resultado = dict(schema_version=1, programa=nombre_fuente(fuente), modo=args.modo, pasada=pasada,
                             equivalente=False, referencia='esperados.json spill u O0', errores=[str(error)], compilaciones={})
        resultado['archivo'] = identificador + '.json'
        (salida / resultado['archivo']).write_text(json.dumps(resultado, indent=2, ensure_ascii=False) + '\n')
        nuevos.append(resultado)
        clave = (resultado['modo'], resultado['programa'])
        indice['resultados'] = [r for r in indice['resultados'] if (r['modo'], r['programa']) != clave]
        indice['resultados'].append(resultado)
        print(f'{args.modo}: {resultado["programa"]}: {"equivalente" if resultado["equivalente"] else "FALLA"}', flush=True)
    indice['resultados'].sort(key=lambda r: (r['modo'], PASADAS.index(r['pasada']) if r['pasada'] else 0, r['programa']))
    indice_path.write_text(json.dumps(indice, indent=2, ensure_ascii=False) + '\n')
    escribir_html(indice, salida)
    return int(any(not r['equivalente'] for r in nuevos))


if __name__ == '__main__':
    raise SystemExit(main())
