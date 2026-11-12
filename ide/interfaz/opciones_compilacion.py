"""Diálogo de perfiles: limita las casillas a las capacidades de ccom."""
from PySide6.QtWidgets import (QDialog, QVBoxLayout, QFormLayout, QComboBox,
    QLineEdit, QCheckBox, QDialogButtonBox, QLabel, QPushButton, QHBoxLayout)
from PySide6.QtWidgets import QFileDialog
from ide.compilacion import MODOS, OPCIONES, validar


class OpcionesCompilacion(QDialog):
    def __init__(self, datos, parent=None):
        super().__init__(parent)
        self.setWindowTitle('Opciones de compilación')
        self.setMinimumWidth(560)
        self.setStyleSheet("""
            QComboBox { background: #222631; color: #dce2ec; border: 1px solid #414957; padding: 6px; }
            QComboBox QAbstractItemView { background: #222631; color: #dce2ec; selection-background-color: #504135; }
            QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #737c8c; border-radius: 3px; background: #191c24; }
            QCheckBox::indicator:checked { background: #ab83df; border-color: #ab83df; }
            QCheckBox:disabled { color: #7d8592; }
            QCheckBox::indicator:disabled { border-color: #414957; background: #222631; }
        """)
        datos = validar(datos)
        layout = QVBoxLayout(self)
        form = QFormLayout()
        self.modo = QComboBox()
        for clave, texto in MODOS.items():
            self.modo.addItem(texto, clave)
        self.modo.setCurrentIndex(self.modo.findData(datos['modo']))
        form.addRow('Operación', self.modo)
        fila = QHBoxLayout()
        self.destino = QLineEdit(datos['destino'])
        self.destino.setPlaceholderText('Automático: build/ide/<fuente>/nombre')
        self.examinar = QPushButton('Elegir…')
        self.examinar.clicked.connect(self.elegir_destino)
        fila.addWidget(self.destino)
        fila.addWidget(self.examinar)
        form.addRow('Destino', fila)
        layout.addLayout(form)
        self.aviso = QLabel()
        self.aviso.setWordWrap(True)
        layout.addWidget(self.aviso)
        self.casillas = {}
        for clave, (texto, bandera, _) in OPCIONES.items():
            casilla = QCheckBox(f'{texto}  ({bandera})')
            casilla.setChecked(datos[clave])
            layout.addWidget(casilla)
            self.casillas[clave] = casilla
        botones = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        botones.accepted.connect(self.accept)
        botones.rejected.connect(self.reject)
        layout.addWidget(botones)
        self.modo.currentIndexChanged.connect(self.cambiar_modo)
        self.actualizar()

    def cambiar_modo(self):
        self.destino.clear()
        self.actualizar()

    def actualizar(self):
        modo = self.modo.currentData()
        self.destino.setEnabled(modo != 'analizar')
        self.examinar.setEnabled(modo != 'analizar')
        for clave, casilla in self.casillas.items():
            casilla.setEnabled(modo in OPCIONES[clave][2])
        textos = {
            'analizar': 'Comprueba léxico, sintaxis y semántica sin generar artefactos.',
            'ast': 'Exportación sintáctica: no comprueba semántica ni genera un ejecutable.',
            'ensamblar': 'Al ejecutar este modo podrás elegir una entrada .asm. No habilita F6.',
            'compilar': 'Genera .bin, .hex y .mem. El listado .asm permite ejecutar con F6.',
        }
        self.aviso.setText(textos[modo] + ' La operación usa el documento guardado; se pedirá guardar si cambió.')

    def elegir_destino(self):
        filtro = 'AST JSON (*.json)' if self.modo.currentData() == 'ast' else 'Binario (*.bin)'
        ruta, _ = QFileDialog.getSaveFileName(self, 'Destino de salida', self.destino.text(), filtro)
        if ruta:
            self.destino.setText(ruta)

    def datos(self):
        return validar({'modo': self.modo.currentData(), 'destino': self.destino.text().strip(),
                        **{clave: casilla.isChecked() for clave, casilla in self.casillas.items()}})
