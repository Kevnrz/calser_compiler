"""Barra del proyecto y resultados de búsqueda con tareas cancelables."""
from pathlib import Path
from PySide6.QtCore import QEvent, QSize, QThread, QTimer, Qt, Signal
from PySide6.QtWidgets import (QComboBox, QHBoxLayout, QLabel, QLineEdit, QSizePolicy,
                              QStackedWidget, QToolButton, QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget)
from ide.busqueda import buscar_proyecto, ResultadoBusqueda
from ide.interfaz.iconos import icono_archivo


class TareaBusqueda(QThread):
    resultado = Signal(int, object)

    def __init__(self, generacion, argumentos, parent):
        super().__init__(parent)
        self.generacion, self.argumentos = generacion, argumentos

    def run(self):
        try:
            resultado = buscar_proyecto(**self.argumentos, cancelar=self.isInterruptionRequested)
        except (OSError, ValueError) as error:
            resultado = ResultadoBusqueda(avisos=(str(error),))
        if not self.isInterruptionRequested():
            self.resultado.emit(self.generacion, resultado)


class BusquedaProyecto(QWidget):
    abrirResultado = Signal(object)

    def __init__(self, explorador, fuentes, parent=None):
        super().__init__(parent)
        self.explorador, self.fuentes = explorador, fuentes
        self.raiz = Path.cwd()
        self.task = None
        self.generacion = 0
        self.pendiente = False
        self.ultimo = None
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Maximum)
        layout = QVBoxLayout(self); layout.setContentsMargins(0, 0, 0, 0); layout.setSpacing(4)
        self.campo = QLineEdit()
        self.campo.setPlaceholderText('Buscar en el proyecto…')
        self.campo.setClearButtonEnabled(True)
        self.campo.setToolTip('Nombres/rutas de archivos .cls, carpetas y código · Ctrl+Shift+F · Esc limpia')
        self.campo.setAccessibleName('Buscar en el proyecto')
        self.campo.installEventFilter(self)
        layout.addWidget(self.campo)
        self.opciones = QWidget(); fila = QHBoxLayout(self.opciones); fila.setContentsMargins(0, 0, 0, 0)
        self.modo = QComboBox()
        for texto, dato in [('Todo', 'todo'), ('Archivos', 'archivos'), ('Carpetas', 'carpetas'), ('Código', 'codigo')]:
            self.modo.addItem(texto, dato)
        self.modo.setToolTip('Dónde buscar')
        fila.addWidget(self.modo, 1)
        self.mayusculas = QToolButton(); self.mayusculas.setText('Aa'); self.mayusculas.setCheckable(True)
        self.mayusculas.setToolTip('Distinguir mayúsculas y minúsculas')
        self.mayusculas.setStyleSheet('QToolButton:checked { background: #504135; color: #efbb65; }')
        fila.addWidget(self.mayusculas)
        self.refrescar = QToolButton(); self.refrescar.setText('↻'); self.refrescar.setToolTip('Actualizar resultados')
        fila.addWidget(self.refrescar)
        layout.addWidget(self.opciones); self.opciones.hide()
        self.paginas = QStackedWidget()
        self.paginas.addWidget(explorador)
        self.panel = QWidget(); resultados_layout = QVBoxLayout(self.panel)
        resultados_layout.setContentsMargins(0, 0, 0, 0); resultados_layout.setSpacing(4)
        self.estado = QLabel(); self.estado.setWordWrap(True); self.estado.setTextFormat(Qt.TextFormat.PlainText)
        self.estado.setStyleSheet('color: #aeb8c9; font-size: 11px;')
        resultados_layout.addWidget(self.estado)
        self.resultados = QTreeWidget(); self.resultados.setHeaderHidden(True)
        self.resultados.setUniformRowHeights(True)
        self.resultados.setIconSize(QSize(20, 20))
        self.resultados.setToolTip('Doble clic o Enter para abrir un resultado')
        self.resultados.itemActivated.connect(self.activar)
        self.resultados.installEventFilter(self)
        resultados_layout.addWidget(self.resultados, 1)
        self.paginas.addWidget(self.panel)
        self.timer = QTimer(self); self.timer.setSingleShot(True); self.timer.setInterval(240)
        self.timer.timeout.connect(self.iniciar)
        self.campo.textChanged.connect(self.programar)
        self.campo.returnPressed.connect(self.iniciar)
        self.modo.currentIndexChanged.connect(self.programar)
        self.mayusculas.toggled.connect(self.programar)
        self.refrescar.clicked.connect(self.iniciar)

    def cambiar_proyecto(self, raiz):
        self.raiz = Path(raiz)
        self.programar()

    def actualizar_si_activa(self, *_):
        if self.campo.text().strip():
            self.programar()

    def programar(self, *_):
        self.generacion += 1
        self.pendiente = False
        self.timer.stop()
        if self.task:
            self.task.requestInterruption()
        activa = bool(self.campo.text().strip())
        self.opciones.setVisible(activa)
        self.paginas.setCurrentWidget(self.panel if activa else self.explorador)
        if activa:
            self.estado.setText('Buscando…')
            self.resultados.clear()
            self.timer.start()
        else:
            self.ultimo = None

    def iniciar(self):
        self.timer.stop()
        if not self.campo.text().strip():
            return
        self.generacion += 1
        self.pendiente = True
        self.estado.setText('Buscando…')
        if self.task:
            self.task.requestInterruption()
            return
        self.pendiente = False
        argumentos = dict(raiz=self.raiz, consulta=self.campo.text(), modo=self.modo.currentData(),
                          mayusculas=self.mayusculas.isChecked(), buffers=self.fuentes())
        self.task = TareaBusqueda(self.generacion, argumentos, self)
        self.task.resultado.connect(self.recibir, Qt.ConnectionType.QueuedConnection)
        self.task.finished.connect(self.terminar, Qt.ConnectionType.QueuedConnection)
        self.task.start()

    def recibir(self, generacion, resultado):
        if generacion != self.generacion or resultado.cancelado:
            return
        self.ultimo = resultado
        self.resultados.clear()
        grupos, archivos_codigo = {}, {}
        for hit in resultado.coincidencias:
            if hit.tipo not in grupos:
                texto = {'archivo': 'Archivos', 'carpeta': 'Carpetas', 'codigo': 'Código'}[hit.tipo]
                grupos[hit.tipo] = QTreeWidgetItem(self.resultados, [texto])
                grupos[hit.tipo].setExpanded(True)
            relativo = str(hit.archivo.relative_to(self.raiz))
            padre = grupos[hit.tipo]
            if hit.tipo == 'codigo':
                if hit.archivo not in archivos_codigo:
                    archivos_codigo[hit.archivo] = QTreeWidgetItem(padre, [relativo])
                    archivos_codigo[hit.archivo].setIcon(0, icono_archivo(hit.archivo))
                    archivos_codigo[hit.archivo].setExpanded(True)
                padre = archivos_codigo[hit.archivo]
                inicio = max(0, hit.columna - 45)
                fragmento = hit.texto[inicio:inicio + 160].strip()
                texto = f'{hit.linea}:{hit.columna}  {"…" if inicio else ""}{fragmento}'
            else:
                texto = relativo
            item = QTreeWidgetItem(padre, [texto])
            if hit.tipo == 'archivo':
                item.setIcon(0, icono_archivo(hit.archivo))
            item.setData(0, Qt.ItemDataRole.UserRole, hit)
            item.setToolTip(0, f'{hit.archivo}' + (f':{hit.linea}:{hit.columna}\n{hit.texto[:500]}' if hit.linea else ''))
        mensaje = (f'{len(resultado.coincidencias)} coincidencias' if resultado.coincidencias else 'Sin coincidencias') + ' · incluye cambios sin guardar'
        if resultado.avisos:
            mensaje += '\n' + '\n'.join(resultado.avisos)
        self.estado.setText(mensaje)

    def terminar(self):
        tarea, self.task = self.task, None
        tarea.deleteLater()
        if self.pendiente:
            self.iniciar()

    def activar(self, item, columna=0):
        hit = item.data(0, Qt.ItemDataRole.UserRole)
        if hit is not None:
            self.abrirResultado.emit(hit)
        else:
            item.setExpanded(not item.isExpanded())

    def enfocar(self):
        self.campo.setFocus(); self.campo.selectAll()

    def eventFilter(self, widget, event):
        if event.type() == QEvent.Type.KeyPress:
            if event.key() == Qt.Key.Key_Escape:
                self.campo.clear(); self.explorador.setFocus(); return True
            if widget is self.campo and event.key() == Qt.Key.Key_Down:
                self.resultados.setFocus()
                if self.resultados.topLevelItemCount():
                    item = self.resultados.topLevelItem(0)
                    while item and item.data(0, Qt.ItemDataRole.UserRole) is None and item.childCount():
                        item = item.child(0)
                    self.resultados.setCurrentItem(item)
                return True
        return super().eventFilter(widget, event)

    def detener(self):
        self.timer.stop(); self.pendiente = False; self.generacion += 1
        if self.task:
            self.task.requestInterruption()
            return self.task.wait(1500)
        return True
