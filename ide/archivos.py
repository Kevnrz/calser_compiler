"""Lectura, identificación de versiones y guardado de archivos del IDE.

La ventana y los servicios de proyecto utilizan este módulo para distinguir el
texto del editor de la versión guardada en disco. No contiene widgets ni toma
la decisión de sobrescribir: informa cambios mediante CambioExterno para que
la interfaz pueda ofrecer recargar, conservar o guardar en otra ubicación.

Una versión combina metadatos del archivo y una huella SHA-256 de sus bytes.
El guardado escribe un temporal y después reemplaza el destino; evita dejar
medio archivo escrito, pero no bloquea a otros programas que usen esa ruta.
"""
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
    """Descripción inmutable de una versión del disco, no del buffer del editor.

    Comparar dos instancias compara todos sus campos. La huella ayuda a detectar
    cambios de contenido incluso si se conservaron tamaño y fecha de modificación.
    Los datos identifican la versión observada; no garantizan que siga igual después.
    """
    existe: bool
    # Dispositivo e inodo identifican el archivo observado; reemplazar la ruta
    # por otro archivo puede cambiar su identidad aunque tenga contenido igual.
    dispositivo: int = 0
    inodo: int = 0
    # Tamaño en bytes y fecha de modificación en nanosegundos. La huella se
    # calcula sobre bytes, antes de decodificar o normalizar finales de línea.
    tamano: int = 0
    modificado_ns: int = 0
    huella: str = ""


# AUSENTE es una versión concreta: esperamos que todavía no exista el destino.
# _SIN_COMPROBAR significa que el llamador no pidió verificar ninguna versión.
# Son casos diferentes al guardar un archivo nuevo frente a guardar sin control.
AUSENTE = EstadoArchivo(False)
_SIN_COMPROBAR = object()


def _datos(stat_result):
    """Extrae los metadatos utilizados para comparar identidad, tamaño y fecha.
    """
    return (stat_result.st_dev, stat_result.st_ino, stat_result.st_size, stat_result.st_mtime_ns)


def _leer_version(path, *, contenido=False, cancelar=None):
    """Lee una versión y devuelve (EstadoArchivo, bytes solicitados).

        Siempre calcula la huella; solo acumula el contenido si contenido=True para
        no guardar otra copia completa en memoria cuando basta comprobar la versión.
        Si la ruta no existe al abrir, devuelve AUSENTE y bytes vacíos. Si desaparece
        o cambian los metadatos durante la lectura, lanza CambioExterno.

        cancelar, si se entrega, es una función sin argumentos consultada entre
        bloques; devolver verdadero interrumpe con InterruptedError. Otros errores
        de acceso se propagan al llamador.
    """
    path = Path(path)
    try:
        stream = path.open('rb')
    except FileNotFoundError:
        # Una carpeta padre desaparecida también equivale a fuente ausente.
        return AUSENTE, b''
    with stream:
        # fstat inspecciona el archivo realmente abierto; stat de la ruta al
        # final permite detectar también que la sustituyeron durante la lectura.
        antes = os.fstat(stream.fileno())
        if not stat.S_ISREG(antes.st_mode):
            raise OSError('La ruta no es un archivo regular')
        digest = hashlib.sha256()
        partes = []
        while True:
            if cancelar and cancelar():
                raise InterruptedError('Comprobación cancelada')
            # Procesar bloques de 64 KiB limita memoria temporal y ofrece puntos
            # de cancelación sin cargar de una vez todos los archivos vigilados.
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
    """Obtiene la versión sin devolver contenido ni exigir que el archivo exista.

        Lo utilizan el monitor de disco y las comprobaciones de fuente/artefactos.
        Aunque no devuelve texto, recorre los bytes para calcular su huella.
    """
    return _leer_version(path, cancelar=cancelar)[0]


def cargar(path):
    """Devuelve (texto normalizado, versión de los bytes leídos).

        Acepta UTF-8 con BOM opcional y convierte CRLF/CR a LF para el editor. Una
        ruta ausente produce FileNotFoundError; los errores de lectura o codificación
        se propagan para que la interfaz los informe sin reemplazar el buffer.
    """
    version, datos = _leer_version(path, contenido=True)
    if not version.existe:
        raise FileNotFoundError(str(path))
    texto = datos.decode('utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')
    return texto, version


def leer(path):
    """Entrega solo el texto cuando el consumidor no necesita la versión.
    """
    return cargar(path)[0]


def guardar(path, text, *, esperado=_SIN_COMPROBAR):
    """Guarda text como UTF-8/LF y devuelve la versión del archivo preparado.

    esperado permite exigir que el destino siga siendo la versión que conocía
    el editor. Si cambió, lanza CambioExterno antes de reemplazarlo. Pasar AUSENTE
    exige que no exista; omitir esperado evita esa comprobación.

    El temporal se crea junto al destino para reemplazarlo con os.replace en el
    mismo sistema de archivos. No hay compare-and-swap portable: otro programa
    aún podría escribir entre comprobar y reemplazar. La UI no promete bloqueo
    exclusivo. Los errores se propagan y el temporal restante se limpia.
    """
    path = Path(path).absolute()
    # Con una versión esperada, un destino convertido en enlace no debe dirigir
    # silenciosamente la escritura a otro archivo. Sin control se resuelve el enlace.
    if path.is_symlink():
        if esperado is not _SIN_COMPROBAR:
            raise CambioExterno('La ruta ahora es un enlace simbólico. Usa Guardar como para elegir un destino explícito.')
        path = path.resolve()
    descriptor, temporary = tempfile.mkstemp(prefix=f'.{path.name}.', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w', encoding='utf-8', newline='\n') as stream:
            stream.write(text.replace('\r\n', '\n').replace('\r', '\n'))
            # flush entrega el buffer de Python al sistema; fsync solicita
            # sincronizar los datos del temporal antes de reemplazar el destino.
            stream.flush()
            os.fsync(stream.fileno())
        # Conservar los bits de permiso básicos de un destino existente evita
        # cambiarlos por los permisos iniciales del archivo temporal.
        if path.exists():
            os.chmod(temporary, path.stat().st_mode & 0o777)
        # Se calcula sobre el temporal que pasará a ocupar la ruta del destino.
        # La comprobación de esperado se deja lo más cerca posible del replace.
        nueva_version = estado(temporary)
        if esperado is not _SIN_COMPROBAR and estado(path) != esperado:
            raise CambioExterno('El archivo volvió a cambiar. Se canceló el guardado para conservar ambas versiones.')
        os.replace(temporary, path)
        return nueva_version
    finally:
        # Si falló la escritura o la comprobación, no dejar temporales olvidados.
        # Si replace tuvo éxito, esta ruta temporal ya no existe.
        if os.path.exists(temporary):
            os.unlink(temporary)
