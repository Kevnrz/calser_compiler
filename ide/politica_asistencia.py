"""Política conservadora del IDE. No genera propuestas ni confía solo en su etiqueta."""
from dataclasses import dataclass
from ide.ediciones import aplicar_ediciones
from ide.nucleo.parser import analizar_sintaxis

PREDETERMINADAS = {'habilitada': True, 'sugerencias_pausa': False, 'pausa_ms': 700,
                   'reparaciones_automaticas': False, 'reglas_automaticas': []}
MAX_TEXTO_AUTOMATICO = 100_000


def validar_preferencias(datos):
    salida = dict(PREDETERMINADAS, reglas_automaticas=[])
    if not isinstance(datos, dict):
        return salida
    for campo in ('habilitada', 'sugerencias_pausa', 'reparaciones_automaticas'):
        if type(datos.get(campo)) is bool:
            salida[campo] = datos[campo]
    if type(datos.get('pausa_ms')) is int and 300 <= datos['pausa_ms'] <= 3000:
        salida['pausa_ms'] = datos['pausa_ms']
    reglas = datos.get('reglas_automaticas')
    if isinstance(reglas, list) and len(reglas) <= 64:
        salida['reglas_automaticas'] = list(dict.fromkeys(r for r in reglas
            if isinstance(r, str) and 0 < len(r) <= 100 and r.strip() == r and '\x00' not in r))
    return salida


@dataclass(frozen=True)
class ReglaAutomatica:
    codigo: str
    descripcion: str
    insercion: str

    def __post_init__(self):
        if (not isinstance(self.codigo, str) or not self.codigo.strip() or len(self.codigo) > 100
                or not isinstance(self.descripcion, str) or not self.descripcion.strip()
                or self.insercion not in (';', '.', ']', ')')):
            raise ValueError('La regla necesita código, descripción e inserción de un delimitador permitido')


# Añadir solo reglas reales revisadas con sus pruebas positivas y negativas.
# No importar un catálogo declarado por el motor como autorización automática.
REGLAS_AUTOMATICAS: tuple[ReglaAutomatica, ...] = ()


def evaluar_automatizacion(contexto, resultado, reglas, *, cancelar=None):
    """Retornar (propuesta o None, motivo). Se ejecuta en el trabajador del motor."""
    if cancelar and cancelar():
        return None, 'Consulta cancelada'
    if len(contexto.texto) > MAX_TEXTO_AUTOMATICO:
        return None, 'Documento demasiado grande para corrección automática'
    if contexto.cursor != contexto.ancla or len(resultado.propuestas) != 1:
        return None, 'Se requiere una sola propuesta y ninguna selección'
    r = resultado.propuestas[0]
    regla = next((regla for regla in reglas if regla.codigo == r.codigo), None)
    if not regla or not r.automatica_segura or not r.razones or not r.validacion.strip():
        return None, 'Regla no habilitada o sin justificación automática'
    if len(r.ediciones) != 1 or r.cursor_final is not None or not r.precondiciones:
        return None, 'Solo se permite una inserción local con precondiciones y sin mover el cursor'
    texto = contexto.texto
    salto = texto.rfind('\n', 0, contexto.cursor)
    if salto < 0 or texto[salto + 1:contexto.cursor].strip():
        return None, 'No corresponde a una línea recién confirmada'
    inicio_linea = texto.rfind('\n', 0, salto) + 1
    fin = len(texto[:salto].rstrip(' \t'))
    e = r.ediciones[0]
    if (fin <= inicio_linea or e.inicio != fin or e.fin != fin
            or e.texto.strip(' \t') != regla.insercion or len(e.texto) > 8):
        return None, 'La propuesta no inserta únicamente el delimitador al final de la línea confirmada'
    if not any(inicio_linea <= p.inicio < p.fin <= fin and p.texto_esperado
               for p in r.precondiciones):
        return None, 'Falta una precondición no vacía sobre la línea confirmada'
    corregido = aplicar_ediciones(texto, r.ediciones, r.precondiciones)
    antes = analizar_sintaxis(texto, cancelar=cancelar)
    if antes.estado != 'error_sintactico':
        return None, 'El original no presenta un error sintáctico comprobado'
    despues = analizar_sintaxis(corregido, cancelar=cancelar)
    if not despues.aceptado:
        return None, 'El parser no acepta por completo el texto corregido'
    if cancelar and cancelar():
        return None, 'Consulta cancelada'
    return r, 'Regla habilitada, precondiciones vigentes y programa aceptado por el parser'
