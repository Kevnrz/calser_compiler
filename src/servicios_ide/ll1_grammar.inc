/* Datos generados por tools/generar_datos_ll1.py desde la referencia auditada.
 * No editar a mano. PRIMERO, SIGUIENTE y la tabla se calculan en C. */
#define LL1_TERMINALS 50
#define LL1_NONTERMINALS 61
#define LL1_PRODUCTIONS 120
#define LL1_RHS_MAX 13
#define LL1_START 50
typedef struct { unsigned short head, count, rhs[LL1_RHS_MAX]; } Production;
static const char *const names[] = {
    "$",
    "!",
    "!=",
    "%",
    "&",
    "(",
    ")",
    "*",
    "+",
    ",",
    "-",
    ".",
    "/",
    ":",
    ";",
    "<",
    "<=",
    "=",
    "==",
    ">",
    ">=",
    "A",
    "C",
    "E",
    "F",
    "L",
    "P",
    "R",
    "S",
    "Vi",
    "Vm",
    "[",
    "]",
    "^",
    "autenticar",
    "bool",
    "cadena",
    "cambiar_ancla",
    "cargar_llave",
    "cerrar_sesion",
    "cifrar",
    "descifrar",
    "falso",
    "id",
    "int",
    "num",
    "sellar",
    "verdadero",
    "void",
    "|",
    "programa",
    "importaciones",
    "globales",
    "funciones",
    "funciones_p",
    "funcion",
    "retorno",
    "params",
    "lista_param",
    "lista_param_p",
    "param",
    "modif",
    "modif_opt",
    "tipo",
    "dims_opt",
    "dims",
    "dims_p",
    "cuerpo",
    "sentencia",
    "decl",
    "decl_p",
    "inicial",
    "lista_init",
    "elems",
    "elems_p",
    "elem",
    "sent_id",
    "sent_id_p",
    "tras_args",
    "condicional",
    "resto_c",
    "tras_a",
    "ciclo_l",
    "ciclo_s",
    "paso",
    "emit",
    "rewind",
    "crypto",
    "args",
    "lista_arg",
    "lista_arg_p",
    "expr",
    "disy",
    "disy_p",
    "conj",
    "conj_p",
    "igualdad",
    "igualdad_p",
    "relacion",
    "relacion_p",
    "aditivo",
    "aditivo_p",
    "multiplicativo",
    "multiplicativo_p",
    "unario",
    "potencia",
    "potencia_p",
    "primario",
    "acceso",
    "acceso_p",
    "indice",
};
static const int flex_tokens[LL1_TERMINALS] = {
    0,
    TOKEN_NOT,
    TOKEN_NOT_EQUAL,
    TOKEN_MODULO,
    TOKEN_AND,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_MULTIPLY,
    TOKEN_PLUS,
    TOKEN_COMMA,
    TOKEN_MINUS,
    TOKEN_DOT,
    TOKEN_DIVIDE,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_ASSIGN,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_A,
    TOKEN_C,
    TOKEN_E,
    TOKEN_F,
    TOKEN_L,
    TOKEN_P,
    TOKEN_R,
    TOKEN_S,
    TOKEN_VI,
    TOKEN_VM,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_POWER,
    TOKEN_AUTENTICAR,
    TOKEN_BOOL,
    TOKEN_STRING,
    TOKEN_CAMBIAR_ANCLA,
    TOKEN_CARGAR_LLAVE,
    TOKEN_CERRAR_SESION,
    TOKEN_CIFRAR,
    TOKEN_DESCIFRAR,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_INT,
    TOKEN_NUMBER,
    TOKEN_SELLAR,
    TOKEN_TRUE,
    TOKEN_VOID,
    TOKEN_OR,
};
static const Production productions[LL1_PRODUCTIONS] = {
    {50, 3, {51, 52, 53}}, /* 1 programa */
    {51, 4, {26, 36, 14, 51}}, /* 2 importaciones */
    {51, 0, {0}}, /* 3 importaciones */
    {52, 2, {69, 52}}, /* 4 globales */
    {52, 0, {0}}, /* 5 globales */
    {53, 2, {55, 54}}, /* 6 funciones */
    {54, 2, {55, 54}}, /* 7 funciones_p */
    {54, 0, {0}}, /* 8 funciones_p */
    {55, 10, {24, 43, 31, 57, 32, 13, 56, 14, 67, 11}}, /* 9 funcion */
    {56, 2, {63, 43}}, /* 10 retorno */
    {56, 1, {48}}, /* 11 retorno */
    {57, 1, {58}}, /* 12 params */
    {57, 0, {0}}, /* 13 params */
    {58, 2, {60, 59}}, /* 14 lista_param */
    {59, 3, {9, 60, 59}}, /* 15 lista_param_p */
    {59, 0, {0}}, /* 16 lista_param_p */
    {60, 4, {62, 63, 43, 64}}, /* 17 param */
    {61, 1, {30}}, /* 18 modif */
    {61, 1, {29}}, /* 19 modif */
    {62, 1, {61}}, /* 20 modif_opt */
    {62, 0, {0}}, /* 21 modif_opt */
    {63, 1, {44}}, /* 22 tipo */
    {63, 1, {35}}, /* 23 tipo */
    {64, 1, {65}}, /* 24 dims_opt */
    {64, 0, {0}}, /* 25 dims_opt */
    {65, 4, {31, 45, 32, 66}}, /* 26 dims */
    {66, 3, {31, 45, 32}}, /* 27 dims_p */
    {66, 0, {0}}, /* 28 dims_p */
    {67, 2, {68, 67}}, /* 29 cuerpo */
    {67, 0, {0}}, /* 30 cuerpo */
    {68, 1, {69}}, /* 31 sentencia */
    {68, 1, {76}}, /* 32 sentencia */
    {68, 1, {79}}, /* 33 sentencia */
    {68, 1, {82}}, /* 34 sentencia */
    {68, 1, {83}}, /* 35 sentencia */
    {68, 1, {85}}, /* 36 sentencia */
    {68, 1, {86}}, /* 37 sentencia */
    {68, 1, {87}}, /* 38 sentencia */
    {69, 6, {61, 43, 13, 63, 64, 70}}, /* 39 decl */
    {70, 1, {14}}, /* 40 decl_p */
    {70, 3, {17, 71, 14}}, /* 41 decl_p */
    {71, 1, {91}}, /* 42 inicial */
    {71, 1, {72}}, /* 43 inicial */
    {72, 3, {31, 73, 32}}, /* 44 lista_init */
    {73, 2, {75, 74}}, /* 45 elems */
    {74, 3, {9, 75, 74}}, /* 46 elems_p */
    {74, 0, {0}}, /* 47 elems_p */
    {75, 1, {91}}, /* 48 elem */
    {75, 1, {72}}, /* 49 elem */
    {76, 2, {43, 77}}, /* 50 sent_id */
    {77, 3, {17, 91, 14}}, /* 51 sent_id_p */
    {77, 4, {31, 88, 32, 78}}, /* 52 sent_id_p */
    {78, 3, {17, 91, 14}}, /* 53 tras_args */
    {78, 6, {31, 91, 32, 17, 91, 14}}, /* 54 tras_args */
    {78, 1, {14}}, /* 55 tras_args */
    {79, 8, {22, 31, 91, 32, 14, 67, 80, 11}}, /* 56 condicional */
    {80, 2, {21, 81}}, /* 57 resto_c */
    {80, 0, {0}}, /* 58 resto_c */
    {81, 6, {31, 91, 32, 14, 67, 80}}, /* 59 tras_a */
    {81, 2, {14, 67}}, /* 60 tras_a */
    {82, 7, {25, 31, 91, 32, 14, 67, 11}}, /* 61 ciclo_l */
    {83, 13, {28, 31, 43, 9, 91, 9, 91, 9, 84, 32, 14, 67, 11}}, /* 62 ciclo_s */
    {84, 1, {45}}, /* 63 paso */
    {84, 2, {10, 45}}, /* 64 paso */
    {85, 2, {23, 14}}, /* 65 emit */
    {86, 6, {27, 43, 31, 88, 32, 14}}, /* 66 rewind */
    {87, 5, {34, 31, 91, 32, 14}}, /* 67 crypto */
    {87, 4, {39, 31, 32, 14}}, /* 68 crypto */
    {87, 9, {38, 31, 45, 9, 45, 9, 91, 32, 14}}, /* 69 crypto */
    {87, 4, {46, 31, 32, 14}}, /* 70 crypto */
    {87, 5, {37, 31, 91, 32, 14}}, /* 71 crypto */
    {87, 11, {40, 31, 91, 9, 91, 9, 91, 9, 45, 32, 14}}, /* 72 crypto */
    {87, 11, {41, 31, 91, 9, 91, 9, 91, 9, 45, 32, 14}}, /* 73 crypto */
    {88, 1, {89}}, /* 74 args */
    {88, 0, {0}}, /* 75 args */
    {89, 2, {91, 90}}, /* 76 lista_arg */
    {90, 3, {9, 91, 90}}, /* 77 lista_arg_p */
    {90, 0, {0}}, /* 78 lista_arg_p */
    {91, 1, {92}}, /* 79 expr */
    {92, 2, {94, 93}}, /* 80 disy */
    {93, 3, {49, 94, 93}}, /* 81 disy_p */
    {93, 0, {0}}, /* 82 disy_p */
    {94, 2, {96, 95}}, /* 83 conj */
    {95, 3, {4, 96, 95}}, /* 84 conj_p */
    {95, 0, {0}}, /* 85 conj_p */
    {96, 2, {98, 97}}, /* 86 igualdad */
    {97, 3, {18, 98, 97}}, /* 87 igualdad_p */
    {97, 3, {2, 98, 97}}, /* 88 igualdad_p */
    {97, 0, {0}}, /* 89 igualdad_p */
    {98, 2, {100, 99}}, /* 90 relacion */
    {99, 3, {15, 100, 99}}, /* 91 relacion_p */
    {99, 3, {19, 100, 99}}, /* 92 relacion_p */
    {99, 3, {16, 100, 99}}, /* 93 relacion_p */
    {99, 3, {20, 100, 99}}, /* 94 relacion_p */
    {99, 0, {0}}, /* 95 relacion_p */
    {100, 2, {102, 101}}, /* 96 aditivo */
    {101, 3, {8, 102, 101}}, /* 97 aditivo_p */
    {101, 3, {10, 102, 101}}, /* 98 aditivo_p */
    {101, 0, {0}}, /* 99 aditivo_p */
    {102, 2, {104, 103}}, /* 100 multiplicativo */
    {103, 3, {7, 104, 103}}, /* 101 multiplicativo_p */
    {103, 3, {12, 104, 103}}, /* 102 multiplicativo_p */
    {103, 3, {3, 104, 103}}, /* 103 multiplicativo_p */
    {103, 0, {0}}, /* 104 multiplicativo_p */
    {104, 2, {10, 104}}, /* 105 unario */
    {104, 2, {1, 104}}, /* 106 unario */
    {104, 1, {105}}, /* 107 unario */
    {105, 2, {107, 106}}, /* 108 potencia */
    {106, 2, {33, 104}}, /* 109 potencia_p */
    {106, 0, {0}}, /* 110 potencia_p */
    {107, 3, {5, 91, 6}}, /* 111 primario */
    {107, 1, {108}}, /* 112 primario */
    {107, 1, {45}}, /* 113 primario */
    {107, 1, {47}}, /* 114 primario */
    {107, 1, {42}}, /* 115 primario */
    {108, 2, {43, 109}}, /* 116 acceso */
    {109, 4, {31, 88, 32, 110}}, /* 117 acceso_p */
    {109, 0, {0}}, /* 118 acceso_p */
    {110, 3, {31, 91, 32}}, /* 119 indice */
    {110, 0, {0}}, /* 120 indice */
};
