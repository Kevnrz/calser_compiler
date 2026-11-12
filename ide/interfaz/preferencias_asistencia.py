"""Preferencias locales de asistencia; ninguna consulta ni edición al abrir el diálogo."""
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QCheckBox, QDialog, QDialogButtonBox, QFormLayout, QLabel,
                              QListWidget, QListWidgetItem, QSpinBox, QVBoxLayout, QWidget)
from ide.politica_asistencia import validar_preferencias


class PreferenciasAsistencia(QDialog):
    def __init__(self, configuracion, reglas, parent=None):
        super().__init__(parent)
        self.setWindowTitle('Preferencias de asistencia')
        self.resize(510, 440)
        d = validar_preferencias(configuracion)
        layout = QVBoxLayout(self)
        self.habilitada = QCheckBox('Habilitar asistencia de código')
        self.habilitada.setChecked(d['habilitada']); layout.addWidget(self.habilitada)
        self.opciones = QWidget(); formulario = QFormLayout(self.opciones)
        self.inline = QCheckBox('Sugerencias y cierres en gris al escribir (Tab acepta)')
        self.inline.setChecked(d['sugerencias_inline']); formulario.addRow(self.inline)
        self.pausa = QCheckBox('Lista por pausa (sin vista gris)')
        self.pausa.setChecked(d['sugerencias_pausa']); formulario.addRow(self.pausa)
        self.espera = QSpinBox(); self.espera.setRange(300, 3000); self.espera.setSingleStep(100)
        self.espera.setSuffix(' ms'); self.espera.setValue(d['pausa_ms'])
        formulario.addRow('Pausa de asistencia', self.espera)
        self.automaticas = QCheckBox('Corregir al confirmar una línea con Enter')
        self.automaticas.setChecked(d['reparaciones_automaticas'] and bool(reglas))
        self.automaticas.setEnabled(bool(reglas)); formulario.addRow(self.automaticas)
        self.lista = QListWidget(); self.lista.setMaximumHeight(140)
        for regla in reglas:
            item = QListWidgetItem(regla.descripcion)
            item.setData(Qt.ItemDataRole.UserRole, regla.codigo)
            item.setToolTip(regla.codigo)
            item.setFlags(item.flags() | Qt.ItemFlag.ItemIsUserCheckable)
            item.setCheckState(Qt.CheckState.Checked if regla.codigo in d['reglas_automaticas'] else Qt.CheckState.Unchecked)
            self.lista.addItem(item)
        formulario.addRow('Reglas permitidas', self.lista)
        self.lista.setEnabled(self.automaticas.isChecked())
        self.automaticas.toggled.connect(self.lista.setEnabled)
        self.automaticas.toggled.connect(self.seleccionar_regla_inicial)
        if not reglas:
            estado = QLabel('Todavía no hay reglas automáticas revisadas. Las propuestas del motor podrán revisarse y aplicarse manualmente.')
            estado.setWordWrap(True); formulario.addRow(estado)
        layout.addWidget(self.opciones)
        ayuda = QLabel('Gris: Tab acepta · Esc descarta · Alt+[ / Alt+] cambia la propuesta\n'
                       'Ctrl+Espacio: lista de sugerencias · Alt+Enter: reparaciones\n'
                       'La pausa no acepta sugerencias. Las correcciones automáticas requieren una regla permitida, '
                       'precondiciones y validación del parser. Ctrl+Z deshace; no se guardan archivos automáticamente.')
        ayuda.setWordWrap(True); layout.addWidget(ayuda)
        botones = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        botones.accepted.connect(self.accept); botones.rejected.connect(self.reject)
        layout.addWidget(botones)
        self.habilitada.toggled.connect(self.opciones.setEnabled)
        self.opciones.setEnabled(d['habilitada'])

    def seleccionar_regla_inicial(self, activada):
        # Una sola activación basta para usar la regla revisada disponible.
        # El usuario puede personalizar después la selección del catálogo.
        if activada and self.lista.count() and not any(self.lista.item(i).checkState() == Qt.CheckState.Checked for i in range(self.lista.count())):
            self.lista.item(0).setCheckState(Qt.CheckState.Checked)

    def datos(self):
        return validar_preferencias({'habilitada': self.habilitada.isChecked(),
            'sugerencias_inline': self.inline.isChecked(),
            'sugerencias_pausa': self.pausa.isChecked(), 'pausa_ms': self.espera.value(),
            'reparaciones_automaticas': self.automaticas.isChecked(),
            'reglas_automaticas': [self.lista.item(i).data(Qt.ItemDataRole.UserRole)
                for i in range(self.lista.count()) if self.lista.item(i).checkState() == Qt.CheckState.Checked]})
