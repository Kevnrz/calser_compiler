"""Ventana CALSIDE: archivos, diagnósticos, terminales y clones."""
from pathlib import Path
import hashlib
import os
import codecs
from html import escape

from PySide6.QtCore import QProcess, QTimer, Qt
from PySide6.QtGui import QAction, QColor, QPainter, QPolygon, QTextCursor
from PySide6.QtCore import QPoint
from PySide6.QtWidgets import (
    QFileDialog, QHBoxLayout, QHeaderView, QInputDialog, QLabel, QMainWindow,
    QMenuBar, QMessageBox, QPlainTextEdit, QPushButton, QSplitter, QTabWidget,
    QSizePolicy, QToolButton, QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget,
)

from ide import archivos, ccom
from ide.interfaz.editor import Editor
from ide.interfaz.explorador import Explorador
from ide.interfaz.proyecto import ProyectoVentana
from ide.interfaz.terminal import Terminal
from ide.interfaz.clones import PanelClones
from ide.interfaz.analisis import CoordinadorAnalisis
from ide.interfaz.comprobacion import ComprobadorCcom, SolicitudCcom


class Esquina(QWidget):
    """Guiño de identidad estático; conserva los controles nativos de ventana."""
    def __init__(self):
        super().__init__()
        self.setFixedSize(32, 32)
        self.setToolTip("Calser · Esquina de papel. Animación pendiente.")

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QColor("#ab83df"))
        painter.drawPolygon(QPolygon([QPoint(0, 0), QPoint(32, 0), QPoint(32, 32)]))


