"""Presentación/aplicación de reparaciones nativas con contexto de documento.

PanelReparaciones muestra estados, propuestas, cambios y motivos; no analiza
Calser. El controlador de ventana pide el proveedor C mediante AsistenciaEditor
y conserva el contexto de la copia. Aplicar vuelve a verificar identidad,
revisión, cursor/selección y precondiciones, con una operación de Deshacer.
Cambiar el documento o cancelar invalida la vista previa. El panel solo recibe
modelos en caracteres Python; el aplicador traduce a UTF-16. Ni buscar propuestas
ni aplicarlas guarda el archivo, y una propuesta manual no habilita automatizar."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QAction, QFont
from PySide6.QtWidgets import (QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPlainTextEdit,
                              QPushButton, QSplitter, QTabWidget, QVBoxLayout, QWidget)
from shiboken6 import isValid
from ide.ediciones import validar_ediciones
from ide.interfaz.ediciones import aplicar_reparacion
from ide.nucleo.asistencia import EstadoAsistencia

MAX_CAMBIO_PREVIA = 12000


def vista_previa(texto, reparacion):
    """Mostrar todos los cambios sin un diff global de coste cuadrático."""
    ediciones = validar_ediciones(texto, reparacion.ediciones, reparacion.precondiciones)
    if sum(e.fin - e.inicio + len(e.texto) for e in ediciones) > MAX_CAMBIO_PREVIA:
        raise ValueError('La propuesta supera los 12000 caracteres de cambios revisables en este panel. No se aplicará desde esta vista.')
    lineas = []
    for numero, e in enumerate(ediciones, 1):
        fila = texto.count('\n', 0, e.inicio) + 1
        columna = e.inicio - texto.rfind('\n', 0, e.inicio)
        tipo = 'insertar' if e.inicio == e.fin else ('eliminar' if not e.texto else 'reemplazar')
        lineas.append(f'Cambio {numero} · línea {fila}, columna {columna} · {tipo}')
        # Hasta 60 caracteres de la misma línea alrededor del rango. El cambio
        # completo siempre se muestra; solo se recorta el contexto inalterado.
        desde = max(texto.rfind('\n', 0, e.inicio) + 1, e.inicio - 60)
        fin_linea = texto.find('\n', e.fin)
        hasta = min(len(texto) if fin_linea < 0 else fin_linea, e.fin + 60)
        prefijo, sufijo = texto[desde:e.inicio], texto[e.fin:hasta]
        antes, despues = prefijo + texto[e.inicio:e.fin] + sufijo, prefijo + e.texto + sufijo
        lineas.extend('- ' + s for s in antes.split('\n'))
        lineas.extend('+ ' + s for s in despues.split('\n'))
        if e.texto.endswith('\n') or texto[e.inicio:e.fin].endswith('\n'):
            lineas.append('(Los saltos de línea del cambio se muestran como líneas adicionales.)')
        lineas.append('')
    return '\n'.join(lineas)


def breve(texto, limite=4000):
    return texto if len(texto) <= limite else texto[:limite] + '\n… explicación truncada'


class PanelReparaciones(QWidget):
    solicitar = Signal()
    cancelar = Signal()
    aplicar = Signal(object, object)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.contexto = None
        self.vigente = False
        self.ocupado = False
        self.previa_valida = False
        layout = QVBoxLayout(self); layout.setContentsMargins(8, 6, 8, 6); layout.setSpacing(5)
        fila = QHBoxLayout()
        self.consultar = QPushButton('Buscar reparaciones'); self.consultar.setToolTip('Consultar el documento actual · Alt+Enter en el editor')
        self.cancelar_boton = QPushButton('Cancelar'); self.cancelar_boton.setEnabled(False)
        fila.addWidget(self.consultar); fila.addWidget(self.cancelar_boton); fila.addStretch()
        layout.addLayout(fila)
        self.estado = QLabel('Alt+Enter o Buscar reparaciones consulta el motor C. Revisa la vista previa antes de aplicar manualmente.')
        self.estado.setTextFormat(Qt.TextFormat.PlainText); self.estado.setWordWrap(True); layout.addWidget(self.estado)
        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        self.lista = QListWidget(); self.lista.setMinimumSize(120, 60)
        self.lista.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.splitter.addWidget(self.lista)
        detalles = QWidget(); detalle_layout = QVBoxLayout(detalles)
        detalle_layout.setContentsMargins(6,0,0,0); detalle_layout.setSpacing(4)
        self.descripcion = QLabel(); self.descripcion.setWordWrap(True); self.descripcion.setTextFormat(Qt.TextFormat.PlainText)
        self.descripcion.setMaximumHeight(80); detalle_layout.addWidget(self.descripcion)
        self.explicacion = QPlainTextEdit(); self.explicacion.setReadOnly(True)
        self.explicacion.setPlaceholderText('Aquí se mostrarán las razones y la validación del motor.')
        self.previa = QPlainTextEdit(); self.previa.setReadOnly(True); self.previa.setFont(QFont('monospace',10))
        self.previa.setPlaceholderText('Selecciona una propuesta para revisar sus cambios antes de aplicar.')
        self.detalles_tabs = QTabWidget()
        self.detalles_tabs.addTab(self.previa, 'Cambios')
        self.detalles_tabs.addTab(self.explicacion, 'Motivo')
        detalle_layout.addWidget(self.detalles_tabs,1)
        self.splitter.addWidget(detalles); self.splitter.setStretchFactor(1,1); self.splitter.setSizes([220,580])
        layout.addWidget(self.splitter,1)
        pie = QHBoxLayout()
        self.aplicar_boton = QPushButton('Aplicar reparación'); self.aplicar_boton.setEnabled(False)
        pie.addWidget(self.aplicar_boton)
        aviso = QLabel('Ctrl+Z deshace · sin guardar')
        aviso.setWordWrap(True); aviso.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        aviso.setStyleSheet('color: #aeb8c9; font-size: 11px;'); pie.addWidget(aviso,1)
        layout.addLayout(pie)
        self.consultar.clicked.connect(self.solicitar.emit)
        self.cancelar_boton.clicked.connect(self.cancelar.emit)
        self.cancelar_accion = QAction(self)
        self.cancelar_accion.setShortcut('Escape')
        self.cancelar_accion.setShortcutContext(Qt.ShortcutContext.WidgetWithChildrenShortcut)
        self.cancelar_accion.setEnabled(False)
        self.cancelar_accion.triggered.connect(self.cancelar.emit)
        self.addAction(self.cancelar_accion)
        self.aplicar_boton.clicked.connect(self.elegir)
        self.lista.currentItemChanged.connect(self.seleccionar)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, 'splitter'):
            orientacion = Qt.Orientation.Vertical if self.width() < 620 else Qt.Orientation.Horizontal
            if self.splitter.orientation() != orientacion:
                self.splitter.setOrientation(orientacion)
                self.splitter.setSizes([120,360] if orientacion == Qt.Orientation.Vertical else [220,580])

    def limpiar(self, mensaje):
        self.contexto = None; self.vigente = self.ocupado = self.previa_valida = False
        self.lista.clear(); self.descripcion.clear(); self.explicacion.clear(); self.previa.clear()
        self.estado.setText(mensaje)
        self.consultar.setEnabled(True); self.cancelar_boton.setEnabled(False); self.aplicar_boton.setEnabled(False)
        self.cancelar_accion.setEnabled(False)

    def iniciar(self, contexto, nombre):
        self.limpiar(f'Consultando reparaciones · {nombre}…')
        self.contexto = contexto; self.vigente = self.ocupado = True
        self.consultar.setEnabled(False); self.cancelar_boton.setEnabled(True)
        self.cancelar_accion.setEnabled(True)

    def recibir(self, respuesta):
        if self.contexto != respuesta.solicitud.contexto or not self.vigente:
            return
        self.ocupado = False; self.consultar.setEnabled(True)
        resultado = respuesta.resultado
        self.lista.clear()
        if resultado.estado != EstadoAsistencia.OK:
            mensaje = resultado.explicacion or ('No hay reparaciones propuestas para este documento' if resultado.estado == EstadoAsistencia.SIN_OPCIONES else 'Consulta de reparaciones cancelada')
            self.limpiar(mensaje)
            return
        for r in resultado.propuestas:
            item = QListWidgetItem(breve(r.descripcion,200))
            item.setToolTip(breve(r.codigo + '\n' + r.descripcion))
            item.setData(Qt.ItemDataRole.UserRole,r)
            self.lista.addItem(item)
        self.estado.setText(f'{len(resultado.propuestas)} propuestas · revisa los cambios y elige Aplicar reparación')
        self.lista.setCurrentRow(0)
        self.cancelar_boton.setEnabled(True)

    def seleccionar(self, item, anterior=None):
        self.aplicar_boton.setEnabled(False); self.previa_valida = False
        self.descripcion.clear(); self.explicacion.clear(); self.previa.clear()
        if item is None or self.contexto is None:
            return
        r = item.data(Qt.ItemDataRole.UserRole)
        self.descripcion.setText(breve(r.descripcion,1000))
        metadatos = [f'Regla: {r.codigo}', 'El motor propone cambios; el IDE exige permiso y contexto vigente para aplicarlos.']
        if r.automatica_segura:
            metadatos.append('El motor la marca como candidata automática; este panel exige Aplicar igualmente.')
        if r.razones:
            metadatos.append('Razones:\n' + '\n'.join('• ' + s for s in r.razones))
        if r.validacion:
            metadatos.append('Validación del motor:\n' + r.validacion)
        metadatos.append(f'Ediciones: {len(r.ediciones)} · precondiciones: {len(r.precondiciones)}')
        self.explicacion.setPlainText(breve('\n'.join(metadatos)))
        try:
            self.previa.setPlainText(vista_previa(self.contexto.texto,r))
            self.previa_valida = True
        except (ValueError,TypeError) as error:
            self.previa.setPlainText(str(error))
        self.aplicar_boton.setEnabled(self.vigente and not self.ocupado and self.previa_valida)

    def elegir(self):
        item = self.lista.currentItem()
        if self.vigente and not self.ocupado and self.previa_valida and item is not None:
            self.aplicar.emit(self.contexto,item.data(Qt.ItemDataRole.UserRole))

    def invalidar(self, documento):
        if self.contexto and self.contexto.documento == documento:
            self.vigente = self.ocupado = False
            self.consultar.setEnabled(True); self.aplicar_boton.setEnabled(False)
            self.cancelar_boton.setEnabled(True)
            self.estado.setText('La propuesta quedó desactualizada. Vuelve a consultar antes de aplicar.')


class ReparacionesVentana:
    def solicitar_reparaciones(self):
        if not self.asistencia.configuracion['habilitada']:
            self.reparaciones_panel.limpiar('Asistencia desactivada · Editar → Preferencias de asistencia')
            return
        editor = self.editor()
        self.results_action.setChecked(True); self.panels.setCurrentWidget(self.reparaciones_panel)
        if editor is None or editor.isReadOnly():
            self.reparaciones_panel.limpiar('Selecciona un documento editable para buscar reparaciones')
            return
        if editor.path and editor.path.suffix.lower() == '.asm':
            self.reparaciones_panel.limpiar('Las reparaciones de Calser no se aplican a entradas .asm')
            return
        editor.setFocus()
        contexto = editor.contexto_asistencia()
        self.reparaciones_panel.iniciar(contexto,editor.path.name if editor.path else 'Sin título')
        if self.workspace_splitter.orientation() == Qt.Orientation.Vertical:
            tamanos = self.workspace_splitter.sizes()
            total = sum(tamanos)
            altura = min(max(tamanos[1],280), max(120,total - 160))
            self.workspace_splitter.setSizes([max(160,total - altura),altura])
        return self.asistencia.solicitar('reparaciones')

    def recibir_reparaciones(self, respuesta):
        self.reparaciones_panel.recibir(respuesta)
        self.statusBar().showMessage(self.reparaciones_panel.estado.text())

    def cancelar_reparaciones(self):
        contexto = self.reparaciones_panel.contexto
        if contexto:
            self.asistencia.coordinador.invalidar(contexto.documento,tipo='reparaciones')
        self.reparaciones_panel.limpiar('Consulta cancelada · el documento se conserva')
        self.statusBar().showMessage('Reparaciones canceladas · sin cambios')

    def aplicar_reparacion_propuesta(self, contexto, reparacion):
        editor = self.editor()
        if editor is None or not isValid(editor) or contexto.documento != editor.documento_id:
            self.reparaciones_panel.invalidar(contexto.documento)
            return
        self.asistencia.coordinador.invalidar(contexto.documento)
        resultado = aplicar_reparacion(editor,contexto,reparacion)
        if resultado.aplicada:
            self.reparaciones_panel.limpiar('Reparación aplicada · Ctrl+Z deshace · cambios sin guardar')
            editor.setFocus()
        else:
            self.reparaciones_panel.invalidar(contexto.documento)
            self.reparaciones_panel.estado.setText(resultado.explicacion)
        self.statusBar().showMessage(self.reparaciones_panel.estado.text())
