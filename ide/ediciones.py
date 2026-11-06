"""Validación y transformación de ediciones del texto original, sin Qt."""
from ide.nucleo.asistencia import EdicionTexto, PrecondicionTexto, cadena, entero

MAX_TEXTO = 2_000_000
MAX_EDICIONES = 128


def validar_ediciones(texto, ediciones, precondiciones=()):
    cadena(texto, 'texto')
    if len(texto) > MAX_TEXTO:
        raise ValueError('El documento supera el límite de 2 millones de caracteres')
    ediciones = tuple(ediciones)
    if len(ediciones) > MAX_EDICIONES:
        raise ValueError('Máximo 128 ediciones por propuesta')
    ordenadas = sorted(ediciones, key=lambda e: (e.inicio, e.fin)) if all(isinstance(e, EdicionTexto) for e in ediciones) else None
    if ordenadas is None:
        raise ValueError('Cada edición debe ser EdicionTexto')
    anterior = None
    for edicion in ordenadas:
        if edicion.fin > len(texto):
            raise ValueError('Rango fuera del texto original')
        # Qt normaliza estos caracteres al leer/escribir texto plano. No hacerlo
        # silenciosamente sobre una propuesta cuyo cursor depende de su longitud.
        if any(c in edicion.texto for c in ('\r', '\u2028', '\u2029', '\u00a0')):
            raise ValueError('El texto nuevo debe usar LF y espacios normales para el editor')
        if anterior is not None:
            if (edicion.inicio < anterior.fin or edicion.inicio == anterior.inicio
                    or (edicion.inicio == anterior.fin and (edicion.inicio == edicion.fin or anterior.inicio == anterior.fin))):
                raise ValueError('Ediciones superpuestas o inserciones ambiguas en el mismo límite; fusionarlas en el motor')
        anterior = edicion
    nuevo_largo = len(texto) + sum(len(e.texto) - (e.fin - e.inicio) for e in ordenadas)
    if nuevo_largo > MAX_TEXTO:
        raise ValueError('El resultado supera el límite de 2 millones de caracteres')
    precondiciones = tuple(precondiciones)
    if len(precondiciones) > MAX_EDICIONES:
        raise ValueError('Máximo 128 precondiciones por propuesta')
    for p in precondiciones:
        if not isinstance(p, PrecondicionTexto) or p.fin > len(texto):
            raise ValueError('Precondición inválida')
        if texto[p.inicio:p.fin] != p.texto_esperado:
            raise ValueError('El texto no cumple una precondición de la propuesta')
    return tuple(ordenadas)


def aplicar_ediciones(texto, ediciones, precondiciones=()):
    ordenadas = validar_ediciones(texto, ediciones, precondiciones)
    partes, inicio = [], 0
    for e in ordenadas:
        partes.extend((texto[inicio:e.inicio], e.texto))
        inicio = e.fin
    partes.append(texto[inicio:])
    return ''.join(partes)


def python_a_utf16(texto, posicion):
    entero(posicion, 'posicion')
    if posicion > len(texto):
        raise ValueError('Posición Python fuera del documento')
    return len(texto[:posicion].encode('utf-16-le')) // 2


def utf16_a_python(texto, posicion):
    entero(posicion, 'posicion')
    datos = texto.encode('utf-16-le')
    if posicion > len(datos) // 2:
        raise ValueError('Posición UTF-16 fuera del documento')
    try:
        return len(datos[:posicion * 2].decode('utf-16-le'))
    except UnicodeError as error:
        raise ValueError('La posición divide un par sustituto UTF-16') from error


def mapa_utf16(texto, posiciones):
    resultado, anterior, unidades = {}, 0, 0
    for posicion in sorted(set(posiciones)):
        entero(posicion, 'posicion')
        if posicion > len(texto):
            raise ValueError('Posición fuera del documento')
        unidades += len(texto[anterior:posicion].encode('utf-16-le')) // 2
        resultado[posicion] = unidades
        anterior = posicion
    return resultado


def trasladar_posicion(posicion, ordenadas, *, derecha=True):
    """Dentro de un reemplazo, elegir su borde izquierdo o derecho."""
    delta = 0
    for e in ordenadas:
        if posicion < e.inicio:
            break
        if posicion > e.fin or (posicion == e.fin and e.fin > e.inicio):
            delta += len(e.texto) - (e.fin - e.inicio)
        else:
            return e.inicio + delta + (len(e.texto) if derecha else 0)
    return posicion + delta
