"""Lectura consistente y guardado atómico con comprobación de versión de disco."""
from dataclasses import dataclass
import hashlib
import os
import stat
import tempfile
from pathlib import Path


class CambioExterno(OSError):
    """El archivo ya no coincide con la versión que se pretende reemplazar."""


@dataclass(frozen=True)
class EstadoArchivo:
    existe: bool
    dispositivo: int = 0
    inodo: int = 0
    tamano: int = 0
    modificado_ns: int = 0
    huella: str = ""


AUSENTE = EstadoArchivo(False)
_SIN_COMPROBAR = object()


def _datos(stat_result):
    return (stat_result.st_dev, stat_result.st_ino, stat_result.st_size, stat_result.st_mtime_ns)


def _leer_version(path, *, contenido=False, cancelar=None):
    path = Path(path)
    try:
        stream = path.open('rb')
    except FileNotFoundError:
        # Una carpeta padre desaparecida también equivale a fuente ausente.
        return AUSENTE, b''
    with stream:
        antes = os.fstat(stream.fileno())
        if not stat.S_ISREG(antes.st_mode):
            raise OSError('La ruta no es un archivo regular')
        digest = hashlib.sha256()
        partes = []
        while True:
            if cancelar and cancelar():
                raise InterruptedError('Comprobación cancelada')
            bloque = stream.read(65536)
            if not bloque:
                break
            digest.update(bloque)
            if contenido:
                partes.append(bloque)
        despues = os.fstat(stream.fileno())
    try:
        actual = path.stat()
    except FileNotFoundError as error:
        raise CambioExterno('El archivo desapareció durante la lectura') from error
    if _datos(antes) != _datos(despues) or _datos(actual) != _datos(despues):
        raise CambioExterno('El archivo cambió durante la lectura; vuelve a intentar')
    return EstadoArchivo(True, *_datos(despues), digest.hexdigest()), b''.join(partes)


def estado(path, *, cancelar=None):
    return _leer_version(path, cancelar=cancelar)[0]


def cargar(path):
    version, datos = _leer_version(path, contenido=True)
    if not version.existe:
        raise FileNotFoundError(str(path))
    texto = datos.decode('utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')
    return texto, version


def leer(path):
    return cargar(path)[0]


def guardar(path, text, *, esperado=_SIN_COMPROBAR):
    """Reemplazo atómico. Si se entrega esperado, comprobarlo antes del replace.

    No hay compare-and-swap portable del sistema de archivos: un escritor ajeno
    aún podría escribir entre la última comprobación y os.replace. La UI no
    promete un bloqueo exclusivo frente a otros programas.
    """
    path = Path(path).absolute()
    if path.is_symlink():
        if esperado is not _SIN_COMPROBAR:
            raise CambioExterno('La ruta ahora es un enlace simbólico. Usa Guardar como para elegir un destino explícito.')
        path = path.resolve()
    descriptor, temporary = tempfile.mkstemp(prefix=f'.{path.name}.', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w', encoding='utf-8', newline='\n') as stream:
            stream.write(text.replace('\r\n', '\n').replace('\r', '\n'))
            stream.flush()
            os.fsync(stream.fileno())
        if path.exists():
            os.chmod(temporary, path.stat().st_mode & 0o777)
        nueva_version = estado(temporary)
        if esperado is not _SIN_COMPROBAR and estado(path) != esperado:
            raise CambioExterno('El archivo volvió a cambiar. Se canceló el guardado para conservar ambas versiones.')
        os.replace(temporary, path)
        return nueva_version
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
