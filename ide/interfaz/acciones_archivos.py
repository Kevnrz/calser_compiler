"""Acciones explícitas del explorador sobre archivos, con pestañas conservadas."""
from pathlib import Path
from ide.interfaz import papelera as servicio_papelera
import stat

from PySide6.QtCore import QDateTime, QFile, QMimeData, Qt, QUrl
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import QApplication, QInputDialog, QMenu, QMessageBox

from ide import archivos
from ide.interfaz.historial_archivos import HistorialArchivos
from ide.interfaz.operaciones_carpetas import OperacionCarpeta, dentro
from ide.interfaz.seleccion_archivos import SeleccionArchivos, archivos_portapapeles


def nombre_valido(nombre):
    return bool(nombre and nombre.strip() and nombre not in ('.', '..')
                and not any(c in nombre for c in ('/', '\\', '\x00', '\n', '\r')))


def archivo_portapapeles():
    mime = QApplication.clipboard().mimeData()
    if mime is None:
        return None
    urls = mime.urls()
    if len(urls) != 1 or not urls[0].isLocalFile():
        return None
    path = Path(urls[0].toLocalFile()).absolute()
    gnome = bytes(mime.data('x-special/gnome-copied-files')).splitlines()
    cortar = bytes(mime.data('application/x-kde-cutselection')) == b'1' or bool(gnome and gnome[0] == b'cut')
    return path, cortar


