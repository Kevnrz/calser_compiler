#!/usr/bin/env python3
"""C0.1: errores de generación con nombre del fuente, posición y sin salidas.

Usa: ccom y biblioteca estándar de Python. Lo usa make check.
La expresión pasa el semántico y supera los doce temporales del generador.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def main():
    """Comprueba el diagnóstico en ambos modos de organización.

    Entrada: ninguna; requiere ccom actualizado.
    Salida: 0 si pasan las aserciones.
    Efectos: crea fuente y rutas de salida en una carpeta temporal que elimina.
    Restricciones: no depende del nombre aleatorio de la carpeta ni de tiempos.
    """
    expression = 'x'
    for _ in range(12):
        expression = 'x + (' + expression + ')'
    assignment = '    r = ' + expression + ' ;'
    column = assignment.rindex('x') + 1
    text = ('F main [] : void ;\n'
            '    Vm x : int = 1 ;\n'
            '    Vm r : int ;\n'
            + assignment + '\n'
            '    E ;\n'
            '.\n')

    with tempfile.TemporaryDirectory(prefix='calser_diagnosticos_') as temporary:
        folder = Path(temporary)
        source = folder / 'fuente con espacios.cls'
        source.write_text(text, encoding='utf-8')
        for flags in ([], ['--no-schedule']):
            result = subprocess.run(
                [str(ROOT / 'ccom'), '-v', '-s', *flags, str(source),
                 '-o', str(folder / 'salida.bin')],
                capture_output=True, text=True, timeout=30)
            assert result.returncode == 1, result
            assert result.stdout == '', result.stdout
            assert result.stderr.splitlines() == [
                'ccom: fase 1-2: analisis lexico y sintactico ... ok',
                'ccom: fase 3: analisis semantico ... ok',
                'ccom: fase 3: tabla de etiquetas ... ok',
                f'{source}:4:{column}: error de generacion: '
                'sin registros temporales disponibles',
                'ccom: fase 4: generacion de codigo ... con errores',
            ], result.stderr
            assert set(folder.iterdir()) == {source}, list(folder.iterdir())
            assert source.read_text(encoding='utf-8') == text

    print('PASS diagnosticos de generacion: ruta, linea, columna, texto y sin salidas en ambos modos')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
