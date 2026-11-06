"""Lista manual de sugerencias junto al cursor; sin motor provisional de producción."""
import weakref
from PySide6.QtCore import QEvent, QObject, QPoint, Qt, QTimer, Signal
from PySide6.QtWidgets import QApplication, QFrame, QLabel, QListWidget, QListWidgetItem, QVBoxLayout, QWidget
from shiboken6 import isValid
from ide.nucleo.asistencia import EstadoAsistencia
from ide.politica_asistencia import REGLAS_AUTOMATICAS, ReglaAutomatica, validar_preferencias
from ide.interfaz.asistencia import CoordinadorAsistencia
from ide.interfaz.ediciones import aplicar_reparacion, aplicar_sugerencia


class ListaSugerencias(QFrame):
    elegida = Signal(object)

    def __init__(self, parent):
        super().__init__(parent, Qt.WindowType.ToolTip | Qt.WindowType.FramelessWindowHint)
        self.setObjectName('listaSugerencias')
        self.setAttribute(Qt.WidgetAttribute.WA_ShowWithoutActivating)
        self.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.setStyleSheet('''QFrame#listaSugerencias { background: #222631; border: 1px solid #98703a; border-radius: 5px; }
            QListWidget { background: #222631; border: none; outline: none; }
            QListWidget::item { padding: 6px; color: #dce2ec; }
            QListWidget::item:selected { background: #504135; color: #ffcf7c; }
            QLabel { background: transparent; border: none; color: #b7c1cf; }''')
        layout = QVBoxLayout(self); layout.setContentsMargins(7, 7, 7, 7); layout.setSpacing(4)
        self.titulo = QLabel('Sugerencias'); layout.addWidget(self.titulo)
        self.lista = QListWidget(); self.lista.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.lista.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.lista.setMouseTracking(True)
        layout.addWidget(self.lista)
        self.detalle = QLabel(); self.detalle.setTextFormat(Qt.TextFormat.PlainText)
        self.detalle.setWordWrap(True); self.detalle.setMaximumHeight(76); layout.addWidget(self.detalle)
        self.ayuda = QLabel('↑↓ elegir · Enter/Tab aceptar · Esc cancelar')
        self.ayuda.setStyleSheet('font-size: 10px; color: #929dad;'); layout.addWidget(self.ayuda)
        self.lista.currentItemChanged.connect(self.cambiar_detalle)
        self.lista.itemClicked.connect(lambda _: self.aceptar())

    def mostrar(self, opciones, editor):
        self.lista.clear()
        for opcion in opciones:
            item = QListWidgetItem(f'{opcion.etiqueta}   · {opcion.categoria}')
            item.setData(Qt.ItemDataRole.UserRole, opcion)
            item.setToolTip(opcion.detalle)
            self.lista.addItem(item)
        self.lista.setCurrentRow(0)
        self.lista.setFixedHeight(min(8, len(opciones)) * max(28, self.lista.sizeHintForRow(0)) + 4)
        pantalla = editor.screen().availableGeometry().adjusted(8, 8, -8, -8)
        self.setFixedWidth(min(430, max(100, pantalla.width() - 16)))
        self.adjustSize()
        rect = editor.cursorRect()
        punto = editor.viewport().mapToGlobal(QPoint(rect.left(), rect.bottom() + 4))
        x = max(pantalla.left(), min(punto.x(), pantalla.right() - self.width() + 1))
        y = punto.y()
        if y + self.height() > pantalla.bottom():
            y = editor.viewport().mapToGlobal(rect.topLeft()).y() - self.height() - 4
        y = max(pantalla.top(), min(y, pantalla.bottom() - self.height() + 1))
        self.move(x, y); self.show(); self.raise_()

    def cambiar_detalle(self, item, anterior=None):
        texto = item.data(Qt.ItemDataRole.UserRole).detalle if item else ''
        self.detalle.setText(texto)
        self.detalle.setToolTip(texto)
        self.detalle.setVisible(bool(texto))

    def desplazar(self, cantidad):
        self.lista.setCurrentRow(max(0, min(self.lista.count() - 1, self.lista.currentRow() + cantidad)))

    def aceptar(self):
        item = self.lista.currentItem()
        if item:
            self.elegida.emit(item.data(Qt.ItemDataRole.UserRole))


