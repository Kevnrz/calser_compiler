"""Texto de asistencia superpuesto al viewport, ajeno al documento y a Deshacer.

Solo dibuja una inserción al final de la línea. El controlador conserva la
propuesta original de C y su snapshot; este widget nunca decide ni aplica código.
"""
from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QColor, QFontMetricsF, QPainter
from PySide6.QtWidgets import QWidget


class SugerenciaInline(QWidget):
    def __init__(self, editor):
        super().__init__(editor.viewport())
        self.editor = editor
        self.texto = ''
        self.setAttribute(Qt.WidgetAttribute.WA_TransparentForMouseEvents)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self.hide()

    def mostrar(self, texto):
        cursor = self.editor.textCursor()
        if (not texto or '\n' in texto or '\r' in texto or cursor.hasSelection()
                or not cursor.atBlockEnd() or self.editor.isReadOnly()):
            self.ocultar()
            return False
        self.texto = texto
        self.setFont(self.editor.font())
        rect = self.editor.cursorRect()
        if not self.editor.viewport().rect().intersects(rect):
            self.ocultar()
            return False
        ancho = min(self.editor.viewport().width() - rect.x(),
                    int(QFontMetricsF(self.font()).horizontalAdvance(texto)) + 4)
        if ancho <= 0:
            self.ocultar()
            return False
        self.setGeometry(rect.x(), rect.y(), ancho, rect.height())
        self.show()
        self.raise_()
        self.update()
        return True

    def ocultar(self):
        self.texto = ''
        self.hide()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setFont(self.font())
        painter.setPen(QColor('#85909f'))
        metrics = QFontMetricsF(self.font())
        baseline = (self.height() - metrics.height()) / 2 + metrics.ascent()
        painter.drawText(QPointF(0, baseline), self.texto)