class AccionesArchivos(HistorialArchivos, SeleccionArchivos):
    def menu_archivo(self, path, posicion):
        menu = self.crear_menu_archivo(path)
        menu.exec(posicion)
        menu.deleteLater()

    def crear_menu_archivo(self, path):
        path = Path(path).absolute()
        seleccion = self.seleccion_para(path)
        if len(seleccion) > 1:
            return self.menu_seleccion(seleccion, path)
        archivo = path.is_file() and not path.is_symlink()
        directorio = path.is_dir() and not path.is_symlink()
        carpeta = path if directorio else path.parent
        ocupado = getattr(self, 'operacion_carpetas', None) is not None
        protegido = directorio and dentro(self.project, path)
        modificable = (archivo or directorio) and not protegido and not ocupado
        menu = QMenu(self)
        def accion(texto, callback, habilitada=True):
            a = menu.addAction(texto)
            a.setEnabled(habilitada)
            a.triggered.connect(lambda checked=False: callback())
            return a
        accion('Nuevo archivo…', lambda: self.crear_entrada(carpeta, False), not ocupado)
        accion('Nueva carpeta…', lambda: self.crear_entrada(carpeta, True), not ocupado)
        menu.addSeparator()
        if directorio:
            if path != self.project:
                accion('Expandir carpeta', lambda: self.tree.expandir_ruta(path))
                accion('Contraer carpeta', lambda: self.tree.contraer_ruta(path))
            accion('Expandir todo', self.tree.expandir_todo)
            accion('Contraer todo', self.tree.contraer_todo)
            accion('Restablecer orden alfabético', lambda: self.tree.orden_alfabetico(path))
        else:
            accion('Abrir', lambda: self.abrir(path), archivo and not self.ruta_ocupada(path))
        menu.addSeparator()
        accion('Renombrar…', lambda: self.renombrar_archivo(path), modificable)
        accion('Eliminar carpeta…' if directorio else 'Eliminar archivo…', lambda: self.eliminar_archivo(path), modificable)
        accion('Deshacer eliminación', self.deshacer_eliminacion, bool(getattr(self, '_eliminaciones', [])) and not ocupado)
        menu.addSeparator()
        accion('Copiar', lambda: self.copiar_archivo(path), (archivo or directorio) and not ocupado)
        accion('Cortar', lambda: self.copiar_archivo(path, cortar=True), modificable)
        contenido = archivos_portapapeles()
        pegable = bool(contenido and all(p.exists() and not p.is_symlink() for p in contenido[0]))
        accion('Pegar', lambda: self.pegar_archivo(carpeta), pegable and not ocupado)
        if ocupado:
            accion('Cancelar copia de carpeta', self.cancelar_copia_carpeta,
                   self.operacion_carpetas.tipo == 'copiar')
        menu.addSeparator()
        accion('Copiar ubicación', lambda: QApplication.clipboard().setText(str(path)))
        accion('Abrir ubicación', lambda: self.abrir_ubicacion(path))
        accion('Propiedades', lambda: self.propiedades_archivo(path))
        return menu

    def editores_de_archivo(self, path):
        return [self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).path is not None and dentro(self.tabs.widget(i).path, path)]

    def permitir_mover(self, path):
        if path.is_dir() and dentro(self.project, path):
            self.error('Carpeta protegida', 'La raíz del proyecto y sus ancestros no se pueden mover ni eliminar desde el explorador.')
            return False
        if self.ruta_ocupada(path):
            self.error('Operación en curso', 'Espera a que termine la operación de carpetas.')
            return False
        if self.run_process and self.run_info and dentro(self.run_info.fuente, path):
            self.error('Programa en ejecución', 'Cancela o espera la ejecución antes de mover o eliminar este archivo.')
            return False
        if self.process and self.compilation and dentro(self.compilation[2], path):
            self.error('Archivo en compilación', 'Cancela o espera la compilación antes de mover o eliminar este archivo.')
            return False
        return True

    def renombrar_archivo(self, path):
        nombre, ok = QInputDialog.getText(self, 'Renombrar carpeta' if path.is_dir() else 'Renombrar archivo', 'Nuevo nombre:', text=path.name)
        if not ok:
            return False
        if not nombre_valido(nombre):
            self.error('Nombre inválido', 'Usa un nombre de archivo sin rutas ni saltos de línea.')
            return False
        destino = path.with_name(nombre)
        if destino == path:
            return True
        return self.mover_archivo(path, destino)

    def mover_archivo(self, path, destino):
        path, destino = Path(path).absolute(), Path(destino).absolute()
        if not self.permitir_mover(path):
            return False
        if self.editores_de_archivo(destino):
            self.error('Archivo abierto', 'El destino está abierto en otra pestaña. Elegí otro nombre.')
            return False
        if path.is_dir() and not path.is_symlink():
            return self.iniciar_operacion_carpeta('mover', path, destino)
        try:
            if path.is_symlink() or not path.is_file():
                raise OSError('Selecciona un archivo regular existente.')
            if destino.exists() or destino.is_symlink():
                raise FileExistsError('Ya existe ese destino; no se reemplazó ningún archivo.')
            version = archivos.estado(path)
            file = QFile(str(path))
            if not file.rename(str(destino)):
                raise OSError(file.errorString())
        except OSError as error:
            self.error('No se pudo mover o renombrar', error)
            return False
        try:
            actual = archivos.estado(destino)
            error_lectura = ''
        except OSError as error:
            actual, error_lectura = None, str(error)
        self.actualizar_traslado(path, destino, {path: version}, {path: (actual, error_lectura)})
        return True

    def actualizar_traslado(self, path, destino, antes, despues):
        for editor in self.editores_de_archivo(path):
            viejo = editor.path
            actual, error_lectura = despues.get(viejo, (None, 'No se pudo comprobar la nueva ubicación'))
            self.comprobador.invalidar(editor.documento_id)
            self.analizador.olvidar(editor.documento_id)
            self.results.pop(editor, None)
            self.auto_results.pop(editor, None)
            if antes.get(viejo) is not None and editor.estado_disco == antes[viejo] and actual is not None:
                editor.estado_disco = actual
            editor.path = destino / viejo.relative_to(path)
            editor.estado_observado, editor.error_disco = actual, error_lectura
            editor.cambio_reconocido = None
            editor.establecer_problemas([])
            editor.estado_ccom = 'Esperando léxico…' if self.auto_action.isChecked() else 'Desactivado'
            editor.programar_analisis()
            self.actualizar_titulo(editor)
        self.tree.trasladar_ramas(path, destino)
        self.clones_panel.invalidar(path)
        self.clones_panel.invalidar(destino)
        self.actualizar_problemas()
        self.tree.revelar_ruta(destino)
        self.busqueda_proyecto.actualizar_si_activa()
        self.statusBar().showMessage(f'Trasladado · {destino}')

    def copiar_archivo(self, path, cortar=False):
        path = Path(path).absolute()
        if path.is_symlink() or not (path.is_file() or path.is_dir()):
            self.error('No se pudo copiar', 'Selecciona un archivo o carpeta existente.')
            return False
        if cortar and not self.permitir_mover(path):
            return False
        mime = QMimeData()
        url = QUrl.fromLocalFile(str(path))
        mime.setUrls([url])
        mime.setText(str(path))
        mime.setData('application/x-kde-cutselection', b'1' if cortar else b'0')
        mime.setData('x-special/gnome-copied-files', ('cut\n' if cortar else 'copy\n').encode() + url.toEncoded().data())
        QApplication.clipboard().setMimeData(mime)
        self.statusBar().showMessage(('Cortado' if cortar else 'Copiado') + ' · se usará la versión guardada en disco. Elegí la carpeta y Pegar.')
        return True

    def pegar_archivo(self, carpeta):
        if getattr(self, 'operacion_carpetas', None) is not None:
            self.error('Operación en curso', 'Espera o cancela la copia de carpeta actual.')
            return False
        plural = archivos_portapapeles()
        if plural and len(plural[0]) > 1:
            return self.procesar_seleccion(plural[0], carpeta, 'mover' if plural[1] else 'copiar',
                                          portapapeles=plural if plural[1] else None)
        contenido = archivo_portapapeles()
        if contenido is None:
            self.error('No se pudo pegar', 'El portapapeles debe contener un único archivo o carpeta local.')
            return False
        origen, cortar = contenido
        destino = Path(carpeta).absolute() / origen.name
        if cortar and destino == origen:
            self.statusBar().showMessage('El archivo ya está en esa carpeta')
            return False
        if destino.exists() or destino.is_symlink():
            nombre, ok = QInputDialog.getText(self, 'Elegir otro nombre', 'El destino ya existe. Nombre para la copia o traslado:',
                                            text=f'{origen.stem} - copia{origen.suffix}')
            if not ok:
                return False
            if not nombre_valido(nombre):
                self.error('Nombre inválido', 'Usa un nombre de archivo sin rutas ni saltos de línea.')
                return False
            destino = destino.with_name(nombre)
        if origen.is_dir() and dentro(destino.resolve(), origen.resolve()):
            self.error('Destino inválido', 'Una carpeta no puede copiarse o moverse dentro de sí misma ni de sus descendientes.')
            return False
        if cortar:
            if not self.mover_archivo(origen, destino):
                return False
            if getattr(self, 'operacion_carpetas', None) is not None:
                self.operacion_carpetas.portapapeles_cortado = contenido
                return True
            if archivo_portapapeles() == contenido:
                QApplication.clipboard().clear()
            return True
        if origen.is_dir() and not origen.is_symlink():
            return self.iniciar_operacion_carpeta('copiar', origen, destino)
        try:
            if origen.is_symlink() or not origen.is_file():
                raise OSError('El archivo del portapapeles ya no está disponible.')
            if self.editores_de_archivo(destino):
                raise OSError('El destino está abierto en otra pestaña; elegí otro nombre.')
            file = QFile(str(origen))
            if not file.copy(str(destino)):
                raise OSError(file.errorString())
        except OSError as error:
            self.error('No se pudo pegar; no se reemplazó el destino', error)
            return False
        self.clones_panel.invalidar(destino)
        self.statusBar().showMessage(f'Archivo copiado · {destino}')
        return True

    def eliminar_archivo(self, path):
        path = Path(path).absolute()
        if not self.permitir_mover(path):
            return False
        if path.is_symlink() or not (path.is_file() or path.is_dir()):
            self.error('No se pudo eliminar', 'Selecciona un archivo o carpeta existente.')
            return False
        box = QMessageBox(self)
        box.setWindowTitle('Enviar a la papelera')
        box.setTextFormat(Qt.TextFormat.PlainText)
        box.setText(f'¿Enviar a la papelera {"esta carpeta y todo su contenido" if path.is_dir() else "este archivo"}?\n{path}')
        box.setInformativeText('Si está abierto, su texto seguirá en la pestaña como cambios sin guardar. No se eliminará permanentemente si falla la papelera.')
        box.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        box.setDefaultButton(QMessageBox.StandardButton.No)
        if box.exec() != QMessageBox.StandardButton.Yes:
            return False
        if path.is_dir():
            return self.iniciar_operacion_carpeta('papelera', path)
        registro = self.preparar_eliminacion(path)
        ok, papelera = servicio_papelera.enviar_a_papelera(path)
        if not ok:
            self.error('No se pudo enviar a la papelera', 'El archivo se conserva. Podés usar el gestor de archivos del sistema.')
            return False
        self.marcar_eliminados(path)
        self.registrar_eliminacion(registro, papelera)
        return True

    def marcar_eliminados(self, path):
        for editor in self.editores_de_archivo(path):
            self.results.pop(editor, None)
            editor.estado_observado = archivos.AUSENTE
            editor.error_disco = ''
            editor.cambio_reconocido = None
            editor.document().setModified(True)
            self.actualizar_titulo(editor)
        self.clones_panel.invalidar(path)
        self.actualizar_problemas()
        self.busqueda_proyecto.actualizar_si_activa()
        self.statusBar().showMessage('Enviado a la papelera · los buffers abiertos se conservan')

    def abrir_ubicacion(self, path):
        carpeta = path if path.is_dir() else path.parent
        if not QDesktopServices.openUrl(QUrl.fromLocalFile(str(carpeta))):
            self.error('No se pudo abrir la ubicación', 'No hay un gestor de archivos disponible.')

    def propiedades_archivo(self, path):
        try:
            datos = path.stat()
            carpeta = path.is_dir()
            informacion = (f'Nombre: {path.name}\nUbicación: {path}\n'
                f'Tipo: {"Carpeta" if carpeta else "Archivo"}\n'
                f'Tamaño: {"No calculado para carpetas" if carpeta else str(datos.st_size) + " bytes"}\n'
                f'Modificado: {QDateTime.fromSecsSinceEpoch(int(datos.st_mtime)).toString("yyyy-MM-dd HH:mm:ss t")}\n'
                f'Permisos: {stat.filemode(datos.st_mode)}')
        except OSError as error:
            self.error('No se pudieron leer las propiedades', error)
            return
        box = QMessageBox(self)
        box.setWindowTitle('Propiedades')
        box.setTextFormat(Qt.TextFormat.PlainText)
        box.setText(informacion)
        box.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        box.exec()

    def ruta_ocupada(self, path):
        tarea = getattr(self, 'operacion_carpetas', None)
        return bool(tarea and any(dentro(path, base) or dentro(base, path)
                    for base in (tarea.origen, tarea.destino) if base is not None))

    def iniciar_operacion_carpeta(self, tipo, origen, destino=None):
        if getattr(self, 'operacion_carpetas', None) is not None:
            self.error('Operación en curso', 'Solo se realiza una operación de carpeta a la vez.')
            return False
        if origen.is_symlink() or not origen.is_dir():
            self.error('Carpeta no disponible', str(origen))
            return False
        if destino is not None:
            if destino.exists() or destino.is_symlink() or self.editores_de_archivo(destino):
                self.error('Destino ocupado', 'Elegí otro nombre; no se sobrescribirán archivos ni pestañas.')
                return False
            if dentro(destino.resolve(), origen.resolve()):
                self.error('Destino inválido', 'No se puede colocar una carpeta dentro de sí misma.')
                return False
        tarea = OperacionCarpeta(tipo, origen, destino,
                    [e.path for e in self.editores_de_archivo(origen)], self)
        tarea.eliminacion = self.preparar_eliminacion(origen) if tipo == 'papelera' else None
        tarea.portapapeles_cortado = None
        tarea.ramas_antes = set(self.tree.ramas)
        self.operacion_carpetas = tarea
        tarea.resultado.connect(self.resultado_carpeta, Qt.ConnectionType.QueuedConnection)
        tarea.progreso.connect(self.progreso_carpeta, Qt.ConnectionType.QueuedConnection)
        tarea.finished.connect(self.liberar_operacion_carpeta, Qt.ConnectionType.QueuedConnection)
        self.cancelar_carpeta_action.setEnabled(tipo == 'copiar')
        self.statusBar().showMessage(f'Operación de carpeta en curso · {tipo} · {origen}')
        tarea.start()
        return True

    def progreso_carpeta(self, mensaje):
        self.statusBar().showMessage(mensaje)

    def cancelar_copia_carpeta(self):
        tarea = getattr(self, 'operacion_carpetas', None)
        if tarea is not None and tarea.tipo == 'copiar':
            tarea.requestInterruption()
            self.statusBar().showMessage('Cancelando copia; se conservará cualquier destino parcial')

    def resultado_carpeta(self, resultado):
        tarea = self.operacion_carpetas
        if getattr(tarea, 'lote', False):
            lote = self._lote_archivos
            lote['fallo'] = not resultado.exito
            if not resultado.exito:
                lote['detalle'] = resultado.error + (f' · Copia incompleta conservada en {resultado.destino}' if resultado.parcial else '')
            lote['hechos'] += int(resultado.exito)
        if resultado.exito:
            if resultado.tipo == 'mover':
                self.actualizar_traslado(resultado.origen, resultado.destino, resultado.antes, resultado.despues)
                self.tree.trasladar_ramas(resultado.origen, resultado.destino, tarea.ramas_antes)
                if tarea.portapapeles_cortado and archivo_portapapeles() == tarea.portapapeles_cortado:
                    QApplication.clipboard().clear()
            elif resultado.tipo == 'papelera':
                self.marcar_eliminados(resultado.origen)
                self.registrar_eliminacion(tarea.eliminacion, resultado.destino)
            else:
                self.clones_panel.invalidar(resultado.destino)
                self.tree.revelar_ruta(resultado.destino)
                self.statusBar().showMessage(f'Carpeta copiada · {resultado.destino}')
        else:
            mensaje = resultado.error
            if resultado.parcial:
                mensaje += f'\nCopia incompleta conservada en: {resultado.destino}. El origen se conserva.'
                self.clones_panel.invalidar(resultado.destino)
            if resultado.cancelado:
                self.statusBar().showMessage(mensaje)
            else:
                self.error('No se completó la operación de carpeta', mensaje)

    def liberar_operacion_carpeta(self):
        tarea, self.operacion_carpetas = self.operacion_carpetas, None
        self.cancelar_carpeta_action.setEnabled(False)
        tarea.deleteLater()
        self.busqueda_proyecto.actualizar_si_activa()
        if getattr(tarea, 'lote', False):
            self._continuar_seleccion()

    def crear_entrada(self, carpeta, es_carpeta):
        if self.ruta_ocupada(carpeta):
            self.error('Operación en curso', 'Espera a que termine la operación en esa carpeta.')
            return False
        nombre, ok = QInputDialog.getText(self, 'Nueva carpeta' if es_carpeta else 'Nuevo archivo',
                      'Nombre:', text='carpeta' if es_carpeta else 'programa.cls')
        if not ok:
            return False
        if not nombre_valido(nombre):
            self.error('Nombre inválido', 'Usa un nombre sin rutas ni saltos de línea.')
            return False
        path = Path(carpeta) / nombre
        if not es_carpeta and not path.suffix:
            path = path.with_suffix('.cls')
        try:
            if self.editores_de_archivo(path):
                raise OSError('Esa ubicación está abierta en una pestaña; elegí otro nombre.')
            if es_carpeta:
                path.mkdir()
            else:
                with path.open('x', encoding='utf-8'):
                    pass
        except OSError as error:
            self.error('No se pudo crear; no se reemplazó ningún archivo', error)
            return False
        self.tree.expandir_ruta(Path(carpeta))
        self.tree.revelar_ruta(path)
        self.clones_panel.invalidar(path)
        if not es_carpeta:
            self.abrir(path)
        self.busqueda_proyecto.actualizar_si_activa()
        self.statusBar().showMessage(f'Creado · {path}')
        return True

    def atajo_explorador(self, accion, path):
        seleccion = self.seleccion_para(path)
        if len(seleccion) > 1:
            if accion in ('copiar', 'cortar'):
                self.copiar_seleccion(seleccion, accion == 'cortar')
                return
            if accion == 'eliminar':
                self.eliminar_seleccion(seleccion)
                return
            if accion in ('renombrar', 'propiedades'):
                self.statusBar().showMessage('Selecciona una sola entrada para renombrar o ver sus propiedades')
                return
        acciones = {
            'copiar': lambda: self.copiar_archivo(path),
            'cortar': lambda: self.copiar_archivo(path, cortar=True),
            'pegar': lambda: self.pegar_archivo(path if path.is_dir() else path.parent),
            'renombrar': lambda: self.renombrar_archivo(path),
            'eliminar': lambda: self.eliminar_archivo(path),
            'carpeta': lambda: self.crear_entrada(path if path.is_dir() else path.parent, True),
            'propiedades': lambda: self.propiedades_archivo(path),
        }
        acciones[accion]()

    def revelar_archivo_activo(self):
        editor = self.editor()
        if editor and editor.path and dentro(editor.path, self.project):
            self.busqueda_proyecto.campo.clear()
            self.files_button.setChecked(True)
            self.tree.revelar_ruta(editor.path)
            self.tree.setFocus()
        else:
            self.statusBar().showMessage('El archivo activo no tiene ruta dentro de este proyecto')