class AsistenciaEditor(QObject):
    reparacionesRecibidas = Signal(object)
    contextoInvalidado = Signal(str)

    def __init__(self, ventana):
        super().__init__(ventana)
        self.ventana = ventana
        self.coordinador = CoordinadorAsistencia(self)
        self.coordinador.resultado.connect(self.recibir)
        self.popup = ListaSugerencias(ventana)
        self.popup.elegida.connect(self.aceptar)
        self.editores = weakref.WeakValueDictionary()
        self.contexto_visible = None
        self.actual = None
        self.reglas = REGLAS_AUTOMATICAS
        self.configuracion = validar_preferencias(ventana.datos_preferencias.get('asistencia'))
        self.programada = None
        self.timer_pausa = QTimer(self); self.timer_pausa.setSingleShot(True)
        self.timer_pausa.timeout.connect(self.consultar_por_pausa)
        self.configurar(self.configuracion)
        ventana.tabs.currentChanged.connect(self.cambiar_editor)

    def conectar_editor(self, editor):
        documento = editor.documento_id
        self.editores[documento] = editor
        editor.installEventFilter(self)
        editor.entradaAsistencia.connect(lambda origen: self.programar(editor, origen))
        editor.contenidoCambiado.connect(lambda: self.invalidar(documento))
        editor.cursorPositionChanged.connect(lambda: self.invalidar(documento))
        editor.selectionChanged.connect(lambda: self.invalidar(documento))
        editor.verticalScrollBar().valueChanged.connect(lambda _: self.invalidar(documento, tipo='sugerencias'))
        editor.horizontalScrollBar().valueChanged.connect(lambda _: self.invalidar(documento, tipo='sugerencias'))
        editor.destroyed.connect(lambda: self.olvidar(documento))

    def establecer_motor(self, tipo, motor):
        self.cancelar_programada()
        self.cerrar_lista()
        self.coordinador.establecer_motor(tipo, motor)
        if tipo == 'reparaciones' and self.actual:
            self.contextoInvalidado.emit(self.actual)

    def configurar(self, datos):
        self.configuracion = validar_preferencias(datos)
        self.cancelar_programada()
        self.coordinador.invalidar()
        self.cerrar_lista()
        if self.actual:
            self.contextoInvalidado.emit(self.actual)
        self.ventana.sugerencias_action.setEnabled(self.configuracion['habilitada'])
        self.ventana.reparaciones_action.setEnabled(self.configuracion['habilitada'])

    def establecer_reglas(self, reglas):
        reglas = tuple(reglas)
        if (len(reglas) > 64 or any(not isinstance(r, ReglaAutomatica) for r in reglas)
                or len({r.codigo for r in reglas}) != len(reglas)):
            raise ValueError('Catálogo de reglas automáticas inválido')
        self.reglas = reglas
        self.configurar(self.configuracion)

    def cancelar_programada(self):
        self.timer_pausa.stop(); self.programada = None

    def cancelar_automaticas(self, documento):
        if self.programada and self.programada[0].documento == documento:
            self.cancelar_programada()
        solicitudes = list(self.coordinador.pendientes.values())
        if self.coordinador.task:
            solicitudes.append(self.coordinador.task.solicitud)
        for solicitud in solicitudes:
            if solicitud.contexto.documento == documento and solicitud.origen != 'manual':
                self.coordinador.invalidar(documento, tipo=solicitud.tipo)

    def disponible_automatico(self, editor):
        return bool(self.configuracion['habilitada'] and editor and isValid(editor)
            and editor is self.ventana.editor() and editor.hasFocus() and self.ventana.isActiveWindow()
            and not editor.isReadOnly()
            and not editor.textCursor().hasSelection() and not self.ventana.process
            and not self.ventana.run_process and not self.ventana.tiene_cambio_externo(editor))

    def programar(self, editor, origen):
        self.cancelar_programada()
        if not self.disponible_automatico(editor):
            return
        d = self.configuracion
        reglas = tuple(r for r in self.reglas if r.codigo in d['reglas_automaticas'])
        if origen == 'linea_confirmada' and d['reparaciones_automaticas'] and reglas:
            tipo = 'reparaciones'
        elif origen == 'escritura' and d['sugerencias_pausa']:
            tipo, reglas = 'sugerencias', ()
        else:
            return
        self.programada = (editor.contexto_asistencia(), tipo, origen, reglas)
        self.timer_pausa.start(d['pausa_ms'])

    def consultar_por_pausa(self):
        programada, self.programada = self.programada, None
        if programada is None:
            return
        contexto, tipo, origen, reglas = programada
        editor = self.editores.get(contexto.documento)
        if not self.disponible_automatico(editor) or editor.contexto_asistencia() != contexto:
            return
        self.coordinador.solicitar(tipo, contexto, origen=origen, reglas=reglas)

    def recibir_automatica(self, respuesta, editor):
        if not self.disponible_automatico(editor):
            return
        r = respuesta.automatica
        if r is None:
            if respuesta.resultado.estado == EstadoAsistencia.OK:
                panel = self.ventana.reparaciones_panel
                panel.iniciar(respuesta.solicitud.contexto, editor.path.name if editor.path else 'Sin título')
                panel.recibir(respuesta)
                panel.estado.setText('Requiere revisión manual: ' + respuesta.motivo_automatico)
                self.ventana.statusBar().showMessage('Reparación requiere revisión manual · Alt+Enter', 6000)
            elif respuesta.resultado.explicacion:
                self.ventana.statusBar().showMessage(respuesta.resultado.explicacion, 6000)
            return
        # La revisión completa del parser ocurrió en el trabajador. Aquí se
        # comprueban otra vez permisos, regla habilitada y snapshot antes de editar.
        d = self.configuracion
        if (not d['reparaciones_automaticas'] or r.codigo not in d['reglas_automaticas']
                or not any(regla.codigo == r.codigo for regla in self.reglas)):
            return
        contexto = respuesta.solicitud.contexto
        self.invalidar(contexto.documento)
        resultado = aplicar_reparacion(editor, contexto, r)
        if resultado.aplicada:
            panel = self.ventana.reparaciones_panel
            panel.iniciar(contexto, editor.path.name if editor.path else 'Sin título')
            panel.recibir(respuesta); panel.invalidar(contexto.documento)
            panel.estado.setText('Reparación automática aplicada · Ctrl+Z deshace · sin guardar')
            panel.explicacion.appendPlainText('\nComprobación del IDE: ' + respuesta.motivo_automatico)
            self.ventana.statusBar().showMessage('Reparación automática: ' + r.descripcion + ' · Ctrl+Z deshace', 8000)

    def cerrar_lista(self):
        QApplication.instance().removeEventFilter(self)
        self.popup.hide(); self.contexto_visible = None

    def invalidar(self, documento, *, tipo=None):
        if self.programada and self.programada[0].documento == documento and (tipo is None or self.programada[1] == tipo):
            self.cancelar_programada()
        self.coordinador.invalidar(documento, tipo=tipo)
        if tipo in (None, 'reparaciones'):
            self.contextoInvalidado.emit(documento)
        if tipo in (None, 'sugerencias') and self.contexto_visible and self.contexto_visible.documento == documento:
            self.cerrar_lista()

    def olvidar(self, documento):
        self.invalidar(documento)
        self.editores.pop(documento, None)

    def cambiar_editor(self, *_):
        if self.actual:
            self.invalidar(self.actual)
        editor = self.ventana.editor()
        self.actual = editor.documento_id if editor else None
        self.cerrar_lista()

    def solicitar(self, tipo='sugerencias'):
        self.cancelar_programada()
        if not self.configuracion['habilitada']:
            self.ventana.statusBar().showMessage('Asistencia desactivada · Editar → Preferencias de asistencia')
            return
        editor = self.ventana.editor()
        if editor is None or editor.isReadOnly():
            self.ventana.statusBar().showMessage('Selecciona un documento editable para solicitar asistencia')
            return
        self.cancelar_automaticas(editor.documento_id)
        self.cerrar_lista()
        editor.setFocus()
        contexto = editor.contexto_asistencia()
        self.ventana.statusBar().showMessage(f'Consultando {tipo}…')
        return self.coordinador.solicitar(tipo, contexto)

    def recibir(self, respuesta):
        contexto = respuesta.solicitud.contexto
        editor = self.editores.get(contexto.documento)
        if editor is None or not isValid(editor) or editor is not self.ventana.editor():
            return
        if editor.contexto_asistencia() != contexto:
            return
        if not self.configuracion['habilitada']:
            return
        if respuesta.solicitud.origen == 'linea_confirmada':
            self.recibir_automatica(respuesta, editor)
            return
        if respuesta.solicitud.origen != 'manual' and not self.disponible_automatico(editor):
            return
        if respuesta.solicitud.tipo == 'reparaciones':
            self.reparacionesRecibidas.emit(respuesta)
            return
        resultado = respuesta.resultado
        if resultado.estado == EstadoAsistencia.OK:
            self.contexto_visible = contexto
            QApplication.instance().installEventFilter(self)
            self.popup.mostrar(resultado.sugerencias, editor)
            self.ventana.statusBar().showMessage(f'{len(resultado.sugerencias)} sugerencias · Enter/Tab acepta · Esc cancela')
        else:
            self.cerrar_lista()
            texto = resultado.explicacion or {'sin_opciones': 'No hay sugerencias para este contexto',
                                               'cancelado': 'Consulta de sugerencias cancelada'}.get(resultado.estado.value, 'Asistencia no disponible')
            self.ventana.statusBar().showMessage(texto)

    def aceptar(self, opcion):
        contexto = self.contexto_visible
        editor = self.editores.get(contexto.documento) if contexto else None
        self.cerrar_lista()
        if contexto is None or editor is None or not isValid(editor) or editor is not self.ventana.editor():
            return
        self.invalidar(contexto.documento)
        resultado = aplicar_sugerencia(editor, contexto, opcion)
        self.ventana.statusBar().showMessage('Sugerencia aplicada · Ctrl+Z deshace' if resultado.aplicada else resultado.explicacion)
        editor.setFocus()

    def eventFilter(self, widget, evento):
        tipo = evento.type()
        if tipo not in (QEvent.Type.MouseButtonPress, QEvent.Type.FocusOut, QEvent.Type.Resize,
                        QEvent.Type.Hide, QEvent.Type.InputMethod, QEvent.Type.ShortcutOverride, QEvent.Type.KeyPress):
            return False
        if not isValid(self.ventana) or not isValid(self.ventana.tabs):
            return False
        editor = self.ventana.editor()
        if editor is None:
            return False
        tipo = evento.type()
        visible = self.popup.isVisible()
        if tipo == QEvent.Type.MouseButtonPress and visible:
            if widget is not self.popup and (not isinstance(widget, QWidget) or not self.popup.isAncestorOf(widget)):
                self.cerrar_lista()
                self.coordinador.invalidar(tipo='sugerencias')
        if widget is editor:
            if tipo in (QEvent.Type.FocusOut, QEvent.Type.Resize, QEvent.Type.Hide, QEvent.Type.InputMethod):
                self.cancelar_automaticas(editor.documento_id)
                self.invalidar(editor.documento_id, tipo='sugerencias')
            if tipo in (QEvent.Type.ShortcutOverride, QEvent.Type.KeyPress):
                tecla, mods = evento.key(), evento.modifiers()
                consulta = tecla == Qt.Key.Key_Space and mods == Qt.KeyboardModifier.ControlModifier
                reparar = tecla in (Qt.Key.Key_Return, Qt.Key.Key_Enter) and mods == Qt.KeyboardModifier.AltModifier
                teclas = (Qt.Key.Key_Up, Qt.Key.Key_Down, Qt.Key.Key_PageUp, Qt.Key.Key_PageDown,
                          Qt.Key.Key_Return, Qt.Key.Key_Enter, Qt.Key.Key_Tab, Qt.Key.Key_Escape)
                if tipo == QEvent.Type.ShortcutOverride and (consulta or reparar or (visible and tecla in teclas and mods == Qt.KeyboardModifier.NoModifier)):
                    evento.accept(); return True
                if tipo == QEvent.Type.KeyPress:
                    if consulta:
                        self.solicitar(); evento.accept(); return True
                    if reparar:
                        self.ventana.solicitar_reparaciones(); evento.accept(); return True
                    if tecla == Qt.Key.Key_Escape and (visible or self.programada or self.coordinador.task or self.coordinador.pendientes):
                        self.cerrar_lista(); self.invalidar(editor.documento_id)
                        evento.accept(); return True
                    if visible and mods == Qt.KeyboardModifier.NoModifier:
                        if tecla in (Qt.Key.Key_Up, Qt.Key.Key_Down, Qt.Key.Key_PageUp, Qt.Key.Key_PageDown):
                            self.popup.desplazar({Qt.Key.Key_Up: -1, Qt.Key.Key_Down: 1,
                                                  Qt.Key.Key_PageUp: -8, Qt.Key.Key_PageDown: 8}[tecla])
                            evento.accept(); return True
                        if tecla in (Qt.Key.Key_Return, Qt.Key.Key_Enter, Qt.Key.Key_Tab):
                            self.popup.aceptar(); evento.accept(); return True
                    if visible:
                        self.cerrar_lista(); self.coordinador.invalidar(editor.documento_id)
        return False

    def detener(self):
        self.cancelar_programada()
        self.cerrar_lista()
        return self.coordinador.detener()
