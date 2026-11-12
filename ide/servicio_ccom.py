"""Transporte cancelable para consultas de ccom sobre instantáneas privadas.

Lo usan adaptador_nucleo (M2), sugerencias_ccom (M3), reparaciones_ccom (M4)
y clones (M5). Recibe str para un documento o bytes para varias copias CCL1.
No valida JSON ni interpreta lenguaje: devuelve bytes, identidad y código.
Debe llamarse en un trabajador, nunca bloqueando el hilo gráfico. Todo estado
es local a la llamada; el finally mata/recoge el hijo antes de borrar temporales.
El comprobador M1 conserva QProcess, integrado en el bucle de eventos de Qt.
"""
from pathlib import Path
import subprocess
import tempfile
import time
from ide import ccom


class LimiteServicio(ValueError):
    """Tiempo o salida excedidos, distintos de una consulta sin opciones."""


def consultar(texto, argumentos, *, ejecutable=None, root=None, configurado='',
              cancelar=lambda: False, plazo=5, max_salida=16_000_000):
    """Devuelve (stdout, ruta de snapshot, código), siempre recogiendo el hijo.

    La ruta identifica la respuesta aunque el temporal ya esté eliminado.
    Las salidas van a archivos temporales para no bloquear pipes ni acumular
    memoria sin límite. argumentos es una lista interna, nunca una orden shell.
    """
    programa = ejecutable or ccom.ejecutable(root or Path(__file__).resolve().parents[1], configurado)
    if not programa:
        raise ValueError('ccom no disponible; ejecuta make o configura su ruta.')
    if cancelar():
        raise InterruptedError('Consulta cancelada')
    with tempfile.TemporaryDirectory(prefix='calside_service_') as carpeta:
        fuente = Path(carpeta) / ('documentos.ccl' if isinstance(texto, bytes) else 'documento.cls')
        if isinstance(texto, bytes):
            fuente.write_bytes(texto)
        else:
            fuente.write_text(texto, encoding='utf-8', newline='\n')
        with tempfile.TemporaryFile() as salida, tempfile.TemporaryFile() as errores:
            with subprocess.Popen([programa, *argumentos, str(fuente)], stdout=salida,
                                  stderr=errores, cwd=carpeta) as proceso:
                limite = time.monotonic() + plazo
                try:
                    # Ficheros temporales evitan llenar pipes: incluso si C escribe
                    # mucho, podemos observar tamaño, plazo y cancelación.
                    while proceso.poll() is None:
                        if cancelar():
                            raise InterruptedError('Consulta cancelada')
                        if time.monotonic() > limite:
                            raise LimiteServicio('Tiempo límite del servicio ccom alcanzado.')
                        if salida.tell() > max_salida or errores.tell() > 1_000_000:
                            raise LimiteServicio('La salida del servicio ccom supera el límite.')
                        time.sleep(.01)
                    if cancelar():
                        raise InterruptedError('Consulta cancelada')
                    salida.seek(0); datos = salida.read(max_salida + 1)
                    errores.seek(0); raw_log = errores.read(1_000_001)
                    if len(datos) > max_salida or len(raw_log) > 1_000_000:
                        raise LimiteServicio('La salida del servicio ccom supera el límite.')
                    if proceso.returncode not in (0, 1):
                        log = raw_log.decode('utf-8', errors='replace')
                        raise ValueError(f'ccom no completó {argumentos[0]}; ejecuta make o configura una versión compatible. ' + log[:1000])
                    return datos, fuente, proceso.returncode
                finally:
                    # Nunca dejar un hijo escribiendo en un directorio eliminado.
                    if proceso.poll() is None:
                        proceso.kill()
                    proceso.wait()
