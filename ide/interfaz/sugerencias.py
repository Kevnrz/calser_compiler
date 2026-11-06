"""Lista manual de sugerencias junto al cursor; sin motor provisional de producción."""
import weakref
from PySide6.QtCore import QEvent, QObject, QPoint, Qt, Signal
from PySide6.QtWidgets import QApplication, QFrame, QLabel, QListWidget, QListWidgetItem, QVBoxLayout, QWidget
from shiboken6 import isValid
from ide.nucleo.asistencia import EstadoAsistencia
from ide.interfaz.asistencia import CoordinadorAsistencia
from ide.interfaz.ediciones import aplicar_sugerencia


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
        ventana.tabs.currentChanged.connect(self.cambiar_editor)

    def conectar_editor(self, editor):
        documento = editor.documento_id
        self.editores[documento] = editor
        editor.installEventFilter(self)
        editor.contenidoCambiado.connect(lambda: self.invalidar(documento))
        editor.cursorPositionChanged.connect(lambda: self.invalidar(documento))
        editor.selectionChanged.connect(lambda: self.invalidar(documento))
        editor.verticalScrollBar().valueChanged.connect(lambda _: self.invalidar(documento))
        editor.horizontalScrollBar().valueChanged.connect(lambda _: self.invalidar(documento))
        editor.destroyed.connect(lambda: self.olvidar(documento))

    def establecer_motor(self, tipo, motor):
        self.cerrar_lista()
        self.coordinador.establecer_motor(tipo, motor)

    def cerrar_lista(self):
        QApplication.instance().removeEventFilter(self)
        self.popup.hide(); self.contexto_visible = None

    def invalidar(self, documento):
        self.coordinador.invalidar(documento)
        if self.contexto_visible and self.contexto_visible.documento == documento:
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
        editor = self.ventana.editor()
        if editor is None or editor.isReadOnly():
            self.ventana.statusBar().showMessage('Selecciona un documento editable para solicitar asistencia')
            return
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
        self.coordinador.invalidar(contexto.documento)
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
                self.coordinador.invalidar()
        if widget is editor:
            if tipo in (QEvent.Type.FocusOut, QEvent.Type.Resize, QEvent.Type.Hide, QEvent.Type.InputMethod):
                self.invalidar(editor.documento_id)
            if tipo in (QEvent.Type.ShortcutOverride, QEvent.Type.KeyPress):
                tecla, mods = evento.key(), evento.modifiers()
                consulta = tecla == Qt.Key.Key_Space and mods == Qt.KeyboardModifier.ControlModifier
                teclas = (Qt.Key.Key_Up, Qt.Key.Key_Down, Qt.Key.Key_PageUp, Qt.Key.Key_PageDown,
                          Qt.Key.Key_Return, Qt.Key.Key_Enter, Qt.Key.Key_Tab, Qt.Key.Key_Escape)
                if tipo == QEvent.Type.ShortcutOverride and (consulta or (visible and tecla in teclas and mods == Qt.KeyboardModifier.NoModifier)):
                    evento.accept(); return True
                if tipo == QEvent.Type.KeyPress:
                    if consulta:
                        self.solicitar(); evento.accept(); return True
                    if tecla == Qt.Key.Key_Escape and (visible or self.coordinador.task or self.coordinador.pendientes):
                        self.cerrar_lista(); self.coordinador.invalidar(editor.documento_id)
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
        self.cerrar_lista()
        return self.coordinador.detener()
