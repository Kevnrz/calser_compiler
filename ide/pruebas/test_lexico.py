"""
Pruebas de nucleo/lexico.py: cada caso reproduce una regla de
grammar/lexer.l. Correr desde la raíz del repo:
    make check-ide
"""

import unittest

from ide.nucleo.gramatica import CALSER
from ide.nucleo.lexico import analizar, tipos_posibles


def tipos(texto):
    return [t.tipo for t in analizar(texto)[0]]


class PruebasLexico(unittest.TestCase):

    def test_declaracion_con_posiciones(self):
        tokens, errores = analizar("Vm x : int = 3;")
        self.assertEqual(errores, [])
        self.assertEqual(
            [(t.tipo, t.linea, t.columna) for t in tokens],
            [("Vm", 1, 1), ("id", 1, 4), (":", 1, 6), ("int", 1, 8),
             ("=", 1, 12), ("num", 1, 14), (";", 1, 15)])
        self.assertEqual(tokens[1].valor, "x")
        self.assertEqual(tokens[5].valor, 3)

    def test_prefijo_mas_largo_y_reservadas(self):
        # "integer" y "cifrar_total" son identificadores (prefijo más largo);
        # "Vmx" es Vm seguido de x porque ningún id empieza con mayúscula.
        self.assertEqual(tipos("int integer cifrar cifrar_total"),
                         ["int", "id", "cifrar", "id"])
        self.assertEqual(tipos("Vmx Vi F C A L S E R"),
                         ["Vm", "id", "Vi", "F", "C", "A", "L", "S", "E", "R"])
        self.assertEqual(tipos("verdadero falso void bool"),
                         ["verdadero", "falso", "void", "bool"])

    def test_operadores_de_dos_caracteres(self):
        self.assertEqual(tipos("<= >= == != < > = !"),
                         ["<=", ">=", "==", "!=", "<", ">", "=", "!"])

    def test_numero_seguido_de_identificador(self):
        self.assertEqual(tipos("12abc"), ["num", "id"])

    def test_numero_fuera_de_rango(self):
        tokens, errores = analizar("2147483648 2147483649 007")
        self.assertEqual([t.valor for t in tokens], [2147483648, 0, 7])
        self.assertEqual(len(errores), 1)
        self.assertEqual((errores[0].linea, errores[0].columna), (1, 12))

    def test_identificador_largo(self):
        nombre = "a" * 33
        tokens, errores = analizar(nombre)
        self.assertEqual(tokens[0].valor, "a" * 32)
        self.assertEqual(tokens[0].lexema, nombre)
        self.assertEqual(len(errores), 1)

    def test_comentarios(self):
        texto = "x # linea\n# [no es bloque\n#[ uno\n dos ]# y"
        tokens, errores = analizar(texto)
        self.assertEqual(errores, [])
        self.assertEqual([(t.lexema, t.linea, t.columna) for t in tokens],
                         [("x", 1, 1), ("y", 4, 9)])

    def test_comentario_sin_cerrar_es_fatal(self):
        tokens, errores = analizar("x = 1;\n  #[ sin cerrar\n y")
        self.assertEqual(tipos("x = 1;"), [t.tipo for t in tokens])
        self.assertEqual(len(errores), 1)
        self.assertTrue(errores[0].fatal)
        self.assertEqual((errores[0].linea, errores[0].columna), (2, 3))

    def test_caracter_no_reconocido_se_descarta(self):
        tokens, errores = analizar("a @ b\t$")
        self.assertEqual(tipos("a b"), [t.tipo for t in tokens])
        self.assertEqual([e.columna for e in errores], [3, 7])
        self.assertIn("'@'", errores[0].mensaje)

    def test_tabulacion_cuenta_una_columna(self):
        tokens, _ = analizar("\tx")
        self.assertEqual(tokens[0].columna, 2)

    def test_tipos_coinciden_con_terminales(self):
        # Todo terminal de la gramática lo produce el léxico, y al revés.
        self.assertEqual(set(tipos_posibles()), set(CALSER.terminales))


if __name__ == "__main__":
    unittest.main()
