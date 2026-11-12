"""Puente de clones nativos; Python conserva instantáneas y coordenadas de UI.

Flex y src/clones.c deciden tokens, equivalencia y extensión Tipo I/II. Aquí no
hay expresiones regulares, gramática ni comparación de lexemas. El paquete
CCL1 lleva rutas como identidad y textos como bytes, sin guardar documentos.
"""
from dataclasses import dataclass
from pathlib import Path
import struct
from ide import ccom
from ide.servicio_ccom import consultar, LimiteServicio


class Cancelado(Exception):
    pass


class LimiteAnalisis(ValueError):
    pass


@dataclass(frozen=True)
class Token:
    clase: str
    valor: str
    inicio: int
    fin: int
    linea: int


@dataclass(frozen=True)
class Fragmento:
    archivo: Path
    inicio: int
    fin: int
    linea_inicio: int
    linea_fin: int


@dataclass(frozen=True)
class ParClon:
    tipo: str
    primero: Fragmento
    segundo: Fragmento
    tokens: int


def empaquetar(fuentes, cancelar=lambda: False):
    """Orden estable e identidades únicas; los límites también se aplican en C."""
    if len(fuentes) > 200:
        raise LimiteAnalisis('Más de 200 archivos. Reduce la carpeta analizada.')
    documentos, vistos, piezas, total = [], set(), [b'CCL1', struct.pack('<I', len(fuentes))], 0
    for path, texto in sorted(fuentes.items(), key=lambda item: str(item[0])):
        if cancelar():
            raise Cancelado()
        path = Path(path).resolve()
        if path in vistos:
            raise ValueError('Dos instantáneas identifican el mismo archivo.')
        vistos.add(path)
        if not isinstance(texto, str):
            raise TypeError('Las instantáneas deben ser texto Unicode.')
        nombre, raw = str(path).encode('utf-8'), texto.encode('utf-8')
        total += len(raw)
        if len(raw) > 2_000_000 or total > 20_000_000 or len(nombre) > 16384:
            raise LimiteAnalisis('Límite: 2 MB por documento, 20 MB por consulta y 16384 bytes por ruta.')
        if b'\0' in nombre:
            raise ValueError('Ruta de instantánea inválida.')
        piezas.extend((struct.pack('<II', len(nombre), len(raw)), nombre, raw))
        documentos.append((path, texto, raw))
    return b''.join(piezas), documentos


