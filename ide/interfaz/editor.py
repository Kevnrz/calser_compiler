"""Editor Qt con resaltado del lexer real, margen y diagnósticos por origen."""
from html import escape
from uuid import uuid4
from ide.interfaz.sugerencia_inline import SugerenciaInline

from PySide6.QtCore import QEvent, QRect, QSize, QTimer, Qt, Signal
from PySide6.QtGui import QContextMenuEvent, QColor, QFont, QPainter, QKeySequence, QSyntaxHighlighter, QTextCharFormat, QTextCursor, QTextFormat
from PySide6.QtWidgets import QPlainTextEdit, QTextEdit, QToolTip, QWidget


class Resaltado(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.tramos = {}
        self.formats = {}
        for category, color in {"reservada": "#efbb65", "tipo": "#8bd3cc", "numero": "#c1a0eb", "booleano": "#c1a0eb", "operador": "#efbb65", "identificador": "#d6dce7", "comentario": "#909bac"}.items():
            fmt = QTextCharFormat()
            fmt.setForeground(QColor(color))
            self.formats[category] = fmt

    def actualizar(self, tramos):
        self.tramos = tramos
        self.rehighlight()

    def highlightBlock(self, text):
        for span in self.tramos.get(self.currentBlock().blockNumber(), ()):
            self.setFormat(span.inicio, span.longitud, self.formats[span.categoria])


class Margen(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor

    def sizeHint(self):
        return QSize(self.editor.ancho_margen(), 0)

    def paintEvent(self, event):
        editor = self.editor
        painter = QPainter(self)
        painter.fillRect(event.rect(), QColor("#191c24"))
        block = editor.firstVisibleBlock()
        top = round(editor.blockBoundingGeometry(block).translated(editor.contentOffset()).top())
        while block.isValid() and top <= event.rect().bottom():
            height = round(editor.blockBoundingRect(block).height())
            if block.isVisible() and top + height >= event.rect().top():
                if any(p.linea == block.blockNumber() + 1 for p in editor.problemas):
                    painter.setPen(Qt.PenStyle.NoPen)
                    painter.setBrush(QColor("#f08e9e"))
                    painter.drawEllipse(3, top + max(2, (height - 6) // 2), 6, 6)
                painter.setPen(QColor("#8994a5"))
                painter.drawText(0, top, self.width() - 12, height, Qt.AlignmentFlag.AlignRight, str(block.blockNumber() + 1))
            top += height
            block = block.next()


class Editor(QPlainTextEdit):
    contenidoCambiado = Signal()
    entradaAsistencia = Signal(str)
    analisisSolicitado = Signal(str, int, str)
    estadoAnalisisCambiado = Signal()

    def __init__(self, root):
        super().__init__()
        self.acciones_contextuales = ()
        self.path = None
        self.estado_disco = None
        self.estado_observado = None
        self.error_disco = ''
        self.cambio_reconocido = None
        self.revision = 0
        self.documento_id = uuid4().hex
        self._texto_actual = ""
        self.errores_lexicos = []
        self.analisis_revision = -1
        self.contiene_codigo = None
        self.estado_analisis = "Esperando…"
        self.estado_ccom = "Esperando léxico…"
        self.problemas = []
        self.analisis_timer = QTimer(self)
        self.analisis_timer.setSingleShot(True)
        self.analisis_timer.setInterval(300)
        self.analisis_timer.timeout.connect(self.emitir_analisis)
        self.setFont(QFont("monospace", 12))
        self.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        self.setTabStopDistance(self.fontMetrics().horizontalAdvance(" ") * 4)
        self.sugerencia_inline = SugerenciaInline(self)
        self.margin = Margen(self)
        self.highlighter = Resaltado(self.document())
        self.blockCountChanged.connect(self.actualizar_margen)
        self.updateRequest.connect(self.actualizar_zona)
        self.cursorPositionChanged.connect(self.linea_actual)
        self.textChanged.connect(self.cambio)
        self.actualizar_margen()
        self.linea_actual()

    def canInsertFromMimeData(self, source):
        # Qt ofrece text/uri-list como texto: arrastrar/copiar un archivo no
        # debe escribir file://... en el código. Copiar ubicación como texto
        # plano o pegar código sigue pasando por el editor estándar.
        return not source.hasUrls() and super().canInsertFromMimeData(source)

    def insertFromMimeData(self, source):
        if source.hasUrls():
            return
        super().insertFromMimeData(source)

    def crear_menu_contextual(self):
        """Conserva las acciones estándar y añade accesos a la asistencia.

        Se crean acciones locales para respetar solo aquí lectura/.asm sin
        cambiar el estado de las acciones compartidas del menú principal.
        """
        menu = self.createStandardContextMenu()
        if self.acciones_contextuales:
            menu.addSeparator()
            for original in self.acciones_contextuales:
                accion = menu.addAction(original.text())
                accion.setEnabled(original.isEnabled() and not self.isReadOnly()
                                  and not (self.path and self.path.suffix.lower() == '.asm'))
                # Lanzar después de cerrar el menú: su devolución de foco no
                # debe cancelar la consulta como si el usuario abandonara el editor.
                accion.triggered.connect(lambda checked=False, destino=original:
                                         QTimer.singleShot(0, destino.trigger))
        if getattr(self, 'eliminar_archivo', None):
            menu.addSeparator()
            eliminar = menu.addAction('Eliminar archivo…')
            eliminar.setEnabled(self.path is not None and self.path.exists())
            eliminar.triggered.connect(lambda: QTimer.singleShot(0, self.eliminar_archivo))
        return menu

    def contextMenuEvent(self, event):
        # El clic consulta su posición; dentro de una selección se conserva la
        # selección. La tecla de menú/Shift+F10 usa el cursor existente.
        if event.reason() == QContextMenuEvent.Reason.Mouse:
            cursor = self.cursorForPosition(event.pos())
            actual = self.textCursor()
            if not (actual.hasSelection() and actual.selectionStart() <= cursor.position() < actual.selectionEnd()):
                self.setTextCursor(cursor)
        menu = self.crear_menu_contextual()
        try:
            menu.exec(event.globalPos())
        finally:
            menu.deleteLater()

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.StandardKey.Undo):
            restaurar = getattr(self, 'deshacer_archivo', None)
            if restaurar and restaurar():
                event.accept()
                return
        cursor = self.textCursor()
        revision = self.revision
        sin_seleccion = not cursor.hasSelection()
        final_linea = cursor.atBlockEnd() and bool(cursor.block().text().strip())
        mods = event.modifiers()
        confirmar = (event.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter)
                     and mods == Qt.KeyboardModifier.NoModifier and not event.isAutoRepeat())
        escribir = (bool(event.text()) and event.text().isprintable()
                    and mods in (Qt.KeyboardModifier.NoModifier, Qt.KeyboardModifier.ShiftModifier))
        super().keyPressEvent(event)
        if self.revision != revision and sin_seleccion:
            if confirmar and final_linea:
                self.entradaAsistencia.emit('linea_confirmada')
            elif escribir or (event.key() in (Qt.Key.Key_Backspace, Qt.Key.Key_Delete) and mods == Qt.KeyboardModifier.NoModifier):
                self.entradaAsistencia.emit('escritura')

    def contexto_asistencia(self):
        from ide.interfaz.ediciones import capturar_contexto
        return capturar_contexto(self)

    def aplicar_ediciones(self, contexto, ediciones, *, cursor_final=None, precondiciones=()):
        from ide.interfaz.ediciones import aplicar_ediciones_editor
        return aplicar_ediciones_editor(self, contexto, ediciones, cursor_final=cursor_final,
                                        precondiciones=precondiciones)

    def cambio(self):
        text = self.toPlainText()
        # Rehighlight puede emitir textChanged sin que cambie el texto. No
        # tratar el formato como edición ni reiniciar análisis indefinidamente.
        if text == self._texto_actual:
            return
        self._texto_actual = text
        self.revision += 1
        self.errores_lexicos = []
        self.analisis_revision = -1
        self.contiene_codigo = None
        self.establecer_problemas([])
        self.highlighter.actualizar({})
        self.programar_analisis()
        self.contenidoCambiado.emit()

    def programar_analisis(self):
        self.estado_analisis = "Esperando…"
        self.analisis_timer.start()
        self.estadoAnalisisCambiado.emit()

    def emitir_analisis(self):
        self.estado_analisis = "Analizando…"
        self.estadoAnalisisCambiado.emit()
        self.analisisSolicitado.emit(self.documento_id, self.revision, self.toPlainText())

    def aplicar_analisis(self, result):
        if result.documento != self.documento_id or result.revision != self.revision:
            return False
        modified = self.document().isModified()
        self.errores_lexicos = list(result.problemas)
        self.analisis_revision = result.revision
        self.contiene_codigo = result.contiene_codigo
        self.highlighter.actualizar(result.tramos)
        self.document().setModified(modified)
        self.estado_analisis = f"{len(result.problemas)} error(es)" if result.problemas else "Sin errores léxicos"
        self.estadoAnalisisCambiado.emit()
        return True

    def detener_analisis(self):
        self.analisis_timer.stop()

    def establecer_problemas(self, problems):
        self.problemas = list(problems)
        self.linea_actual()
        self.margin.update()

    def rango_problema(self, problem):
        """Convierte bytes ccom o caracteres del núcleo a UTF-16.

        Una posición al final de línea queda vacía: se señala la línea y no se
        inventa un token sobre el que dibujar el subrayado.
        """
        if not problem.tiene_posicion:
            return None
        block = self.document().findBlockByNumber(problem.linea - 1)
        if not block.isValid():
            return None
        text = block.text()
        if problem.unidad == "unicode":
            start_cp = min(len(text), max(0, problem.columna - 1))
            end_cp = min(len(text), start_cp + max(1, problem.longitud))
            prefix = text[:start_cp]
        else:
            encoded = text.encode("utf-8")
            start_byte = min(len(encoded), max(0, problem.columna - 1))
            prefix = encoded[:start_byte].decode("utf-8", errors="ignore")
            start_cp = len(prefix)
            end_cp = len(encoded[:start_byte + max(1, problem.longitud)].decode("utf-8", errors="ignore"))
        if start_cp < len(text):
            end_cp = max(start_cp + 1, end_cp)
        start = len(prefix.encode("utf-16-le")) // 2
        end = len(text[:end_cp].encode("utf-16-le")) // 2
        cursor = QTextCursor(block)
        cursor.setPosition(block.position() + start)
        cursor.setPosition(block.position() + end, QTextCursor.MoveMode.KeepAnchor)
        return cursor

    def problemas_en_linea(self, line):
        return [p for p in self.problemas if p.linea == line]

    def viewportEvent(self, event):
        if event.type() == QEvent.Type.ToolTip:
            cursor = self.cursorForPosition(event.pos())
            problems = self.problemas_en_linea(cursor.blockNumber() + 1)
            if problems:
                text = "<br><br>".join(f"<b>{escape(p.origen)} · {escape(p.categoria)}</b><br>{escape(p.descripcion)}" for p in problems)
                QToolTip.showText(event.globalPos(), text, self)
                return True
            QToolTip.hideText()
        return super().viewportEvent(event)

    def ancho_margen(self):
        return 20 + self.fontMetrics().horizontalAdvance("9") * len(str(self.blockCount()))

    def actualizar_margen(self, *_):
        self.setViewportMargins(self.ancho_margen(), 0, 0, 0)

    def actualizar_zona(self, rect, dy):
        if dy:
            self.margin.scroll(0, dy)
        else:
            self.margin.update(0, rect.y(), self.margin.width(), rect.height())

    def resizeEvent(self, event):
        super().resizeEvent(event)
        area = self.contentsRect()
        self.margin.setGeometry(QRect(area.left(), area.top(), self.ancho_margen(), area.height()))

    def linea_actual(self):
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(QColor("#242a36"))
        selection.format.setProperty(QTextFormat.Property.FullWidthSelection, True)
        selection.cursor = self.textCursor()
        selection.cursor.clearSelection()
        selections = [selection]
        for problem in self.problemas:
            cursor = self.rango_problema(problem)
            if cursor is None:
                continue
            error = QTextEdit.ExtraSelection()
            error.cursor = cursor
            error.format.setBackground(QColor("#392931"))
            error.format.setUnderlineColor(QColor("#f08e9e"))
            error.format.setUnderlineStyle(QTextCharFormat.UnderlineStyle.WaveUnderline)
            if not cursor.hasSelection():
                error.format.setProperty(QTextFormat.Property.FullWidthSelection, True)
            selections.append(error)
        self.setExtraSelections(selections)
