"""Motor C de reparaciones adaptado a los modelos del IDE; sin Qt ni parser Python.

C obtiene el primer error, enumera cierres y valida cada copia. Este adaptador
solo valida transporte/precondiciones y convierte rangos UTF-8 a offsets Python.
"""
from ide import ccom
from ide.servicio_ccom import consultar, LimiteServicio
from ide.nucleo.asistencia import (EstadoAsistencia as E, ResultadoReparaciones,
                                  Reparacion, EdicionTexto, PrecondicionTexto)

CODIGOS = {'INSERCION_PUNTO_COMA': ';', 'INSERCION_PUNTO_FINAL': '.',
           'INSERCION_CORCHETE_CIERRE': ']', 'INSERCION_PARENTESIS_CIERRE': ')'}


def interpretar(salida, fuente, texto, codigo):
    """Verifica ediciones y precondiciones contra la copia enviada a C.

    Rangos entrantes: bytes UTF-8. Modelos salientes: caracteres Python. La
    etiqueta automatic_safe no autoriza una edición: preferencias, regla y
    vigencia se evalúan después en politica_asistencia y el controlador Qt.
    Solo se copian datos; ninguna propuesta cambia aquí el buffer ni el disco.
    """
    d = ccom.informe_json(salida, fuente, codigo, 'repair')
    exigir = ccom._requerir
    estados = {e.value: e for e in (E.OK, E.SIN_OPCIONES, E.BLOQUEADO, E.LIMITE, E.ERROR)}
    estado = d.get('status')
    exigir(isinstance(estado, str) and estado in estados, 'estado de reparación desconocido')
    exigir(codigo == (1 if estado in ('error', 'limite') else 0), 'estado/código de reparación inconsistente')
    explicacion = d.get('explanation')
    exigir(ccom._texto_json(explicacion) and (estado == 'ok' or bool(explicacion.strip())), 'explicación inválida')
    opciones = d.get('repairs')
    exigir(isinstance(opciones, list) and len(opciones) <= 4 and bool(opciones) == (estado == 'ok'), 'lista de reparaciones inválida')
    if not opciones:
        return ResultadoReparaciones(estados[estado], explicacion=explicacion)
    raw = texto.encode('utf-8')
    fronteras = {0: 0}; offset = 0
    for i, char in enumerate(texto):
        offset += len(char.encode('utf-8')); fronteras[offset] = i + 1
    resultado, vistos = [], set()
    for r in opciones:
        exigir(isinstance(r, dict), 'reparación inválida')
        identificador = r.get('code')
        exigir(isinstance(identificador, str) and identificador in CODIGOS, 'regla de reparación desconocida')
        exigir(identificador not in vistos, 'regla duplicada'); vistos.add(identificador)
        exigir(ccom._texto_json(r.get('description')) and bool(r['description'].strip()), 'descripción inválida')
        exigir(type(r.get('automatic_safe')) is bool, 'clasificación automática inválida')
        edits = r.get('edits')
        exigir(isinstance(edits, list) and len(edits) == 1 and isinstance(edits[0], dict), 'se requiere una inserción de cierre')
        edit = edits[0]; a, b = edit.get('start'), edit.get('end')
        exigir(type(a) is int and type(b) is int and a == b and a in fronteras and edit.get('text') == CODIGOS[identificador], 'edición/rango de cierre inválido')
        condiciones = r.get('preconditions')
        exigir(isinstance(condiciones, list) and 1 <= len(condiciones) <= 4, 'precondiciones ausentes o inválidas')
        precondiciones = []
        for condicion in condiciones:
            exigir(isinstance(condicion, dict), 'precondición inválida')
            ini, fin = condicion.get('start'), condicion.get('end')
            esperado = condicion.get('expected')
            exigir(type(ini) is int and type(fin) is int and ini in fronteras and fin in fronteras and ini < fin,
                    'precondición vacía o fuera del documento')
            exigir(ccom._texto_json(esperado) and esperado.encode('utf-8') == raw[ini:fin], 'precondición no coincide con la instantánea')
            precondiciones.append(PrecondicionTexto(fronteras[ini], fronteras[fin], esperado))
        razones = r.get('reasons')
        exigir(isinstance(razones, list) and 1 <= len(razones) <= 8 and all(ccom._texto_json(s) and s.strip() for s in razones), 'razones inválidas')
        v = r.get('validation')
        exigir(isinstance(v, dict) and type(v.get('accepted')) is bool and v.get('resolved') is True, 'validación incompleta')
        reached, target = v.get('reached'), v.get('target')
        exigir(type(reached) is int and 0 <= reached <= len(raw) + 1 and type(target) is int and a <= target <= len(raw), 'progreso de validación inválido')
        exigir((v['accepted'] and reached == len(raw) + 1) or
                (not v['accepted'] and (reached >= len(raw) + 1 if target == len(raw) else reached > target + 1)),
                'la validación no supera el error original')
        exigir(ccom._texto_json(v.get('description')) and v['description'].strip(), 'falta descripción de validación')
        exigir(not r['automatic_safe'] or (identificador == 'INSERCION_PUNTO_COMA' and v['accepted'] and target < len(raw)), 'clasificación automática incompatible')
        resultado.append(Reparacion(identificador, r['description'], (EdicionTexto(fronteras[a], fronteras[b], edit['text']),),
            automatica_segura=r['automatic_safe'], razones=tuple(razones),
            precondiciones=tuple(precondiciones), validacion=v['description']))
    return ResultadoReparaciones(E.OK, tuple(resultado), explicacion)


def proponer_reparaciones(texto, *, cancelar=None, root=None, configurado=''):
    """Consulta nativa cancelable. No aplica, guarda ni encadena propuestas."""
    if not isinstance(texto, str): raise TypeError('texto debe ser str')
    cancelar = cancelar or (lambda: False)
    if cancelar(): return ResultadoReparaciones(E.CANCELADO)
    if len(texto) > 2_000_000:
        return ResultadoReparaciones(E.LIMITE, explicacion='Documento superior a 2 millones de caracteres.')
    try:
        salida, fuente, codigo = consultar(texto, ['--repair-json'], root=root, configurado=configurado,
                                           cancelar=cancelar, plazo=2.5, max_salida=1_000_000)
        if cancelar(): return ResultadoReparaciones(E.CANCELADO)
        return interpretar(salida, fuente, texto, codigo)
    except InterruptedError:
        return ResultadoReparaciones(E.CANCELADO)
    except LimiteServicio as error:
        return ResultadoReparaciones(E.LIMITE, explicacion=str(error))
    except (ValueError, OSError, UnicodeError) as error:
        return ResultadoReparaciones(E.ERROR, explicacion=str(error))


def comprobar_sintaxis(texto, *, cancelar=None, root=None, configurado=''):
    """Verificación independiente en C para la política automática del IDE.

    Retorna un estado nativo; un fallo/cancelación se propaga y nunca se trata
    como aceptación. Solo sintaxis: no interpreta errores humanos ni semántica.
    """
    salida, fuente, codigo = consultar(texto, ['--syntax-json'], root=root, configurado=configurado,
        cancelar=cancelar or (lambda: False), plazo=2.5, max_salida=1_000_000)
    d = ccom.informe_json(salida, fuente, codigo, 'syntax')
    estado = d.get('status')
    ccom._requerir(isinstance(estado, str) and estado in ('aceptado', 'error_sintactico', 'error_lexico', 'limite', 'error'), 'estado sintáctico desconocido')
    ccom._requerir(codigo == (0 if estado == 'aceptado' else 1), 'código sintáctico inconsistente')
    return estado
