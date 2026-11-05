"""
Núcleo del IDE de Calser (D4, D5): módulos sin interfaz gráfica.

  lexico     texto -> tokens y errores léxicos (equivalente a grammar/lexer.l)
  gramatica  gramática LL(1) de Calser como datos (ide/docs/gramatica_ll1.md)
  tabla      PRIMERO, SIGUIENTE y tabla predictiva M (Aho §4.4.2-4.4.3)

La interfaz solo llama funciones de estos módulos; ninguno depende de ella.
"""
