"""Árbol lazy de Qt: no recorre recursivamente el proyecto en el hilo gráfico."""
from pathlib import Path
import os
from PySide6.QtCore import QDir, QSortFilterProxyModel, Qt, Signal
from PySide6.QtWidgets import QFileSystemModel, QTreeView

EXCLUIDAS = frozenset({'.git', '.venv', 'venv', '__pycache__', 'build', 'dist',
                      'node_modules', '.pytest_cache', '.mypy_cache', '.ruff_cache',
                      '.idea', '.tox'})


class FiltroProyecto(QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.raiz = ''
        self.setDynamicSortFilter(True)

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


class Explorador(QTreeView):
    abrirArchivo = Signal(object)
    errorAcceso = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.archivos = QFileSystemModel(self)
        self.archivos.setReadOnly(True)
        self.archivos.setFilter(QDir.Filter.AllDirs | QDir.Filter.Files |
                                QDir.Filter.NoDotAndDotDot | QDir.Filter.NoSymLinks)
        self.archivos.setNameFilters(['*.cls'])
        self.archivos.setNameFilterDisables(False)
        self.filtro = FiltroProyecto(self)
        self.filtro.setSourceModel(self.archivos)
        self.setModel(self.filtro)
        self.setHeaderHidden(True)
        for columna in (1, 2, 3):
            self.hideColumn(columna)
        self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.SortOrder.AscendingOrder)
        self.setEditTriggers(QTreeView.EditTrigger.NoEditTriggers)
        self.activated.connect(self.abrir)
        self.expanded.connect(self.verificar_acceso)
        self.setToolTip('Subcarpetas y archivos .cls · se excluyen carpetas de compilación, dependencias y enlaces simbólicos')

    def cargar(self, path):
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

    def abrir(self, indice):
        fuente = self.filtro.mapToSource(indice)
        if not self.archivos.isDir(fuente):
            self.abrirArchivo.emit(Path(self.archivos.filePath(fuente)))

    def verificar_acceso(self, indice):
        ruta = self.archivos.filePath(self.filtro.mapToSource(indice))
        if not os.access(ruta, os.R_OK | os.X_OK):
            self.errorAcceso.emit(f'No se puede leer la carpeta: {ruta}')
