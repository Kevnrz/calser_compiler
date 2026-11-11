"""Presentación de marca y coordinación del arranque, sin bucles de espera.

Qt revela mediante trazos el SVG geométrico aprobado para CALSIDE.
El segundo fragmento del hexágono e IDE son morados; D es el reflejo de C. Los widgets se construyen en el hilo Qt,
por etapas de eventos. Los servicios de Ventana conservan sus trabajadores.
"""
from math import cos, hypot, pi, sin, sqrt
from pathlib import Path

from PySide6.QtCore import QObject, QRectF, QSettings, Qt, QTimer, QVariantAnimation, Signal
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPainterPathStroker
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtWidgets import QApplication, QLabel, QMessageBox, QVBoxLayout, QWidget

from ide import preferencias


# Geometría de los trazos centrales de la composición aprobada. El SVG conserva
# los contornos exactos; estas polilíneas solo guían su revelado, nunca los
# sustituyen por contornos de otra fuente o por un barrido rectangular.
GROSOR = 16 * sqrt(2)
ANCHO_LETRAS = (96, 112, 80, 80, GROSOR, 96, 96)
LETRAS = (
    (((88, 8), (16, 80), (88, 152)),),
    (((11.9866595, 160), (56, 34.247598), (100.0133405, 160)),),
    (((GROSOR / 2, 0), (GROSOR / 2, 91.3137085), (72, 152)),),
    (((64, 8), (16, 56), (64, 104), (16, 152)),),
    (((GROSOR / 2, 0), (GROSOR / 2, 160)),),
    (((8, 8), (80, 80), (8, 152)),),
    (((88, 8), (16, 80), (88, 152)), ((32, 80), (88, 80))),
)


