"""Protección de disco y preferencias de la ventana; sin motores de asistencia."""
import os
from pathlib import Path

from PySide6.QtCore import QSettings, Qt
from PySide6.QtGui import QFont
from PySide6.QtWidgets import QFontDialog, QHBoxLayout, QLabel, QMessageBox, QPushButton, QWidget

from ide import archivos, preferencias
from ide.interfaz.disco import MonitorDisco


class ProyectoVentana:
    def iniciar_proyecto(self, settings):
        self.settings = QSettings('Calser', 'CALSIDE') if settings is None else settings
        self.datos_preferencias = preferencias.cargar(self.settings)
        self.fuente_editor = QFont('monospace', 12)
        self.monitor = MonitorDisco(self.solicitudes_disco, self)
        self.monitor.resultado.connect(self.recibir_disco)

    def crear_aviso_externo(self, layout):
        self.aviso_externo = QWidget()
        self.aviso_externo.setStyleSheet('background: #3b3025; color: #f3d4a4;')
        fila = QHBoxLayout(self.aviso_externo)
        fila.setContentsMargins(10, 5, 10, 5)
        self.texto_externo = QLabel()
        self.texto_externo.setWordWrap(True)
        self.texto_externo.setTextFormat(Qt.TextFormat.PlainText)
        fila.addWidget(self.texto_externo, 1)
        for texto, accion in [('Recargar', self.recargar), ('Conservar', self.conservar),
                              ('Guardar como…', lambda: self.guardar(como=True))]:
            boton = QPushButton(texto)
            boton.clicked.connect(accion)
            fila.addWidget(boton)
        self.aviso_externo.hide()
        layout.addWidget(self.aviso_externo)

    def solicitudes_disco(self):
        return [(e.documento_id, e.path, e.estado_disco) for e in
                (self.tabs.widget(i) for i in range(self.tabs.count())) if e.path]

    def recibir_disco(self, resultado):
        documento, path, base, estado, error = resultado
        editor = next((self.tabs.widget(i) for i in range(self.tabs.count())
                       if self.tabs.widget(i).documento_id == documento), None)
        if editor is None or editor.path != path or editor.estado_disco != base:
            return  # Resultado de antes de Guardar como, recargar o guardar.
        cambio_previo = (editor.estado_observado, editor.error_disco)
        editor.estado_observado, editor.error_disco = estado, error
        if self.tiene_cambio_externo(editor) and cambio_previo != (estado, error):
            self.clones_panel.invalidar(path)
            self.results.pop(editor, None)
            self.actualizar_problemas()
        self.actualizar_titulo(editor)

    def tiene_cambio_externo(self, editor):
        return bool(editor.path and (editor.error_disco or editor.estado_observado != editor.estado_disco))

    def actualizar_aviso_externo(self):
        editor = self.editor()
        visible = bool(editor and self.tiene_cambio_externo(editor) and
                       editor.cambio_reconocido != (editor.estado_observado, editor.error_disco))
        if visible:
            if editor.error_disco:
                texto = 'No se puede comprobar el archivo: ' + editor.error_disco
            elif not editor.estado_observado.existe:
                texto = 'El archivo fue borrado o movido fuera de CALSIDE. Tu texto sigue en el editor.'
            else:
                texto = 'El archivo cambió fuera de CALSIDE. Tu texto sigue en el editor.'
            self.texto_externo.setText(texto)
        self.aviso_externo.setVisible(visible)

    def establecer_version(self, editor, estado):
        editor.estado_disco = editor.estado_observado = estado
        editor.error_disco = ''
        editor.cambio_reconocido = None

    def conservar(self, checked=False):
        editor = self.editor()
        if editor and self.tiene_cambio_externo(editor):
            editor.cambio_reconocido = (editor.estado_observado, editor.error_disco)
            editor.document().setModified(True)
            self.actualizar_aviso_externo()
            self.statusBar().showMessage('Texto conservado · al guardar se comprobará el disco nuevamente')
            editor.setFocus()

    def recargar(self, checked=False, editor=None):
        editor = editor or self.editor()
        if not editor or editor.path is None:
            return False
        if editor.document().isModified():
            answer = QMessageBox.question(self, 'Recargar desde disco',
                'Se descartarán los cambios de esta pestaña para leer el archivo del disco. ¿Continuar?',
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No, QMessageBox.StandardButton.No)
            if answer != QMessageBox.StandardButton.Yes:
                return False
        try:
            texto, estado = archivos.cargar(editor.path)
        except (OSError, UnicodeError) as error:
            self.error('No se pudo recargar; tu texto se conserva', error)
            return False
        posicion = editor.textCursor().position()
        self.results.pop(editor, None)
        self.auto_results.pop(editor, None)
        self.comprobador.invalidar(editor.documento_id)
        self.establecer_version(editor, estado)
        editor.setPlainText(texto)
        editor.document().setModified(False)
        cursor = editor.textCursor()
        cursor.setPosition(min(posicion, editor.document().characterCount() - 1))
        editor.setTextCursor(cursor)
        self.actualizar_titulo(editor)
        self.actualizar_problemas()
        self.clones_panel.invalidar(editor.path)
        editor.programar_analisis()
        editor.setFocus()
        self.statusBar().showMessage('Archivo recargado desde disco')
        return True

    def resolver_conflicto(self, editor, estado):
        box = QMessageBox(self)
        box.setWindowTitle('El archivo cambió fuera de CALSIDE')
        box.setIcon(QMessageBox.Icon.Warning)
        box.setText('Guardar reemplazaría otra versión del archivo.' if estado.existe else
                    'El archivo fue borrado o movido. Guardar volverá a crearlo.')
        box.setInformativeText('Tu texto permanece en el editor. Elegí qué versión conservar.')
        sobrescribir = box.addButton('Sobrescribir' if estado.existe else 'Recrear archivo', QMessageBox.ButtonRole.DestructiveRole)
        recargar = box.addButton('Recargar disco', QMessageBox.ButtonRole.ActionRole) if estado.existe else None
        copia = box.addButton('Guardar como…', QMessageBox.ButtonRole.ActionRole)
        cancelar = box.addButton(QMessageBox.StandardButton.Cancel)
        box.setDefaultButton(cancelar)
        box.setEscapeButton(cancelar)
        box.exec()
        elegido = box.clickedButton()
        if elegido is sobrescribir:
            return 'sobrescribir'
        if recargar is not None and elegido is recargar:
            return 'recargar'
        return 'copia' if elegido is copia else 'cancelar'

    def restaurar_preferencias(self):
        d = self.datos_preferencias
        self.fuente_editor = QFont(d.get('familia') or 'monospace', d.get('puntos', 12))
        for i in range(self.tabs.count()):
            self.aplicar_fuente(self.tabs.widget(i))
        configurado = d.get('ccom', '')
        invalido = configurado and not (Path(configurado).is_file() and os.access(configurado, os.X_OK))
        self.configured_ccom = '' if invalido else configurado
        if d.get('panel_derecho', False):
            self.mover_panel()
        self.files_button.setChecked(d.get('explorador', True))
        self.results_action.setChecked(d.get('resultados', True))
        self.auto_action.setChecked(d.get('automatico', True))
        self.resize(d.get('ancho', 1250), d.get('alto', 800))
        if 'divisiones' in d:
            self.project_splitter.setSizes(d['divisiones'])
        if 'paneles' in d:
            self.workspace_splitter.setSizes(d['paneles'])
        proyecto = Path(d.get('proyecto') or self.root)
        self.cargar_carpeta(proyecto if proyecto.is_dir() else self.root)
        if invalido:
            self.statusBar().showMessage('La ruta guardada de ccom ya no está disponible; se usará la búsqueda habitual')

    def aplicar_fuente(self, editor):
        editor.setFont(self.fuente_editor)
        editor.setTabStopDistance(editor.fontMetrics().horizontalAdvance(' ') * 4)
        editor.actualizar_margen()

    def configurar_fuente(self):
        ok, fuente = QFontDialog.getFont(self.fuente_editor, self, 'Fuente del editor')
        if ok:
            fuente.setPointSize(max(6, min(40, fuente.pointSize())))
            self.fuente_editor = fuente
            for i in range(self.tabs.count()):
                self.aplicar_fuente(self.tabs.widget(i))

    def guardar_preferencias(self):
        if not preferencias.guardar(self.settings, {
            'version': 1, 'proyecto': str(self.project), 'ccom': self.configured_ccom,
            'familia': self.fuente_editor.family(), 'puntos': self.fuente_editor.pointSize(),
            'ancho': self.width(), 'alto': self.height(),
            'explorador': self.files_button.isChecked(), 'resultados': self.results_action.isChecked(),
            'panel_derecho': self.workspace_splitter.orientation() == Qt.Orientation.Horizontal,
            'automatico': self.auto_action.isChecked(),
            'divisiones': self.project_splitter.sizes(), 'paneles': self.workspace_splitter.sizes(),
        }):
            self.error('No se pudieron guardar las preferencias', 'Revisa los permisos de la carpeta de configuración.')
