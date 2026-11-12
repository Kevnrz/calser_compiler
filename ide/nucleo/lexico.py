"""

Estado M6: API de consulta/estudio y pruebas del LL(1), conservada por decisión
explícita. La interfaz productiva usa Flex/Bison en ccom, no este módulo.

lexico.py — analizador léxico de Calser para el IDE.

Reproduce las reglas de grammar/lexer.l (libro §3.5.3, enunciado CE1108
§8.2 y §8.4) sobre un str de Python, sin Flex:

  - prefijo más largo; con igual largo gana la regla que va primero
    (por eso "int" es palabra reservada y "integer" es identificador);
  - espacios, tabulaciones y \\r se ignoran; una tabulación es UNA columna;
  - "#[ ... ]#" es comentario de bloque; si no cierra es error fatal en la
    posición del "#[" y el análisis termina ahí;
  - "#" seguido de algo que no sea "[" es comentario hasta fin de línea;
  - número [0-9]+ fuera de 32 bits (> 2147483648) es error y vale 0;
  - identificador [a-z_][a-zA-Z0-9_]* de más de 32 caracteres es error y
    su valor se trunca a 32;
  - cualquier otro carácter es error y se descarta (recuperación L-01).

Diferencia con Flex: las columnas cuentan caracteres, no bytes. Una "á"
ocupa una columna y da un solo error (Flex da dos, uno por byte). Para el
modelo LL(1) se conserva esa convención; Qt recibe las posiciones del backend C.

El tipo de cada token es el nombre del terminal en la gramática
(gramatica.py): "id", "num", "F", "Vm", "[", "==", "|", ...
"""

from dataclasses import dataclass
from typing import List, Optional, Tuple, Union

# Límites de lexer.l (§8.4).
MAX_IDENTIFICADOR = 32
MAX_LITERAL = 2147483648

# Palabras reservadas que empiezan con minúscula: el patrón de identificador
# también las reconoce, con el mismo largo, y en lexer.l van antes.
RESERVADAS_MINUSCULA = frozenset({
    "int", "bool", "void",
    "autenticar", "cerrar_sesion", "cargar_llave", "sellar",
    "cambiar_ancla", "cifrar", "descifrar",
    "verdadero", "falso",
})

# Palabras reservadas con mayúscula. Ningún identificador empieza con
# mayúscula, así que "Vmx" es Vm seguido de x, igual que en Flex.
RESERVADAS_MAYUSCULA = ("Vm", "Vi", "F", "C", "A", "L", "S", "E", "R")

# Operadores: los de dos caracteres se prueban antes (prefijo más largo).
OPERADORES_2 = ("==", "!=", "<=", ">=")
OPERADORES_1 = frozenset("|&<>+-*/%!^=[]():;,.")


@dataclass(frozen=True)
class Token:
    """Un token. tipo = terminal de la gramática; valor = número o nombre."""
    tipo: str
    lexema: str
    linea: int
    columna: int
    valor: Union[int, str, None] = None

    @property
    def columna_fin(self) -> int:
        """Última columna que ocupa el token (como yylloc.last_column)."""
        return self.columna + len(self.lexema) - 1


@dataclass(frozen=True)
class ErrorLexico:
    """Error léxico con posición; fatal = el análisis se cortó ahí."""
    mensaje: str
    linea: int
    columna: int
    largo: int
    fatal: bool = False


def _mostrar(caracter: str) -> str:
    """Igual que lexer.l: ASCII imprimible tal cual, lo demás como \\xHH."""
    codigo = ord(caracter)
    if 0x20 <= codigo <= 0x7E:
        return caracter
    return "\\x%02X" % codigo if codigo <= 0xFF else "\\u%04X" % codigo


