"""Panel de clones C: reúne copias, lanza el trabajador y presenta fragmentos.

Analisis recorre archivos permitidos y superpone buffers con nombre sin guardar.
No tokeniza ni compara: ide.clones envía el paquete privado al ccom configurado.
Los resultados son Fragmento con caracteres Python, convertidos desde UTF-8.
PanelClones conserva las copias usadas para mostrar rangos; editar, cambiar
carpeta/mínimo/ejecutable invalida tabla y navegación. El trabajador no toca Qt
fuera de sus señales y la cancelación llega al proceso mediante el adaptador.
La ventana comprueba nuevamente el contenido antes de abrir una selección."""
import os
from pathlib import Path

from PySide6.QtCore import QThread, Qt, Signal
from PySide6.QtGui import QFont
from PySide6.QtWidgets import QHBoxLayout, QHeaderView, QLabel, QPlainTextEdit, QPushButton, QSizePolicy, QSpinBox, QSplitter, QToolButton, QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget

from ide.archivos import leer
from ide.clones import Cancelado, LimiteAnalisis, detectar


class Analisis(QThread):
    resultado = Signal(object, object)
    fallo = Signal(str)
    cancelado = Signal()

    def __init__(self, directory, buffers, minimum, parent, root=None, configurado=""):
        super().__init__(parent)
        self.directory, self.buffers, self.minimum = directory, buffers, minimum
        self.root, self.configurado = root, configurado

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
            if sum(len(t.encode("utf-8")) for t in sources.values()) > 20_000_000:
                raise LimiteAnalisis("Más de 20 MB de texto. Reduce la carpeta analizada.")
            pairs = detectar(sources, self.minimum, self.isInterruptionRequested, root=self.root, configurado=self.configurado)
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
        self.list.setMinimumHeight(65)
        self.list.header().setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        self.list.header().setSectionResizeMode(2, QHeaderView.ResizeMode.Stretch)
        self.list.itemSelectionChanged.connect(self.comparar)
        self.details = QSplitter(Qt.Orientation.Vertical)
        self.details.setObjectName("detalleClones")
        self.details.setHandleWidth(6)
        self.details.setChildrenCollapsible(False)
        self.details.setStyleSheet("QSplitter#detalleClones::handle:hover { background: #665137; }")
        self.details.addWidget(self.list)
        self.detail_sizes = None
        self.details.splitterMoved.connect(lambda *_: self.recordar_distribucion())
        self.comparison = QSplitter(Qt.Orientation.Horizontal)
        self.previews = []
        self.open_buttons = []
        self.preview_labels = []
        for label in ("Fragmento A", "Fragmento B"):
            side = QWidget()
            column = QVBoxLayout(side)
            column.setContentsMargins(0, 0, 0, 0)
            column.setSpacing(3)
            header = QHBoxLayout()
            header.setContentsMargins(4, 0, 0, 0)
            name = QLabel(label)
            name.setTextFormat(Qt.TextFormat.PlainText)
            name.setMinimumWidth(0)
            name.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Preferred)
            name.setStyleSheet("color: #efbb65;")
            header.addWidget(name, 1)
            button = QToolButton()
            button.setText("Abrir ↗")
            button.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
            button.clicked.connect(lambda checked=False, i=len(self.previews): self.abrir(i))
            header.addWidget(button)
            self.preview_labels.append(name)
            column.addLayout(header)
            preview = QPlainTextEdit()
            preview.setReadOnly(True)
            preview.setFont(QFont("monospace", 10))
            preview.setMinimumSize(80, 65)
            column.addWidget(preview)
            self.previews.append(preview)
            self.open_buttons.append(button)
            self.comparison.addWidget(side)
        self.comparison.hide()
        self.details.addWidget(self.comparison)
        self.details.setStretchFactor(0, 0)
        self.details.setStretchFactor(1, 1)
        handle = self.details.handle(1)
        handle.setCursor(Qt.CursorShape.SplitVCursor)
        handle.setToolTip("Arrastra para repartir el espacio entre resultados y comparación")
        layout.addWidget(self.details, 1)

    def analizar(self, directory, buffers, *, root=None, configurado=""):
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
        self.task = Analisis(self.directory, dict(buffers), self.minimum.value(), self, root, configurado)
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
            titulo = f"{fragment.archivo.name} · L{fragment.linea_inicio}–{fragment.linea_fin}"
            self.preview_labels[index].setText(titulo)
            self.preview_labels[index].setToolTip(f"{fragment.archivo}:{fragment.linea_inicio}–{fragment.linea_fin}")
            self.open_buttons[index].setToolTip(f"Abrir {titulo} en el editor")
            self.open_buttons[index].setAccessibleName(f"Abrir {titulo}")
        estaba_oculta = self.comparison.isHidden()
        self.comparison.show()
        if estaba_oculta:
            self.details.setSizes(self.detail_sizes or [85, 240])

    def recordar_distribucion(self):
        if not self.comparison.isHidden():
            self.detail_sizes = self.details.sizes()

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
