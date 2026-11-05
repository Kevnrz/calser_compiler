"""Editor Qt: margen, línea activa y resaltado visual reutilizado del plugin.

El resaltado no valida sintaxis ni sustituye al núcleo LL(1).
"""
import json
import re
from html import escape

from PySide6.QtCore import QEvent, QRect, QSize, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QSyntaxHighlighter, QTextCharFormat, QTextCursor, QTextFormat
from PySide6.QtWidgets import QPlainTextEdit, QTextEdit, QToolTip, QWidget


class Resaltado(QSyntaxHighlighter):
    def __init__(self, document, root):
        super().__init__(document)
        self.rules = []
        grammar = root / "editors/vscode/syntaxes/calser.tmLanguage.json"
        if grammar.exists():
            for item in json.loads(grammar.read_text())["patterns"]:
                if "match" not in item or item["name"].startswith("comment"):
                    continue
                name = item["name"]
                color = "#d6dce7"
                if name.startswith(("keyword", "storage.modifier")):
                    color = "#efbb65"
                elif name.startswith("storage.type"):
                    color = "#8bd3cc"
                elif name.startswith("constant"):
                    color = "#c1a0eb"
                elif name.startswith("entity"):
                    color = "#9ac4ee"
                fmt = QTextCharFormat()
                fmt.setForeground(QColor(color))
                self.rules.append((re.compile(item["match"]), fmt))
        self.comment = QTextCharFormat()
        self.comment.setForeground(QColor("#909bac"))

    def highlightBlock(self, text):
        # Python counts code points; QTextDocument positions count UTF-16 units.
        def apply(start, end, fmt):
            offset = len(text[:start].encode("utf-16-le")) // 2
            length = len(text[start:end].encode("utf-16-le")) // 2
            self.setFormat(offset, length, fmt)
        for pattern, fmt in self.rules:
            for match in pattern.finditer(text):
                apply(match.start(), match.end(), fmt)
        self.setCurrentBlockState(0)
        pos = 0
        in_comment = self.previousBlockState() == 1
        while pos < len(text):
            start = pos if in_comment else text.find("#", pos)
            if start < 0:
                break
            if in_comment or text.startswith("#[", start):
                end = text.find("]#", start if in_comment else start + 2)
                if end < 0:
                    apply(start, len(text), self.comment)
                    self.setCurrentBlockState(1)
                    break
                apply(start, end + 2, self.comment)
                pos = end + 2
                in_comment = False
            else:
                apply(start, len(text), self.comment)
                break


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
    def __init__(self, root):
        super().__init__()
        self.path = None
        self.revision = 0
        self.problemas = []
        self.setFont(QFont("monospace", 12))
        self.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        self.setTabStopDistance(self.fontMetrics().horizontalAdvance(" ") * 4)
        self.margin = Margen(self)
        self.highlighter = Resaltado(self.document(), root)
        self.blockCountChanged.connect(self.actualizar_margen)
        self.updateRequest.connect(self.actualizar_zona)
        self.cursorPositionChanged.connect(self.linea_actual)
        self.textChanged.connect(self.cambio)
        self.actualizar_margen()
        self.linea_actual()

    def cambio(self):
        self.revision += 1
        self.establecer_problemas([])

    def establecer_problemas(self, problems):
        self.problemas = list(problems)
        self.linea_actual()
        self.margin.update()

    def rango_problema(self, problem):
        """Convierte posiciones UTF-8 de ccom a UTF-16 sin partir caracteres.

        Una posición al final de línea queda vacía: se señala la línea y no se
        inventa un token sobre el que dibujar el subrayado.
        """
        block = self.document().findBlockByNumber(problem.linea - 1)
        if not block.isValid():
            return None
        text = block.text()
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
                text = "<br><br>".join(f"<b>ccom · {escape(p.categoria)}</b><br>{escape(p.descripcion)}" for p in problems)
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
