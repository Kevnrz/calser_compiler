"""Validación y persistencia de la configuración de CALSIDE.

ProyectoVentana reúne los ajustes y los aplica a los widgets; este módulo solo
filtra sus datos y los lee/escribe como JSON mediante el objeto QSettings que
recibe. No importa Qt ni restaura documentos, buffers o procesos de terminal.

Se utiliza la clave estado_v1 con un esquema versionado. Los campos desconocidos
o inválidos se omiten; el consumidor utiliza sus valores predeterminados. El
bloque de asistencia delega su validación a politica_asistencia.
"""
import json
from pathlib import Path
from ide.politica_asistencia import validar_preferencias


def validar(datos):
    """Devuelve un diccionario nuevo con los ajustes admitidos del esquema 1.

        Una versión ausente/desconocida o un contenedor inválido devuelve {}. Los
        límites por campo evitan restaurar tamaños absurdos o estructuras excesivas.
        Validar una ruta aquí comprueba su forma, no su existencia ni sus permisos;
        los componentes que la utilizan realizan esas comprobaciones después.
    """
    # type(... ) is int rechaza también True: bool es subclase de int en Python,
    # pero una marca booleana no representa una versión ni una dimensión.
    if not isinstance(datos, dict) or type(datos.get('version')) is not int or datos.get('version') != 1:
        return {}
    limpio = {'version': 1}
    if 'asistencia' in datos:
        limpio['asistencia'] = validar_preferencias(datos['asistencia'])
    for campo in ('proyecto', 'ccom', 'familia'):
        valor = datos.get(campo)
        if isinstance(valor, str) and len(valor) <= 4096 and '\x00' not in valor:
            limpio[campo] = valor
    # Exigir booleanos reales evita interpretar la cadena "false" como activada.
    for campo in ('explorador', 'resultados', 'panel_derecho', 'automatico'):
        if type(datos.get(campo)) is bool:
            limpio[campo] = datos[campo]
    for campo, minimo, maximo in (('puntos', 6, 40), ('ancho', 800, 3840), ('alto', 520, 2160)):
        valor = datos.get(campo)
        if type(valor) is int and minimo <= valor <= maximo:
            limpio[campo] = valor
    # Cada divisor de la ventana tiene dos paneles: se conservan dos tamaños
    # enteros con un mínimo visible, no una lista arbitraria de dimensiones.
    for campo in ('divisiones', 'paneles'):
        valor = datos.get(campo)
        if (isinstance(valor, list) and len(valor) == 2 and
                all(type(x) is int and 40 <= x <= 10000 for x in valor)):
            limpio[campo] = valor
    # Las ramas se guardan relativas al proyecto. Rechazar rutas absolutas y ..
    # impide usar la restauración del árbol para salir de la raíz elegida.
    ramas = datos.get('carpetas_abiertas')
    if isinstance(ramas, list) and len(ramas) <= 1000:
        limpio['carpetas_abiertas'] = [p for p in ramas if isinstance(p, str) and len(p) <= 4096
                                     and p and '\x00' not in p and not Path(p).is_absolute()
                                     and '..' not in Path(p).parts]
    # Orden visual: carpeta relativa -> nombres de sus hijos, sin rutas anidadas.
    # El límite global complementa al de cada carpeta. dict.fromkeys elimina
    # duplicados conservando la primera aparición y, con ella, el orden elegido.
    orden = datos.get('orden_explorador')
    if isinstance(orden, dict) and len(orden) <= 200:
        limpio['orden_explorador'] = {}
        cantidad = 0
        for padre, nombres in orden.items():
            if (not isinstance(padre, str) or not padre or len(padre) > 4096
                    or '\x00' in padre or Path(padre).is_absolute() or '..' in Path(padre).parts
                    or not isinstance(nombres, list) or len(nombres) > 1000):
                continue
            validos = list(dict.fromkeys(n for n in nombres if isinstance(n, str) and n
                           and len(n) <= 255 and n not in ('.', '..')
                           and not any(c in n for c in ('/', '\\', '\x00', '\n', '\r'))))
            if cantidad + len(validos) > 3000:
                break
            if validos:
                limpio['orden_explorador'][padre] = validos
                cantidad += len(validos)
    return limpio


def cargar(settings):
    """Lee estado_v1, interpreta su JSON y devuelve los ajustes validados.

        settings=False desactiva la persistencia, como en pruebas aisladas. Una cadena
        demasiado grande, JSON dañado o tipos incompatibles devuelve {} para iniciar
        con valores predeterminados en lugar de impedir abrir el IDE.
    """
    if settings is False:
        return {}
    try:
        raw = settings.value('estado_v1', '{}')
        if not isinstance(raw, str) or len(raw) > 32768:
            return {}
        return validar(json.loads(raw))
    except (TypeError, ValueError):
        return {}


def guardar(settings, datos):
    """Valida y guarda el estado; devuelve si la operación terminó correctamente.

        Se reserva primero el estado básico y luego se agrega la parte que quepa de
        ramas y orden. El límite de 32768 cuenta caracteres del JSON, no bytes. Si ni
        el resultado final cabe, devuelve False sin escribirlo. settings=False es un
        guardado desactivado que devuelve True; no indica una escritura real.
    """
    if settings is False:
        return True
    limpio = validar(datos)
    # Separar lo opcional para que un árbol muy grande no desplace ajustes como
    # fuente o permisos de asistencia. Los 90 caracteres reservan un margen para
    # las claves y separadores añadidos al reconstruir el estado.
    ramas = limpio.pop('carpetas_abiertas', [])
    orden = limpio.pop('orden_explorador', {})
    presupuesto = max(0, 32768 - len(json.dumps(limpio, ensure_ascii=False)) - 90)
    # Reservar espacio para ambos datos opcionales sin invalidar el estado básico.
    # Si hay orden manual, repartir inicialmente el espacio. Lo no consumido por
    # las ramas permanece disponible para guardar carpetas del orden manual.
    limite_ramas = presupuesto // 2 if orden else presupuesto
    conservar = []
    for rama in ramas:
        costo = len(json.dumps(rama, ensure_ascii=False)) + 2
        if costo > limite_ramas:
            break
        conservar.append(rama)
        limite_ramas -= costo
        presupuesto -= costo
    limpio['carpetas_abiertas'] = conservar
    limpio['orden_explorador'] = {}
    for padre, nombres in orden.items():
        costo = len(json.dumps({padre: nombres}, ensure_ascii=False)) + 2
        if costo <= presupuesto:
            limpio['orden_explorador'][padre] = nombres
            presupuesto -= costo
    raw = json.dumps(limpio, ensure_ascii=False)
    if len(raw) > 32768:
        return False
    # sync solicita persistir lo acumulado en QSettings; su estado informa fallos
    # de escritura para que la ventana pueda avisar al usuario.
    settings.setValue('estado_v1', raw)
    settings.sync()
    return settings.status() == settings.Status.NoError
