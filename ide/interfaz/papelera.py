"""Acceso a la papelera mediante la API de instancia de QFile.

El overload estático de PySide puede devolver solo bool aunque su firma anuncie
una tupla. La instancia actualiza fileName() con la ubicación recuperable real.
"""
from PySide6.QtCore import QFile


def enviar_a_papelera(path):
    archivo = QFile(str(path))
    if not archivo.moveToTrash():
        return False, ''
    destino = archivo.fileName()
    return True, destino if destino != str(path) else ''
