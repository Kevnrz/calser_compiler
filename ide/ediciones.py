"""Validación de cambios de texto y conversiones de posiciones, sin Qt.

Los motores describen cambios mediante EdicionTexto y PrecondicionTexto, cuyos
rangos se refieren al texto original y usan índices Python [inicio, fin). Este
módulo comprueba esas propuestas, construye un resultado en memoria y calcula
cómo trasladar posiciones cuando cambia el largo del texto.

No edita documentos abiertos, guarda archivos ni crea historial de Deshacer.
El aplicador de interfaz/ediciones.py utiliza estas funciones, comprueba el
contexto del editor y realiza después la modificación real mediante Qt.
"""
from ide.nucleo.asistencia import EdicionTexto, PrecondicionTexto, cadena, entero

# Límites de trabajo por propuesta; se comprueban antes de modificar el editor.
MAX_TEXTO = 2_000_000
MAX_EDICIONES = 128


def validar_ediciones(texto, ediciones, precondiciones=()):
    """Valida una propuesta y devuelve sus ediciones ordenadas por posición.

        Comprueba tipo de datos, límites, rangos no ambiguos y el texto que exigen las
        precondiciones. Los modelos compartidos ya validan, al construirse, índices
        no negativos y fin >= inicio. Los fallos de esta función lanzan ValueError.

        No modifica texto ni los objetos recibidos. Las precondiciones pueden
        solaparse: son comprobaciones del original, no operaciones de escritura.
    """
    cadena(texto, 'texto')
    if len(texto) > MAX_TEXTO:
        raise ValueError('El documento supera el límite de 2 millones de caracteres')
    ediciones = tuple(ediciones)
    if len(ediciones) > MAX_EDICIONES:
        raise ValueError('Máximo 128 ediciones por propuesta')
    # Ordenar permite comprobar solapamientos mirando solo la edición anterior.
    # Se devuelve una nueva secuencia; no se reordenan los datos del motor in situ.
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
        # Dos inserciones en el mismo punto, o una inserción pegada al límite de
        # otra edición, no definen aquí un orden inequívoco. El motor debe unirlas
        # en una edición. Dos reemplazos no vacíos contiguos sí pueden aceptarse.
        if anterior is not None:
            if (edicion.inicio < anterior.fin or edicion.inicio == anterior.inicio
                    or (edicion.inicio == anterior.fin and (edicion.inicio == edicion.fin or anterior.inicio == anterior.fin))):
                raise ValueError('Ediciones superpuestas o inserciones ambiguas en el mismo límite; fusionarlas en el motor')
        anterior = edicion
    # Una propuesta pequeña sobre un texto permitido puede generar un resultado
    # demasiado grande; el límite se aplica también al tamaño final calculado.
    nuevo_largo = len(texto) + sum(len(e.texto) - (e.fin - e.inicio) for e in ordenadas)
    if nuevo_largo > MAX_TEXTO:
        raise ValueError('El resultado supera el límite de 2 millones de caracteres')
    precondiciones = tuple(precondiciones)
    if len(precondiciones) > MAX_EDICIONES:
        raise ValueError('Máximo 128 precondiciones por propuesta')
    # Se compara contra el original, no contra un resultado parcial. Por ejemplo,
    # una reparación puede exigir que el identificador que analizó siga intacto.
    for p in precondiciones:
        if not isinstance(p, PrecondicionTexto) or p.fin > len(texto):
            raise ValueError('Precondición inválida')
        if texto[p.inicio:p.fin] != p.texto_esperado:
            raise ValueError('El texto no cumple una precondición de la propuesta')
    return tuple(ordenadas)


def aplicar_ediciones(texto, ediciones, precondiciones=()):
    """Construye y devuelve el texto corregido después de validar la propuesta.

        Conserva los fragmentos ajenos a cada rango y agrega el texto nuevo. Usar
        partes y join evita recalcular offsets sobre cadenas ya modificadas y evita
        concatenaciones sucesivas de todo el documento. El original queda intacto.
    """
    ordenadas = validar_ediciones(texto, ediciones, precondiciones)
    partes, inicio = [], 0
    for e in ordenadas:
        partes.extend((texto[inicio:e.inicio], e.texto))
        inicio = e.fin
    partes.append(texto[inicio:])
    return ''.join(partes)


def python_a_utf16(texto, posicion):
    """Convierte una frontera Python base 0 a unidades UTF-16, sin contar BOM.

        Por ejemplo, en "a😀b", la posición Python 2 (después del emoji) equivale
        a la posición UTF-16 3. Se acepta también la frontera al final del texto.
    """
    entero(posicion, 'posicion')
    if posicion > len(texto):
        raise ValueError('Posición Python fuera del documento')
    return len(texto[:posicion].encode('utf-16-le')) // 2


def utf16_a_python(texto, posicion):
    """Convierte una frontera UTF-16 base 0 a un índice de caracteres Python.

        Si cae entre las dos unidades que representan un carácter como 😀, no hay
        una frontera Python equivalente: lanza ValueError en lugar de redondearla
        y desplazar silenciosamente una selección o una edición.
    """
    entero(posicion, 'posicion')
    datos = texto.encode('utf-16-le')
    if posicion > len(datos) // 2:
        raise ValueError('Posición UTF-16 fuera del documento')
    try:
        return len(datos[:posicion * 2].decode('utf-16-le'))
    except UnicodeError as error:
        raise ValueError('La posición divide un par sustituto UTF-16') from error


def mapa_utf16(texto, posiciones):
    """Devuelve {posición Python: posición UTF-16} para varias fronteras.

        Ordena y elimina repeticiones para recorrer solo los segmentos nuevos entre
        posiciones. Es útil al aplicar varias ediciones: evita volver a codificar
        todo el prefijo del documento por cada inicio y fin de rango.
    """
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
    """Traslada una posición Python del original al resultado de las ediciones.

    ordenadas debe contener ediciones válidas en orden ascendente, como las que
    devuelve validar_ediciones. delta acumula el cambio de largo de las que ya
    quedaron antes de la posición.

    Si cae dentro de un reemplazo o justo en una inserción, derecha decide si
    queda antes o después del texto nuevo. Permite conservar la orientación de
    una selección al trasladar por separado cursor y ancla. La frontera final
    de un reemplazo no vacío se traslada junto al texto que viene después.
    """
    delta = 0
    for e in ordenadas:
        if posicion < e.inicio:
            break
        if posicion > e.fin or (posicion == e.fin and e.fin > e.inicio):
            delta += len(e.texto) - (e.fin - e.inicio)
        else:
            return e.inicio + delta + (len(e.texto) if derecha else 0)
    return posicion + delta
