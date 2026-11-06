"""Aplicador en el hilo gráfico: contexto estricto y una operación de Deshacer."""
from dataclasses import dataclass
from PySide6.QtCore import QThread
from PySide6.QtGui import QTextCursor
from shiboken6 import isValid

from ide.nucleo.asistencia import ContextoDocumento, Reparacion, Sugerencia, entero
from ide.ediciones import (aplicar_ediciones, mapa_utf16, python_a_utf16,
                           trasladar_posicion, utf16_a_python, validar_ediciones)


@dataclass(frozen=True)
class ResultadoAplicacion:
    estado: str
    explicacion: str = ''

    @property
    def aplicada(self):
        return self.estado == 'aplicada'


def capturar_contexto(editor):
    if not isValid(editor) or QThread.currentThread() != editor.thread():
        raise ValueError('Capturar el contexto requiere un editor abierto en su hilo gráfico')
    texto, cursor = editor.toPlainText(), editor.textCursor()
    return ContextoDocumento(editor.documento_id, editor.revision, texto,
                            utf16_a_python(texto, cursor.position()), utf16_a_python(texto, cursor.anchor()))


def aplicar_ediciones_editor(editor, contexto, ediciones, *, cursor_final=None, precondiciones=()):
    if not isValid(editor):
        return ResultadoAplicacion('obsoleta', 'El editor fue cerrado')
    if QThread.currentThread() != editor.thread():
        return ResultadoAplicacion('invalida', 'Aplicar requiere el hilo gráfico del editor')
    if not isinstance(contexto, ContextoDocumento):
        return ResultadoAplicacion('invalida', 'Falta un ContextoDocumento válido')
    if editor.isReadOnly():
        return ResultadoAplicacion('solo_lectura', 'El documento es de solo lectura')
    try:
        if capturar_contexto(editor) != contexto:
            return ResultadoAplicacion('obsoleta', 'Cambió el documento, su revisión, el texto, el cursor o la selección')
        ordenadas = validar_ediciones(contexto.texto, ediciones, precondiciones)
        resultado = aplicar_ediciones(contexto.texto, ordenadas)
        if cursor_final is not None:
            entero(cursor_final, 'cursor_final')
            if cursor_final > len(resultado):
                raise ValueError('cursor_final fuera del texto resultante')
            posicion = ancla = cursor_final
        else:
            posicion = trasladar_posicion(contexto.cursor, ordenadas, derecha=contexto.cursor >= contexto.ancla)
            ancla = trasladar_posicion(contexto.ancla, ordenadas, derecha=contexto.ancla >= contexto.cursor)
        posiciones = mapa_utf16(contexto.texto, [p for e in ordenadas for p in (e.inicio, e.fin)])
        final = mapa_utf16(resultado, (posicion, ancla))
    except (TypeError, ValueError, UnicodeError) as error:
        return ResultadoAplicacion('invalida', str(error))
    if resultado == contexto.texto:
        return ResultadoAplicacion('sin_cambios', 'La propuesta no modifica el documento')
    bloque = editor.textCursor()
    bloque.beginEditBlock()
    try:
        for edicion in reversed(ordenadas):
            cursor = QTextCursor(editor.document())
            cursor.setPosition(posiciones[edicion.inicio])
            cursor.setPosition(posiciones[edicion.fin], QTextCursor.MoveMode.KeepAnchor)
            cursor.insertText(edicion.texto)
    finally:
        bloque.endEditBlock()
    cursor = editor.textCursor()
    cursor.setPosition(final[ancla])
    cursor.setPosition(final[posicion], QTextCursor.MoveMode.KeepAnchor)
    editor.setTextCursor(cursor)
    return ResultadoAplicacion('aplicada')


def aplicar_sugerencia(editor, contexto, sugerencia):
    if not isinstance(sugerencia, Sugerencia):
        return ResultadoAplicacion('invalida', 'Se esperaba Sugerencia')
    return aplicar_ediciones_editor(editor, contexto, (sugerencia.edicion,), cursor_final=sugerencia.cursor_final)


def aplicar_reparacion(editor, contexto, reparacion):
    if not isinstance(reparacion, Reparacion):
        return ResultadoAplicacion('invalida', 'Se esperaba Reparacion')
    # automatica_segura no dispara nada: esta función solo aplica una propuesta
    # elegida por el llamador. El permiso/disparador automático se añade en E6.
    return aplicar_ediciones_editor(editor, contexto, reparacion.ediciones,
                                   cursor_final=reparacion.cursor_final, precondiciones=reparacion.precondiciones)
