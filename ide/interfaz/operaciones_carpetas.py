"""Operaciones de carpetas fuera del hilo gráfico, sin seguir enlaces al copiar."""
from dataclasses import dataclass, field
import os
from pathlib import Path
from ide.interfaz import papelera as servicio_papelera
import shutil
import stat
import time

from PySide6.QtCore import QDir, QFile, QThread, Signal
from ide import archivos


class CopiaCancelada(OSError):
    pass


def dentro(path, base):
    return Path(path) == Path(base) or Path(path).is_relative_to(base)


@dataclass
class ResultadoCarpeta:
    tipo: str
    origen: Path
    destino: Path | None
    exito: bool = False
    cancelado: bool = False
    parcial: bool = False
    error: str = ''
    antes: dict = field(default_factory=dict)
    despues: dict = field(default_factory=dict)


def copiar_carpeta(origen, destino, cancelar, avance, al_crear=lambda: None):
    """Crea solo destinos nuevos. Los parciales quedan identificados al fallar.

    No hay borrado automático de parciales: podrían contener cambios ajenos.
    Se conserva el origen y no se sigue ningún enlace de su árbol.
    """
    def comprobar():
        if cancelar():
            raise CopiaCancelada('Copia cancelada')
    comprobar()
    destino.mkdir()  # Sin exist_ok: nunca fusionar ni sobrescribir.
    al_crear()
    pendientes = [(origen, destino, False)]
    cantidad = 0
    ultimo = 0.0
    while pendientes:
        comprobar()
        fuente, copia, final = pendientes.pop()
        if final:
            shutil.copystat(fuente, copia, follow_symlinks=False)
            continue
        pendientes.append((fuente, copia, True))
        if fuente.is_symlink():
            raise OSError(f'Una carpeta fue sustituida por un enlace durante la copia: {fuente}')
        with os.scandir(fuente) as entradas:
            for entrada in entradas:
                comprobar()
                src, dst = Path(entrada.path), copia / entrada.name
                if entrada.is_symlink():
                    dst.symlink_to(os.readlink(src))
                elif entrada.is_dir(follow_symlinks=False):
                    dst.mkdir()
                    pendientes.append((src, dst, False))
                elif entrada.is_file(follow_symlinks=False):
                    # O_NOFOLLOW rechaza un enlace que sustituya esta entrada
                    # de archivo después de inspeccionarla.
                    fd = os.open(src, os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_NONBLOCK', 0))
                    with os.fdopen(fd, 'rb') as entrada_archivo:
                        if not stat.S_ISREG(os.fstat(entrada_archivo.fileno()).st_mode):
                            raise OSError(f'La entrada dejó de ser un archivo regular: {src}')
                        with dst.open('xb') as salida:
                            while True:
                                comprobar()
                                bloque = entrada_archivo.read(1024 * 1024)
                                if not bloque:
                                    break
                                salida.write(bloque)
                    shutil.copystat(src, dst, follow_symlinks=False)
                    cantidad += 1
                else:
                    raise OSError(f'Tipo de archivo no admitido: {src}')
                ahora = time.monotonic()
                if ahora - ultimo >= .1:
                    avance(cantidad)
                    ultimo = ahora
    return cantidad


class OperacionCarpeta(QThread):
    resultado = Signal(object)
    progreso = Signal(str)

    def __init__(self, tipo, origen, destino=None, documentos=(), parent=None):
        super().__init__(parent)
        self.tipo, self.origen, self.destino = tipo, origen, destino
        self.documentos = tuple(documentos)

    def run(self):
        r = ResultadoCarpeta(self.tipo, self.origen, self.destino)
        try:
            if self.origen.is_symlink() or not self.origen.is_dir():
                raise OSError('La carpeta de origen ya no está disponible.')
            if self.tipo == 'copiar':
                if self.destino.exists() or self.destino.is_symlink():
                    raise FileExistsError('El destino ya existe; no se reemplazó.')
                if dentro(self.destino.resolve(), self.origen.resolve()):
                    raise OSError('No se puede copiar una carpeta dentro de sí misma.')
                # Registrar si llegamos a crear el destino; un error previo no
                # convierte un destino ajeno en una copia parcial nuestra.
                try:
                    copiar_carpeta(self.origen, self.destino,
                                   self.isInterruptionRequested,
                                   lambda n: self.progreso.emit(f'Copiando carpeta · {n} archivos · {self.destino}'),
                                   lambda: setattr(r, 'parcial', True))
                except OSError:
                    raise
                r.parcial = False
            elif self.tipo == 'mover':
                for path in self.documentos:
                    try:
                        r.antes[path] = archivos.estado(path)
                    except OSError:
                        r.antes[path] = None
                if self.destino.exists() or self.destino.is_symlink():
                    raise FileExistsError('El destino ya existe; no se reemplazó.')
                if dentro(self.destino.resolve(), self.origen.resolve()):
                    raise OSError('No se puede mover una carpeta dentro de sí misma.')
                if self.origen.stat().st_dev != self.destino.parent.stat().st_dev:
                    raise OSError('El traslado de carpetas entre discos requiere el gestor del sistema. Podés copiarla desde CALSIDE.')
                if not QDir().rename(str(self.origen), str(self.destino)):
                    raise OSError('No se pudo mover la carpeta; revisa permisos y destino.')
                for path in self.documentos:
                    nuevo = self.destino / path.relative_to(self.origen)
                    try:
                        r.despues[path] = (archivos.estado(nuevo), '')
                    except OSError as error:
                        r.despues[path] = (None, str(error))
            elif self.tipo == 'papelera':
                ok, papelera = servicio_papelera.enviar_a_papelera(self.origen)
                r.destino = Path(papelera) if papelera else None
                if not ok:
                    raise OSError('La papelera no está disponible. No se intentó borrar permanentemente.')
            else:
                raise ValueError('Operación desconocida')
            r.exito = True
        except CopiaCancelada as error:
            r.cancelado, r.error = True, str(error)
        except (OSError, ValueError) as error:
            r.error = str(error)
        self.resultado.emit(r)
