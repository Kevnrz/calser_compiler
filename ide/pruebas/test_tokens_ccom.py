"""M2: transporte léxico, presentación y cancelación del ejecutable real."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch
from ide import ccom
from ide.adaptador_nucleo import analizar_documento, convertir_lexico

ROOT = Path(__file__).resolve().parents[2]
FUENTE = Path('/tmp/snapshot-flex.cls')


def informe(texto='int x', tokens=None):
    return {'schema_version': 1, 'operation': 'lex', 'offset_unit': 'utf8',
            'source': str(FUENTE), 'status': 'ok', 'exit_code': 0, 'error_count': 0,
            'truncated': False, 'message': '', 'diagnostics': [], 'fatal': False,
            'tokens': tokens if tokens is not None else [{'kind': 'tipo', 'start': 0, 'end': 3},
                                                        {'kind': 'identificador', 'start': 4, 'end': 5}]}


class TransporteFlex(unittest.TestCase):
    def leer(self, datos, texto='int x'):
        return ccom.lexico_json(json.dumps(datos), FUENTE, texto, 0)

    def test_categorias_vienen_del_servicio_sin_reglas_python(self):
        datos = informe('x', [{'kind': 'numero', 'start': 0, 'end': 1}])
        resultado = convertir_lexico('d', 4, 'x', json.dumps(datos), FUENTE, 0)
        self.assertEqual(resultado.tramos[0][0].categoria, 'numero')
        self.assertEqual((resultado.documento, resultado.revision), ('d', 4))

    def test_tokens_fuera_de_rango_solapados_no_enteros_o_estilo_desconocido(self):
        casos = ([{'kind': 'inventado', 'start': 0, 'end': 1}],
                 [{'kind': 'tipo', 'start': -1, 'end': 1}],
                 [{'kind': 'tipo', 'start': 0, 'end': 99}],
                 [{'kind': 'tipo', 'start': False, 'end': 1}],
                 [{'kind': 'tipo', 'start': 0, 'end': 0}],
                 [{'kind': 'tipo', 'start': 0, 'end': 3}, {'kind': 'tipo', 'start': 2, 'end': 4}],
                 [None], None)
        for tokens in casos:
            datos = informe(); datos['tokens'] = tokens
            with self.subTest(tokens=tokens), self.assertRaises(ccom.ErrorProtocolo):
                self.leer(datos)

    def test_rechaza_tokens_que_cortan_caracter_utf8(self):
        for inicio, fin in ((1, 4), (0, 2)):
            datos = informe('😀', [{'kind': 'comentario', 'start': inicio, 'end': fin}])
            with self.assertRaises(ccom.ErrorProtocolo):
                self.leer(datos, '😀')

    def test_fatal_y_campos_obligatorios(self):
        for cambio in ({'fatal': True}, {'fatal': 'no'}, {'operation': 'check'}, {'tokens': [] , 'truncated': True}):
            datos = informe(); datos.update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo): self.leer(datos)
        datos = informe(); del datos['tokens']
        with self.assertRaises(ccom.ErrorProtocolo): self.leer(datos)

    def test_comentario_multilinea_utf16_y_linea_vacia(self):
        texto = '#[ a😀\n\nfin ]# int'
        resultado = analizar_documento('doc', 8, texto)
        self.assertEqual(resultado.tramos[0][0].longitud, 6)
        self.assertNotIn(1, resultado.tramos)
        self.assertEqual(resultado.tramos[2][-1].categoria, 'tipo')
        self.assertEqual(resultado.tramos[2][-1].inicio, 7)

    def test_flex_manda_prefijos_reservadas_y_rangos_de_errores(self):
        resultado = analizar_documento('doc', 1, 'Vmx intx 2147483649 ' + 'a' * 33)
        self.assertEqual([t.categoria for t in resultado.tramos[0]][:3],
                         ['reservada', 'identificador', 'identificador'])
        self.assertEqual(len(resultado.problemas), 2)
        self.assertTrue(all(p.origen == 'ccom léxico' and p.unidad == 'utf8' for p in resultado.problemas))
        self.assertTrue(all(p.rango_utf8 for p in resultado.problemas))

    def test_no_hay_fallback_al_lexer_python(self):
        with patch('ide.nucleo.lexico.analizar', side_effect=AssertionError('No llamar al lexer Python')):
            resultado = analizar_documento('doc', 1, 'Vm x : bool = verdadero ;')
        self.assertFalse(resultado.problemas)
        with patch.object(ccom, 'ejecutable', return_value=None), self.assertRaisesRegex(ValueError, 'ccom no disponible'):
            analizar_documento('doc', 2, 'int')

    def test_cancelacion_mata_el_proceso_y_limpia_snapshot(self):
        with tempfile.TemporaryDirectory() as tmp:
            carpeta = Path(tmp)
            registro = carpeta / 'registro'
            falso = carpeta / 'ccom lento'
            falso.write_text('#!/usr/bin/python3\nimport os,sys,time\nfrom pathlib import Path\n'
                             f'Path({str(registro)!r}).write_text(str(os.getpid())+"\\n"+sys.argv[-1])\ntime.sleep(20)\n')
            falso.chmod(0o755)
            inicio = time.monotonic()
            with self.assertRaises(InterruptedError):
                analizar_documento('doc', 1, 'int', ejecutable=str(falso), cancelar=registro.exists)
            self.assertLess(time.monotonic() - inicio, 2)
            pid, snapshot = registro.read_text().splitlines()
            self.assertFalse(Path(snapshot).exists())
            with self.assertRaises(ProcessLookupError): os.kill(int(pid), 0)

    def test_ejecutable_viejo_o_protocolo_malformado_falla_explicito(self):
        with tempfile.TemporaryDirectory() as tmp:
            falso = Path(tmp) / 'ccom viejo'
            falso.write_text('#!/usr/bin/python3\nprint("sin JSON")\n')
            falso.chmod(0o755)
            with self.assertRaises(ccom.ErrorProtocolo):
                analizar_documento('doc', 1, 'int', ejecutable=str(falso))

    def test_diagnosticos_lexicos_coinciden_con_check_json(self):
        texto = 'F main [] : void ; @ 😀 E ; .'
        with tempfile.TemporaryDirectory() as tmp:
            fuente = Path(tmp) / 'prueba.cls'; fuente.write_text(texto)
            respuestas = []
            for modo in ('--tokens-json', '--check-json'):
                p = subprocess.run([str(ROOT / 'ccom'), modo, str(fuente)], capture_output=True, timeout=5)
                respuestas.append(json.loads(p.stdout)['diagnostics'])
            self.assertEqual(respuestas[0], respuestas[1])

    def test_timeout_del_servicio_no_se_publica_como_lexico_correcto(self):
        with tempfile.TemporaryDirectory() as tmp:
            falso = Path(tmp) / 'ccom detenido'
            falso.write_text('#!/usr/bin/python3\nimport time\ntime.sleep(20)\n')
            falso.chmod(0o755)
            with patch('ide.servicio_ccom.time.monotonic', side_effect=[0, 6]):
                with self.assertRaisesRegex(ValueError, 'Tiempo límite'):
                    analizar_documento('doc', 1, 'int', ejecutable=str(falso))
