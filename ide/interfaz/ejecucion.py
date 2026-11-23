"""Ejecución explícita de una compilación vigente mediante el simulador del repo."""
from dataclasses import dataclass
from pathlib import Path
import codecs
import shutil
import sys
import tempfile

from PySide6.QtCore import QProcess, QTimer
from PySide6.QtGui import QTextCursor
from ide import archivos


SIMULADORES = {'sin_cache': 'Sin caché', '0': 'Caché: solo memoria principal',
               '1': 'Caché: L1', '2': 'Caché: L1 + L2'}
LATENCIAS = {2: 39, 4: 53, 5: 60}


def argumentos_simulador(root, asm, memoria, traza, configuracion):
    """Construye la misma invocación de F6, agregando caché solo si se eligió."""
    cache = configuracion['simulador']
    script = 'sim_asm.py' if cache == 'sin_cache' else 'sim_cache.py'
    argumentos = [str(Path(root) / 'tools' / script), str(asm), '--data-image', str(memoria),
                  '--traza', str(traza), '--max-bundles', '100000', '--traza-max', '5000']
    if cache != 'sin_cache':
        argumentos += ['--cache', cache, '--mem-div', str(configuracion['mem_div'])]
    return argumentos


@dataclass(frozen=True)
class CompilacionEjecutable:
    fuente: Path
    revision: int
    estado_fuente: object
    asm: Path
    memoria: Path
    estado_asm: object
    estado_memoria: object

    def vigente(self, editor):
        try:
            return (editor.path == self.fuente and editor.revision == self.revision
                    and not editor.document().isModified()
                    and archivos.estado(self.fuente) == self.estado_fuente
                    and archivos.estado(self.asm) == self.estado_asm
                    and archivos.estado(self.memoria) == self.estado_memoria)
        except OSError:
            return False


