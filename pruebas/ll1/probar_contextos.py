"""Contrasta las consultas nativas de cursor con la referencia LL(1) de desarrollo."""
from pathlib import Path
import argparse
import json
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from ide.nucleo.parser import contexto_en


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binario', required=True, type=Path)
    args = parser.parse_args()
    binary = args.binario.resolve()
    programs = [
        'F main [] : void ; E ; .',
        'F suma [int a, int b] : int r ; r = a + b ; E ; .',
        'F main [] : void ; Vm a : int [2][2] = [[1,2],[3,4]] ; a[0][1] = 9 ; E ; .',
        'F main [] : void ; C [verdadero] ; E ; A [falso] ; E ; A ; E ; . .',
        'F main [] : void ; S [i,0,3,1] ; L [i < 2] ; E ; . . E ; .',
        '# á😀\r\nF main [] : void ; #[ texto ]# Vm x : int = 3 ; E ; .',
        'F main [] : void ; x = 1 + ; E ; .',
        'F main [] : void ; x = desconocida[3] ; E ; .',
    ]
    cases = [(text, cursor, cursor) for text in programs for cursor in range(len(text) + 1)]
    for before, fragment, after in [
        ('F main [] : ', 'vo', ' @@@'), ('', 'V', ''),
        ('F main [] : void ; Vm x : ', 'in', ''),
        ('F main [] : void ; x = a ', '!', ''),
    ]:
        cases.append((before + fragment + after, len(before + fragment), len(before)))
    cases += [('#[ abierto', 10, 10), ('# linea', 7, 7)]
    status = {'prefijo_valido': 'prefix_valid', 'comentario': 'comment',
              'error_sintactico': 'syntax_error', 'error_lexico': 'lexical_error'}
    compared = 0
    with tempfile.TemporaryDirectory(prefix='calser_contextos_') as temporary:
        source = Path(temporary) / 'contexto con espacios.cls'
        for text, cursor, point in cases:
            reference = contexto_en(text, cursor, point)
            source.write_text(text)
            cb, pb = len(text[:cursor].encode()), len(text[:point].encode())
            native = subprocess.run([str(binary), '--context', str(cb), str(pb), str(source)],
                                    capture_output=True, text=True, timeout=10)
            if native.returncode not in (0, 1):
                raise AssertionError(f'Consulta fallida\n{native.stderr}')
            result = json.loads(native.stdout)
            if result['status'] != status[reference.estado]:
                raise AssertionError(f'Estado diferente en {text!r} cursor {cursor} punto {point}\n{result}\n{reference}')
            if reference.valido:
                if set(result['expected']) != set(reference.esperados) or result['can_end'] != reference.puede_terminar:
                    raise AssertionError(f'Contexto diferente en {text!r} cursor {cursor}\n{result}\n{reference}')
            elif reference.estado == 'comentario' and result['expected']:
                raise AssertionError('No debe sugerir dentro de comentarios')
            compared += 1
    print(f'PASS contexto nativo y referencia LL(1) en {compared} posiciones de cursor')


if __name__ == '__main__':
    main()
