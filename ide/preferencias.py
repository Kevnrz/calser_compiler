"""Validación de preferencias de presentación. Nunca restaura buffers o procesos."""
import json


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
    settings.setValue('estado_v1', json.dumps(validar(datos), ensure_ascii=False))
    settings.sync()
    return settings.status() == settings.Status.NoError
