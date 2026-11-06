"""Validación de preferencias de presentación. Nunca restaura buffers o procesos."""
import json
from pathlib import Path


def validar(datos):
    if not isinstance(datos, dict) or type(datos.get('version')) is not int or datos.get('version') != 1:
        return {}
    limpio = {'version': 1}
    for campo in ('proyecto', 'ccom', 'familia'):
        valor = datos.get(campo)
        if isinstance(valor, str) and len(valor) <= 4096 and '\x00' not in valor:
            limpio[campo] = valor
    for campo in ('explorador', 'resultados', 'panel_derecho', 'automatico'):
        if type(datos.get(campo)) is bool:
            limpio[campo] = datos[campo]
    for campo, minimo, maximo in (('puntos', 6, 40), ('ancho', 800, 3840), ('alto', 520, 2160)):
        valor = datos.get(campo)
        if type(valor) is int and minimo <= valor <= maximo:
            limpio[campo] = valor
    for campo in ('divisiones', 'paneles'):
        valor = datos.get(campo)
        if (isinstance(valor, list) and len(valor) == 2 and
                all(type(x) is int and 40 <= x <= 10000 for x in valor)):
            limpio[campo] = valor
    ramas = datos.get('carpetas_abiertas')
    if isinstance(ramas, list) and len(ramas) <= 1000:
        limpio['carpetas_abiertas'] = [p for p in ramas if isinstance(p, str) and len(p) <= 4096
                                     and p and '\x00' not in p and not Path(p).is_absolute()
                                     and '..' not in Path(p).parts]
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
    if settings is False:
        return True
    limpio = validar(datos)
    ramas = limpio.pop('carpetas_abiertas', [])
    orden = limpio.pop('orden_explorador', {})
    presupuesto = max(0, 32768 - len(json.dumps(limpio, ensure_ascii=False)) - 90)
    # Reservar espacio para ambos datos opcionales sin invalidar el estado básico.
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
    settings.setValue('estado_v1', raw)
    settings.sync()
    return settings.status() == settings.Status.NoError