def analizar(texto: str) -> Tuple[List[Token], List[ErrorLexico]]:
    """
    Convierte el texto en tokens. Nunca lanza excepción: los errores se
    devuelven en la segunda lista, como hace el IDE con el subrayado.

    Ejemplo: "Vm x : int = 3;" da
      Vm(1,1) id"x"(1,4) :(1,6) int(1,8) =(1,12) num 3(1,14) ;(1,15)
    """
    tokens: List[Token] = []
    errores: List[ErrorLexico] = []
    i, n = 0, len(texto)
    linea, columna = 1, 1

    def emitir(tipo: str, lexema: str, valor=None) -> None:
        tokens.append(Token(tipo, lexema, linea, columna, valor))

    while i < n:
        c = texto[i]

        # Espacios y saltos de línea.
        if c in " \t\r":
            i += 1
            columna += 1
            continue
        if c == "\n":
            i += 1
            linea += 1
            columna = 1
            continue

        # Comentarios (L-03, L-04).
        if c == "#":
            if texto.startswith("#[", i):
                fin = texto.find("]#", i + 2)
                if fin < 0:
                    errores.append(ErrorLexico(
                        "comentario abierto en la linea %d nunca se cierra"
                        % linea, linea, columna, 2, fatal=True))
                    break
                # Avanza contando líneas y columnas dentro del comentario.
                for ch in texto[i:fin + 2]:
                    if ch == "\n":
                        linea += 1
                        columna = 1
                    else:
                        columna += 1
                i = fin + 2
                continue
            fin = texto.find("\n", i)
            fin = n if fin < 0 else fin
            columna += fin - i
            i = fin
            continue

        # Número: [0-9]+ (L-06).
        if c.isascii() and c.isdigit():
            j = i
            while j < n and texto[j].isascii() and texto[j].isdigit():
                j += 1
            lexema = texto[i:j]
            digitos = lexema.lstrip("0") or "0"
            valor = MAX_LITERAL + 1 if len(digitos) > 10 else int(digitos)
            if valor > MAX_LITERAL:
                errores.append(ErrorLexico(
                    "constante entera fuera del rango de 32 bits",
                    linea, columna, len(lexema)))
                valor = 0
            emitir("num", lexema, valor)
            columna += j - i
            i = j
            continue

        # Identificador o palabra reservada en minúscula (L-02).
        if c == "_" or ("a" <= c <= "z"):
            j = i + 1
            while j < n and (texto[j] == "_" or
                             (texto[j].isascii() and texto[j].isalnum())):
                j += 1
            lexema = texto[i:j]
            if lexema in RESERVADAS_MINUSCULA:
                emitir(lexema, lexema)
            else:
                if len(lexema) > MAX_IDENTIFICADOR:
                    errores.append(ErrorLexico(
                        "identificador excede la longitud maxima",
                        linea, columna, len(lexema)))
                emitir("id", lexema, lexema[:MAX_IDENTIFICADOR])
            columna += j - i
            i = j
            continue

        # Palabras reservadas con mayúscula.
        reservada: Optional[str] = next(
            (p for p in RESERVADAS_MAYUSCULA if texto.startswith(p, i)), None)
        if reservada:
            emitir(reservada, reservada)
            columna += len(reservada)
            i += len(reservada)
            continue

        # Operadores y delimitadores.
        par = texto[i:i + 2]
        if par in OPERADORES_2:
            emitir(par, par)
            columna += 2
            i += 2
            continue
        if c in OPERADORES_1:
            emitir(c, c)
            columna += 1
            i += 1
            continue

        # Carácter no reconocido: se reporta y se descarta (L-01).
        errores.append(ErrorLexico(
            "caracter no reconocido: '%s'" % _mostrar(c), linea, columna, 1))
        columna += 1
        i += 1

    return tokens, errores


def tipos_posibles() -> frozenset:
    """Todos los tipos de token que puede devolver analizar()."""
    return (RESERVADAS_MINUSCULA | frozenset(RESERVADAS_MAYUSCULA)
            | frozenset(OPERADORES_2) | OPERADORES_1 | {"id", "num"})
