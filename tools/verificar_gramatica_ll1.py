"""Contrasta la gramática LL(1) de referencia con las reglas actuales de Bison.

Es una herramienta de desarrollo. No analiza documentos del editor ni cambia
el motor productivo. Bison exporta sus reglas a XML en una carpeta temporal.
Las acciones del AST y las reglas de recuperación no forman parte del lenguaje
válido. Las reescrituras conservan los terminales y eliminan recursión izquierda.
"""
from collections import defaultdict
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from ide.nucleo.gramatica import CALSER, Gramatica
from ide.nucleo.tabla import construir_tabla


def reglas_bison(xml):
    """Lee reglas válidas y descarta recuperación, aceptación y acciones internas."""
    reglas = defaultdict(list)
    for regla in ET.parse(xml).findall('./grammar/rules/rule'):
        cabeza = regla.findtext('lhs')
        cuerpo = [s.text for s in regla.findall('./rhs/symbol')]
        if cabeza.startswith('$') or 'error' in cuerpo:
            continue
        simbolos = []
        for simbolo in cuerpo:
            if simbolo.startswith('$@') or simbolo.startswith('@'):
                continue
            simbolos.append({'"identificador"': 'id', '"numero"': 'num'}.get(
                simbolo, simbolo.removeprefix('"').removesuffix('"')))
        reglas[cabeza].append(tuple(simbolos))
    return dict(reglas)


# Cada factorización tiene una precondición con las reglas Bison originales.
# Si cambia una de ellas, hay que revisar la equivalencia antes de continuar.
REESCRITURAS = (
    ('globales -> ε | globales decl', 'globales -> decl globales | ε'),
    ('cuerpo -> ε | cuerpo sentencia', 'cuerpo -> sentencia cuerpo | ε'),
    ('''condicional -> C [ expr ] ; cuerpo alts residual .
alts -> ε | alts A [ expr ] ; cuerpo
residual -> ε | A ; cuerpo''', '''condicional -> C [ expr ] ; cuerpo resto_c .
resto_c -> A tras_a | ε
tras_a -> [ expr ] ; cuerpo resto_c | ; cuerpo'''),
    ('dims -> [ num ] | [ num ] [ num ]', '''dims -> [ num ] dims_p
dims_p -> [ num ] | ε'''),
    ('decl -> modif id : tipo dims_opt ; | modif id : tipo dims_opt = inicial ;',
     '''decl -> modif id : tipo dims_opt decl_p
decl_p -> ; | = inicial ;'''),
    ('sent_id -> id = expr ; | id [ args ] = expr ; | id [ args ] [ expr ] = expr ; | id [ args ] ;',
     '''sent_id -> id sent_id_p
sent_id_p -> = expr ; | [ args ] tras_args
tras_args -> = expr ; | [ expr ] = expr ; | ;'''),
    ('potencia -> primario | primario ^ unario', '''potencia -> primario potencia_p
potencia_p -> ^ unario | ε'''),
    ('acceso -> id | id [ args ] | id [ args ] [ expr ]', '''acceso -> id acceso_p
acceso_p -> [ args ] indice | ε
indice -> [ expr ] | ε'''),
)


def como_reglas(gramatica):
    """Agrupa cuerpos por cabeza sin depender del número de cada producción."""
    return {c: [p.cuerpo for p in gramatica.alternativas(c)]
            for c in gramatica.no_terminales}


