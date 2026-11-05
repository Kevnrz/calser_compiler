"""Análisis de clones en segundo plano y comparación de fragmentos."""
import os
from pathlib import Path

from PySide6.QtCore import QThread, Qt, Signal
from PySide6.QtGui import QFont
from PySide6.QtWidgets import QHBoxLayout, QHeaderView, QLabel, QPlainTextEdit, QPushButton, QSpinBox, QSplitter, QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget

from ide.archivos import leer
from ide.clones import Cancelado, LimiteAnalisis, detectar


class Analisis(QThread):
    resultado = Signal(object, object)
    fallo = Signal(str)
    cancelado = Signal()

    def __init__(self, directory, buffers, minimum, parent):
        super().__init__(parent)
        self.directory, self.buffers, self.minimum = directory, buffers, minimum

    def run(self):
        try:
            sources = {}
            excluded = {".git", ".venv", "venv", "build", "bin", "obj", "node_modules", "__pycache__"}
            def onerror(error):
                raise error
            for folder, directories, filenames in os.walk(self.directory, onerror=onerror):
                directories[:] = sorted(d for d in directories if d not in excluded and not d.startswith("."))
                for filename in sorted(filenames):
                    if self.isInterruptionRequested():
                        raise Cancelado()
                    path = Path(folder, filename)
                    if path.suffix != ".cls" or path.is_symlink():
                        continue
                    path = path.resolve()
                    if not path.is_relative_to(self.directory):
                        continue
                    if len(sources) >= 200 or path.stat().st_size > 2_000_000:
                        raise LimiteAnalisis("Límite: 200 archivos y 2 MB por archivo. Reduce la carpeta analizada.")
                    sources[path] = self.buffers[path] if path in self.buffers else leer(path)
            # Incluye documentos nuevos con ruta asignada que aún no estén en
            # disco; los documentos sin nombre se excluyen explícitamente.
            def permitido(path):
                return path.suffix == ".cls" and path.is_relative_to(self.directory) and not any(part in excluded or part.startswith(".") for part in path.relative_to(self.directory).parts[:-1])
            sources.update({p: text for p, text in self.buffers.items() if permitido(p)})
            if any(len(text.encode("utf-8")) > 2_000_000 for text in sources.values()):
                raise LimiteAnalisis("Un documento supera 2 MB. Reduce el tamaño o la carpeta analizada.")
            if sum(len(t) for t in sources.values()) > 20_000_000:
                raise LimiteAnalisis("Más de 20 MB de texto. Reduce la carpeta analizada.")
            pairs = detectar(sources, self.minimum, self.isInterruptionRequested)
            if self.isInterruptionRequested():
                raise Cancelado()
            self.resultado.emit(pairs, sources)
        except Cancelado:
            self.cancelado.emit()
        except (OSError, UnicodeError, ValueError) as error:
            self.fallo.emit(str(error))


