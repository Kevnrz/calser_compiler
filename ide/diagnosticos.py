"""Modelo común de los problemas que presenta CALSIDE en el editor y la tabla.

El adaptador del lexer y el lector de mensajes de ccom producen ``Problema``.
La ventana reúne esos resultados y el editor utiliza sus posiciones para
subrayar, mostrar detalles y navegar al fragmento correspondiente.

Las posiciones conservan su unidad de origen: ccom informa columnas en bytes
UTF-8; los motores Python aún pendientes de integración pueden usar Unicode. El editor las convierte a
UTF-16 al construir selecciones de Qt; este módulo no depende de la interfaz.
"""
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Problema:
    """Datos de un diagnóstico, sin widgets ni acciones sobre el documento.

    Es inmutable: para asociar otros datos se construye otra instancia, evitando
    alterar accidentalmente un resultado compartido entre componentes.
    """
    # Puede faltar la ruta en documentos sin guardar. documento permite
    # identificar su pestaña sin depender del nombre o ubicación del archivo.
    archivo: Path | None

    # Línea y columna comienzan en 1. Pueden faltar en errores generales del
    # archivo, por ejemplo «falta main»; esos errores también deben contarse.
    linea: int | None
    columna: int | None

    # Se conserva el mensaje recibido. Las propiedades inferiores obtienen
    # la categoría y una descripción legible sin modificar ese texto original.
    mensaje: str
    longitud: int = 1

    # Los valores predeterminados permiten representar directamente un mensaje
    # de ccom. El adaptador Flex declara origen="ccom léxico" y conserva unidad="utf8".
    # longitud se interpreta junto a la unidad, no como un tamaño universal.
    origen: str = "ccom"
    unidad: str = "utf8"
    documento: str | None = None

    # El servicio JSON ya clasifica en C. Conservar sus datos evita volver a
    # deducir la categoría del texto humano. rango_utf8 guarda el rango nativo,
    # aunque la marca visual se amplíe para no partir un carácter Unicode.
    codigo: str | None = None
    categoria_explicita: str | None = None
    rango_utf8: tuple[int, int] | None = None

    @property
    def tiene_posicion(self):
        """Indica si se puede intentar ubicar el problema en una línea/columna.

        No comprueba que la ubicación exista en el texto actual: esa validación
        corresponde al editor. Un diagnóstico sin posición sigue siendo válido
        para la tabla, aunque no deba inventársele un subrayado.
        """
        return (self.linea is not None and self.columna is not None
                and self.linea > 0 and self.columna > 0)

    @property
    def categoria(self):
        """Usa la categoría estructurada, o el prefijo de lectores clásicos.

        JSON ya aporta la clasificación hecha por C. Para mensajes textuales
        anteriores se admiten variantes con/sin tilde y se utiliza Compilación
        cuando no se reconoce una etapa léxica, sintáctica o semántica.
        """
        if self.categoria_explicita is not None:
            return self.categoria_explicita
        category = self.mensaje.partition(":")[0].lower()
        if "lexico" in category or "léxico" in category:
            return "Léxico"
        if "sintactico" in category or "sintáctico" in category:
            return "Sintáctico"
        if "semantico" in category or "semántico" in category:
            return "Semántico"
        return "Compilación"

    @property
    def descripcion(self):
        """Quita un encabezado de error reconocido, conservando el detalle.

        Por ejemplo, «error semantico: falta main» se muestra como «falta main».
        Si el prefijo no comienza por «error», se conserva el mensaje completo
        para no eliminar información de un formato que no reconocemos.
        """
        if self.categoria_explicita is not None:
            return self.mensaje
        prefix, separator, rest = self.mensaje.partition(":")
        return rest.strip() if separator and prefix.lower().startswith("error") else self.mensaje
