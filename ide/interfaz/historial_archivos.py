"""Deshacer eliminaciones de esta sesión sin sobrescribir archivos existentes.

La papelera del sistema conserva los bytes guardados; las pestañas conservan
sus buffers. Cada selección eliminada es un grupo, incluso si una operación
posterior del lote falla. No se mezclan estos pasos con el historial del texto.
"""
from pathlib import Path

from PySide6.QtCore import QFile, QDir
from PySide6.QtWidgets import QMenu
from ide import archivos


class HistorialArchivos:
    def preparar_eliminacion(self, path):
        """Captura la versión guardada y el estado de pestañas antes de borrar."""
        pestañas = []
        for editor in self.editores_de_archivo(path):
            try:
                disco = archivos.estado(editor.path)
            except OSError:
                disco = None
            pestañas.append((editor, editor.path, editor.revision,
                            editor.document().isModified(), editor.estado_disco, disco))
        return {'origen': path, 'padre_real': path.parent.resolve(), 'pestañas': pestañas}

    def registrar_eliminacion(self, registro, papelera):
        # Algunas plataformas no devuelven ubicación recuperable. Nunca inventar
        # una ruta ni recurrir a eliminar permanentemente como alternativa.
        if not papelera:
            self.statusBar().showMessage('Enviado a la papelera · restaura desde el gestor del sistema')
            return
        registro['papelera'] = Path(papelera)
        historial = self.__dict__.setdefault('_eliminaciones', [])
        lote = getattr(self, '_lote_archivos', None)
        if lote is not None:
            grupo = lote.setdefault('eliminaciones', [])
            if not grupo:
                historial.append(grupo)
        else:
            grupo = []
            historial.append(grupo)
        grupo.append(registro)
        self._eliminacion_prioritaria = True
        self.actualizar_aviso_externo()
        self.tree.setFocus()
        self.statusBar().showMessage('Enviado a la papelera · Ctrl+Z para restaurar')

    def deshacer_eliminacion_pendiente(self):
        """Ctrl+Z restaura si eliminar fue la última acción, aun con foco en código.

        Una edición posterior devuelve la prioridad al historial de escritura.
        Consumir el atajo también si falla evita borrar texto por un conflicto.
        """
        if not getattr(self, '_eliminacion_prioritaria', False) or not getattr(self, '_eliminaciones', []):
            return False
        self.deshacer_eliminacion()
        return True

    def deshacer_eliminacion(self):
        """Restaura el último grupo. Ante un conflicto conserva los pendientes."""
        historial = getattr(self, '_eliminaciones', [])
        if not historial:
            return False
        if self.operacion_carpetas is not None or getattr(self, '_lote_archivos', None):
            self.error('Operación en curso', 'Espera a que termine antes de restaurar.')
            return False
        grupo = historial[-1]
        while grupo:
            registro = grupo[-1]
            origen, papelera = registro['origen'], registro['papelera']
            try:
                if origen.exists() or origen.is_symlink():
                    raise OSError(f'Ya existe {origen}. No se reemplazó; cambia su nombre antes de reintentar.')
                if not origen.parent.is_dir() or origen.parent.resolve() != registro['padre_real']:
                    raise OSError(f'La carpeta original no está disponible: {origen.parent}')
                if papelera.is_symlink() or not papelera.exists():
                    raise OSError('La entrada ya no está disponible en la papelera.')
                if not self.permitir_mover(origen):
                    return False
                if papelera.is_dir():
                    ok = QDir().rename(str(papelera), str(origen))
                else:
                    ok = QFile(str(papelera)).rename(str(origen))
                if not ok:
                    raise OSError('No se pudo restaurar. La entrada permanece en la papelera.')
            except OSError as error:
                self.error('No se pudo deshacer la eliminación', str(error))
                return False
            grupo.pop()  # No repetir un traslado que ya se completó.
            abiertos = {self.tabs.widget(i) for i in range(self.tabs.count())}
            for editor, path, revision, modificado, esperado, anterior in registro['pestañas']:
                if editor not in abiertos or editor.path != path:
                    continue
                try:
                    actual = archivos.estado(path)
                except OSError:
                    continue  # El monitor informará el error; conservar el buffer.
                if anterior is not None and esperado == anterior and actual.huella == anterior.huella:
                    editor.estado_disco = actual
                    if editor.revision == revision:
                        editor.document().setModified(modificado)
                editor.estado_observado = actual
                editor.cambio_reconocido = None
                self.actualizar_titulo(editor)
            self.clones_panel.invalidar(origen)
            self.tree.revelar_ruta(origen)
        historial.pop()
        self._eliminacion_prioritaria = bool(historial)
        self.actualizar_problemas()
        self.busqueda_proyecto.actualizar_si_activa()
        self.statusBar().showMessage('Eliminación deshecha · archivos restaurados')
        return True

    def menu_pestana_archivo(self, posicion):
        barra = self.tabs.tabBar()
        indice = barra.tabAt(posicion)
        if indice < 0:
            return
        editor = self.tabs.widget(indice)
        menu = QMenu(self)
        eliminar = menu.addAction('Eliminar archivo…')
        eliminar.setEnabled(editor.path is not None and editor.path.exists())
        eliminar.triggered.connect(lambda: self.eliminar_archivo(editor.path))
        restaurar = menu.addAction('Deshacer eliminación')
        restaurar.setEnabled(bool(getattr(self, '_eliminaciones', [])))
        restaurar.triggered.connect(self.deshacer_eliminacion)
        menu.exec(barra.mapToGlobal(posicion))
        menu.deleteLater()
