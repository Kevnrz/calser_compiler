"""Resaltado local del listado fijo de tac_print; no consulta al compilador."""
from PySide6.QtCore import QRegularExpression
from PySide6.QtGui import QColor, QSyntaxHighlighter, QTextCharFormat


class ResaltadorTac(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        # Paleta de Resaltado (Calser): reservada, tipo, comentario,
        # identificador y número, respectivamente.
        self.reglas = []
        for patron, color in (
            (r"-?\b\d+\b", "#c1a0eb"),
            (r"\bt\d+\b", "#d6dce7"),
            (r"\bB\d+\b", "#8bd3cc"),
            (r"\b(successors|entry|initial_data)\b", "#909bac"),
            (r"\b(if|goto|return|call|param|address)\b", "#efbb65"),
            (r"^function \w+", "#efbb65"),
            (r"\$\w+", "#909bac"),
        ):
            formato = QTextCharFormat()
            formato.setForeground(QColor(color))
            self.reglas.append((QRegularExpression(patron), formato))

    def highlightBlock(self, text):
        for patron, formato in self.reglas:
            coincidencias = patron.globalMatch(text)
            while coincidencias.hasNext():
                coincidencia = coincidencias.next()
                self.setFormat(coincidencia.capturedStart(), coincidencia.capturedLength(), formato)