class EjecucionVentana:
    def iniciar_ejecucion(self):
        self.compilados = {}
        self.run_process = None
        self.run_info = None
        self.run_timer = QTimer(self)
        self.run_timer.setSingleShot(True)
        self.run_timer.timeout.connect(self.agotar_ejecucion)

    def registrar_compilacion_ejecutable(self, editor):
        asm = self.compilation_output.with_suffix('.asm')
        memoria = self.compilation_output.with_suffix('.mem')
        try:
            a, m = archivos.estado(asm), archivos.estado(memoria)
            if not a.existe or not m.existe:
                raise OSError('ccom no generó el listado .asm y la imagen .mem necesarios.')
            self.compilados[editor] = CompilacionEjecutable(editor.path, editor.revision,
                        self.compilation_disk, asm, memoria, a, m)
        except OSError as error:
            self.escribir_salida(f'No disponible para ejecutar: {error}\n')

    def ejecutar(self):
        if self.process or self.run_process or self.metricas_process:
            return False
        editor = self.editor()
        info = self.compilados.get(editor)
        if info is None or not info.vigente(editor):
            self.error('Compila antes de ejecutar', 'Selecciona Compilar programa con listado .asm y usa F5 para generar una compilación correcta del documento actual. Luego usa Ejecutar (F6). No se ejecutará una versión anterior.')
            return False
        if any(self.ruta_ocupada(p) for p in (info.fuente, info.asm, info.memoria)):
            self.error('Operación en curso', 'Espera a que termine la operación en los archivos del programa.')
            return False
        simulador = Path(argumentos_simulador(self.root, info.asm, info.memoria,
                                             'traza.txt', self.configuracion_simulacion)[0])
        if not simulador.is_file():
            self.error('Simulador no disponible', f'No se encontró {simulador}')
            return False
        try:
            carpeta = Path(tempfile.mkdtemp(prefix='ejecucion-', dir=info.asm.parent))
            entradas = []
            for original, esperado in ((info.asm, info.estado_asm), (info.memoria, info.estado_memoria)):
                copia = carpeta / original.name
                shutil.copyfile(original, copia)
                copiado = archivos.estado(copia)
                if copiado.huella != esperado.huella or copiado.tamano != esperado.tamano:
                    raise OSError('Los artefactos cambiaron al preparar la ejecución. Vuelve a compilar.')
                entradas.append(copia)
        except OSError as error:
            self.error('No se pudo preparar la ejecución', error)
            return False
        self.run_trace = carpeta / 'traza.txt'
        self.run_info, self.run_editor = info, editor
        self.run_cancelado = self.run_timeout = False
        self.run_decoder = codecs.getincrementaldecoder('utf-8')(errors='replace')
        argumentos = argumentos_simulador(self.root, entradas[0], entradas[1],
                                          self.run_trace, self.configuracion_simulacion)
        configuracion = SIMULADORES[self.configuracion_simulacion['simulador']]
        if self.configuracion_simulacion['simulador'] != 'sin_cache':
            divisor = self.configuracion_simulacion['mem_div']
            configuracion += f' · MEM_DIV={divisor} ({LATENCIAS[divisor]} ciclos)'
        self.run_output.setPlainText(f'Ejecutando · {info.fuente.name}\n'
                                    f'Simulador: {configuracion}\n'
                                    f'Comando: {sys.executable} {argumentos}\n\n')
        self.results_action.setChecked(True)
        self.panels.setCurrentWidget(self.run_output)
        proceso = QProcess(self)
        self.run_process = proceso
        proceso.setWorkingDirectory(str(info.fuente.parent))
        proceso.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        proceso.readyReadStandardOutput.connect(self.leer_ejecucion)
        proceso.finished.connect(self.terminar_ejecucion)
        proceso.errorOccurred.connect(self.error_ejecucion)
        self.controles_ejecucion(True)
        self.comprobador.suspender(True)
        self.statusBar().showMessage('Ejecutando… puedes seguir editando · Cancelar detiene el programa')
        self.run_timer.start(30000)
        proceso.start(sys.executable, argumentos)
        return True

    def controles_ejecucion(self, activa):
        self.metricas_action.setEnabled(not activa)
        self.simulador_menu.setEnabled(not activa)
        self.latencia_menu.setEnabled(not activa and self.configuracion_simulacion['simulador'] != 'sin_cache')
        self.opciones_compilacion_action.setEnabled(not activa)
        for accion in self.acciones_modos.values():
            accion.setEnabled(not activa)
        self.compile_button.setEnabled(not activa)
        self.compile_action.setEnabled(not activa)
        self.run_button.setEnabled(not activa)
        self.run_action.setEnabled(not activa)
        self.stop_button.setEnabled(activa)

    def escribir_ejecucion(self, texto):
        cursor = self.run_output.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(texto)
        self.run_output.setTextCursor(cursor)

    def leer_ejecucion(self):
        if self.run_process:
            self.escribir_ejecucion(self.run_decoder.decode(bytes(self.run_process.readAllStandardOutput())))

    def terminar_ejecucion(self, codigo, estado):
        if self.run_process is None:
            return
        self.leer_ejecucion()
        self.escribir_ejecucion(self.run_decoder.decode(b'', final=True))
        try:
            with self.run_trace.open('rb') as fuente:
                datos = fuente.read(2_000_001)
            self.escribir_ejecucion('\n' + datos[:2_000_000].decode('utf-8', errors='replace'))
            if len(datos) > 2_000_000:
                self.escribir_ejecucion('\nVista de traza truncada a 2 MB.\n')
            self.escribir_ejecucion(f'\nTraza guardada: {self.run_trace}\n')
        except OSError:
            self.escribir_ejecucion('\nNo se generó una traza disponible para esta ejecución.\n')
        if self.run_timeout:
            mensaje = 'Ejecución detenida · límite de 30 segundos'
        elif self.run_cancelado:
            mensaje = 'Ejecución cancelada'
        elif estado == QProcess.ExitStatus.CrashExit:
            mensaje = 'El simulador terminó inesperadamente'
        elif codigo == 0:
            mensaje = 'Ejecución terminada correctamente'
        elif codigo == 1:
            mensaje = 'Ejecución con avisos o límite de bundles · revisa la salida'
        else:
            mensaje = f'Ejecución fallida · código {codigo}'
        if self.tabs.indexOf(self.run_editor) < 0 or not self.run_info.vigente(self.run_editor):
            mensaje += ' · corresponde a la versión compilada anterior'
        self.escribir_ejecucion(f'\n{mensaje}\n')
        self.statusBar().showMessage(mensaje)
        self.liberar_ejecucion()

    def error_ejecucion(self, error):
        if self.run_process and error == QProcess.ProcessError.FailedToStart:
            self.escribir_ejecucion(f'\nNo se pudo iniciar el simulador: {self.run_process.errorString()}\n')
            self.statusBar().showMessage('No se pudo iniciar el simulador')
            self.liberar_ejecucion()

    def liberar_ejecucion(self):
        self.run_timer.stop()
        proceso, self.run_process = self.run_process, None
        if proceso:
            proceso.deleteLater()
        self.run_info = None
        self.controles_ejecucion(False)
        self.comprobador.suspender(False)

    def cancelar_ejecucion(self):
        if self.run_process:
            self.run_cancelado = True
            self.run_process.kill()

    def agotar_ejecucion(self):
        if self.run_process:
            self.run_timeout = True
            self.run_process.kill()
