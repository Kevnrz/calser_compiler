"""M1: protocolo del compilador real y validación del transporte del IDE."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from ide import ccom
from ide.pruebas import test_comprobacion as base

ROOT = Path(__file__).resolve().parents[2]
FUENTE = Path('/tmp/instantanea-calside.cls')
TEXTO = 'F main [] : void ;\n    a = 1 ;\n    E ;\n.\n'


def respuesta(texto=TEXTO):
    raw = texto.encode()
    inicio = raw.index(b'a =')
    fila = raw.count(b'\n', 0, inicio) + 1
    columna = inicio - raw.rfind(b'\n', 0, inicio)
    return {'schema_version': 1, 'operation': 'check', 'offset_unit': 'utf8', 'source': str(FUENTE),
            'status': 'errors', 'exit_code': 1, 'error_count': 1, 'truncated': False, 'message': '',
            'diagnostics': [{'code': 'CCOM_SEMANTIC', 'category': 'semantic', 'severity': 'error',
                             'message': "'a' no ha sido declarado",
                             'range': {'start': inicio, 'end': inicio + 1, 'line': fila, 'column': columna}}]}


class Transporte(unittest.TestCase):
    def leer(self, datos, texto=TEXTO, codigo=1):
        return ccom.diagnosticos_json(json.dumps(datos), FUENTE, texto, codigo)

    def test_clasificacion_y_rango_provienen_de_datos_sin_regex(self):
        d = respuesta(); d['diagnostics'][0]['message'] = 'Detalle sin encabezado ni palabras de categoría'
        with patch.object(ccom.re, 'match', side_effect=AssertionError('No usar regex')):
            p = self.leer(d)[0]
        self.assertEqual(p.categoria, 'Semántico')
        self.assertEqual(p.descripcion, d['diagnostics'][0]['message'])
        self.assertEqual(p.codigo, 'CCOM_SEMANTIC')
        self.assertEqual(p.rango_utf8, (d['diagnostics'][0]['range']['start'], d['diagnostics'][0]['range']['end']))

    def test_mensaje_estructurado_con_dos_puntos_no_pierde_detalle(self):
        d = respuesta(); d['diagnostics'][0]['message'] = 'error explicado: conservar todo'
        self.assertEqual(self.leer(d)[0].descripcion, 'error explicado: conservar todo')

    def test_error_general_no_inventa_posicion(self):
        d = respuesta(); d['diagnostics'][0]['range'] = None
        p = self.leer(d)[0]
        self.assertFalse(p.tiene_posicion); self.assertIsNone(p.rango_utf8)
        self.assertEqual(p.longitud, 0)

    def test_programa_correcto_requiere_lista_vacia_y_codigo_cero(self):
        d = respuesta(); d.update(status='ok', exit_code=0, error_count=0, diagnostics=[])
        self.assertEqual(self.leer(d, codigo=0), [])
        with self.assertRaises(ccom.ErrorProtocolo): self.leer(d, codigo=1)

    def test_rechaza_json_vacio_invalido_no_utf8_y_campos_duplicados(self):
        for texto in ('', 'correcto', '[]', '{"schema_version":1,"schema_version":1}', b'\xff', '{"x":NaN}'):
            with self.subTest(texto=texto), self.assertRaises(ccom.ErrorProtocolo):
                ccom.diagnosticos_json(texto, FUENTE, TEXTO, 1)

    def test_rechaza_version_operacion_unidad_fuente_y_salida_incompatibles(self):
        cambios = ({'schema_version': 2}, {'schema_version': True}, {'operation': 'compile'},
                   {'offset_unit': 'unicode'}, {'source': '/otro.cls'}, {'exit_code': True},
                   {'exit_code': 0}, {'status': 'inventado'}, {'truncated': 'false'},
                   {'error_count': 2}, {'error_count': True}, {'diagnostics': None}, {'message': '\ud800'})
        for cambio in cambios:
            d = respuesta(); d.update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo): self.leer(d)

    def test_rechaza_exito_con_problemas_y_error_sin_problemas(self):
        for cambio in ({'status': 'ok', 'exit_code': 0}, {'diagnostics': [], 'error_count': 0}):
            d = respuesta(); d.update(cambio)
            with self.assertRaises(ccom.ErrorProtocolo): self.leer(d, codigo=d['exit_code'])

    def test_error_de_servicio_y_limite_no_equivalen_a_programa_valido(self):
        for estado in ('error', 'limit'):
            d = respuesta(); d.update(status=estado, message='Motivo del servicio', truncated=estado == 'limit')
            with self.assertRaisesRegex(ccom.ErrorProtocolo, 'Motivo del servicio'): self.leer(d)
        d = respuesta(); d['truncated'] = True
        with self.assertRaises(ccom.ErrorProtocolo): self.leer(d)

    def test_rechaza_diagnosticos_malformados_y_unicode_invalido(self):
        for cambio in ({'category': []}, {'category': 'nuevo'}, {'code': 'otro'}, {'severity': 'warning'},
                       {'message': ''}, {'message': '\ud800'}, {'message': 'x' * 4097}):
            d = respuesta(); d['diagnostics'][0].update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo): self.leer(d)
        d = respuesta(); del d['diagnostics'][0]['range']
        with self.assertRaises(ccom.ErrorProtocolo): self.leer(d)

    def test_rechaza_rangos_fuera_de_texto_o_metadatos_inconsistentes(self):
        for cambio in ({'start': -1}, {'end': 999999}, {'start': 40, 'end': 1},
                       {'start': True}, {'line': 0}, {'column': 1}, {'end': TEXTO.index('\n', 20) + 1}):
            d = respuesta(); d['diagnostics'][0]['range'].update(cambio)
            with self.subTest(cambio=cambio), self.assertRaises(ccom.ErrorProtocolo): self.leer(d)

    def test_rango_de_insercion_en_fin_de_linea_se_conserva_vacio(self):
        d = respuesta(); r = d['diagnostics'][0]['range']
        fin = TEXTO.index('\n', 20); base_linea = TEXTO.rfind('\n', 0, fin)
        r.update(start=fin, end=fin, line=2, column=fin - base_linea)
        p = self.leer(d)[0]
        self.assertEqual(p.longitud, 0); self.assertEqual(p.rango_utf8, (fin, fin))

    def test_error_por_byte_amplia_solo_marca_visual_del_emoji(self):
        texto = '😀 a = 1'
        d = respuesta(texto); q = d['diagnostics'][0]
        q.update(category='lexical', code='CCOM_LEXICAL')
        q['range'].update(start=1, end=2, line=1, column=2)
        p = self.leer(d, texto)[0]
        self.assertEqual(p.rango_utf8, (1, 2))
        self.assertEqual((p.columna, p.longitud), (1, 4))

    def test_limita_cantidad_de_diagnosticos_y_tamano_de_respuesta(self):
        d = respuesta(); d.update(diagnostics=d['diagnostics'] * 129, error_count=129)
        with self.assertRaises(ccom.ErrorProtocolo): self.leer(d)
        with self.assertRaises(ccom.ErrorProtocolo):
            ccom.diagnosticos_json(' ' * 1_000_001, FUENTE, TEXTO, 0)

    @unittest.skipUnless((ROOT / 'ccom').is_file(), 'Construir ccom')
    def test_respuesta_real_con_varios_semanticos_y_unicode(self):
        texto = '# café 😀\nF main [] : void ;\n    a = 1 ;\n    b = 2 ;\n    E ;\n.\n'
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'fuente con espacios.cls'; path.write_text(texto)
            r = subprocess.run([str(ROOT / 'ccom'), '--check-json', str(path)], capture_output=True, timeout=10)
            problemas = ccom.diagnosticos_json(r.stdout, path, texto, r.returncode)
            self.assertEqual(len(problemas), 2)
            self.assertEqual([p.linea for p in problemas], [3, 4])
            self.assertTrue(all(p.categoria == 'Semántico' for p in problemas))


@unittest.skipUnless((ROOT / 'ccom').is_file(), 'Construir ccom')
class Interfaz(unittest.TestCase):
    setUp = base.ComprobacionAutomatica.setUp
    cleanup = base.ComprobacionAutomatica.cleanup
    esperar = base.ComprobacionAutomatica.esperar
    escribir = base.ComprobacionAutomatica.escribir
    comprobar = base.ComprobacionAutomatica.comprobar
    fake = base.ComprobacionAutomatica.fake

    def test_servicio_real_varios_errores_en_documento_sin_nombre(self):
        texto = 'F main [] : void ;\n    a = 1 ;\n    b = 2 ;\n    E ;\n.\n'
        e = self.escribir(texto); self.comprobar(e)
        problemas = self.window.auto_results[e]
        self.assertEqual(len(problemas), 2)
        self.assertTrue(all(p.documento == e.documento_id and p.archivo is None for p in problemas))
        self.assertEqual(self.window.problems.topLevelItemCount(), 2)
        self.assertIn('--check-json', self.window.auto_output.toPlainText())
        self.assertNotIn('schema_version', self.window.auto_output.toPlainText())
        self.assertEqual(e.toPlainText(), texto)

    def test_json_invalido_nunca_se_muestra_como_analisis_correcto(self):
        self.fake("print('esto no es JSON')\n")
        e = self.escribir(base.VALIDO)
        self.esperar(lambda: e.estado_ccom == 'No disponible')
        self.assertNotIn(e, self.window.auto_results)
        self.assertIsNone(self.window.comprobador.temporary)
        self.assertIn('JSON', self.window.auto_output.toPlainText())

    def test_json_con_version_no_soportada_produce_fallo_explicito(self):
        d = respuesta(); d.update(schema_version=99, exit_code=0)
        self.fake('print(' + repr(json.dumps(d)) + ')\n')
        e = self.escribir(base.VALIDO)
        self.esperar(lambda: e.estado_ccom == 'No disponible')
        self.assertIn('versión de protocolo', self.window.auto_output.toPlainText())
        self.assertEqual(e.problemas, [])

    def test_transporte_de_errores_lexicos_c_no_corta_caracter_al_navegar(self):
        from ide.interfaz.comprobacion import SolicitudCcom
        # E3 mantiene su filtro léxico en M1. Consultar directamente el servicio
        # verifica la futura integración léxica sin cambiar ese comportamiento.
        self.window.auto_action.setChecked(False)
        self.window.comprobador.activar(True)
        texto = '😀\n' + base.VALIDO
        e = self.escribir(texto)
        self.window.comprobador.programar(SolicitudCcom(e.documento_id, e.revision, texto, None, self.folder))
        self.comprobar(e)
        problemas = self.window.auto_results[e]
        self.assertEqual(len(problemas), 4)
        for p in problemas:
            self.assertEqual(e.rango_problema(p).selectedText(), '😀')