def derivar_ll1(original):
    """Aplica factorizaciones revisadas y elimina recursión izquierda directa.

    Las listas anulables globales/cuerpo se escriben con recursión derecha.
    El resto sigue A -> beta A_p y A_p -> alfa A_p | epsilon.
    No se promete convertir una gramática arbitraria a LL(1).
    """
    reglas = {c: list(cuerpos) for c, cuerpos in original.items()}
    for antes, despues in REESCRITURAS:
        originales = como_reglas(Gramatica(antes))
        for cabeza, cuerpos in originales.items():
            if set(reglas.get(cabeza, [])) != set(cuerpos):
                raise ValueError(f'Cambió la regla {cabeza}. Revisar su transformación LL(1)')
        for cabeza in originales:
            del reglas[cabeza]
        for cabeza, cuerpos in como_reglas(Gramatica(despues)).items():
            if cabeza in reglas:
                raise ValueError(f'La factorización duplicaría {cabeza}')
            reglas[cabeza] = cuerpos
    for cabeza, cuerpos in list(reglas.items()):
        recursivos = [c[1:] for c in cuerpos if c and c[0] == cabeza]
        if not recursivos:
            continue
        bases = [c for c in cuerpos if not c or c[0] != cabeza]
        nuevo = cabeza + '_p'
        if not bases or any(not c for c in recursivos) or nuevo in reglas:
            raise ValueError(f'Recursión no admitida en {cabeza}')
        reglas[cabeza] = [c + (nuevo,) for c in bases]
        reglas[nuevo] = [c + (nuevo,) for c in recursivos] + [()]
    return reglas


def verificar(reglas):
    """Exige igualdad de las reglas transformadas y tabla sin ambigüedades."""
    derivadas = derivar_ll1(reglas)
    referencia = como_reglas(CALSER)
    distintas = sorted(c for c in derivadas.keys() | referencia.keys()
                       if set(derivadas.get(c, [])) != set(referencia.get(c, [])))
    if distintas:
        raise ValueError('La referencia LL(1) difiere en ' + ', '.join(distintas))
    tabla = construir_tabla(CALSER)
    if not tabla.es_ll1:
        raise ValueError(f'La tabla tiene {len(tabla.conflictos)} conflictos')
    return tabla



def verificar_terminales(lexer):
    """Contrasta las reglas literales de Flex y las categorías id y num.

    No implementa otro lexer. Solo inspecciona las declaraciones LEX_RETURN
    para comprobar el alfabeto que deberá recibir el futuro parser C.
    """
    literales = re.findall(r'^\s*("(?:[^"\\]|\\.)*")\s+LEX_RETURN\(TOKEN_\w+,', lexer, re.M)
    terminales = {json.loads(literal) for literal in literales}
    for token, nombre in (('TOKEN_IDENTIFIER', 'id'), ('TOKEN_NUMBER', 'num')):
        if not re.search(r'LEX_RETURN\(' + token + r'\s*,', lexer):
            raise ValueError(f'Falta la categoría Flex {token}')
        terminales.add(nombre)
    diferencia = terminales ^ set(CALSER.terminales)
    if diferencia:
        raise ValueError('Los terminales Flex y LL(1) difieren en ' + ', '.join(sorted(diferencia)))
    return terminales


def main():
    """Exporta las reglas reales de Bison y comprueba la referencia LL(1)."""
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--bison', default='bison', help='Ejecutable de Bison')
    args = cli.parse_args()
    with tempfile.TemporaryDirectory(prefix='calser_gramatica_') as temporary:
        folder = Path(temporary)
        xml = folder / 'parser.xml'
        result = subprocess.run([args.bison, '--warnings=error=conflicts-sr,error=conflicts-rr',
                                 '--xml=' + str(xml), '-o', str(folder / 'parser.c'),
                                 str(ROOT / 'grammar/parser.y')],
                                capture_output=True, text=True, timeout=30)
        if result.returncode:
            raise ValueError(result.stderr.strip())
        reglas = reglas_bison(xml)
        tabla = verificar(reglas)
        verificar_terminales((ROOT / 'grammar/lexer.l').read_text())
    print(json.dumps({'estado': 'ok', 'bison_producciones_validas': sum(map(len, reglas.values())),
                      'bison_no_terminales': len(reglas),
                      'll1_producciones': len(CALSER.producciones),
                      'll1_no_terminales': len(CALSER.no_terminales),
                      'terminales': len(CALSER.terminales), 'celdas': len(tabla.m),
                      'conflictos': len(tabla.conflictos)}, ensure_ascii=False))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
