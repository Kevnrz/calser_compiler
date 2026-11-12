"""Adaptador del motor C al contrato compartido de asistencia (sin Qt).

C decide contexto, fragmento, filtrado y orden. Aquí solo se valida el protocolo
JSON y se traducen fronteras UTF-8 a offsets Python del aplicador existente.
"""
from ide import ccom
from ide.servicio_ccom import consultar, LimiteServicio
from ide.nucleo.asistencia import EstadoAsistencia, ResultadoSugerencias, Sugerencia


def interpretar(salida, fuente, texto, cursor, codigo):
    """Valida campos M3 y convierte bytes del original a caracteres Python.

    No controla vigencia/revisión ni toca Qt; el coordinador y el aplicador
    comprueban esos datos justo antes de mostrar/aplicar. Los mapas son locales
    a esta instantánea y se descartan al retornar; ninguna caché cruza textos.
    """
    requerir = ccom._requerir
    datos = ccom.informe_json(salida, fuente, codigo, 'suggest')
    cursor_bytes = len(texto[:cursor].encode('utf-8'))
    requerir(type(datos.get('cursor')) is int and datos['cursor'] == cursor_bytes, 'cursor de otra consulta')
    estados = {e.value: e for e in (EstadoAsistencia.OK, EstadoAsistencia.SIN_OPCIONES,
               EstadoAsistencia.BLOQUEADO, EstadoAsistencia.LIMITE, EstadoAsistencia.ERROR)}
    estado = datos.get('status')
    requerir(isinstance(estado, str) and estado in estados, 'estado de sugerencias desconocido')
    requerir(codigo == (1 if estado in ('error', 'limite') else 0), 'estado/código de sugerencias inconsistente')
    explicacion = datos.get('explanation')
    requerir(ccom._texto_json(explicacion) and (estado == 'ok' or bool(explicacion.strip())), 'explicación de sugerencias inválida')
    opciones = datos.get('suggestions')
    requerir(isinstance(opciones, list) and len(opciones) <= 64, 'lista de sugerencias inválida')
    requerir(bool(opciones) == (estado == 'ok'), 'estado/lista de sugerencias inconsistente')
    if not opciones:
        return ResultadoSugerencias(estados[estado], explicacion=explicacion)
    # Solo conversiones de coordenadas: no se vuelve a tokenizar el documento.
    fronteras = {0: 0}
    offset = 0
    for i, char in enumerate(texto):
        offset += len(char.encode('utf-8'))
        fronteras[offset] = i + 1
    sugerencias, vistas = [], set()
    for opcion in opciones:
        requerir(isinstance(opcion, dict), 'sugerencia inválida')
        inicio, fin = opcion.get('start'), opcion.get('end')
        requerir(type(inicio) is int and type(fin) is int and inicio in fronteras and fin in fronteras
                  and inicio <= cursor_bytes <= fin, 'rango de sugerencia inválido o fuera del cursor')
        for campo in ('label', 'text', 'detail'):
            requerir(ccom._texto_json(opcion.get(campo), 4096 if campo == 'detail' else 64) and bool(opcion[campo].strip())
                      and '\0' not in opcion[campo], 'texto de sugerencia inválido')
        categoria = opcion.get('category')
        requerir(isinstance(categoria, str) and categoria in ('palabra', 'delimitador', 'operador'), 'categoría de sugerencia inválida')
        clave = (inicio, fin, opcion['text'])
        requerir(clave not in vistas, 'sugerencia duplicada')
        vistas.add(clave)
        preferida = opcion.get('inline_preferred', False)
        requerir(type(preferida) is bool, 'preferencia inline inválida')
        relativo = opcion.get('cursor_relative')
        if relativo is not None:
            raw = opcion['text'].encode('utf-8')
            requerir(type(relativo) is int and 0 <= relativo <= len(raw), 'cursor relativo inválido')
            try:
                relativo = len(raw[:relativo].decode('utf-8'))
            except UnicodeError:
                requerir(False, 'cursor relativo parte un carácter UTF-8')
        sugerencias.append(Sugerencia(opcion['label'], opcion['text'], fronteras[inicio], fronteras[fin], categoria,
                                      opcion['detail'], cursor_relativo=relativo, preferida_inline=preferida))
    return ResultadoSugerencias(estados[estado], tuple(sugerencias), explicacion)


def sugerir(texto, cursor, *, cancelar=None, root=None, configurado=''):
    """Motor compatible con CALSIDE: consulta C, nunca modifica el documento."""
    if not isinstance(texto, str):
        raise TypeError('texto debe ser str')
    if type(cursor) is not int or not 0 <= cursor <= len(texto):
        raise ValueError('cursor fuera del documento')
    cancelar = cancelar or (lambda: False)
    if cancelar():
        return ResultadoSugerencias(EstadoAsistencia.CANCELADO)
    if len(texto) > 2_000_000:
        return ResultadoSugerencias(EstadoAsistencia.LIMITE, explicacion='Documento superior a 2 millones de caracteres.')
    try:
        posicion = len(texto[:cursor].encode('utf-8'))
        salida, fuente, codigo = consultar(texto, ['--suggest-json', str(posicion)], root=root,
            configurado=configurado, cancelar=cancelar, plazo=2.5, max_salida=1_000_000)
        if cancelar():
            return ResultadoSugerencias(EstadoAsistencia.CANCELADO)
        return interpretar(salida, fuente, texto, cursor, codigo)
    except InterruptedError:
        return ResultadoSugerencias(EstadoAsistencia.CANCELADO)
    except LimiteServicio as error:
        return ResultadoSugerencias(EstadoAsistencia.LIMITE, explicacion=str(error))
    except (OSError, ValueError, UnicodeError) as error:
        return ResultadoSugerencias(EstadoAsistencia.ERROR, explicacion=str(error))
