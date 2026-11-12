"""Servicios para localizar ccom y convertir sus mensajes en diagnósticos.

Este módulo no inicia el compilador. Centraliza localización del ejecutable,
lectura textual de F5 y validación JSON común. ComprobadorCcom usa QProcess
para --check-json; los adaptadores M2–M5 usan servicio_ccom desde trabajadores.
Los estados y decisiones del lenguaje provienen de C; el regex de diagnosticos
solo lee mensajes de la CLI clásica y no reconoce código Calser.

La entrada de diagnosticos es texto de salida del compilador, no código Calser.
Solo se reconocen los formatos previstos; la interfaz conserva por separado la
salida completa para que resúmenes o explicaciones no interpretadas sigan visibles.
"""
import json
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


class ErrorProtocolo(ValueError):
    """Respuesta incompatible, incompleta o fallo explícito del servicio C."""


_CATEGORIAS_JSON = {'lexical': ('Léxico', 'CCOM_LEXICAL'),
                    'syntax': ('Sintáctico', 'CCOM_SYNTAX'),
                    'semantic': ('Semántico', 'CCOM_SEMANTIC'),
                    'compilation': ('Compilación', 'CCOM_COMPILATION')}


def _objeto_json(pares):
    objeto = {}
    for clave, valor in pares:
        if clave in objeto:
            raise ErrorProtocolo('Respuesta JSON con campos duplicados')
        objeto[clave] = valor
    return objeto


def _requerir(condicion, mensaje):
    if not condicion:
        raise ErrorProtocolo('Respuesta de ccom incompatible: ' + mensaje)


def _texto_json(valor, limite=4096):
    if not isinstance(valor, str):
        return False
    try:
        return len(valor.encode('utf-8')) <= limite
    except UnicodeError:
        return False


def informe_json(salida, source, codigo, operacion, *, limite=1_000_000):
    """Cabecera común de los seis servicios JSON, sin interpretar Calser.

    Comprueba UTF-8, duplicados, versión, operación, fuente y código real del
    proceso. Cada adaptador valida después sus estados y datos particulares.
    No se uniforman los estados: por ejemplo, `syntax` y `check` significan
    cosas distintas. No lee archivos ni ejecuta procesos. Retorna un dict nuevo.
    """
    _requerir(isinstance(salida, (str, bytes)) and len(salida) <= limite,
              'informe ausente o demasiado grande')
    _requerir(type(codigo) is int and codigo in (0, 1), 'código de proceso inválido')
    try:
        contenido = salida.decode('utf-8') if isinstance(salida, bytes) else salida
        datos = json.loads(contenido, object_pairs_hook=_objeto_json,
                           parse_constant=lambda valor: _requerir(False, 'constante JSON inválida'))
    except (ValueError, UnicodeError, TypeError, RecursionError) as error:
        raise ErrorProtocolo('ccom terminó sin un diagnóstico reconocible en JSON; '
                            'ejecuta make o configura una versión compatible.') from error
    _requerir(isinstance(datos, dict), 'se esperaba un objeto')
    _requerir(type(datos.get('schema_version')) is int and datos['schema_version'] == 1,
              'versión de protocolo no soportada')
    _requerir(datos.get('operation') == operacion and datos.get('offset_unit') == 'utf8',
              'operación o unidad de posiciones no soportada')
    _requerir(isinstance(datos.get('source'), str) and datos['source'] == str(source),
              'el resultado pertenece a otra fuente')
    _requerir(type(datos.get('exit_code')) is int and datos['exit_code'] == codigo,
              'código de salida inconsistente')
    return datos


def diagnosticos_json(salida, source, texto, codigo, *, operacion="check"):
    """Valida protocolo v1 y devuelve Problema sin interpretar mensajes humanos.

    source/texto identifican la instantánea enviada, no se leen desde el disco.
    Los rangos del servicio son bytes UTF-8 del original. Solo la marca visual
    se amplía si un error léxico señala parte de un carácter; el rango nativo
    queda guardado en rango_utf8. Las ediciones tienen otro contrato más estricto.

    Un ejecutable antiguo, JSON inválido o un estado de error/límite se informa
    con ErrorProtocolo; nunca se recurre silenciosamente al lector textual.
    """
    datos = informe_json(salida, source, codigo, operacion,
                         limite=16_000_000 if operacion == 'lex' else 1_000_000)
    return _problemas_json(datos, source, texto, codigo)


