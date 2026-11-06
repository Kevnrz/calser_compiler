"""Presentación del arrastre y orden manual entre entradas hermanas."""
from pathlib import Path
from PySide6.QtCore import QPoint, QRect, QRectF, Qt
from PySide6.QtGui import QColor, QIcon, QPainter, QPen, QPixmap


class OrdenExplorador:
    def imagen_arrastre(self, rutas):
        """Tarjeta con alfa real, independiente del cursor que use el escritorio."""
        filas = rutas[:3]
        alto = 16 + len(filas) * 28 + (24 if len(rutas) > 3 else 0)
        ancho = min(300, max(160, max(self.fontMetrics().horizontalAdvance(p.name) for p in filas) + 58))
        escala = self.devicePixelRatioF()
        imagen = QPixmap(round(ancho * escala), round(alto * escala))
        imagen.setDevicePixelRatio(escala)
        imagen.fill(Qt.GlobalColor.transparent)
        pintor = QPainter(imagen)
        pintor.setRenderHint(QPainter.RenderHint.Antialiasing)
        pintor.setPen(QPen(QColor(232, 178, 80, 205), 1))
        pintor.setBrush(QColor(36, 40, 51, 188))
        pintor.drawRoundedRect(QRectF(1, 1, ancho - 2, alto - 2), 8, 8)
        pintor.setFont(self.font())
        pintor.setOpacity(.82)
        for n, path in enumerate(filas):
            y = 8 + n * 28
            icono = self.indice_ruta(path).data(Qt.ItemDataRole.DecorationRole)
            if isinstance(icono, QIcon):
                icono.paint(pintor, QRect(10, y + 4, 20, 20))
            pintor.setPen(QColor('#f0d09a'))
            texto = self.fontMetrics().elidedText(path.name, Qt.TextElideMode.ElideMiddle, ancho - 52)
            pintor.drawText(QRect(40, y, ancho - 48, 28), Qt.AlignmentFlag.AlignVCenter, texto)
        if len(rutas) > 3:
            pintor.setPen(QColor('#c4c8d1'))
            pintor.drawText(QRect(12, alto - 28, ancho - 24, 22), Qt.AlignmentFlag.AlignVCenter,
                           f'+ {len(rutas) - 3} entradas más')
        pintor.end()
        return imagen

    def intencion_arrastre(self, event):
        if event.source() is not self or not event.mimeData().hasUrls():
            return None
        urls = event.mimeData().urls()
        if any(not u.isLocalFile() for u in urls):
            return None
        rutas = [Path(u.toLocalFile()) for u in urls]
        if not rutas or any(not self._en_proyecto(p) or p == Path(self.filtro.raiz) for p in rutas):
            return None
        punto = event.position().toPoint()
        indice = self.indexAt(punto)
        if indice.isValid():
            referencia = self._path(indice)
            fila = self.visualRect(indice)
            margen = max(4, min(10, fila.height() // 4))
            borde = 'antes' if punto.y() < fila.top() + margen else (
                'despues' if punto.y() > fila.bottom() - margen else None)
            if borde:
                if referencia in rutas or any(p.parent != referencia.parent for p in rutas):
                    return None
                return borde, referencia
            destino = referencia
        else:
            destino = Path(self.filtro.raiz)
        if not destino.is_dir() or destino.is_symlink():
            return None
        if any(p == destino or destino.is_relative_to(p) for p in rutas):
            return None
        return 'dentro', destino

    def orden_presentacion(self):
        raiz = Path(self.filtro.raiz)
        return {str(Path(p).relative_to(raiz)): list(nombres)
                for p, nombres in self.filtro.orden.items() if Path(p).is_relative_to(raiz)}

    def restaurar_orden(self, datos):
        # La misma validación se aplica a configuración y caché de sesión.
        from ide.preferencias import validar
        limpio = validar({'version': 1, 'orden_explorador': datos}).get('orden_explorador', {})
        raiz = Path(self.filtro.raiz)
        self.filtro.orden = {str(raiz / p): nombres for p, nombres in limpio.items()}
        self.filtro.actualizar_orden()

    def reordenar(self, rutas, referencia, despues=False):
        referencia = Path(referencia)
        rutas = list(dict.fromkeys(Path(p) for p in rutas))
        if (not rutas or referencia in rutas or not self._en_proyecto(referencia)
                or any(p.parent != referencia.parent or not p.exists() or p.is_symlink() for p in rutas)):
            return False
        padre = self.indice_ruta(referencia.parent)
        hermanos = [self._path(self.filtro.index(i, 0, padre)) for i in range(self.filtro.rowCount(padre))]
        if referencia not in hermanos or any(p not in hermanos for p in rutas):
            return False
        bloque = [p for p in hermanos if p in rutas]
        restantes = [p for p in hermanos if p not in rutas]
        posicion = restantes.index(referencia) + int(despues)
        nuevos = restantes[:posicion] + bloque + restantes[posicion:]
        self.filtro.orden[str(referencia.parent)] = [p.name for p in nuevos]
        self.filtro.actualizar_orden()
        self.errorAcceso.emit('Orden del explorador actualizado · las ubicaciones en disco se conservan')
        return True

    def orden_alfabetico(self, carpeta):
        self.filtro.orden.pop(str(carpeta), None)
        self.filtro.actualizar_orden()

    def trasladar_orden(self, origen, destino):
        origen, destino = Path(origen), Path(destino)
        nuevo = {}
        for padre, nombres in self.filtro.orden.items():
            path = Path(padre)
            if path.is_relative_to(origen):
                path = destino / path.relative_to(origen)
            elif path == origen.parent and origen.name in nombres:
                nombres = [destino.name if n == origen.name and origen.parent == destino.parent else n
                           for n in nombres if n != origen.name or origen.parent == destino.parent]
            if self._en_proyecto(path):
                nuevo[str(path)] = nombres
        self.filtro.orden = nuevo
        self.filtro.actualizar_orden()
