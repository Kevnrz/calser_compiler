"""Perfiles de invocación de ccom. No realiza análisis del lenguaje.

Solo admite banderas conocidas, nunca órdenes de shell ni argumentos libres.
La ventana vuelve a validar el perfil al ejecutarlo y captura una copia por trabajo.
"""
from pathlib import Path
import hashlib

MODOS = {'compilar': 'Compilar programa', 'analizar': 'Analizar sin generar archivos',
         'ast': 'Exportar AST a JSON', 'ensamblar': 'Ensamblar archivo .asm'}
OPCIONES = {
    'asm': ('Generar listado .asm (necesario para F6)', '-s', {'compilar'}),
    'ast_texto': ('Mostrar AST textual', '-t', {'compilar', 'ast'}),
    'tablas': ('Mostrar tablas de símbolos y etiquetas', '-m', {'compilar'}),
    'bundles': ('Mostrar bundles', '-p', {'compilar', 'ensamblar'}),
    'progreso': ('Mostrar progreso de fases', '-v', set(MODOS)),
    'tiempo': ('Medir duración', '--time', set(MODOS)),
    'sin_schedule': ('Desactivar organización de bundles', '--no-schedule', {'compilar'}),
}


def validar(datos):
    """Filtra preferencias y descarta opciones incompatibles con el modo."""
    datos = datos if isinstance(datos, dict) else {}
    modo = datos.get('modo')
    modo = modo if isinstance(modo, str) and modo in MODOS else 'compilar'
    limpio = {'modo': modo}
    destino = datos.get('destino', '')
    limpio['destino'] = destino if isinstance(destino, str) and len(destino) <= 4096 and not any(c in destino for c in '\0\n\r') else ''
    if modo == 'analizar':
        limpio['destino'] = ''
    for clave, (_, _, modos) in OPCIONES.items():
        valor = datos.get(clave, clave == 'asm')
        limpio[clave] = modo in modos and type(valor) is bool and valor
    return limpio


def preparar(datos, fuente):
    """Devuelve perfil, destino, argumentos y salidas previstas sin tocar disco."""
    perfil = validar(datos)
    fuente = Path(fuente).resolve()
    modo = perfil['modo']
    extension = '.asm' if modo == 'ensamblar' else '.cls'
    if fuente.suffix.lower() != extension:
        raise ValueError(f"{MODOS[modo]} requiere una entrada {extension}.")
    destino = None
    salidas = []
    if modo != 'analizar':
        carpeta = fuente.parent / 'build' / 'ide' / hashlib.sha256(str(fuente).encode()).hexdigest()[:10]
        sufijo = '.json' if modo == 'ast' else '.bin'
        destino = Path(perfil['destino']).expanduser() if perfil['destino'] else carpeta / (fuente.stem + sufijo)
        if not destino.is_absolute():
            destino = fuente.parent / destino
        # Comprobar antes de resolver: resolve ocultaría un enlace en la salida.
        if destino.is_symlink():
            raise ValueError('El destino no puede ser un enlace simbólico.')
        destino = destino.parent.resolve() / destino.name
        if destino.suffix.lower() != sufijo:
            raise ValueError(f'El destino debe terminar en {sufijo}.')
        salidas = [destino] if modo == 'ast' else [destino.with_suffix(s) for s in ('.bin', '.hex', '.mem')]
        if modo == 'compilar' and perfil['asm']:
            salidas.append(destino.with_suffix('.asm'))
        if fuente in salidas:
            raise ValueError('El destino sobrescribiría la entrada.')
    if modo == 'analizar':
        args = ['--check', str(fuente)]
    elif modo == 'ast':
        args = ['--ast-json', str(destino), str(fuente)]
    else:
        args = (['-x'] if modo == 'ensamblar' else []) + [str(fuente), '-o', str(destino)]
    args += [bandera for clave, (_, bandera, _) in OPCIONES.items() if perfil[clave]]
    return perfil, destino, args, salidas
