"""Búsqueda literal y acotada del proyecto, sin dependencias de la interfaz."""
from dataclasses import dataclass
import os
from pathlib import Path
import re
import stat

EXCLUIDAS = frozenset({'.git', '.venv', 'venv', '__pycache__', 'build', 'dist',
                      'node_modules', '.pytest_cache', '.mypy_cache', '.ruff_cache', '.idea', '.tox'})


@dataclass(frozen=True)
class Coincidencia:
    tipo: str
    archivo: Path
    linea: int = 0
    columna: int = 0
    longitud: int = 0
    texto: str = ''


@dataclass(frozen=True)
class ResultadoBusqueda:
    coincidencias: tuple = ()
    avisos: tuple = ()
    cancelado: bool = False


def buscar_proyecto(raiz, consulta, modo='todo', mayusculas=False, buffers=None,
                    cancelar=lambda: False, limite=500, max_entradas=20000,
                    max_archivo=2_000_000, max_total=20_000_000):
    raiz = Path(raiz).absolute()
    if not consulta or not consulta.strip():
        return ResultadoBusqueda()
    patron = re.compile(re.escape(consulta), 0 if mayusculas else re.IGNORECASE)
    buffers = buffers or {}
    encontrados, avisos, vistos = [], set(), set()
    visitadas = total = 0

    def permitido(p):
        return (p.is_relative_to(raiz) and p.suffix == '.cls'
                and not any(n in EXCLUIDAS for n in p.relative_to(raiz).parts[:-1])
                and not any(q.is_symlink() for q in (p,) + tuple(p.parents) if q != raiz and q.is_relative_to(raiz)))

    def contenido(path):
        nonlocal total
        if path in buffers:
            texto = buffers[path]
            tamano = len(texto.encode('utf-8'))
        else:
            descriptor = os.open(path, os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_NONBLOCK', 0))
            with os.fdopen(descriptor, 'rb') as fuente:
                datos = os.fstat(fuente.fileno())
                if not stat.S_ISREG(datos.st_mode):
                    raise OSError('Entrada no regular')
                if datos.st_size > max_archivo or total + datos.st_size > max_total:
                    avisos.add('Se omitió contenido por el límite de tamaño (2 MB por archivo / 20 MB en total).')
                    return
                partes, tamano = [], 0
                while bloque := fuente.read(65536):
                    if cancelar():
                        return
                    tamano += len(bloque)
                    if tamano > max_archivo or total + tamano > max_total:
                        avisos.add('Se omitió contenido por el límite de tamaño (2 MB por archivo / 20 MB en total).')
                        return
                    partes.append(bloque)
                texto = b''.join(partes).decode('utf-8-sig')
        if tamano > max_archivo or total + tamano > max_total:
            avisos.add('Se omitió contenido por el límite de tamaño (2 MB por archivo / 20 MB en total).')
            return
        total += tamano
        for numero, linea in enumerate(texto.splitlines(), 1):
            if cancelar():
                return
            for match in patron.finditer(linea):
                encontrados.append(Coincidencia('codigo', path, numero, match.start() + 1,
                                                match.end() - match.start(), linea))
                if len(encontrados) >= limite:
                    return

    def archivo(path):
        vistos.add(path)
        if modo in ('todo', 'archivos') and patron.search(str(path.relative_to(raiz))):
            encontrados.append(Coincidencia('archivo', path))
        if modo in ('todo', 'codigo') and len(encontrados) < limite:
            try:
                contenido(path)
            except (OSError, UnicodeError):
                avisos.add('Hay archivos no legibles o que no son UTF-8; se omitió su contenido.')

    pila = [raiz]
    while pila and len(encontrados) < limite:
        if cancelar():
            return ResultadoBusqueda(cancelado=True)
        carpeta = pila.pop()
        if carpeta.is_symlink():
            continue
        try:
            with os.scandir(carpeta) as entradas:
                for entrada in entradas:
                    if cancelar():
                        return ResultadoBusqueda(cancelado=True)
                    visitadas += 1
                    if visitadas > max_entradas:
                        avisos.add('Se alcanzó el límite de entradas recorridas; acota la búsqueda.')
                        pila.clear()
                        break
                    path = Path(entrada.path)
                    if entrada.is_symlink():
                        continue
                    if entrada.is_dir(follow_symlinks=False):
                        if entrada.name in EXCLUIDAS:
                            continue
                        if modo in ('todo', 'carpetas') and patron.search(str(path.relative_to(raiz))):
                            encontrados.append(Coincidencia('carpeta', path))
                        pila.append(path)
                    elif path.suffix == '.cls' and entrada.is_file(follow_symlinks=False):
                        archivo(path)
                    if len(encontrados) >= limite:
                        break
        except OSError:
            avisos.add('Hay carpetas o entradas sin acceso; los resultados pueden estar incompletos.')
    if len(encontrados) < limite:
        for path in buffers:
            if cancelar():
                return ResultadoBusqueda(cancelado=True)
            if path not in vistos and permitido(path):
                archivo(path)
                if len(encontrados) >= limite:
                    break
    if cancelar():
        return ResultadoBusqueda(cancelado=True)
    if len(encontrados) >= limite:
        avisos.add(f'Se muestran las primeras {limite} coincidencias; acota la búsqueda.')
    encontrados.sort(key=lambda h: (h.tipo, str(h.archivo).casefold(), h.linea, h.columna))
    return ResultadoBusqueda(tuple(encontrados), tuple(sorted(avisos)))