def _problemas_json(datos, source, texto, codigo):
    """Contenido M1/M2 tras validar su cabecera; conserva rangos nativos."""
    _requerir(type(datos.get('truncated')) is bool and _texto_json(datos.get('message')),
              'estado de completitud inválido')
    estado = datos.get('status')
    _requerir(estado in ('ok', 'errors', 'error', 'limit'), 'estado desconocido')
    _requerir(type(datos.get('error_count')) is int and datos['error_count'] >= 0,
              'contador de errores inválido')
    opciones = datos.get('diagnostics')
    _requerir(isinstance(opciones, list) and len(opciones) <= 128, 'lista de diagnósticos inválida')
    if estado in ('error', 'limit'):
        _requerir(codigo == 1 and bool(datos['message'].strip()), 'fallo del servicio sin explicación')
        raise ErrorProtocolo(datos['message'])
    _requerir(not datos['truncated'], 'informe truncado')
    _requerir(datos['error_count'] == len(opciones), 'contador y lista no coinciden')
    _requerir((estado == 'ok' and codigo == 0 and not opciones)
              or (estado == 'errors' and codigo == 1 and bool(opciones)),
              'estado, diagnósticos y código no coinciden')
    raw = texto.encode('utf-8')
    problemas = []
    for d in opciones:
        _requerir(isinstance(d, dict), 'diagnóstico inválido')
        categoria = d.get('category')
        _requerir(isinstance(categoria, str) and categoria in _CATEGORIAS_JSON, 'categoría desconocida')
        etiqueta, identificador = _CATEGORIAS_JSON[categoria]
        _requerir(d.get('code') == identificador and d.get('severity') == 'error',
                  'código o severidad no soportado')
        _requerir(_texto_json(d.get('message')) and bool(d['message'].strip()), 'mensaje inválido')
        _requerir('range' in d, 'falta el campo de rango')
        rango = d['range']
        nativo = None
        linea = columna = None
        largo = 0
        if rango is not None:
            _requerir(isinstance(rango, dict), 'rango inválido')
            _requerir(all(type(rango.get(k)) is int for k in ('start', 'end', 'line', 'column')),
                      'posiciones no enteras')
            inicio, fin = rango['start'], rango['end']
            _requerir(0 <= inicio <= fin <= len(raw), 'rango fuera de la instantánea')
            fila = raw.count(b'\n', 0, inicio) + 1
            base = raw.rfind(b'\n', 0, inicio) + 1
            _requerir(rango['line'] == fila and rango['column'] == inicio - base + 1,
                      'línea/columna no coinciden con el rango')
            _requerir(b'\n' not in raw[inicio:fin], 'el rango atraviesa líneas')
            nativo = (inicio, fin)
            # Un @ es un byte; un carácter no ASCII inválido para Calser puede
            # producir varios errores por byte. Se conserva cada diagnóstico,
            # pero su presentación nunca selecciona solo media letra/emoji.
            while inicio > 0 and inicio < len(raw) and raw[inicio] & 0xc0 == 0x80:
                inicio -= 1
            while fin < len(raw) and raw[fin] & 0xc0 == 0x80:
                fin += 1
            linea, columna, largo = fila, inicio - base + 1, fin - inicio
        problemas.append(Problema(Path(source).resolve(), linea, columna, d['message'], largo,
                                  codigo=identificador, categoria_explicita=etiqueta, rango_utf8=nativo))
    return problemas


ESTILOS_LEXICOS = frozenset(('reservada', 'tipo', 'numero', 'booleano',
                            'operador', 'identificador', 'comentario'))


def lexico_json(salida, source, texto, codigo):
    """Valida tokens clasificados por C; no deduce categorías de sus lexemas.

    Los rangos son bytes UTF-8 [start,end), ordenados y sin solapamientos.
    Comentarios pueden atravesar líneas; los errores conservan el contrato M1.
    Un informe parcial o de otra operación se rechaza antes de aplicar colores.
    """
    datos = informe_json(salida, source, codigo, 'lex', limite=16_000_000)
    problemas = _problemas_json(datos, source, texto, codigo)
    _requerir(type(datos.get('fatal')) is bool, 'estado léxico fatal inválido')
    tokens = datos.get('tokens')
    _requerir(isinstance(tokens, list) and len(tokens) <= 100000, 'lista de tokens inválida')
    raw = texto.encode('utf-8')
    anterior = 0
    for token in tokens:
        _requerir(isinstance(token, dict), 'token inválido')
        clase = token.get('kind')
        _requerir(isinstance(clase, str) and clase in ESTILOS_LEXICOS, 'estilo léxico desconocido')
        inicio, fin = token.get('start'), token.get('end')
        _requerir(type(inicio) is int and type(fin) is int and anterior <= inicio < fin <= len(raw),
                  'tokens desordenados, solapados o fuera del documento')
        _requerir(raw[inicio] & 0xc0 != 0x80 and (fin == len(raw) or raw[fin] & 0xc0 != 0x80),
                  'token corta un carácter UTF-8')
        anterior = fin
    _requerir(not datos['fatal'] or bool(problemas), 'fallo léxico sin diagnóstico')
    _requerir(all(p.categoria == 'Léxico' for p in problemas), 'el servicio léxico devolvió otra fase')
    return tokens, problemas, datos['fatal']
