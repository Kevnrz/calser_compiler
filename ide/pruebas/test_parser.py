"""Contrato del parser, prefijos de edición y comparación sintáctica con Bison."""
from concurrent.futures import ThreadPoolExecutor
from dataclasses import FrozenInstanceError
from pathlib import Path
import subprocess
import tempfile
import unittest

from ide.nucleo.gramatica import FIN, Gramatica
from ide.nucleo.lexico import analizar as analizar_lexico
from ide.nucleo.parser import ParserPredictivo, analizar_sintaxis, contexto_en

ROOT = Path(__file__).resolve().parents[2]
CABECERA = 'F main [] : void ;\n'
VALIDO = CABECERA + '    Vm total : int = 3 + 2 ;\n    E ;\n.\n'


class ParserCompleto(unittest.TestCase):
    def test_programa_y_solo_sintaxis(self):
        for texto in (VALIDO, CABECERA + 'total = saldo + 1 ; E ; .',
                      'Vm global : bool = verdadero ;\n' + VALIDO,
                      'F f [int n] : int res ; res = n ; E ; .\n' + VALIDO):
            with self.subTest(texto=texto):
                r = analizar_sintaxis(texto)
                self.assertTrue(r.aceptado, r)
                self.assertEqual(r.estado, 'aceptado')
                self.assertEqual(r.pila, ())
                self.assertEqual(r.esperados, ())
                self.assertEqual(r.offset_alcanzado, len(texto))

    def test_gramatica_pequena_epsilon_y_tokens_sobrantes(self):
        p = ParserPredictivo(Gramatica('S -> id cola\ncola -> + id cola | ε'))
        for texto in ('x', 'x + y', 'x + y + z'):
            self.assertTrue(p.analizar_sintaxis(texto).aceptado)
        self.assertEqual(p.analizar_sintaxis('x y').diagnosticos[0].esperados, ('$', '+'))
        self.assertFalse(p.analizar_sintaxis('').aceptado)
        vacio = ParserPredictivo(Gramatica('S -> ε'))
        self.assertTrue(vacio.analizar_sintaxis('').aceptado)
        self.assertFalse(vacio.analizar_sintaxis('x').aceptado)

    def test_eof_virtual_y_posicion_tras_crlf(self):
        r = analizar_sintaxis(CABECERA + 'E ;\r\n')
        d = r.diagnosticos[0]
        self.assertEqual(d.codigo, 'SINTAXIS_EOF')
        self.assertEqual(d.encontrado.tipo, FIN)
        self.assertEqual((d.inicio, d.fin), (len(CABECERA + 'E ;\r\n'),) * 2)
        self.assertEqual((d.linea, d.columna), (3, 1))
        self.assertFalse(r.aceptado)

    def test_primer_error_y_pila_util_para_reparaciones(self):
        texto = CABECERA + 'Vm x : int = ; Vm y : int = ; E ; .'
        r = analizar_sintaxis(texto)
        self.assertEqual(r.estado, 'error_sintactico')
        self.assertEqual(len(r.diagnosticos), 1)
        d = r.diagnosticos[0]
        self.assertEqual(d.inicio, texto.index(';', len(CABECERA)))
        self.assertEqual(d.encontrado.lexema, ';')
        self.assertIn('num', d.esperados)
        self.assertIn('id', d.esperados)
        self.assertNotIn(';', d.esperados)
        self.assertEqual(d.pila, r.pila)
        self.assertEqual(d.pila[-1], 'inicial')
        self.assertEqual(d.pila[0], FIN)

    def test_vs_se_detecta_en_e(self):
        texto = CABECERA + 'vs\nE ; .'
        d = analizar_sintaxis(texto).diagnosticos[0]
        self.assertEqual(d.encontrado.lexema, 'E')
        self.assertEqual((d.linea, d.columna), (3, 1))
        self.assertEqual(d.esperados, ('=', '['))

    def test_delimitadores_y_errores_al_final(self):
        for texto in (VALIDO.replace('[]', '['), VALIDO.replace('3 + 2', '(3 + 2'),
                      VALIDO.replace('3 + 2 ;', '3 + 2'), VALIDO.replace('.', ''),
                      VALIDO + 'total'):
            with self.subTest(texto=texto):
                self.assertFalse(analizar_sintaxis(texto).aceptado)

    def test_lexico_no_se_oculta_aceptando_tokens_restantes(self):
        for texto in ('@' + VALIDO, VALIDO + '@', CABECERA + '#[ sin cerrar',
                      CABECERA + 'Vm x : int = 99999999999999999 ; .'):
            r = analizar_sintaxis(texto)
            self.assertEqual(r.estado, 'error_lexico')
            self.assertTrue(r.errores_lexicos)
            self.assertFalse(r.aceptado)
            self.assertEqual(r.diagnosticos, ())

    def test_error_sintactico_anterior_a_error_lexico(self):
        r = analizar_sintaxis('int @')
        self.assertEqual(r.estado, 'error_sintactico')
        self.assertEqual(r.diagnosticos[0].encontrado.lexema, 'int')
        self.assertTrue(r.errores_lexicos)

    def test_unicode_tabs_crlf_y_comentarios(self):
        texto = '# á😀\r\nF main [] : void ;\r\n\tVm x : int = ;\r\n.'
        d = analizar_sintaxis(texto).diagnosticos[0]
        self.assertEqual((d.linea, d.columna), (3, 15))
        self.assertEqual(d.inicio, texto.index(';', texto.index('Vm')))
        self.assertEqual(d.fin, d.inicio + 1)
        self.assertTrue(analizar_sintaxis('#[ α\n😀 ]#\n' + VALIDO).aceptado)

    def test_traza_optativa_limitada_y_snapshot_inmutable(self):
        p = ParserPredictivo(max_traza=3)
        sin = p.analizar_sintaxis(VALIDO)
        r = p.analizar_sintaxis(VALIDO, traza=True)
        self.assertEqual(sin.traza, ())
        self.assertEqual(len(r.traza), 3)
        self.assertTrue(r.traza_truncada)
        self.assertEqual(r.traza[0].accion, 'expandir')
        self.assertEqual(r.traza[0].pila, ('$', 'programa'))
        self.assertEqual(r.traza[0].produccion.cabeza, 'programa')
        self.assertTrue(r.aceptado)
        with self.assertRaises(FrozenInstanceError):
            r.estado = 'otro'

    def test_limites_y_cancelacion(self):
        for p, motivo in ((ParserPredictivo(max_caracteres=4), 'max_caracteres'),
                          (ParserPredictivo(max_pasos=2), 'max_pasos'),
                          (ParserPredictivo(max_pila=2), 'max_pila')):
            r = p.analizar_sintaxis(VALIDO)
            self.assertEqual((r.estado, r.motivo), ('limite', motivo))
            self.assertFalse(r.aceptado)
        self.assertEqual(analizar_sintaxis(VALIDO, cancelar=lambda: True).estado, 'cancelado')
        cuenta = iter([False] * 5 + [True])
        r = analizar_sintaxis(VALIDO, cancelar=lambda: next(cuenta, True))
        self.assertEqual(r.estado, 'cancelado')
        self.assertGreater(r.pasos, 0)

    def test_configuracion_y_parametros_invalidos(self):
        with self.assertRaises(ValueError):
            ParserPredictivo(Gramatica('S -> id | id + id'))
        for nombre in ('max_caracteres', 'max_pasos', 'max_pila', 'max_traza'):
            for valor in (0, -1, True, 1.5):
                with self.assertRaises(ValueError):
                    ParserPredictivo(**{nombre: valor})
        with self.assertRaises(TypeError):
            analizar_sintaxis(None)

    def test_reutilizacion_concurrente_sin_estado_compartido(self):
        p = ParserPredictivo()
        textos = [VALIDO, 'int', '', CABECERA + '.'] * 8
        with ThreadPoolExecutor(max_workers=4) as executor:
            estados = list(executor.map(p.analizar_sintaxis, textos))
        self.assertEqual([r.aceptado for r in estados], [True, False, False, True] * 8)
        self.assertTrue(p.analizar_sintaxis(VALIDO).aceptado)


