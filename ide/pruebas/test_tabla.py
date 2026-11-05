"""
Pruebas de nucleo/tabla.py contra los ejemplos de Aho et al. y contra los
números de ide/docs/gramatica_ll1.md. Correr desde la raíz del repo:
    python3 -m unittest discover -s ide/pruebas -t ide
"""

import unittest

from nucleo.gramatica import CALSER, EPSILON, FIN, Gramatica
from nucleo.tabla import construir_tabla

# Aho Ej. 4.28 (gramática de expresiones sin recursión izquierda).
AHO_428 = Gramatica("""
E -> T Ep
Ep -> + T Ep | ε
T -> F Tp
Tp -> * F Tp | ε
F -> ( E ) | id
""")

# Aho Ej. 4.33: if-then-else ambiguo; no es LL(1), conflicto en M[S', e].
AHO_433 = Gramatica("""
S -> i E t S Sp | a
Sp -> e S | ε
E -> b
""")

# C/A de Calser sin la factorización de §2 de gramatica_ll1.md: solo con la
# recursión izquierda quitada. Debe chocar en M[alts, A].
CA_SIN_FACTORIZAR = Gramatica("""
condicional -> C [ x ] ; cuerpo alts residual .
alts -> A [ x ] ; cuerpo alts | ε
residual -> A ; cuerpo | ε
cuerpo -> s cuerpo | ε
""")


class PruebasAho(unittest.TestCase):

    def test_primero_y_siguiente_ej_430(self):
        t = construir_tabla(AHO_428)
        self.assertEqual(t.primero["E"], {"(", "id"})
        self.assertEqual(t.primero["Ep"], {"+", EPSILON})
        self.assertEqual(t.primero["Tp"], {"*", EPSILON})
        self.assertEqual(t.siguiente["E"], {")", FIN})
        self.assertEqual(t.siguiente["Ep"], {")", FIN})
        self.assertEqual(t.siguiente["T"], {"+", ")", FIN})
        self.assertEqual(t.siguiente["F"], {"+", "*", ")", FIN})

    def test_tabla_fig_417(self):
        t = construir_tabla(AHO_428)
        self.assertTrue(t.es_ll1)
        self.assertEqual(len(t.m), 13)       # celdas no vacías de la Fig. 4.17
        self.assertEqual(str(t.entrada("Ep", "+")), "Ep -> + T Ep")
        self.assertEqual(str(t.entrada("Ep", ")")), "Ep -> ε")
        self.assertEqual(str(t.entrada("Tp", "+")), "Tp -> ε")
        self.assertIsNone(t.entrada("E", "+"))

    def test_conflicto_ej_433(self):
        t = construir_tabla(AHO_433)
        self.assertFalse(t.es_ll1)
        self.assertEqual([(a, x) for a, x, _, _ in t.conflictos], [("Sp", "e")])


class PruebasCalser(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.t = construir_tabla(CALSER)

    def test_conteos_del_documento(self):
        # gramatica_ll1.md: 60 no terminales, 47 terminales,
        # 118 producciones, 336 entradas, 0 conflictos.
        self.assertEqual(len(CALSER.no_terminales), 60)
        self.assertEqual(len(CALSER.terminales), 47)
        self.assertEqual(len(CALSER.producciones), 118)
        self.assertEqual(len(self.t.m), 336)
        self.assertEqual(self.t.conflictos, [])

    def test_anulables(self):
        self.assertEqual(len(self.t.anulables()), 21)
        self.assertIn("resto_c", self.t.anulables())
        self.assertNotIn("sentencia", self.t.anulables())

    def test_filas_c_a(self):
        fila = {a: str(p) for a, p in self.t.fila("resto_c").items()}
        self.assertEqual(fila, {"A": "resto_c -> A tras_a",
                                ".": "resto_c -> ε"})
        self.assertEqual(str(self.t.entrada("tras_a", "[")),
                         "tras_a -> [ expr ] ; cuerpo resto_c")
        self.assertEqual(str(self.t.entrada("tras_a", ";")),
                         "tras_a -> ; cuerpo")
        self.assertEqual(str(self.t.entrada("cuerpo", "A")), "cuerpo -> ε")

    def test_menos_binario_y_unario(self):
        self.assertEqual(str(self.t.entrada("aditivo_p", "-")),
                         "aditivo_p -> - multiplicativo aditivo_p")
        self.assertEqual(str(self.t.entrada("multiplicativo_p", "-")),
                         "multiplicativo_p -> ε")
        self.assertEqual(str(self.t.entrada("unario", "-")),
                         "unario -> - unario")

    def test_siguiente_de_expr(self):
        self.assertEqual(self.t.siguiente["expr"], {")", ",", ";", "]"})

    def test_c_a_sin_factorizar_no_es_ll1(self):
        t = construir_tabla(CA_SIN_FACTORIZAR)
        self.assertEqual([(a, x) for a, x, _, _ in t.conflictos],
                         [("alts", "A")])


if __name__ == "__main__":
    unittest.main()