def interpretar(salida, fuente, codigo, documentos, minimo, cancelar=lambda: False):
    """Valida el contrato C antes de permitir previsualización o navegación.

    Convierte solo los extremos usados. Evita un diccionario por cada carácter
    de hasta 20 MB; los segmentos UTF-8 de cada documento se decodifican una vez.
    """
    requerir = ccom._requerir
    datos = ccom.informe_json(salida, fuente, codigo, 'clones')
    requerir(type(datos.get('minimum')) is int and datos['minimum'] == minimo, 'umbral de otra consulta')
    estado = datos.get('status')
    requerir(isinstance(estado, str) and estado in ('ok', 'limit', 'error', 'lexical_error'), 'estado de clones desconocido')
    requerir(codigo == (estado != 'ok'), 'estado/código de clones inconsistente')
    explicacion = datos.get('explanation')
    requerir(ccom._texto_json(explicacion) and (estado == 'ok' or bool(explicacion.strip())), 'explicación de clones inválida')
    archivos = datos.get('files')
    requerir(isinstance(archivos, list) and len(archivos) <= 200, 'lista de archivos inválida')
    requerir(len(archivos) == len(documentos) or (estado in ('error', 'limit') and not archivos), 'faltan instantáneas en respuesta')
    for i, archivo in enumerate(archivos):
        path, _, raw = documentos[i]
        requerir(isinstance(archivo, dict) and type(archivo.get('id')) is int and archivo['id'] == i
                  and archivo.get('path') == str(path) and type(archivo.get('bytes')) is int
                  and archivo['bytes'] == len(raw), 'identidad/tamaño de instantánea inconsistente')
    pares = datos.get('pairs')
    requerir(isinstance(pares, list) and len(pares) <= 500, 'lista de pares inválida')
    requerir(estado == 'ok' or not pares, 'no se aceptan clones parciales')
    if estado != 'ok':
        if estado == 'limit':
            raise LimiteAnalisis(explicacion)
        raise ValueError(explicacion)
    extremos = [set() for _ in documentos]
    vistos = set()
    for par in pares:
        if cancelar():
            raise Cancelado()
        requerir(isinstance(par, dict) and par.get('type') in ('I', 'II'), 'tipo de clon inválido')
        requerir(type(par.get('tokens')) is int and minimo <= par['tokens'] <= 100000, 'longitud de clon inválida')
        for lado in ('a', 'b'):
            f = par.get(lado)
            requerir(isinstance(f, dict), 'fragmento inválido')
            i, inicio, fin = f.get('file'), f.get('start'), f.get('end')
            requerir(type(i) is int and 0 <= i < len(documentos), 'archivo de fragmento inválido')
            raw = documentos[i][2]
            requerir(type(inicio) is int and type(fin) is int and 0 <= inicio < fin <= len(raw), 'rango de fragmento inválido')
            requerir(raw[inicio] & 0xc0 != 0x80 and (fin == len(raw) or raw[fin] & 0xc0 != 0x80), 'fragmento corta UTF-8')
            requerir(type(f.get('line_start')) is int and type(f.get('line_end')) is int
                      and 1 <= f['line_start'] <= f['line_end'], 'líneas de fragmento inválidas')
            extremos[i].update((inicio, fin))
        a, b = par['a'], par['b']
        requerir(a['file'] < b['file'], 'clones del mismo documento o par invertido')
        clave = (par['type'], a['file'], a['start'], a['end'], b['file'], b['start'], b['end'])
        requerir(clave not in vistos, 'par de clones duplicado')
        vistos.add(clave)
    coordenadas = []
    for (_, _, raw), offsets in zip(documentos, extremos):
        anterior, caracteres, linea, mapa = 0, 0, 1, {}
        for offset in sorted(offsets):
            if cancelar():
                raise Cancelado()
            segmento = raw[anterior:offset]
            caracteres += len(segmento.decode('utf-8'))
            linea += segmento.count(b'\n')
            mapa[offset] = (caracteres, linea)
            anterior = offset
        coordenadas.append(mapa)
    resultado = []
    for par in pares:
        fragmentos = []
        for lado in ('a', 'b'):
            f = par[lado]; i = f['file']
            inicio, primera = coordenadas[i][f['start']]
            fin, ultima = coordenadas[i][f['end']]
            requerir((primera, ultima) == (f['line_start'], f['line_end']), 'líneas ajenas al fragmento')
            fragmentos.append(Fragmento(documentos[i][0], inicio, fin, primera, ultima))
        resultado.append(ParClon(par['type'], *fragmentos, par['tokens']))
    return resultado


def detectar(fuentes, minimo=20, cancelar=lambda: False, *, root=None, configurado=''):
    """Consulta C en un hijo cancelable. Nunca guarda los buffers del editor."""
    if type(minimo) is not int or not 5 <= minimo <= 100000:
        raise ValueError('El mínimo debe estar entre 5 y 100000 tokens.')
    if cancelar():
        raise Cancelado()
    paquete, documentos = empaquetar(fuentes, cancelar)
    try:
        salida, fuente, codigo = consultar(paquete, ['--clones-json', str(minimo)], root=root,
            configurado=configurado, cancelar=cancelar, plazo=15, max_salida=1_000_000)
        if cancelar():
            raise Cancelado()
        return interpretar(salida, fuente, codigo, documentos, minimo, cancelar)
    except InterruptedError as error:
        raise Cancelado() from error
    except LimiteServicio as error:
        raise LimiteAnalisis(str(error)) from error


def tokenizar(texto, cancelar=lambda: False):
    """Compatibilidad para inspección/pruebas; Flex sigue siendo el único lexer.

    El detector productivo no usa este helper: envía todos los textos al motor C.
    """
    try:
        salida, fuente, codigo = consultar(texto, ['--tokens-json'], cancelar=cancelar)
        spans, problemas, fatal = ccom.lexico_json(salida, fuente, texto, codigo)
        if problemas or fatal:
            raise ValueError('Hay errores léxicos; no se pueden comparar clones.')
        raw = texto.encode('utf-8')
        tokens = []
        anterior, caracteres, linea = 0, 0, 1
        for span in spans:
            if cancelar():
                raise Cancelado()
            a, b = span['start'], span['end']
            if span['kind'] == 'comentario':
                continue
            salto = raw[anterior:a]
            caracteres += len(salto.decode('utf-8')); linea += salto.count(b'\n')
            valor = raw[a:b].decode('utf-8')
            tokens.append(Token(span['kind'], valor, caracteres, caracteres + len(valor), linea))
            caracteres += len(valor); anterior = b
        return tokens
    except InterruptedError as error:
        raise Cancelado() from error