class MarcaInicio:
    """Dibuja el SVG aprobado con máscaras Qt que recorren cada trazo.

    El recurso es completamente vectorial y se carga una sola vez. La escena
    lógica de 1600×900 coincide con la imagen aprobada; no depende de fuentes,
    DPI, PNG ni del directorio de trabajo. No se consulta el disco por fotograma.
    """
    DIBUJO_HEXAGONO = 1200
    FIN_MOVIMIENTO = 2100
    # 1800 / 0,9: un 10 % menos de velocidad de trazado, manteniendo las
    # demás fases y los 300 ms finales.
    FIN_LETRAS = 4100
    DURACION = 4400
    # Conservar la velocidad de giro (~43°/s) al extender el trazado de letras.
    # Ajustar el ángulo inicial permite llegar a 0° sin recolocación al final.
    GIRO_TOTAL = 180 * DURACION / 4200
    CENTRO_HEXAGONO = 384.686292

    def __init__(self, root):
        recurso = Path(root) / 'assets/branding/calside-inicio.svg'
        self.svg = QSvgRenderer(str(recurso))
        if not self.svg.isValid():
            raise RuntimeError(f'No se pudo cargar la marca de inicio: {recurso}')
        radio = 160 - GROSOR / sqrt(3)
        vertices = [(radio * cos(-pi / 2 + i * pi / 3),
                     radio * sin(-pi / 2 + i * pi / 3)) for i in range(6)]
        self.fragmentos = []
        for i, vertice in enumerate(vertices):
            extremos = []
            for vecino in (vertices[(i - 1) % 6], vertices[(i + 1) % 6]):
                # Recortar media separación (9/2) en cada extremo. Se numera
                # desde el fragmento superior en sentido horario: el 2 es morado.
                factor = .5 - 4.5 / radio
                extremos.append(tuple(v + (a - v) * factor for v, a in zip(vertice, vecino)))
            puntos = (extremos[0], vertice, extremos[1])
            self.fragmentos.append((tuple((x + self.CENTRO_HEXAGONO, y + 450)
                                          for x, y in puntos),))
        self.posiciones = []
        x = 591.250356
        for ancho in ANCHO_LETRAS:
            self.posiciones.append(x)
            x += ancho + 30

    def revelar(self, pintor, trazos, progreso, *, fragmento=False):
        if progreso <= 0:
            return
        mascara = QPainterPathStroker()
        # La máscara algo más ancha deja ver los ingletes y puntas del SVG.
        # El límite visible sigue siendo su contorno exacto, no esta máscara.
        mascara.setWidth(GROSOR * 2)
        mascara.setCapStyle(Qt.PenCapStyle.SquareCap)
        mascara.setJoinStyle(Qt.PenJoinStyle.MiterJoin)
        pintor.save()
        if fragmento:
            # Cada fragmento se limita a su propio contorno. Así el extremo
            # de la máscara no atraviesa una división y adelanta el siguiente.
            limite = QPainterPathStroker()
            limite.setWidth(GROSOR + 4)
            limite.setCapStyle(Qt.PenCapStyle.FlatCap)
            limite.setJoinStyle(Qt.PenJoinStyle.MiterJoin)
            puntos = list(trazos[0])
            # Un margen de una unidad evita recortar el antialiasing de las
            # tapas; queda dentro de la separación de nueve entre fragmentos.
            for extremo, vecino in ((0, 1), (-1, -2)):
                x, y = puntos[extremo]
                a, b = puntos[vecino]
                longitud = hypot(x - a, y - b)
                puntos[extremo] = (x + (x - a) / longitud, y + (y - b) / longitud)
            pintor.setClipPath(limite.createStroke(trazar((tuple(puntos),), 1)))
        pintor.setClipPath(mascara.createStroke(trazar(trazos, progreso)),
                           Qt.ClipOperation.IntersectClip)
        self.svg.render(pintor, QRectF(0, 0, 1600, 900))
        pintor.restore()

    def dibujar(self, pintor, area, tiempo):
        pintor.save()
        pintor.setRenderHint(QPainter.RenderHint.Antialiasing)
        pintor.fillRect(area, QColor('#191c24'))
        escala = min(area.width() / 1600, area.height() / 900)
        pintor.translate(area.center().x() - 800 * escala,
                         area.center().y() - 450 * escala)
        pintor.scale(escala, escala)
        if tiempo >= self.DURACION:
            # Solo al terminar el giro se usa la composición estática aprobada.
            self.svg.render(pintor, QRectF(0, 0, 1600, 900))
        else:
            movimiento = max(0., min(1., (tiempo - self.DIBUJO_HEXAGONO) /
                                         (self.FIN_MOVIMIENTO - self.DIBUJO_HEXAGONO)))
            suave = movimiento * movimiento * (3 - 2 * movimiento)
            pintor.save()
            pintor.translate(self.CENTRO_HEXAGONO + (800 - self.CENTRO_HEXAGONO) * (1 - suave), 450)
            progreso_giro = max(0., min(1., tiempo / self.DURACION))
            pintor.rotate(self.GIRO_TOTAL * (progreso_giro - 1))
            pintor.translate(-self.CENTRO_HEXAGONO, -450)
            for indice, trazos in enumerate(self.fragmentos):
                self.revelar(pintor, trazos, tiempo / self.DIBUJO_HEXAGONO * 6 - indice, fragmento=True)
            pintor.restore()
            for indice, trazos in enumerate(LETRAS):
                progreso = (tiempo - self.FIN_MOVIMIENTO) / (self.FIN_LETRAS - self.FIN_MOVIMIENTO) * 7 - indice
                if progreso <= 0:
                    break
                absolutos = tuple(tuple((x + self.posiciones[indice], y + 370) for x, y in puntos)
                                  for puntos in trazos)
                self.revelar(pintor, absolutos, progreso)
        pintor.restore()

def trazar(trazos, progreso):
    """Recorta polilíneas por longitud real, sin unir trazos separados."""
    longitud = sum(hypot(b[0] - a[0], b[1] - a[1])
                   for puntos in trazos for a, b in zip(puntos, puntos[1:]))
    restante = longitud * max(0., min(1., progreso))
    camino = QPainterPath()
    for puntos in trazos:
        camino.moveTo(*puntos[0])
        for a, b in zip(puntos, puntos[1:]):
            distancia = hypot(b[0] - a[0], b[1] - a[1])
            if restante <= 0:
                return camino
            fraccion = min(1., restante / distancia)
            camino.lineTo(a[0] + (b[0] - a[0]) * fraccion,
                          a[1] + (b[1] - a[1]) * fraccion)
            restante -= distancia
    return camino