class Ventana(ProyectoVentana, QMainWindow):
    def __init__(self, root, *, preferencias=None):
        super().__init__()
        self.root = Path(root)
        self.project = self.root
        self.configured_ccom = ""
        self.process = None
        self.stderr = ""
        self.compilation = None
        self.results = {}
        self.auto_results = {}
        self.setWindowTitle("CALSIDE · Prototipo")
        self.resize(1250, 800)
        self.setMinimumSize(800, 520)
        self.setStyleSheet("""
            QMainWindow, QWidget { background: #191c24; color: #dce2ec; }
            QMenuBar, QMenu, QStatusBar { background: #222631; }
            QMenu::item:selected { background: #3a3340; }
            QMenuBar::item:selected { background: #34303a; }
            QMenuBar::item { padding: 6px 9px; }
            QToolButton { border: none; padding: 5px 8px; color: #b7c1cf; }
            QToolButton:hover { background: #393344; color: #efbb65; }
            QLineEdit { background: #222631; border: 1px solid #414957; padding: 7px; color: #e2e7ef; }
            QPlainTextEdit { background: #191c24; border: none; selection-background-color: #504135; selection-color: #fff2d3; }
            QTabWidget::pane { border: none; }
            QTabBar::tab { padding: 10px 18px; background: #222631; color: #a5afbf; border-top: 2px solid transparent; }
            QTabBar::tab:selected { background: #191c24; color: #efbb65; border-top-color: #efbb65; }
            QTreeWidget, QTreeView { border: none; background: #20242e; padding: 5px; }
            QTreeWidget::item, QTreeView::item { padding: 6px; }
            QTreeWidget::item:selected, QTreeView::item:selected { background: #383126; color: #efbb65; }
            QHeaderView::section { background: #272c37; padding: 6px; border: none; }
            QPushButton { background: #2c313e; border: 1px solid #424a59; border-radius: 5px; padding: 7px 12px; }
            QPushButton:hover { border-color: #efbb65; }
            QPushButton:disabled { color: #8992a1; }
            QPushButton#compilar { background: #403525; color: #f0bf72; border-color: #665137; }
            QSplitter::handle { background: #303644; width: 2px; height: 2px; }
            QToolTip { background: #303644; color: #ffffff; border: 1px solid #666077; }
        """)
        self.crear_interfaz()
        self.crear_acciones()
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.agotar_tiempo)
        self.analizador = CoordinadorAnalisis(self)
        self.analizador.resultado.connect(self.recibir_analisis)
        self.analizador.fallo.connect(self.fallo_analisis)
        self.comprobador = ComprobadorCcom(self.root, self)
        self.comprobador.resultado.connect(self.recibir_comprobacion)
        self.comprobador.fallo.connect(self.fallo_comprobacion)
        self.comprobador.estado.connect(self.estado_comprobacion)
        self.iniciar_proyecto(preferencias)
        self.nuevo()
        self.restaurar_preferencias()

    def crear_interfaz(self):
        body = QWidget()
        layout = QVBoxLayout(body)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        header = QWidget()
        header.setObjectName("cabecera")
        header.setStyleSheet("QWidget#cabecera { background: #272c37; border-bottom: 1px solid #363d4b; }")
        header.setFixedHeight(44)
        bar = QHBoxLayout(header)
        bar.setContentsMargins(16, 0, 0, 0)
        brand = QLabel("⬡  CALSIDE")
        brand.setStyleSheet("background: transparent; color: #efbb65; font-size: 15px;")
        bar.addWidget(brand)
        self.context = QLabel("Análisis en vivo · F5 compila")
        self.context.setStyleSheet("background: transparent; color: #a5afbf;")
        bar.addWidget(self.context)
        bar.addStretch()
        bar.addWidget(Esquina(), alignment=Qt.AlignmentFlag.AlignTop)
        layout.addWidget(header)

        # QMainWindow.menuBar() siempre va sobre el widget central. Una barra
        # embebida permite respetar el orden cabecera → menús → editor del diseño.
        toolbar = QWidget()
        toolbar.setFixedHeight(48)
        toolbar.setStyleSheet("background: #222631;")
        tools = QHBoxLayout(toolbar)
        tools.setContentsMargins(8, 4, 12, 4)
        self.menus = QMenuBar(toolbar)
        self.menus.setNativeMenuBar(False)
        self.menus.setSizePolicy(QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed)
        tools.addWidget(self.menus, 0, Qt.AlignmentFlag.AlignVCenter)
        tools.addStretch()
        self.files_button = QToolButton()
        self.files_button.setText("Archivos")
        self.files_button.setToolTip("Mostrar u ocultar el explorador · Ctrl+B")
        self.files_button.setCheckable(True)
        self.files_button.setChecked(True)
        tools.addWidget(self.files_button)
        self.compile_button = QPushButton("▷  Compilar  F5")
        self.compile_button.setObjectName("compilar")
        self.compile_button.clicked.connect(self.compilar)
        tools.addWidget(self.compile_button)
        self.stop_button = QPushButton("Cancelar")
        self.stop_button.setEnabled(False)
        self.stop_button.clicked.connect(self.cancelar)
        tools.addWidget(self.stop_button)
        layout.addWidget(toolbar)

        self.crear_aviso_externo(layout)
        horizontal = QSplitter(Qt.Orientation.Horizontal)
        self.project_splitter = horizontal
        sidebar = QWidget()
        self.sidebar = sidebar
        side = QVBoxLayout(sidebar)
        side.setContentsMargins(8, 10, 4, 4)
        project_button = QPushButton("Abrir carpeta…")
        project_button.clicked.connect(self.elegir_carpeta)
        side.addWidget(project_button)
        project_header = QHBoxLayout()
        self.project_label = QLabel("PROYECTO")
        project_header.addWidget(self.project_label, 1)
        close_files = QToolButton()
        close_files.setText("×")
        close_files.setToolTip("Ocultar explorador · Ctrl+B")
        close_files.clicked.connect(lambda: self.files_button.setChecked(False))
        project_header.addWidget(close_files)
        side.addLayout(project_header)
        self.tree = Explorador()
        self.tree.abrirArchivo.connect(self.abrir)
        self.tree.errorAcceso.connect(lambda mensaje: self.statusBar().showMessage(mensaje))
        side.addWidget(self.tree)
        horizontal.addWidget(sidebar)

        vertical = QSplitter(Qt.Orientation.Vertical)
        self.workspace_splitter = vertical
        self.terminals = []
        self.terminal_number = 0
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.setMovable(True)
        self.tabs.tabCloseRequested.connect(self.cerrar_tab)
        self.tabs.currentChanged.connect(self.actualizar_estado)
        editor_area = QWidget()
        editor_area.setMinimumSize(200, 160)
        editor_layout = QVBoxLayout(editor_area)
        editor_layout.setContentsMargins(0, 0, 0, 0)
        editor_layout.setSpacing(0)
        editor_layout.addWidget(self.tabs, 1)
        self.diagnostic_message = QLabel()
        self.diagnostic_message.setWordWrap(True)
        self.diagnostic_message.setTextFormat(Qt.TextFormat.PlainText)
        self.diagnostic_message.setStyleSheet("background: #392931; color: #ffc0ca; padding: 9px 14px; border-left: 2px solid #f08e9e;")
        self.diagnostic_message.hide()
        editor_layout.addWidget(self.diagnostic_message)
        vertical.addWidget(editor_area)
        self.panels = QTabWidget()
        self.panels.setMinimumSize(200, 120)
        self.panels.setTabsClosable(True)
        self.panels.setUsesScrollButtons(True)
        self.panels.tabCloseRequested.connect(self.cerrar_panel)
        self.problems = QTreeWidget()
        self.problems.setHeaderLabels(["Descripción", "Archivo", "Línea", "Columna", "Tipo", "Origen"])
        self.problems.setRootIsDecorated(False)
        self.problems.header().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.problems.itemActivated.connect(self.ir_problema)
        self.problems.itemClicked.connect(self.ir_problema)
        self.output = QPlainTextEdit()
        self.output.setReadOnly(True)
        self.output.setPlaceholderText("Aquí aparecerá la salida real de ccom al compilar.")
        self.auto_output = QPlainTextEdit()
        self.auto_output.setReadOnly(True)
        self.auto_output.setPlaceholderText("Estado y mensajes del último análisis automático. No se ha compilado ni guardado el documento.")
        self.clones_panel = PanelClones()
        self.clones_panel.solicitar.connect(self.analizar_clones)
        self.clones_panel.abrirFragmento.connect(self.ir_clone)
        self.panels.addTab(self.problems, "Problemas · 0")
        self.panels.addTab(self.output, "Salida de ccom")
        self.panels.addTab(self.clones_panel, "Clones")
        self.panels.addTab(self.auto_output, "Análisis de ccom")
        for i in range(4):
            from PySide6.QtWidgets import QTabBar
            self.panels.tabBar().setTabButton(i, QTabBar.ButtonPosition.RightSide, None)
            self.panels.tabBar().setTabButton(i, QTabBar.ButtonPosition.LeftSide, None)
        panel_controls = QWidget()
        controls = QHBoxLayout(panel_controls)
        controls.setContentsMargins(0, 0, 4, 0)
        controls.setSpacing(2)
        terminal_button = QToolButton()
        terminal_button.setText("＋")
        terminal_button.setToolTip("Nueva terminal · Ctrl+Shift+T")
        terminal_button.clicked.connect(self.nueva_terminal)
        controls.addWidget(terminal_button)
        self.move_panel_button = QToolButton()
        self.move_panel_button.setText("⇥")
        self.move_panel_button.setToolTip("Mover panel a la derecha")
        self.move_panel_button.clicked.connect(self.mover_panel)
        controls.addWidget(self.move_panel_button)
        self.panels.setCornerWidget(panel_controls, Qt.Corner.TopRightCorner)
        vertical.addWidget(self.panels)
        vertical.setStretchFactor(0, 1)
        vertical.setStretchFactor(1, 0)
        vertical.setSizes([500, 180])
        horizontal.addWidget(vertical)
        horizontal.setSizes([230, 1020])
        horizontal.setStretchFactor(1, 1)
        layout.addWidget(horizontal, 1)
        self.setCentralWidget(body)
        self.position = QLabel()
        self.analysis_state = QLabel()
        self.analysis_state.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Preferred)
        self.analysis_state.setContentsMargins(10, 0, 6, 0)
        self.analysis_state.setMaximumWidth(480)
        self.statusBar().addPermanentWidget(self.analysis_state, 1)
        self.statusBar().addPermanentWidget(self.position)
        self.statusBar().showMessage("Listo · abre un archivo o crea uno nuevo")

    def accion(self, menu, text, shortcut, callback):
        action = QAction(text, self)
        if shortcut:
            action.setShortcut(shortcut)
        action.triggered.connect(callback)
        menu.addAction(action)
        return action

    def crear_acciones(self):
        menu = self.menus.addMenu("Archivo")
        for text, key, callback in [
            ("Nuevo", "Ctrl+N", self.nuevo), ("Abrir archivo…", "Ctrl+O", self.elegir_archivo),
            ("Abrir carpeta…", "Ctrl+Shift+O", self.elegir_carpeta), ("Guardar", "Ctrl+S", self.guardar),
            ("Guardar como…", "Ctrl+Shift+S", lambda: self.guardar(como=True)),
            ("Recargar desde disco", "Ctrl+Shift+R", self.recargar),
            ("Cerrar pestaña", "Ctrl+W", lambda: self.cerrar_tab(self.tabs.currentIndex())),
            ("Salir", "Ctrl+Q", self.close),
        ]:
            self.accion(menu, text, key, callback)
        edit = self.menus.addMenu("Editar")
        for text, key, method in [("Deshacer", "Ctrl+Z", "undo"), ("Rehacer", "Ctrl+Shift+Z", "redo"),
                                  ("Cortar", "Ctrl+X", "cut"), ("Copiar", "Ctrl+C", "copy"), ("Pegar", "Ctrl+V", "paste")]:
            self.accion(edit, text, key, lambda checked=False, m=method: getattr(self.editor(), m)() if self.editor() else None)
        self.accion(edit, "Buscar…", "Ctrl+F", self.buscar)
        view = self.menus.addMenu("Ver")
        for text, widget in [("Explorador de archivos", self.sidebar), ("Panel de resultados", self.panels)]:
            action = QAction(text, self)
            action.setCheckable(True)
            action.setChecked(True)
            action.toggled.connect(widget.setVisible)
            view.addAction(action)
            if widget is self.sidebar:
                action.setShortcut("Ctrl+B")
                self.files_button.toggled.connect(action.setChecked)
                action.toggled.connect(self.files_button.setChecked)
            else:
                self.results_action = action
        self.accion(view, "Fuente del editor…", None, self.configurar_fuente)
        self.accion(view, "Enfocar explorador", "Ctrl+Shift+E", self.enfocar_explorador)
        self.accion(view, "Mover panel abajo / derecha", "Ctrl+Alt+J", self.mover_panel)
        self.accion(view, "Nueva terminal", "Ctrl+Shift+T", self.nueva_terminal)
        self.accion(view, "Analizar clones de la carpeta", "Ctrl+Shift+D", self.analizar_clones)
        self.auto_action = QAction("Análisis sintáctico y semántico automático", self)
        self.auto_action.setCheckable(True)
        self.auto_action.setChecked(True)
        self.auto_action.toggled.connect(self.activar_comprobacion)
        view.addAction(self.auto_action)
        build = self.menus.addMenu("Compilar")
        self.compile_action = self.accion(build, "Compilar archivo activo", "F5", self.compilar)
        self.accion(build, "Configurar ccom…", None, self.configurar_ccom)

    def mover_panel(self):
        right = self.workspace_splitter.orientation() == Qt.Orientation.Vertical
        self.workspace_splitter.setOrientation(Qt.Orientation.Horizontal if right else Qt.Orientation.Vertical)
        self.workspace_splitter.setSizes([650, 450] if right else [500, 180])
        self.move_panel_button.setText("⇣" if right else "⇥")
        self.move_panel_button.setToolTip("Mover panel abajo" if right else "Mover panel a la derecha")

    def analizar_clones(self):
        buffers = {self.tabs.widget(i).path: self.tabs.widget(i).toPlainText() for i in range(self.tabs.count()) if self.tabs.widget(i).path is not None}
        self.results_action.setChecked(True)
        self.panels.setCurrentWidget(self.clones_panel)
        self.clones_panel.analizar(self.project, buffers)

    def ir_clone(self, fragment, snapshot):
        editor = next((self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).path == fragment.archivo), None)
        try:
            current = editor.toPlainText() if editor else archivos.leer(fragment.archivo)
        except (OSError, UnicodeError) as error:
            self.error("No se pudo abrir el clon", error)
            return
        if current != snapshot:
            self.clones_panel.invalidar(fragment.archivo)
            self.statusBar().showMessage("El archivo cambió desde el análisis. Vuelve a analizar los clones.")
            return
        editor = self.abrir(fragment.archivo)
        if editor:
            start = len(snapshot[:fragment.inicio].encode("utf-16-le")) // 2
            end = len(snapshot[:fragment.fin].encode("utf-16-le")) // 2
            cursor = editor.textCursor()
            cursor.setPosition(start)
            cursor.setPosition(end, QTextCursor.MoveMode.KeepAnchor)
            editor.setTextCursor(cursor)
            editor.centerCursor()
            editor.setFocus()

    def nueva_terminal(self):
        directory = self.project if self.project.is_dir() else self.root
        terminal = Terminal(directory)
        self.terminal_number += 1
        self.terminals.append(terminal)
        self.panels.addTab(terminal, f"Terminal {self.terminal_number}")
        self.results_action.setChecked(True)
        self.panels.setCurrentWidget(terminal)
        terminal.input.setFocus()

    def cerrar_panel(self, index):
        widget = self.panels.widget(index)
        if isinstance(widget, Terminal):
            answer = QMessageBox.question(self, "Cerrar terminal", "Se cerrará esta sesión y sus procesos. ¿Continuar?", QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No, QMessageBox.StandardButton.No)
            if answer != QMessageBox.StandardButton.Yes:
                return
            widget.detener()
            self.terminals.remove(widget)
            self.panels.removeTab(index)
            widget.deleteLater()

    def editor(self):
        return self.tabs.currentWidget()

    def nuevo(self):
        editor = Editor(self.root)
        self.aplicar_fuente(editor)
        self.tabs.addTab(editor, "Sin título")
        self.tabs.setCurrentWidget(editor)
        editor.document().modificationChanged.connect(lambda _: self.actualizar_titulo(editor))
        editor.cursorPositionChanged.connect(self.actualizar_estado)
        editor.contenidoCambiado.connect(lambda: self.edicion(editor))
        editor.analisisSolicitado.connect(self.analizador.solicitar)
        editor.estadoAnalisisCambiado.connect(self.actualizar_estado)
        editor.programar_analisis()
        editor.setFocus()
        return editor

    def actualizar_titulo(self, editor):
        index = self.tabs.indexOf(editor)
        if index >= 0:
            self.tabs.setTabText(index, (editor.path.name if editor.path else "Sin título") + (" ●" if editor.document().isModified() else "") + (" ⚠" if self.tiene_cambio_externo(editor) else ""))
            self.tabs.setTabToolTip(index, str(editor.path or "Documento sin guardar"))
        self.actualizar_estado()

    def actualizar_estado(self, *_):
        editor = self.editor()
        if editor:
            cursor = editor.textCursor()
            self.position.setText(f"Ln {cursor.blockNumber() + 1}, Col {cursor.positionInBlock() + 1}  ·  UTF-8  ·  Calser")
            self.setWindowTitle(f"{editor.path.name if editor.path else 'Sin título'} · CALSIDE")
            self.analysis_state.setText(f"Léxico: {editor.estado_analisis} · Análisis: {editor.estado_ccom}  ·  ")
            self.analysis_state.setToolTip(editor.estado_ccom)
            problems = editor.problemas_en_linea(cursor.blockNumber() + 1)
            self.diagnostic_message.setText("\n".join(f"{p.origen} · {p.categoria}: {p.descripcion}" for p in problems))
            self.diagnostic_message.setVisible(bool(problems))
        self.actualizar_aviso_externo()

    def edicion(self, editor):
        self.analizador.olvidar(editor.documento_id)
        self.comprobador.invalidar(editor.documento_id)
        self.auto_results.pop(editor, None)
        editor.estado_ccom = "Esperando léxico…" if self.auto_action.isChecked() else "Desactivado"
        self.actualizar_titulo(editor)
        if editor.path and editor.document().isModified():
            self.clones_panel.invalidar(editor.path)
        if editor in self.results:
            self.results.pop(editor)
        self.actualizar_problemas()
        if self.compilation and self.compilation[0] is editor:
            self.statusBar().showMessage("Documento modificado · los resultados anteriores pueden estar desactualizados")

    def recibir_analisis(self, result):
        editor = next((self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).documento_id == result.documento), None)
        if editor and editor.aplicar_analisis(result):
            self.actualizar_problemas()
            self.programar_comprobacion(editor)

    def fallo_analisis(self, documento, revision, message):
        editor = next((self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).documento_id == documento), None)
        if editor and editor.revision == revision:
            editor.estado_analisis = f"No disponible: {message}"
            self.actualizar_estado()

    def programar_comprobacion(self, editor):
        if not self.auto_action.isChecked():
            editor.estado_ccom = "Desactivado"
        elif not editor.toPlainText().strip():
            editor.estado_ccom = "Sin contenido"
        elif editor.analisis_revision != editor.revision:
            editor.estado_ccom = "Esperando léxico…"
        elif editor.errores_lexicos:
            editor.estado_ccom = "Corrige el léxico"
        else:
            self.comprobador.configured = self.configured_ccom
            self.comprobador.programar(SolicitudCcom(editor.documento_id, editor.revision, editor.toPlainText(), editor.path, self.project))
        self.actualizar_estado()

    def activar_comprobacion(self, enabled):
        self.comprobador.activar(enabled)
        self.auto_results.clear()
        for i in range(self.tabs.count()):
            self.programar_comprobacion(self.tabs.widget(i))
        self.actualizar_problemas()

    def editor_solicitud(self, request):
        return next((self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).documento_id == request.documento and self.tabs.widget(i).revision == request.revision and self.tabs.widget(i).path == request.fuente), None)

    def estado_comprobacion(self, request, state):
        editor = self.editor_solicitud(request)
        if editor:
            editor.estado_ccom = state
            self.actualizar_estado()

    def recibir_comprobacion(self, result):
        editor = self.editor_solicitud(result.solicitud)
        if editor:
            self.auto_results[editor] = list(result.problemas)
            editor.estado_ccom = f"{len(result.problemas)} error(es)" if result.problemas else "Sin errores"
            self.registro_comprobacion(result.solicitud, editor.estado_ccom, result.salida)
            self.actualizar_problemas()

    def fallo_comprobacion(self, request, message, log):
        editor = self.editor_solicitud(request)
        if editor:
            self.auto_results.pop(editor, None)
            editor.estado_ccom = "No disponible"
            self.registro_comprobacion(request, message, log)
            self.statusBar().showMessage(message)
            self.actualizar_problemas()

    def registro_comprobacion(self, request, message, log):
        name = str(request.fuente or "Sin título")
        self.auto_output.setPlainText(f"Análisis automático · {name} · revisión {request.revision}\nccom --check [copia temporal]\n{message}\n\n{log}")

    def actualizar_problemas(self):
        """Conserva diagnósticos de otros documentos y retira solo los obsoletos."""
        self.problems.clear()
        for index in range(self.tabs.count()):
            editor = self.tabs.widget(index)
            manual = self.results.get(editor, [])
            compiled = list(manual)
            manual_keys = {(p.linea, p.columna, p.categoria, p.descripcion) for p in manual}
            compiled.extend(p for p in self.auto_results.get(editor, []) if (p.linea, p.columna, p.categoria, p.descripcion) not in manual_keys)
            # ccom tiene prioridad si informa el mismo error léxico/rango de
            # esta revisión. Conservar sus errores semánticos/sintácticos.
            keys = set()
            for problem in compiled:
                if problem.categoria == "Léxico" and problem.archivo == editor.path:
                    cursor = editor.rango_problema(problem)
                    if cursor:
                        keys.add((cursor.selectionStart(), cursor.selectionEnd()))
            live = []
            for problem in editor.errores_lexicos:
                cursor = editor.rango_problema(problem)
                if cursor is None or (cursor.selectionStart(), cursor.selectionEnd()) not in keys:
                    live.append(problem)
            problems = list(compiled) + live
            editor.establecer_problemas([p for p in problems if p.documento == editor.documento_id or p.archivo == editor.path])
            for problem in problems:
                path = problem.archivo or editor.path
                name = path.name if path else f"Sin título · pestaña {index + 1}"
                item = QTreeWidgetItem([problem.descripcion, name, str(problem.linea), str(problem.columna), problem.categoria, problem.origen])
                item.setData(0, Qt.ItemDataRole.UserRole, problem)
                item.setToolTip(0, f"<b>{escape(problem.origen)} · {escape(problem.categoria)}</b><br>{escape(problem.descripcion)}<br>{escape(str(path or name))}:{problem.linea}:{problem.columna}")
                item.setToolTip(1, str(path or name))
                self.problems.addTopLevelItem(item)
        self.panels.setTabText(0, f"Problemas · {self.problems.topLevelItemCount()}")
        self.actualizar_estado()

    def elegir_archivo(self):
        filename, _ = QFileDialog.getOpenFileName(self, "Abrir programa", str(self.project), "Calser (*.cls);;Todos los archivos (*)")
        if filename:
            self.abrir(Path(filename))

    def abrir(self, path):
        path = Path(path).resolve()
        for index in range(self.tabs.count()):
            editor = self.tabs.widget(index)
            if editor.path == path:
                self.tabs.setCurrentIndex(index)
                return editor
        try:
            text, version = archivos.cargar(path)
        except (OSError, UnicodeError) as error:
            self.error("No se pudo abrir el archivo", error)
            return None
        editor = self.editor()
        if not editor or editor.path or editor.toPlainText() or editor.document().isModified():
            editor = self.nuevo()
        editor.path = path
        self.establecer_version(editor, version)
        editor.setPlainText(text)
        editor.document().setModified(False)
        self.actualizar_titulo(editor)
        return editor

    def elegir_carpeta(self):
        directory = QFileDialog.getExistingDirectory(self, "Abrir proyecto", str(self.project))
        if directory:
            self.cargar_carpeta(Path(directory))

    def cargar_carpeta(self, directory):
        directory = Path(directory).resolve()
        if not directory.is_dir() or not os.access(directory, os.R_OK | os.X_OK):
            self.error("No se pudo abrir el proyecto", "La carpeta no está disponible o no hay permisos para leerla.")
            return False
        self.clones_panel.invalidar()
        self.project = directory
        self.project_label.setText(directory.name)
        self.project_label.setToolTip(str(directory))
        self.tree.cargar(directory)
        self.statusBar().showMessage("Proyecto · carga en segundo plano", 5000)
        return True

    def enfocar_explorador(self):
        self.files_button.setChecked(True)
        self.tree.setFocus()

    def guardar(self, checked=False, como=False, editor=None):
        editor = editor or self.editor()
        if not editor:
            return False
        path = editor.path
        elegir = como or path is None
        if elegir:
            filename, _ = QFileDialog.getSaveFileName(self, "Guardar programa", str(path or self.project / "programa.cls"), "Calser (*.cls)", options=QFileDialog.Option.DontConfirmOverwrite)
            if not filename:
                return False
            path = Path(filename).resolve()
            if not path.suffix:
                path = path.with_suffix(".cls")
            if any(self.tabs.widget(i) is not editor and self.tabs.widget(i).path == path for i in range(self.tabs.count())):
                self.error("Archivo abierto", "Ese archivo ya está abierto en otra pestaña.")
                return False
        try:
            actual = archivos.estado(path)
            if editor.path == path:
                esperado = editor.estado_disco
                if actual != esperado:
                    editor.estado_observado, editor.error_disco = actual, ''
                    self.actualizar_titulo(editor)
                    accion = self.resolver_conflicto(editor, actual)
                    if accion == 'recargar':
                        self.recargar(editor=editor)
                        return False
                    if accion == 'copia':
                        return self.guardar(como=True, editor=editor)
                    if accion != 'sobrescribir':
                        return False
                    esperado = actual
            else:
                esperado = actual
                if actual.existe:
                    respuesta = QMessageBox.question(self, "Reemplazar archivo", f"¿Reemplazar {path}?", QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No, QMessageBox.StandardButton.No)
                    if respuesta != QMessageBox.StandardButton.Yes:
                        return False
            version = archivos.guardar(path, editor.toPlainText(), esperado=esperado)
        except (OSError, UnicodeError) as error:
            self.error("No se pudo guardar; tu texto se conserva", error)
            self.monitor.comprobar()
            return False
        if editor.path != path:
            self.clones_panel.invalidar()
            self.comprobador.invalidar(editor.documento_id)
            self.auto_results.pop(editor, None)
            self.results.pop(editor, None)
            editor.establecer_problemas([])
        editor.path = path
        self.establecer_version(editor, version)
        editor.document().setModified(False)
        self.actualizar_titulo(editor)
        self.actualizar_problemas()
        self.programar_comprobacion(editor)
        self.statusBar().showMessage(f"Guardado · {path}")
        return True

    def confirmar_cierre(self, editor):
        if not editor.document().isModified():
            return True
        answer = QMessageBox.question(self, "Cambios sin guardar", f"¿Guardar los cambios en {editor.path.name if editor.path else 'Sin título'}?", QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel, QMessageBox.StandardButton.Save)
        if answer == QMessageBox.StandardButton.Cancel:
            return False
        return answer == QMessageBox.StandardButton.Discard or self.guardar(editor=editor)

    def cerrar_tab(self, index):
        if index < 0:
            return
        editor = self.tabs.widget(index)
        if self.process and self.compilation[0] is editor:
            self.statusBar().showMessage("Cancela la compilación antes de cerrar este documento")
            return
        if self.confirmar_cierre(editor):
            editor.detener_analisis()
            self.analizador.olvidar(editor.documento_id)
            self.comprobador.invalidar(editor.documento_id)
            self.auto_results.pop(editor, None)
            if editor.path:
                self.clones_panel.invalidar(editor.path)
            self.results.pop(editor, None)
            self.tabs.removeTab(index)
            self.actualizar_problemas()
            editor.deleteLater()
            if not self.tabs.count():
                self.nuevo()

    def buscar(self):
        text, ok = QInputDialog.getText(self, "Buscar", "Texto:")
        editor = self.editor()
        if ok and text and editor and not editor.find(text):
            cursor = editor.textCursor()
            cursor.movePosition(QTextCursor.MoveOperation.Start)
            editor.setTextCursor(cursor)
            if not editor.find(text):
                self.statusBar().showMessage("No se encontraron coincidencias")

    def configurar_ccom(self):
        filename, _ = QFileDialog.getOpenFileName(self, "Seleccionar ejecutable ccom", str(self.root))
        if filename:
            self.configured_ccom = filename
            for i in range(self.tabs.count()):
                editor = self.tabs.widget(i)
                self.auto_results.pop(editor, None)
                self.programar_comprobacion(editor)
            self.actualizar_problemas()

    def compilar(self):
        if self.process:
            return
        editor = self.editor()
        executable = ccom.ejecutable(self.root, self.configured_ccom)
        if not executable:
            self.error("ccom no disponible", "Ejecuta make en el repositorio o selecciona el ejecutable en Compilar → Configurar ccom.")
            return
        if not editor or ((editor.path is None or editor.document().isModified()) and not self.guardar(editor=editor)):
            return
        try:
            if archivos.estado(editor.path) != editor.estado_disco:
                if not self.guardar(editor=editor):
                    return
            self.compilation_disk = editor.estado_disco
            destination = editor.path.parent / "build" / "ide" / hashlib.sha256(str(editor.path).encode()).hexdigest()[:10]
            destination.mkdir(parents=True, exist_ok=True)
        except OSError as error:
            self.error("No se pudo crear la carpeta de salida", error)
            return
        self.stderr = ""
        self.stdout_decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        self.stderr_decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        self.compilation = (editor, editor.revision, editor.path)
        self.comprobador.invalidar(editor.documento_id)
        self.comprobador.suspender(True)
        self.auto_results.pop(editor, None)
        editor.estado_ccom = "Compilando con F5"
        self.results.pop(editor, None)
        self.actualizar_problemas()
        arguments = [str(editor.path), "-o", str(destination / (editor.path.stem + ".bin")), "-s"]
        self.output.setPlainText(f"Ejecutable: {executable}\nArgumentos: {arguments}\nDirectorio: {editor.path.parent}\n\n")
        self.panels.setCurrentWidget(self.output)
        self.process = QProcess(self)
        self.process.setWorkingDirectory(str(editor.path.parent))
        self.process.readyReadStandardOutput.connect(self.leer_stdout)
        self.process.readyReadStandardError.connect(self.leer_stderr)
        self.process.finished.connect(self.terminar)
        self.process.errorOccurred.connect(self.error_proceso)
        self.compile_button.setEnabled(False)
        self.compile_action.setEnabled(False)
        self.stop_button.setEnabled(True)
        self.statusBar().showMessage("Compilando… puedes seguir editando")
        self.timer.start(30000)
        self.process.start(executable, arguments)

    def escribir_salida(self, text):
        cursor = self.output.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(text)
        self.output.setTextCursor(cursor)

    def leer_stdout(self):
        self.escribir_salida(self.stdout_decoder.decode(bytes(self.process.readAllStandardOutput())))

    def leer_stderr(self):
        text = self.stderr_decoder.decode(bytes(self.process.readAllStandardError()))
        self.stderr += text
        self.escribir_salida(text)

    def terminar(self, code, status):
        if not self.process:
            return
        self.leer_stdout()
        self.leer_stderr()
        self.escribir_salida(self.stdout_decoder.decode(b"", final=True))
        tail = self.stderr_decoder.decode(b"", final=True)
        self.stderr += tail
        self.escribir_salida(tail)
        editor, revision, source = self.compilation
        current = self.tabs.indexOf(editor) >= 0 and editor.revision == revision and editor.path == source
        try:
            current = current and archivos.estado(source) == self.compilation_disk
        except OSError:
            current = False
        crashed = status == QProcess.ExitStatus.CrashExit
        if current:
            self.results[editor] = ccom.diagnosticos(self.stderr, source, source.parent)
            self.actualizar_problemas()
            if self.problems.topLevelItemCount():
                self.panels.setCurrentWidget(self.problems)
        message = "Proceso interrumpido" if crashed else ("Compilación correcta" if code == 0 else f"Compilación fallida · código {code}")
        if not current:
            message += " · resultado de una versión anterior; vuelve a compilar"
        self.escribir_salida(f"\n{message}\n")
        self.statusBar().showMessage(message)
        self.liberar_proceso()

    def error_proceso(self, error):
        if error == QProcess.ProcessError.FailedToStart:
            message = self.process.errorString()
            self.escribir_salida(f"\nNo se pudo iniciar ccom: {message}\n")
            self.statusBar().showMessage("No se pudo iniciar el compilador")
            self.liberar_proceso()

    def liberar_proceso(self):
        self.timer.stop()
        self.process.deleteLater()
        self.process = None
        self.comprobador.suspender(False)
        if self.compilation:
            editor, revision, source = self.compilation
            if self.tabs.indexOf(editor) >= 0 and editor.revision == revision:
                editor.estado_ccom = "Ver compilación" if self.auto_action.isChecked() else "Desactivado"
                self.actualizar_estado()
        self.compile_button.setEnabled(True)
        self.compile_action.setEnabled(True)
        self.stop_button.setEnabled(False)

    def cancelar(self):
        if self.process:
            self.process.kill()

    def agotar_tiempo(self):
        self.escribir_salida("\nTiempo límite de 30 segundos alcanzado.\n")
        self.cancelar()

    def ir_problema(self, item, column):
        problem = item.data(0, Qt.ItemDataRole.UserRole)
        editor = next((self.tabs.widget(i) for i in range(self.tabs.count()) if self.tabs.widget(i).documento_id == problem.documento), None)
        if editor:
            self.tabs.setCurrentWidget(editor)
        elif problem.archivo is not None:
            editor = self.abrir(problem.archivo)
        if editor:
            cursor = editor.rango_problema(problem)
            if cursor is None:
                return
            editor.setTextCursor(cursor)
            editor.centerCursor()
            editor.setFocus()

    def error(self, title, error):
        QMessageBox.warning(self, title, str(error))

    def closeEvent(self, event):
        if self.process:
            self.statusBar().showMessage("Cancela la compilación antes de cerrar el IDE")
            event.ignore()
            return
        for index in range(self.tabs.count()):
            editor = self.tabs.widget(index)
            self.tabs.setCurrentIndex(index)
            if not self.confirmar_cierre(editor):
                event.ignore()
                return
        if any(t.activa() for t in self.terminals):
            answer = QMessageBox.question(self, "Terminales activas", "Al salir se cerrarán las sesiones de terminal y sus procesos. ¿Salir?", QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No, QMessageBox.StandardButton.No)
            if answer != QMessageBox.StandardButton.Yes:
                event.ignore()
                return
        if not self.monitor.detener():
            self.statusBar().showMessage("Terminando comprobación de archivos… intenta cerrar de nuevo")
            event.ignore()
            return
        if not self.clones_panel.detener():
            self.monitor.timer.start()
            self.statusBar().showMessage("Cancelando análisis de clones… intenta cerrar de nuevo en un momento")
            event.ignore()
            return
        if not self.analizador.detener():
            self.monitor.timer.start()
            self.statusBar().showMessage("Terminando análisis léxico… intenta cerrar de nuevo en un momento")
            event.ignore()
            return
        if not self.comprobador.detener():
            self.monitor.timer.start()
            self.analizador.cerrando = False
            for index in range(self.tabs.count()):
                self.tabs.widget(index).programar_analisis()
            self.statusBar().showMessage("Terminando análisis de ccom… intenta cerrar de nuevo")
            event.ignore()
            return
        for index in range(self.tabs.count()):
            self.tabs.widget(index).detener_analisis()
        for terminal in self.terminals:
            terminal.detener()
        self.guardar_preferencias()
        event.accept()

    def changeEvent(self, event):
        super().changeEvent(event)
        from PySide6.QtCore import QEvent
        if event.type() == QEvent.Type.ActivationChange and self.isActiveWindow() and hasattr(self, 'monitor'):
            self.monitor.comprobar()
