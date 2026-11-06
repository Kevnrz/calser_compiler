"""Pruebas del motor de sugerencias LL(1)."""
import unittest

from ide.nucleo.sugerencias import proponer_sugerencias, sugerir


class SugerenciasTests(unittest.TestCase):
    def test_palabra_parcial(self):
        texto = "F main [] : vo"
        r = sugerir(texto, len(texto))
        self.assertEqual(r.estado, "prefijo_valido")
        s = next(s for s in r.sugerencias if s.texto == "void")
        self.assertEqual((s.inicio, s.fin), (12, 14))
        self.assertEqual(texto[:s.inicio] + s.texto + texto[s.fin:],
                         "F main [] : void")
        self.assertFalse(s.automatica_segura)

    def test_continuacion_cuerpo(self):
        texto = "F main [] : void ;\nE ;\n"
        r = proponer_sugerencias(texto, len(texto))
        self.assertIn(".", [s.texto for s in r.sugerencias])

    def test_no_inventa_identificador(self):
        r = sugerir("F ", 2)
        self.assertIn("id", r.esperados)
        self.assertEqual(r.sugerencias, ())

    def test_comentarios(self):
        for texto in ("F main [] : void ;\n# comentario",
                      "F main [] : void ;\n#[ sin cerrar"):
            r = sugerir(texto, len(texto))
            self.assertEqual(r.estado, "comentario")
            self.assertFalse(r.sugerencias)

    def test_prefijo_invalido(self):
        r = sugerir("F main [] : void ;\nVm x : int = ;", len("F main [] : void ;\nVm x : int = ;"))
        self.assertEqual(r.estado, "error_sintactico")
        self.assertFalse(r.sugerencias)

    def test_limite_determinismo(self):
        texto = "F main [] : void ;\n"
        a = sugerir(texto, len(texto), max_resultados=2)
        b = sugerir(texto, len(texto), max_resultados=2)
        self.assertEqual(a, b)
        self.assertTrue(a.limite_alcanzado)
        self.assertEqual(len(a.sugerencias), 2)

    def test_unicode_crlf_tabs(self):
        texto = "# á😀\r\nF main [] : vo"
        r = sugerir(texto, len(texto))
        self.assertEqual(r.sugerencias[0].inicio, texto.index("vo"))
        self.assertEqual(r.sugerencias[0].fin, len(texto))

    def test_errores_argumentos(self):
        with self.assertRaises(ValueError):
            sugerir("F", 2)
        with self.assertRaises(ValueError):
            sugerir("", 0, max_resultados=0)
        with self.assertRaises(TypeError):
            sugerir(None, 0)

    def test_cancelacion(self):
        r = sugerir("F ", 2, cancelar=lambda: True)
        self.assertEqual(r.estado, "cancelado")
        self.assertFalse(r.sugerencias)


if __name__ == "__main__":
    unittest.main()