class Presentacion(QWidget):
    terminada = Signal()
    DURACION = MarcaInicio.DURACION

    def __init__(self, root):
        super().__init__(None, Qt.WindowType.FramelessWindowHint)
        self.setWindowTitle('CALSIDE · Iniciando')
        self.setAccessibleName('Inicio de CALSIDE. Clic o Escape para omitir.')
        self.resize(760, 430)
        pantalla = QApplication.primaryScreen().availableGeometry()
        self.resize(min(self.width(), pantalla.width()), min(self.height(), pantalla.height()))
        self.move(pantalla.center() - self.rect().center())
        self.marca = MarcaInicio(root)
        self.tiempo = 0.
        self.finalizada = False
        self.animacion = QVariantAnimation(self)
        self.animacion.setStartValue(0.)
        self.animacion.setEndValue(float(self.DURACION))
        self.animacion.setDuration(self.DURACION)
        self.animacion.valueChanged.connect(self.actualizar)
        self.animacion.finished.connect(self.omitir)
        layout = QVBoxLayout(self)
        layout.addStretch()
        self.aviso = QLabel('Clic o Esc para omitir', self)
        self.aviso.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.aviso.setAttribute(Qt.WidgetAttribute.WA_TransparentForMouseEvents)
        self.aviso.setStyleSheet('color: #a5afbf; background: transparent; padding: 16px;')
        layout.addWidget(self.aviso)

    def iniciar(self):
        self.show()
        self.activateWindow()
        self.setFocus()
        self.animacion.start()

    def actualizar(self, tiempo):
        self.tiempo = float(tiempo)
        self.update()

    def omitir(self):
        # También se usa al completar el reloj; una entrada repetida no debe
        # mostrar dos veces la ventana ni reiniciar la inicialización.
        if self.finalizada:
            return
        self.finalizada = True
        self.animacion.stop()
        self.tiempo = float(self.DURACION)
        self.aviso.setText('Preparando el editor…')
        self.update()
        self.terminada.emit()

    def mousePressEvent(self, event):
        self.omitir()
        event.accept()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self.omitir()
            event.accept()
        else:
            super().keyPressEvent(event)

    def closeEvent(self, event):
        # Cerrar la presentación equivale a omitir; no cancela la aplicación.
        event.ignore()
        self.omitir()

    def paintEvent(self, event):
        pintor = QPainter(self)
        self.marca.dibujar(pintor, QRectF(self.rect()), self.tiempo)


class Inicio(QObject):
    """Entrega el control a Qt entre creación, proyecto y cada archivo inicial.

    Una barrera de dos condiciones une la inicialización real y la presentación.
    Omitir elimina solo el tiempo visual pendiente. Nunca se construyen widgets
    en trabajadores ni se llama a processEvents dentro de sus constructores.
    """
    def __init__(self, app, root, argumentos=(), *, settings=None, fabrica=None):
        super().__init__(app)
        self.app, self.root = app, Path(root)
        self.argumentos = list(argumentos)
        self.settings = QSettings('Calser', 'CALSIDE') if settings is None else settings
        self.fabrica = fabrica
        self.ventana = None
        self.lista = self.mostrada = False
        self.presentacion = None
        self.visual_terminada = not preferencias.cargar(self.settings).get('animacion_inicio', True)
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.paso)
        self.etapa = 0

    def iniciar(self):
        if not self.visual_terminada:
            self.presentacion = Presentacion(self.root)
            self.presentacion.terminada.connect(self.terminar_visual)
            self.presentacion.iniciar()
        self.timer.start(0)

    def terminar_visual(self):
        self.visual_terminada = True
        self.mostrar_si_lista()

    def paso(self):
        try:
            if self.etapa == 0:
                if self.fabrica is None:
                    from ide.interfaz.ventana import Ventana
                    self.fabrica = Ventana
                self.ventana = self.fabrica(self.root, preferencias=self.settings)
            elif self.etapa == 1:
                if self.argumentos:
                    self.ventana.cargar_carpeta(Path(self.argumentos[0]).resolve().parent)
            elif self.argumentos:
                self.ventana.abrir(Path(self.argumentos.pop(0)))
            else:
                self.lista = True
                self.mostrar_si_lista()
                return
            self.etapa += 1
            self.timer.start(0)
        except Exception as error:
            # Un fallo del constructor no puede dejar un splash eterno y una
            # aplicación invisible; el error permanece también en stderr.
            import traceback
            traceback.print_exc()
            if self.presentacion:
                self.presentacion.hide()
                self.presentacion.animacion.stop()
            QMessageBox.critical(None, 'No se pudo iniciar CALSIDE', str(error))
            self.app.exit(1)

    def mostrar_si_lista(self):
        if not (self.lista and self.visual_terminada) or self.mostrada:
            return
        self.mostrada = True
        # Mostrar primero evita que Qt interprete el cierre del splash como
        # cierre de la última ventana de la aplicación.
        self.ventana.show()
        if self.presentacion:
            self.presentacion.hide()
            self.presentacion.deleteLater()
            self.presentacion = None
        self.ventana.raise_()
        self.ventana.activateWindow()