class PanelClones(QWidget):
    solicitar = Signal()
    abrirFragmento = Signal(object, str)

    def __init__(self):
        super().__init__()
        self.task = None
        self.sources = {}
        self.pairs = []
        self.directory = None
        self.obsolete = False
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 6, 8, 6)
        row = QHBoxLayout()
        self.button = QPushButton("Analizar carpeta")
        self.button.clicked.connect(self.solicitar)
        row.addWidget(self.button)
        self.cancel = QPushButton("Cancelar")
        self.cancel.setEnabled(False)
        self.cancel.clicked.connect(self.cancelar)
        row.addWidget(self.cancel)
        row.addWidget(QLabel("Mín. tokens:"))
        self.minimum = QSpinBox()
        self.minimum.setRange(5, 200)
        self.minimum.setValue(20)
        self.minimum.setToolTip("Longitud mínima del fragmento; aumenta el valor para evitar coincidencias triviales.")
        self.minimum.valueChanged.connect(lambda: self.invalidar())
        row.addWidget(self.minimum)
        row.addStretch()
        layout.addLayout(row)
        self.status = QLabel("Selecciona una carpeta y analiza sus archivos .cls. Incluye subcarpetas y cambios sin guardar de documentos con nombre.")
        self.status.setWordWrap(True)
        layout.addWidget(self.status)
        self.list = QTreeWidget()
        self.list.setHeaderLabels(["Tipo", "Fragmento A", "Fragmento B", "Tokens"])
        self.list.setRootIsDecorated(False)
        self.list.setMinimumHeight(115)
        self.list.header().setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        self.list.header().setSectionResizeMode(2, QHeaderView.ResizeMode.Stretch)
        self.list.itemSelectionChanged.connect(self.comparar)
        layout.addWidget(self.list, 1)
        self.comparison = QSplitter(Qt.Orientation.Horizontal)
        self.previews = []
        self.open_buttons = []
        for label in ("Abrir A", "Abrir B"):
            side = QWidget()
            column = QVBoxLayout(side)
            column.setContentsMargins(0, 0, 0, 0)
            button = QPushButton(label)
            button.clicked.connect(lambda checked=False, i=len(self.previews): self.abrir(i))
            column.addWidget(button)
            preview = QPlainTextEdit()
            preview.setReadOnly(True)
            preview.setFont(QFont("monospace", 10))
            preview.setMinimumSize(100, 120)
            column.addWidget(preview)
            self.previews.append(preview)
            self.open_buttons.append(button)
            self.comparison.addWidget(side)
        self.comparison.hide()
        layout.addWidget(self.comparison, 1)

    def analizar(self, directory, buffers):
        if self.task and self.task.isRunning():
            return
        self.directory = Path(directory).resolve()
        self.obsolete = False
        self.sources = {}
        self.pairs = []
        self.list.clear()
        self.comparison.hide()
        self.button.setEnabled(False)
        self.cancel.setEnabled(True)
        self.status.setText(f"Analizando {self.directory} · buffers abiertos + archivos en disco…")
        self.task = Analisis(self.directory, dict(buffers), self.minimum.value(), self)
        self.task.resultado.connect(self.recibir)
        self.task.fallo.connect(self.error)
        self.task.cancelado.connect(lambda: self.status.setText("Análisis cancelado. Vuelve a analizar para obtener resultados actuales."))
        self.task.finished.connect(self.terminar)
        self.task.start()

    def recibir(self, pairs, sources):
        if self.obsolete:
            return
        self.pairs, self.sources = pairs, sources
        for pair in pairs:
            def location(fragment):
                path = fragment.archivo.relative_to(self.directory)
                return f"{path}:{fragment.linea_inicio}–{fragment.linea_fin}"
            item = QTreeWidgetItem([f"Tipo {pair.tipo}", location(pair.primero), location(pair.segundo), str(pair.tokens)])
            item.setData(0, Qt.ItemDataRole.UserRole, pair)
            item.setToolTip(1, str(pair.primero.archivo))
            item.setToolTip(2, str(pair.segundo.archivo))
            self.list.addTopLevelItem(item)
        self.status.setText(f"{len(pairs)} pares en {len(sources)} archivos · {self.directory} · mínimo {self.minimum.value()} tokens. Se usó el texto sin guardar de los archivos abiertos.")

    def comparar(self):
        items = self.list.selectedItems()
        if not items:
            self.comparison.hide()
            return
        pair = items[0].data(0, Qt.ItemDataRole.UserRole)
        for index, fragment in enumerate((pair.primero, pair.segundo)):
            text = self.sources[fragment.archivo]
            self.previews[index].setPlainText(text[fragment.inicio:fragment.fin])
            self.open_buttons[index].setText(f"Abrir {fragment.archivo.name} · L{fragment.linea_inicio}")
        self.comparison.show()

    def abrir(self, index):
        items = self.list.selectedItems()
        if items and not self.obsolete:
            pair = items[0].data(0, Qt.ItemDataRole.UserRole)
            fragment = (pair.primero, pair.segundo)[index]
            self.abrirFragmento.emit(fragment, self.sources[fragment.archivo])

    def error(self, message):
        self.status.setText(f"No se completó el análisis: {message}")

    def terminar(self):
        self.button.setEnabled(True)
        self.cancel.setEnabled(False)
        task = self.task
        self.task = None
        task.deleteLater()

    def cancelar(self):
        if self.task:
            self.obsolete = True
            self.task.requestInterruption()

    def invalidar(self, path=None):
        if path is not None and self.directory is not None and not Path(path).resolve().is_relative_to(self.directory):
            return
        if not self.sources and not self.task:
            return
        self.cancelar()
        self.obsolete = True
        self.sources = {}
        self.pairs = []
        self.list.clear()
        self.comparison.hide()
        self.status.setText("El contenido o la configuración cambió. Vuelve a analizar los clones.")

    def detener(self):
        if self.task:
            self.cancelar()
            return self.task.wait(3000)
        return True
