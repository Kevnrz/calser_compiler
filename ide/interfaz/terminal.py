"""Terminal básica de comandos Linux con una sesión PTY por pestaña.

La UI usa entrada por líneas; no es un emulador de pantalla para vim/top.
El proceso auxiliar posee el PTY y evita hacer fork dentro del proceso Qt.
"""
import codecs
import re
import sys
from pathlib import Path

from PySide6.QtCore import QProcess, Qt
from PySide6.QtGui import QFont, QTextCursor
from PySide6.QtWidgets import QHBoxLayout, QLabel, QLineEdit, QPlainTextEdit, QPushButton, QVBoxLayout, QWidget


class Entrada(QLineEdit):
    def __init__(self):
        super().__init__()
        self.history = []
        self.position = 0

    def keyPressEvent(self, event):
        if event.key() in (Qt.Key.Key_Up, Qt.Key.Key_Down):
            self.position = max(0, min(len(self.history), self.position + (-1 if event.key() == Qt.Key.Key_Up else 1)))
            self.setText(self.history[self.position] if self.position < len(self.history) else "")
            return
        super().keyPressEvent(event)


class Terminal(QWidget):
    def __init__(self, directory):
        super().__init__()
        self.decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 6, 8, 6)
        self.screen = QPlainTextEdit()
        self.screen.setReadOnly(True)
        self.screen.setFont(QFont("monospace", 11))
        self.screen.document().setMaximumBlockCount(10000)
        self.screen.setPlaceholderText("Iniciando sesión…")
        layout.addWidget(self.screen)
        row = QHBoxLayout()
        self.input = Entrada()
        self.input.setPlaceholderText("Comando · Enter para ejecutar · ↑↓ historial")
        self.input.returnPressed.connect(self.enviar)
        row.addWidget(self.input, 1)
        interrupt = QPushButton("Interrumpir")
        interrupt.setToolTip("Enviar Ctrl+C al proceso de esta terminal")
        interrupt.clicked.connect(lambda: self.escribir(b"\x03"))
        row.addWidget(interrupt)
        layout.addLayout(row)
        note = QLabel("Shell Linux · sesión independiente")
        note.setWordWrap(True)
        note.setToolTip("Entrada por líneas. Las aplicaciones de pantalla completa como Vim/top todavía no están soportadas.")
        layout.addWidget(note)
        self.process = QProcess(self)
        self.process.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        self.process.readyReadStandardOutput.connect(self.leer)
        self.process.finished.connect(self.finalizar)
        self.process.errorOccurred.connect(self.fallo)
        if sys.platform != "linux":
            self.screen.setPlainText("La terminal PTY de este prototipo está disponible en Linux.")
            self.input.setEnabled(False)
            return
        helper = Path(__file__).with_name("terminal_pty.py")
        self.process.start(sys.executable, ["-u", str(helper), str(directory)])

    def activa(self):
        return self.process.state() != QProcess.ProcessState.NotRunning

    def escribir(self, data):
        if self.activa():
            self.process.write(data)

    def enviar(self):
        command = self.input.text()
        if command:
            self.input.history.append(command)
        self.input.position = len(self.input.history)
        self.escribir((command + "\n").encode("utf-8"))
        self.input.clear()

    def leer(self):
        text = self.decoder.decode(bytes(self.process.readAllStandardOutput()))
        # TERM=dumb evita secuencias de pantalla en el shell. Conservamos toda
        # salida textual; esta vista no pretende emular aplicaciones curses.
        text = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", text).replace("\r", "")
        cursor = self.screen.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(text)
        self.screen.setTextCursor(cursor)

    def finalizar(self, *_):
        self.leer()
        self.screen.appendPlainText("\n[Sesión terminada]")
        self.input.setEnabled(False)

    def fallo(self, error):
        if error == QProcess.ProcessError.FailedToStart:
            self.screen.appendPlainText(self.process.errorString())
            self.input.setEnabled(False)

    def detener(self):
        if self.activa():
            # EOF en el canal de control permite al auxiliar colgar el PTY y
            # recoger el proceso hijo sin bloquear con un shell esperando entrada.
            self.process.closeWriteChannel()
            if not self.process.waitForFinished(1500):
                self.process.terminate()
                self.process.waitForFinished(1000)
