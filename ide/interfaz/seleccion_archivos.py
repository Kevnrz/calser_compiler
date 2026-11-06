"""Operaciones de selección múltiple; las carpetas se procesan una a una."""
from pathlib import Path
from PySide6.QtCore import QFile, QMimeData, Qt, QUrl
from PySide6.QtWidgets import QApplication, QMenu, QMessageBox


def raices_seleccionadas(rutas):
    """No repetir un descendiente cuando también se seleccionó su carpeta."""
    rutas = list(dict.fromkeys(Path(p).absolute() for p in rutas))
    return [p for p in rutas if not any(p != q and p.is_relative_to(q) for q in rutas)]


def archivos_portapapeles():
    mime = QApplication.clipboard().mimeData()
    if mime is None:
        return None
    urls = mime.urls()
    if not urls or any(not u.isLocalFile() for u in urls):
        return None
    rutas = raices_seleccionadas(u.toLocalFile() for u in urls)
    gnome = bytes(mime.data('x-special/gnome-copied-files')).splitlines()
    cortar = bytes(mime.data('application/x-kde-cutselection')) == b'1' or bool(gnome and gnome[0] == b'cut')
    return rutas, cortar


class SeleccionArchivos:
    def seleccion_para(self, path):
        rutas = self.tree.rutas_seleccionadas()
        return raices_seleccionadas(rutas) if path in rutas else [path]

    def copiar_seleccion(self, rutas, cortar=False):
        if self.operacion_carpetas is not None:
            self.error('Operación en curso', 'Espera a que termine la operación actual.')
            return False
        rutas = raices_seleccionadas(rutas)
        if not rutas:
            return False
        for p in rutas:
            if p.is_symlink() or not (p.is_file() or p.is_dir()):
                self.error('Entrada no disponible', str(p))
                return False
            if cortar and not self.permitir_mover(p):
                return False
        mime = QMimeData()
        urls = [QUrl.fromLocalFile(str(p)) for p in rutas]
        mime.setUrls(urls)
        mime.setText('\n'.join(str(p) for p in rutas))
        mime.setData('application/x-kde-cutselection', b'1' if cortar else b'0')
        mime.setData('x-special/gnome-copied-files', ('cut\n' if cortar else 'copy\n').encode()
                     + b'\n'.join(bytes(u.toEncoded()) for u in urls))
        QApplication.clipboard().setMimeData(mime)
        self.statusBar().showMessage(f'{len(rutas)} entradas {"cortadas" if cortar else "copiadas"} · se usará la versión guardada en disco.')
        return True

    def menu_seleccion(self, rutas, path):
        menu = QMenu(self)
        def accion(texto, callback, enabled=True):
            a = menu.addAction(texto)
            a.setEnabled(enabled)
            a.triggered.connect(lambda checked=False: callback())
        ocupado = self.operacion_carpetas is not None
        accion(f'Abrir archivos seleccionados ({len(rutas)})',
               lambda: [self.abrir(p) for p in rutas if p.is_file() and not self.ruta_ocupada(p)], not ocupado)
        menu.addSeparator()
        accion('Copiar', lambda: self.copiar_seleccion(rutas), not ocupado)
        accion('Cortar', lambda: self.copiar_seleccion(rutas, True), not ocupado)
        accion('Enviar a la papelera…', lambda: self.eliminar_seleccion(rutas), not ocupado)
        menu.addSeparator()
        accion('Copiar ubicación', lambda: QApplication.clipboard().setText('\n'.join(map(str, rutas))))
        accion('Abrir ubicación', lambda: self.abrir_ubicacion(path))
        return menu

    def trasladar_seleccion(self, rutas, destino):
        return self.procesar_seleccion(rutas, destino, 'mover')

    def eliminar_seleccion(self, rutas):
        rutas = raices_seleccionadas(rutas)
        if any(not self.permitir_mover(p) for p in rutas):
            return False
        box = QMessageBox(self)
        box.setWindowTitle('Enviar selección a la papelera')
        box.setTextFormat(Qt.TextFormat.PlainText)
        box.setText(f'¿Enviar {len(rutas)} entradas a la papelera? Las carpetas incluyen todo su contenido.\n'
                    + '\n'.join(str(p) for p in rutas[:12]) + ('\n…' if len(rutas) > 12 else ''))
        box.setInformativeText('Los textos abiertos se conservarán como cambios sin guardar.')
        box.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        box.setDefaultButton(QMessageBox.StandardButton.No)
        if box.exec() != QMessageBox.StandardButton.Yes:
            return False
        return self.procesar_seleccion(rutas, None, 'papelera')

    def procesar_seleccion(self, rutas, destino, tipo, portapapeles=None):
        if self.operacion_carpetas is not None or getattr(self, '_lote_archivos', None):
            self.error('Operación en curso', 'Espera a que termine la operación actual.')
            return False
        rutas = raices_seleccionadas(rutas)
        carpeta = Path(destino).absolute() if destino is not None else None
        if carpeta is not None and (not carpeta.is_dir() or carpeta.is_symlink()):
            self.error('Destino inválido', 'Suelta la selección sobre una carpeta del proyecto o en el espacio vacío de la raíz.')
            return False
        trabajos, destinos = [], set()
        for origen in rutas:
            final = carpeta / origen.name if carpeta else None
            if origen.is_symlink() or not (origen.is_dir() or origen.is_file()):
                self.error('Entrada no disponible', str(origen))
                return False
            if tipo in ('mover', 'papelera') and not self.permitir_mover(origen):
                return False
            if tipo == 'mover' and final == origen:
                continue
            if final is not None:
                if final.is_relative_to(origen):
                    self.error('Destino inválido', 'No se puede colocar una carpeta dentro de sí misma.')
                    return False
                if final in destinos or final.exists() or final.is_symlink() or self.editores_de_archivo(final):
                    self.error('Destino ocupado', f'No se inició el traslado o copia: {final}\nNo se reemplazarán ni combinarán entradas. Cambia el nombre o el destino.')
                    return False
                if tipo == 'mover' and origen.is_dir() and origen.stat().st_dev != carpeta.stat().st_dev:
                    self.error('Destino en otro disco', 'Para carpetas entre sistemas de archivos, usa Copiar/Pegar y luego revisa la copia.')
                    return False
                destinos.add(final)
            trabajos.append((tipo, origen, final))
        if not trabajos:
            self.statusBar().showMessage('La selección ya está en esa carpeta')
            return False
        self._lote_archivos = {'pendientes': trabajos, 'total': len(trabajos), 'hechos': 0,
                               'portapapeles': portapapeles, 'fallo': False}
        self._continuar_seleccion()
        return True

    def _continuar_seleccion(self):
        lote = self._lote_archivos
        while lote['pendientes'] and not lote['fallo']:
            tipo, origen, destino = lote['pendientes'].pop(0)
            if tipo in ('mover', 'papelera') and not self.permitir_mover(origen):
                lote['fallo'] = True
                break
            if origen.is_dir() and not origen.is_symlink():
                if self.iniciar_operacion_carpeta(tipo, origen, destino):
                    self.operacion_carpetas.lote = True
                    return
                lote['fallo'] = True
                break
            if tipo == 'mover':
                ok = self.mover_archivo(origen, destino)
            else:
                if origen.is_symlink() or not origen.is_file():
                    self.error('Entrada no disponible', str(origen))
                    ok = False
                elif tipo == 'copiar':
                    ok = not self.editores_de_archivo(destino) and QFile(str(origen)).copy(str(destino))
                    if ok:
                        self.clones_panel.invalidar(destino)
                    else:
                        self.error('No se pudo copiar', f'El destino no se sobrescribió: {destino}')
                else:
                    ok, _ = QFile.moveToTrash(str(origen))
                    if ok:
                        self.marcar_eliminados(origen)
                    else:
                        self.error('No se pudo enviar a papelera', str(origen))
            if not ok:
                lote['fallo'] = True
                break
            lote['hechos'] += 1
        self._lote_archivos = None
        if not lote['fallo'] and lote['portapapeles'] and archivos_portapapeles() == lote['portapapeles']:
            QApplication.clipboard().clear()
        self.statusBar().showMessage(f'{"Operación detenida" if lote["fallo"] else "Operación completada"} · '
                                     f'{lote["hechos"]}/{lote["total"]} entradas procesadas'
                                     + (f' · {lote["detalle"]}' if lote.get('detalle') else ''))