class ContextosDeEdicion(unittest.TestCase):
    def contexto(self, texto):
        return contexto_en(texto, len(texto))

    def test_inicio_y_tipo(self):
        self.assertEqual(self.contexto('').esperados, ('F', 'Vi', 'Vm'))
        c = self.contexto(CABECERA + 'Vm total : ')
        self.assertTrue(c.valido)
        self.assertEqual(c.esperados, ('bool', 'int'))
        self.assertFalse(c.puede_terminar)

    def test_expresion_y_precedencia(self):
        for prefijo in ('Vm x : int = ', 'Vm x : int = 3 + ', 'Vm x : int = !'):
            c = self.contexto(CABECERA + prefijo)
            self.assertTrue(c.valido)
            esperados = {'(', '-', '!', 'id', 'num', 'verdadero', 'falso'}
            if prefijo == 'Vm x : int = ':
                esperados.add('[')  # Inicializador de arreglo, además de expresión.
            self.assertEqual(set(c.esperados), esperados)
        c = self.contexto(CABECERA + 'Vm x : int = 3 ')
        self.assertIn(';', c.esperados)
        self.assertNotIn(']', c.esperados)
        self.assertNotIn(')', c.esperados)

    def test_first_de_pila_no_follow_global(self):
        p = ParserPredictivo(Gramatica('S -> id A ; | num A )\nA -> + | ε'))
        c = p.contexto_en('x ', 2)
        self.assertEqual(c.esperados, ('+', ';'))
        self.assertNotIn(')', c.esperados)
        self.assertEqual(p.contexto_en('3 ', 2).esperados, (')', '+'))

    def test_no_exigir_fin_de_programa_en_prefijo(self):
        texto = CABECERA + 'Vm x : int = '
        self.assertEqual(analizar_sintaxis(texto).estado, 'error_sintactico')
        self.assertEqual(self.contexto(texto).estado, 'prefijo_valido')
        c = self.contexto(VALIDO)
        self.assertTrue(c.puede_terminar)
        self.assertEqual(c.esperados, ('$', 'F'))

    def test_fragmento_explicito_y_sufijo_ignorado(self):
        texto = 'F main [] : void ; Vm total : void @@@'
        inicio = texto.index('void', texto.index('Vm'))
        c = contexto_en(texto, inicio + 2, inicio)
        self.assertEqual(c.prefijo, 'vo')
        self.assertEqual(c.punto, inicio)
        self.assertEqual(c.esperados, ('bool', 'int'))
        self.assertEqual(c.errores_lexicos, ())
        # El motor de sugerencias filtra: void NO sería válido para esta variable.
        texto = 'F main [] : vo'
        c = contexto_en(texto, len(texto), texto.index('vo'))
        self.assertIn('void', c.esperados)
        self.assertTrue(c.valido)
        # Sin inicio explícito se interpreta el prefijo como tokens completos.
        self.assertEqual(self.contexto(texto).estado, 'error_sintactico')

    def test_cursor_antes_dentro_y_despues_de_identificador(self):
        texto = CABECERA + 'Vm total : int ;'
        inicio = texto.index('total')
        self.assertEqual(contexto_en(texto, inicio).esperados, ('id',))
        dentro = contexto_en(texto, inicio + 2, inicio)
        self.assertEqual(dentro.esperados, ('id',))
        self.assertEqual(dentro.prefijo, 'to')
        self.assertEqual(contexto_en(texto, inicio + 5).esperados, (':',))

    def test_comentarios_y_bordes(self):
        for comentario, tipo in (('# comentario', 'linea'), ('#[ texto', 'bloque'),
                                 ('#[ # no anida\n', 'bloque'), ('#', 'linea')):
            texto = CABECERA + comentario
            c = self.contexto(texto)
            self.assertEqual(c.estado, 'comentario')
            self.assertEqual(c.tipo_comentario, tipo)
            self.assertEqual(c.esperados, ())
            self.assertEqual(c.errores_lexicos, ())
        for texto in (CABECERA + '# comentario\n', CABECERA + '#[ cerrado ]#'):
            self.assertTrue(self.contexto(texto).valido)
        self.assertEqual(self.contexto('int # comentario').estado, 'error_sintactico')
        self.assertEqual(self.contexto('@ # comentario').estado, 'error_lexico')
        texto = CABECERA + '#[ cerrado ]#'
        self.assertEqual(contexto_en(texto, texto.index('cerrado') + 2).estado, 'comentario')
        # Justo antes de escribir # sigue siendo código.
        self.assertTrue(contexto_en(texto, texto.index('#')).valido)

    def test_prefijos_de_programas_validos_y_terminales_esperados(self):
        textos = [VALIDO, (ROOT / 'pruebas/p02_factorial.cls').read_text(),
                  (ROOT / 'pruebas/p09_crypto.cls').read_text()]
        for texto in textos:
            tokens, errores = analizar_lexico(texto)
            self.assertEqual(errores, [])
            lineas = [0] + [i + 1 for i, ch in enumerate(texto) if ch == '\n']
            for token in tokens:
                inicio = lineas[token.linea - 1] + token.columna - 1
                for punto in (inicio, inicio + len(token.lexema)):
                    with self.subTest(punto=punto, token=token.lexema):
                        c = contexto_en(texto, punto)
                        self.assertTrue(c.valido, c)
                antes = contexto_en(texto, inicio)
                self.assertIn(token.tipo, antes.esperados)

    def test_error_lexico_ignorado_en_fragmento_pero_no_antes(self):
        texto = CABECERA + 'V'
        c = contexto_en(texto, len(texto), len(CABECERA))
        self.assertTrue(c.valido)
        self.assertIn('Vm', c.esperados)
        self.assertEqual(c.prefijo, 'V')
        texto = CABECERA + '@ vo'
        c = contexto_en(texto, len(texto), texto.rindex('vo'))
        self.assertEqual(c.estado, 'error_lexico')

    def test_errores_previos_bloquean_y_sufijo_no(self):
        self.assertEqual(self.contexto(CABECERA + 'Vm x int ; ').estado, 'error_sintactico')
        self.assertEqual(self.contexto(CABECERA + '@ ').estado, 'error_lexico')
        texto = CABECERA + 'Vm x : int = ; @'
        self.assertTrue(contexto_en(texto, len(CABECERA)).valido)

    def test_rangos_y_fragmentos_invalidos(self):
        for cursor in (-1, 4, True, None):
            with self.assertRaises(ValueError):
                contexto_en('abc', cursor)
        for inicio in (-1, 4, True):
            with self.assertRaises(ValueError):
                contexto_en('abc', 3, inicio)
        for texto, cursor, inicio in (('total', 4, 2), ('x +', 3, 0), ('x y', 3, 0)):
            with self.assertRaises(ValueError):
                contexto_en(texto, cursor, inicio)

    def test_limites_y_cancelacion_en_contexto(self):
        p = ParserPredictivo(max_caracteres=3)
        self.assertEqual(p.contexto_en('texto', 0).estado, 'limite')
        self.assertEqual(contexto_en('', 0, cancelar=lambda: True).estado, 'cancelado')


