"""Servicios para localizar ccom y convertir sus mensajes en diagnósticos.

Este módulo no inicia el compilador. Las llamadas reales mediante QProcess
están en Ventana.compilar (F5) y ComprobadorCcom.iniciar (análisis con --check).
Ambos utilizan ejecutable para localizarlo y diagnosticos para leer sus errores.

La entrada de diagnosticos es texto de salida del compilador, no código Calser.
Solo se reconocen los formatos previstos; la interfaz conserva por separado la
salida completa para que resúmenes o explicaciones no interpretadas sigan visibles.
"""
import os
import re
import shutil
from pathlib import Path
# Reutilizar el modelo común mantiene también disponible ccom.Problema para
# código existente que lo importaba desde aquí antes de separarlo en otro módulo.
from ide.diagnosticos import Problema


def ejecutable(root, configured=""):
    """Devuelve la ruta absoluta del primer ccom utilizable, o None.

    Se busca en este orden: ruta configurada, raíz del repositorio y PATH del
    sistema. Cada candidato debe ser un archivo con permiso de ejecución. Una
    ruta configurada que dejó de funcionar permite probar las alternativas;
    esta comprobación no ejecuta ccom ni verifica su versión o compatibilidad.
    """
    candidates = [configured, str(Path(root) / "ccom"), shutil.which("ccom")]
    for value in candidates:
        if value and Path(value).is_file() and os.access(value, os.X_OK):
            return str(Path(value).resolve())
    return None


def diagnosticos(text, source, cwd):
    """Convierte los mensajes reconocidos en una lista de Problema.

    text es la salida textual que entrega el llamador; source es el archivo de
    referencia y cwd el directorio desde el que se ejecutó ccom. Ambos permiten
    resolver mensajes que utilizan rutas relativas o el nombre genérico ccom.

    Formatos admitidos:
        archivo.cls:línea:columna: mensaje
        archivo.cls: error semantico: mensaje general sin posición

    Las columnas y longitudes se mantienen en la unidad de ccom (UTF-8). Este
    lector no abre el fuente, no convierte a posiciones Qt y no determina si el
    proceso tuvo éxito: esas tareas corresponden a otros componentes.
    """
    result = []
    lines = text.splitlines()
    for index, line in enumerate(lines):
        # Primero se intenta el formato con posición. El nombre puede contener
        # espacios o dos puntos: el resto del patrón debe encontrar dos números.
        match = re.match(r"^(.+?):(\d+):(\d+):\s*(.+)$", line)
        general = False
        if match:
            filename, row, column, message = match.groups()
        else:
            # Sin línea/columna solo se admiten encabezados explícitos de error.
            # Así no se convierten resúmenes o líneas del fuente en problemas.
            match = re.match(
                r"^(\S.*?):\s*(error(?: (?:lexico|léxico|sintactico|sintáctico|semantico|semántico))?:\s*.+)$",
                line, re.IGNORECASE)
            if not match:
                continue
            filename, message = match.groups()
            row = column = None
            general = True
        # Un mensaje firmado como ccom se refiere al fuente de esta llamada.
        # Las rutas relativas se interpretan respecto al directorio del proceso,
        # que no tiene por qué coincidir con el directorio actual de la aplicación.
        path = Path(source) if filename == "ccom" else Path(filename)
        if not path.is_absolute():
            path = Path(cwd) / path
        # Los generales no tienen rango. Si hay posición pero no un señalador
        # reconocible, se conserva una longitud mínima de una unidad.
        length = 0 if general else 1
        # Formato del señalador: una línea «N | código» y después «| ^^^».
        # Contar sus ^ permite conservar el rango informado por el compilador;
        # deducirlo de palabras del mensaje podría subrayar otro fragmento.
        if not general and index + 2 < len(lines) and re.match(r"^\s*\d+\s*\|", lines[index + 1]):
            marker = re.match(r"^\s*\|[ \t]*(\^+)\s*$", lines[index + 2])
            if marker:
                length = len(marker.group(1))
        # Los problemas generales conservan None en línea/columna. Los ubicados
        # se normalizan a una base mínima de 1; el editor validará el rango real.
        result.append(Problema(path.resolve(), None if general else max(1, int(row)),
                               None if general else max(1, int(column)), message, length))
    return result
