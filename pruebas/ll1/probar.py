"""Compara el reconocedor C con Bison usando sintaxis, sin analizar semántica.

El auxiliar C aborta si intenta llamar a yyparse. Python solo organiza las
entradas y compara resultados de los dos ejecutables independientes.
"""
from pathlib import Path
import argparse
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def compare(binary, source):
    native = subprocess.run([str(binary), str(source)], capture_output=True, text=True, timeout=10)
    # AST se detiene antes de semántica y conserva el parser Bison independiente.
    with tempfile.TemporaryDirectory(prefix='calser_oraculo_') as temporary:
        bison = subprocess.run([str(ROOT / 'ccom'), '--ast-json',
                                str(Path(temporary) / 'arbol.json'), str(source)],
                               capture_output=True, text=True, timeout=10)
    if native.returncode not in (0, 1) or bison.returncode not in (0, 1):
        raise AssertionError(f'Fallo de proceso en {source}\n{native.stderr}\n{bison.stderr}')
    left = json.loads(native.stdout)
    if left['status'] in ('internal_error', 'limit', 'cancelled'):
        raise AssertionError(f'Consulta no resuelta en {source}\n{left}')
    if left['accepted'] != (bison.returncode == 0):
        raise AssertionError(f'Diferencia en {source}\nLL(1) {left}\nBison {bison.stderr}')
    if left['accepted'] != (native.returncode == 0):
        raise AssertionError(f'Código de salida incoherente en {source}')
    return left['accepted']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', required=True, type=Path)
    args = parser.parse_args()
    binary = args.binario.resolve()
    subprocess.run([str(binary), '--self-test'], check=True, timeout=15)
    corpus = sorted([*(ROOT / 'pruebas').rglob('*.cls'),
                     *(ROOT / 'ide/ejemplos').rglob('*.cls'),
                     *(ROOT / 'ejercicios').rglob('*.cls')])
    accepted = sum(compare(binary, source) for source in corpus)
    program = ('F main [ ] : void ; Vm matriz : int [ 2 ] [ 2 ] = [ [ 1 , 2 ] , [ 3 , 4 ] ] ; '
               'Vm total : int = 0 ; S [ i , 0 , 2 , 1 ] ; S [ j , 0 , 2 , 1 ] ; '
               'total = total + matriz [ i ] [ j ] ; . . E ; .')
    tokens = program.split()
    mutations = {program, '', '# comentario', program + ' @', '#[ sin cerrar'}
    for i in range(len(tokens)):
        mutations.add(' '.join(tokens[:i] + tokens[i + 1:]))
        for symbol in (';', ']', 'F'):
            mutations.add(' '.join(tokens[:i] + [symbol] + tokens[i:]))
    with tempfile.TemporaryDirectory(prefix='calser_ll1_') as temporary:
        source = Path(temporary) / 'consulta con espacios.cls'
        for text in sorted(mutations):
            source.write_text(text)
            compare(binary, source)
    print(f'PASS equivalencia sintáctica en {len(corpus)} archivos, {accepted} aceptados')
    print(f'PASS equivalencia sintáctica en {len(mutations)} variantes de arreglos y ciclos')


if __name__ == '__main__':
    main()