@unittest.skipUnless((ROOT / 'ccom').is_file(), 'Construir ccom con make para comparar Bison')
class ComparacionBison(unittest.TestCase):
    def test_corpus_sintactico_sin_semantica(self):
        fuentes = sorted((ROOT / 'pruebas').rglob('*.cls'))
        fuentes += sorted((ROOT / 'ide/ejemplos').rglob('*.cls'))
        comparadas = 0
        with tempfile.TemporaryDirectory(prefix='calser_parser_') as carpeta:
            carpeta = Path(carpeta)
            for fuente in fuentes:
                texto = fuente.read_text(encoding='utf-8')
                nuestro = analizar_sintaxis(texto)
                if nuestro.errores_lexicos:
                    continue  # La equivalencia del lexer tiene sus propias pruebas.
                with self.subTest(fuente=str(fuente.relative_to(ROOT))):
                    salida = carpeta / 'arbol.json'
                    if salida.exists():
                        salida.unlink()
                    bison = subprocess.run([str(ROOT / 'ccom'), '--ast-json', str(salida), str(fuente)],
                                           cwd=carpeta, capture_output=True, text=True, timeout=5)
                    self.assertIn(bison.returncode, (0, 1), bison.stderr)
                    self.assertEqual(nuestro.aceptado, bison.returncode == 0,
                                     f'{nuestro}\n{bison.stderr}')
                    comparadas += 1
        self.assertGreaterEqual(comparadas, 80)
