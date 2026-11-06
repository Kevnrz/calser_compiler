"""Árbol lazy de Qt: no recorre recursivamente el proyecto en el hilo gráfico."""
from collections import deque
from pathlib import Path
import os
from PySide6.QtCore import QDir, QEvent, QItemSelectionModel, QMimeData, QPoint, QRect, QSize, QSortFilterProxyModel, QTimer, Qt, QUrl, Signal
from PySide6.QtGui import QDrag, QPainter, QPen
from PySide6.QtWidgets import QFileSystemModel, QTreeView, QRubberBand
from ide.interfaz.orden_explorador import OrdenExplorador

from ide.busqueda import EXCLUIDAS
from ide.interfaz.iconos import icono_cls


class FiltroProyecto(QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.raiz = ''
        self.orden = {}
        self._rangos = {}
        self.setDynamicSortFilter(True)

    def actualizar_orden(self):
        self._rangos = {padre: {nombre: i for i, nombre in enumerate(nombres)}
                        for padre, nombres in self.orden.items()}
        self.invalidate()
        self.sort(0, Qt.SortOrder.AscendingOrder)

    def lessThan(self, izquierda, derecha):
        modelo = self.sourceModel()
        padre = modelo.filePath(izquierda.parent())
        rangos = self._rangos.get(padre)
        if rangos:
            a = rangos.get(modelo.fileName(izquierda), len(rangos))
            b = rangos.get(modelo.fileName(derecha), len(rangos))
            if a != b:
                return a < b
        return super().lessThan(izquierda, derecha)

    def data(self, indice, role=Qt.ItemDataRole.DisplayRole):
        if role == Qt.ItemDataRole.DecorationRole and indice.isValid() and indice.column() == 0:
            info = self.sourceModel().fileInfo(self.mapToSource(indice))
            if info.suffix() == 'cls' and not info.isDir() and not info.isSymLink():
                return icono_cls()
        return super().data(indice, role)

    def filterAcceptsRow(self, row, parent):
        model = self.sourceModel()
        info = model.fileInfo(model.index(row, 0, parent))
        if info.isSymLink():
            return False
        if info.isDir():
            ruta = info.absoluteFilePath()
            ancestro = self.raiz.startswith(ruta.rstrip('/') + '/')
            return ruta == self.raiz or ancestro or info.fileName() not in EXCLUIDAS
        return info.suffix() == 'cls'


class Explorador(OrdenExplorador, QTreeView):
    abrirArchivo = Signal(object)
    menuSolicitado = Signal(object, object)
    operacionSolicitada = Signal(str, object)
    trasladoSolicitado = Signal(object, object)
    errorAcceso = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ramas = set()
        self._cache_ramas = {}
        self._cache_orden = {}
        self._restaurar = set()
        self._revelar = None
        self._cambiando_raiz = False
        self._expansion_total = False
        self._cola = deque()
        self._cargadas = set()
        self._exploradas = set()
        self._expandidas = set()
        self._timer_expansion = QTimer(self)
        self._timer_expansion.setSingleShot(True)
        self._timer_expansion.timeout.connect(self._paso_expansion)
        self._timer_restaurar = QTimer(self)
        self._timer_restaurar.setSingleShot(True)
        self._timer_restaurar.timeout.connect(self._paso_restauracion)
        self.archivos = QFileSystemModel(self)
        self.archivos.setReadOnly(True)
        self.archivos.setFilter(QDir.Filter.AllDirs | QDir.Filter.Files |
                                QDir.Filter.NoDotAndDotDot | QDir.Filter.NoSymLinks)
        self.archivos.setNameFilters(['*.cls'])
        self.archivos.setNameFilterDisables(False)
        self.filtro = FiltroProyecto(self)
        self.filtro.setSourceModel(self.archivos)
        self.setModel(self.filtro)
        self.setSelectionMode(QTreeView.SelectionMode.ExtendedSelection)
        self.setSelectionBehavior(QTreeView.SelectionBehavior.SelectRows)
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDragDropMode(QTreeView.DragDropMode.DragDrop)
        self.setDefaultDropAction(Qt.DropAction.MoveAction)
        self.setAutoExpandDelay(-1)
        self._timer_hover = QTimer(self)
        self._timer_hover.setSingleShot(True)
        self._timer_hover.timeout.connect(self._expandir_destino_arrastre)
        self._banda = QRubberBand(QRubberBand.Shape.Rectangle, self.viewport())
        self._inicio_banda = None
        self._seleccion_previa = set()
        self._destino_drop = None
        self._intencion_drop = None
        self.setDropIndicatorShown(False)
        self.setIconSize(QSize(20, 20))
        self.setHeaderHidden(True)
        for columna in (1, 2, 3):
            self.hideColumn(columna)
        self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.SortOrder.AscendingOrder)
        self.setEditTriggers(QTreeView.EditTrigger.NoEditTriggers)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self.menu_contextual)
        self.activated.connect(self.abrir)
        self.expanded.connect(self.verificar_acceso)
        self.expanded.connect(self._registrar_abierta)
        self.collapsed.connect(self._registrar_cerrada)
        self.archivos.directoryLoaded.connect(self._directorio_cargado)
        self.archivos.rowsInserted.connect(self._filas_insertadas)
        self.setToolTip('Subcarpetas y archivos .cls · se excluyen carpetas de compilación, dependencias y enlaces simbólicos')

    def cargar(self, path):
        if self.filtro.raiz:
            self._cache_ramas[self.filtro.raiz] = set(self.ramas)
            self._cache_orden[self.filtro.raiz] = self.orden_presentacion()
        self._cambiando_raiz = True
        self._expansion_total = False
        self._cola.clear()
        self._timer_expansion.stop()
        self._revelar = None
        moderna = all(hasattr(self.filtro, nombre) for nombre in ('beginFilterChange', 'endFilterChange'))
        if moderna:
            self.filtro.beginFilterChange()
        self.filtro.raiz = str(Path(path).resolve())
        if moderna:
            self.filtro.endFilterChange(QSortFilterProxyModel.Direction.Rows)
        else:  # Qt 6.8: API anterior, todavía soportada por requirements.txt.
            self.filtro.invalidateRowsFilter()
        indice = self.archivos.setRootPath(self.filtro.raiz)
        self.setRootIndex(self.filtro.mapFromSource(indice))
        self.restaurar_orden(self._cache_orden.get(self.filtro.raiz, {}))
        self.ramas = set(self._cache_ramas.get(self.filtro.raiz, ()))
        self._restaurar = set(self.ramas)
        self._cambiando_raiz = False
        self._timer_restaurar.start(0)

    def abrir(self, indice):
        fuente = self.filtro.mapToSource(indice)
        if not self.archivos.isDir(fuente):
            self.abrirArchivo.emit(Path(self.archivos.filePath(fuente)))

    def verificar_acceso(self, indice):
        fuente = self.filtro.mapToSource(indice)
        ruta = self.archivos.filePath(fuente)
        if self.archivos.canFetchMore(fuente):
            self.archivos.fetchMore(fuente)
        if not os.access(ruta, os.R_OK | os.X_OK):
            self.errorAcceso.emit(f'No se puede leer la carpeta: {ruta}')

    def menu_contextual(self, posicion):
        indice = self.currentIndex() if posicion.x() < 0 or posicion.y() < 0 else self.indexAt(posicion)
        if indice.isValid():
            if not self.selectionModel().isSelected(indice):
                self.setCurrentIndex(indice)
            else:
                self.selectionModel().setCurrentIndex(indice, QItemSelectionModel.SelectionFlag.NoUpdate)
            path = Path(self.archivos.filePath(self.filtro.mapToSource(indice)))
        else:
            path = Path(self.filtro.raiz)
        self.menuSolicitado.emit(path, self.viewport().mapToGlobal(posicion))

    def rutas_seleccionadas(self):
        return [self._path(i) for i in self.selectionModel().selectedRows(0)]

    def mousePressEvent(self, event):
        punto = event.position().toPoint()
        if event.button() == Qt.MouseButton.LeftButton and not self.indexAt(punto).isValid():
            self._inicio_banda = punto
            self._seleccion_previa = set(self.rutas_seleccionadas()) if event.modifiers() & (
                Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier) else set()
            if not self._seleccion_previa:
                self.clearSelection()
            self._banda.setGeometry(QRect(punto, punto))
            event.accept()
            return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._inicio_banda is not None:
            rect = QRect(self._inicio_banda, event.position().toPoint()).normalized()
            self._banda.setGeometry(rect)
            self._banda.show()
            seleccion = self.selectionModel()
            seleccion.clearSelection()
            indice = self.indexAt(QPoint(1, 0))
            if not indice.isValid():
                indice = self.model().index(0, 0, self.rootIndex())
            while indice.isValid():
                fila = self.visualRect(indice)
                if fila.top() > self.viewport().height():
                    break
                fila.setLeft(0)
                fila.setRight(self.viewport().width())
                if fila.intersects(rect) or self._path(indice) in self._seleccion_previa:
                    seleccion.select(indice, QItemSelectionModel.SelectionFlag.Select | QItemSelectionModel.SelectionFlag.Rows)
                indice = self.indexBelow(indice)
            event.accept()
            return
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self._inicio_banda is not None and event.button() == Qt.MouseButton.LeftButton:
            self._inicio_banda = None
            self._banda.hide()
            event.accept()
            return
        super().mouseReleaseEvent(event)

    def startDrag(self, supportedActions):
        rutas = self.rutas_seleccionadas()
        if not rutas:
            return
        mime = QMimeData()
        mime.setUrls([QUrl.fromLocalFile(str(p)) for p in rutas])
        drag = QDrag(self)
        drag.setMimeData(mime)
        drag.setPixmap(self.imagen_arrastre(rutas))
        drag.setHotSpot(QPoint(-14, -12))
        try:
            drag.exec(Qt.DropAction.MoveAction, Qt.DropAction.MoveAction)
        finally:
            self._timer_hover.stop()
            self._intencion_drop = None
            self._destino_drop = None
            self.viewport().update()
            drag.deleteLater()

    def destino_arrastre(self, event):
        intencion = self.intencion_arrastre(event)
        return intencion[1] if intencion and intencion[0] == 'dentro' else None

    def dragEnterEvent(self, event):
        if event.source() is self:
            event.setDropAction(Qt.DropAction.MoveAction)
            event.accept()
        else:
            event.ignore()

    def dragMoveEvent(self, event):
        # Qt mantiene el autodesplazamiento. Solo el centro abre carpetas al esperar.
        super().dragMoveEvent(event)
        anterior = self._intencion_drop
        self._intencion_drop = self.intencion_arrastre(event)
        if anterior != self._intencion_drop:
            self._timer_hover.stop()
            if self._intencion_drop and self._intencion_drop[0] == 'dentro':
                self._timer_hover.start(650)
        self._destino_drop = self.destino_arrastre(event)
        self.viewport().update()
        if self._intencion_drop is not None:
            event.setDropAction(Qt.DropAction.MoveAction)
            event.accept()
        else:
            event.ignore()

    def _expandir_destino_arrastre(self):
        if self._intencion_drop and self._intencion_drop[0] == 'dentro':
            self.expandir_ruta(self._intencion_drop[1])

    def dragLeaveEvent(self, event):
        self._timer_hover.stop()
        self._intencion_drop = None
        self._destino_drop = None
        self.viewport().update()
        super().dragLeaveEvent(event)

    def dropEvent(self, event):
        self._timer_hover.stop()
        intencion = self.intencion_arrastre(event)
        self._intencion_drop = None
        self._destino_drop = None
        self.viewport().update()
        if intencion is None:
            event.ignore()
            return
        modo, destino = intencion
        rutas = [Path(u.toLocalFile()) for u in event.mimeData().urls()]
        if modo == 'dentro':
            self.trasladoSolicitado.emit(rutas, destino)
        elif not self.reordenar(rutas, destino, modo == 'despues'):
            event.ignore()
            return
        event.setDropAction(Qt.DropAction.MoveAction)
        event.accept()

    def paintEvent(self, event):
        super().paintEvent(event)
        if self._intencion_drop is not None:
            modo, destino = self._intencion_drop
            rect = self.visualRect(self.indice_ruta(destino))
            if destino == Path(self.filtro.raiz):
                rect = self.viewport().rect()
            painter = QPainter(self.viewport())
            painter.setPen(QPen(self.palette().highlight().color(), 2))
            if modo == 'dentro':
                painter.drawRect(rect.adjusted(1, 1, -2, -2))
            else:
                y = rect.top() + 1 if modo == 'antes' else rect.bottom() - 1
                painter.drawLine(rect.left(), y, self.viewport().width() - 3, y)
                painter.drawEllipse(QPoint(rect.left() + 2, y), 2, 2)
            painter.end()

    def _path(self, indice):
        return Path(self.archivos.filePath(self.filtro.mapToSource(indice)))

    def ruta_actual(self):
        seleccion = self.rutas_seleccionadas()
        actual = self._path(self.currentIndex()) if self.currentIndex().isValid() else Path(self.filtro.raiz)
        return seleccion[0] if seleccion and actual not in seleccion else actual

    def indice_ruta(self, path):
        return self.filtro.mapFromSource(self.archivos.index(str(path)))

    def _en_proyecto(self, path):
        return bool(self.filtro.raiz and Path(path).is_relative_to(Path(self.filtro.raiz)))

    def _registrar_abierta(self, indice):
        if not self._cambiando_raiz:
            path = self._path(indice)
            if self._en_proyecto(path) and str(path) != self.filtro.raiz:
                self.ramas.add(path)

    def _registrar_cerrada(self, indice):
        if not self._cambiando_raiz:
            path = self._path(indice)
            self.ramas = {p for p in self.ramas if not p.is_relative_to(path)}
            self._restaurar = {p for p in self._restaurar if not p.is_relative_to(path)}
            self._expansion_total = False
            self._cola.clear()
            self._timer_expansion.stop()

    def expandir_ruta(self, path):
        path = Path(path)
        if not self._en_proyecto(path):
            return
        raiz = Path(self.filtro.raiz)
        for parent in reversed((path,) + tuple(path.parents)):
            if parent != raiz and parent.is_relative_to(raiz):
                indice = self.indice_ruta(parent)
                if indice.isValid():
                    self.expand(indice)

    def contraer_ruta(self, path):
        indice = self.indice_ruta(path)
        if indice == self.rootIndex():
            self.contraer_todo()
        elif indice.isValid():
            self.collapse(indice)

    def contraer_todo(self):
        self._expansion_total = False
        self._cola.clear()
        self._timer_expansion.stop()
        self._restaurar.clear()
        self.ramas.clear()
        self.collapseAll()

    def expandir_todo(self):
        if not self.filtro.raiz:
            return
        self._expansion_total = True
        self._exploradas.clear()
        self._expandidas.clear()
        self._cola = deque([(Path(self.filtro.raiz), 0)])
        self._timer_expansion.start(0)

    def _filas_insertadas(self, padre, inicio, fin):
        self._timer_restaurar.start(0)
        if self._expansion_total:
            path = Path(self.archivos.filePath(padre))
            if self._en_proyecto(path):
                self._exploradas.discard(path)
                self._cola.append((path, 0))
                self._timer_expansion.start(0)

    def _directorio_cargado(self, directory):
        path = Path(directory)
        self._cargadas.add(path)
        if self._expansion_total and self._en_proyecto(path) and path not in self._exploradas:
            self._cola.append((path, 0))
            self._timer_expansion.start(0)
        self._timer_restaurar.start(0)

    def _paso_expansion(self):
        trabajo = 0
        while self._cola and self._expansion_total and trabajo < 40:
            path, fila = self._cola.popleft()
            if path in self._exploradas or not self._en_proyecto(path):
                continue
            indice = self.indice_ruta(path)
            if not indice.isValid():
                continue
            if path not in self._expandidas:
                if len(self._expandidas) >= 1000:
                    self._expansion_total = False
                    self._cola.clear()
                    self.errorAcceso.emit('Se alcanzó el límite de 1000 carpetas expandidas; continúa por ramas para mantener ágil el explorador.')
                    return
                self._expandidas.add(path)
                self.expand(indice)
            fuente = self.filtro.mapToSource(indice)
            if self.archivos.canFetchMore(fuente):
                self.archivos.fetchMore(fuente)
                trabajo += 1
                continue
            if path not in self._cargadas:
                trabajo += 1
                continue  # Esperar directoryLoaded antes de recorrer hijos.
            cantidad = self.filtro.rowCount(indice)
            while fila < cantidad and trabajo < 40:
                child = self.filtro.index(fila, 0, indice)
                if self.archivos.isDir(self.filtro.mapToSource(child)):
                    self._cola.append((self._path(child), 0))
                fila += 1
                trabajo += 1
            if fila < cantidad:
                self._cola.appendleft((path, fila))
            else:
                self._exploradas.add(path)
            trabajo += 1
        if self._cola and self._expansion_total:
            self._timer_expansion.start(0)

    def revelar_ruta(self, path):
        path = Path(path)
        if not self._en_proyecto(path):
            return
        self._revelar = path
        self._timer_restaurar.start(0)

    def restaurar_ramas(self, relativas):
        raiz = Path(self.filtro.raiz)
        for relative in relativas[:1000]:
            path = raiz / relative
            if path.is_relative_to(raiz) and '..' not in Path(relative).parts and not Path(relative).is_absolute():
                self._restaurar.add(path)
        self._timer_restaurar.start(0)

    def _paso_restauracion(self):
        resueltas = 0
        for path in sorted(self._restaurar, key=lambda p: (len(p.parts), str(p)))[:20]:
            self.expandir_ruta(path)
            indice = self.indice_ruta(path)
            if indice.isValid():
                self._restaurar.discard(path)
                resueltas += 1
        if self._restaurar and resueltas:
            self._timer_restaurar.start(0)
        if self._revelar is not None:
            self.expandir_ruta(self._revelar.parent)
            indice = self.indice_ruta(self._revelar)
            if indice.isValid():
                self.setCurrentIndex(indice)
                self.scrollTo(indice)
                self._revelar = None

    def carpetas_abiertas(self):
        raiz = Path(self.filtro.raiz)
        return [str(p.relative_to(raiz)) for p in sorted(self.ramas) if p.is_relative_to(raiz)][:1000]

    def trasladar_ramas(self, origen, destino, anteriores=None):
        self.trasladar_orden(origen, destino)
        anteriores = self.ramas if anteriores is None else anteriores
        nuevos = {p for p in self.ramas if not p.is_relative_to(origen)}
        nuevos.update(destino / p.relative_to(origen) for p in anteriores if p.is_relative_to(origen))
        self.ramas = {p for p in nuevos if self._en_proyecto(p)}
        self._restaurar = set(self.ramas)
        self._timer_restaurar.start(0)

    def _accion_tecla(self, event):
        mods = event.modifiers()
        if mods == Qt.KeyboardModifier.ControlModifier:
            return {Qt.Key.Key_C: 'copiar', Qt.Key.Key_X: 'cortar', Qt.Key.Key_V: 'pegar', Qt.Key.Key_A: 'seleccionar_todo'}.get(event.key())
        if mods == Qt.KeyboardModifier.NoModifier:
            return {Qt.Key.Key_F2: 'renombrar', Qt.Key.Key_Delete: 'eliminar'}.get(event.key())
        if mods == Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.ShiftModifier and event.key() == Qt.Key.Key_N:
            return 'carpeta'
        if mods == Qt.KeyboardModifier.AltModifier and event.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter):
            return 'propiedades'
        return None

    def event(self, event):
        if event.type() == QEvent.Type.ShortcutOverride and self._accion_tecla(event):
            event.accept()
            return True
        return super().event(event)

    def keyPressEvent(self, event):
        accion = self._accion_tecla(event)
        if accion == 'seleccionar_todo':
            self.selectAll()
            event.accept()
            return
        if accion:
            self.operacionSolicitada.emit(accion, self.ruta_actual())
            event.accept()
            return
        super().keyPressEvent(event)
