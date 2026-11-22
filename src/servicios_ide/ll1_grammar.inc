/* Datos generados por tools/generar_datos_ll1.py desde la referencia auditada.
 * No editar a mano. PRIMERO, SIGUIENTE y la tabla se calculan en C. */
#define LL1_TERMINALS 48
#define LL1_NONTERMINALS 60
#define LL1_PRODUCTIONS 118
#define LL1_RHS_MAX 13
#define LL1_START 48
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
    "R",
    "S",
    "Vi",
    "Vm",
    "[",
    "]",
    "^",
    "autenticar",
    "bool",
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
    TOKEN_R,
    TOKEN_S,
    TOKEN_VI,
    TOKEN_VM,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_POWER,
    TOKEN_AUTENTICAR,
    TOKEN_BOOL,
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
    {48, 2, {49, 50}}, /* 1 programa */
    {49, 2, {66, 49}}, /* 2 globales */
    {49, 0, {0}}, /* 3 globales */
    {50, 2, {52, 51}}, /* 4 funciones */
    {51, 2, {52, 51}}, /* 5 funciones_p */
    {51, 0, {0}}, /* 6 funciones_p */
    {52, 10, {24, 41, 30, 54, 31, 13, 53, 14, 64, 11}}, /* 7 funcion */
    {53, 2, {60, 41}}, /* 8 retorno */
    {53, 1, {46}}, /* 9 retorno */
    {54, 1, {55}}, /* 10 params */
    {54, 0, {0}}, /* 11 params */
    {55, 2, {57, 56}}, /* 12 lista_param */
    {56, 3, {9, 57, 56}}, /* 13 lista_param_p */
    {56, 0, {0}}, /* 14 lista_param_p */
    {57, 4, {59, 60, 41, 61}}, /* 15 param */
    {58, 1, {29}}, /* 16 modif */
    {58, 1, {28}}, /* 17 modif */
    {59, 1, {58}}, /* 18 modif_opt */
    {59, 0, {0}}, /* 19 modif_opt */
    {60, 1, {42}}, /* 20 tipo */
    {60, 1, {34}}, /* 21 tipo */
    {61, 1, {62}}, /* 22 dims_opt */
    {61, 0, {0}}, /* 23 dims_opt */
    {62, 4, {30, 43, 31, 63}}, /* 24 dims */
    {63, 3, {30, 43, 31}}, /* 25 dims_p */
    {63, 0, {0}}, /* 26 dims_p */
    {64, 2, {65, 64}}, /* 27 cuerpo */
    {64, 0, {0}}, /* 28 cuerpo */
    {65, 1, {66}}, /* 29 sentencia */
    {65, 1, {73}}, /* 30 sentencia */
    {65, 1, {76}}, /* 31 sentencia */
    {65, 1, {79}}, /* 32 sentencia */
    {65, 1, {80}}, /* 33 sentencia */
    {65, 1, {82}}, /* 34 sentencia */
    {65, 1, {83}}, /* 35 sentencia */
    {65, 1, {84}}, /* 36 sentencia */
    {66, 6, {58, 41, 13, 60, 61, 67}}, /* 37 decl */
    {67, 1, {14}}, /* 38 decl_p */
    {67, 3, {17, 68, 14}}, /* 39 decl_p */
    {68, 1, {88}}, /* 40 inicial */
    {68, 1, {69}}, /* 41 inicial */
    {69, 3, {30, 70, 31}}, /* 42 lista_init */
    {70, 2, {72, 71}}, /* 43 elems */
    {71, 3, {9, 72, 71}}, /* 44 elems_p */
    {71, 0, {0}}, /* 45 elems_p */
    {72, 1, {88}}, /* 46 elem */
    {72, 1, {69}}, /* 47 elem */
    {73, 2, {41, 74}}, /* 48 sent_id */
    {74, 3, {17, 88, 14}}, /* 49 sent_id_p */
    {74, 4, {30, 85, 31, 75}}, /* 50 sent_id_p */
    {75, 3, {17, 88, 14}}, /* 51 tras_args */
    {75, 6, {30, 88, 31, 17, 88, 14}}, /* 52 tras_args */
    {75, 1, {14}}, /* 53 tras_args */
    {76, 8, {22, 30, 88, 31, 14, 64, 77, 11}}, /* 54 condicional */
    {77, 2, {21, 78}}, /* 55 resto_c */
    {77, 0, {0}}, /* 56 resto_c */
    {78, 6, {30, 88, 31, 14, 64, 77}}, /* 57 tras_a */
    {78, 2, {14, 64}}, /* 58 tras_a */
    {79, 7, {25, 30, 88, 31, 14, 64, 11}}, /* 59 ciclo_l */
    {80, 13, {27, 30, 41, 9, 88, 9, 88, 9, 81, 31, 14, 64, 11}}, /* 60 ciclo_s */
    {81, 1, {43}}, /* 61 paso */
    {81, 2, {10, 43}}, /* 62 paso */
    {82, 2, {23, 14}}, /* 63 emit */
    {83, 6, {26, 41, 30, 85, 31, 14}}, /* 64 rewind */
    {84, 5, {33, 30, 88, 31, 14}}, /* 65 crypto */
    {84, 4, {37, 30, 31, 14}}, /* 66 crypto */
    {84, 9, {36, 30, 43, 9, 43, 9, 88, 31, 14}}, /* 67 crypto */
    {84, 4, {44, 30, 31, 14}}, /* 68 crypto */
    {84, 5, {35, 30, 88, 31, 14}}, /* 69 crypto */
    {84, 11, {38, 30, 88, 9, 88, 9, 88, 9, 43, 31, 14}}, /* 70 crypto */
    {84, 11, {39, 30, 88, 9, 88, 9, 88, 9, 43, 31, 14}}, /* 71 crypto */
    {85, 1, {86}}, /* 72 args */
    {85, 0, {0}}, /* 73 args */
    {86, 2, {88, 87}}, /* 74 lista_arg */
    {87, 3, {9, 88, 87}}, /* 75 lista_arg_p */
    {87, 0, {0}}, /* 76 lista_arg_p */
    {88, 1, {89}}, /* 77 expr */
    {89, 2, {91, 90}}, /* 78 disy */
    {90, 3, {47, 91, 90}}, /* 79 disy_p */
    {90, 0, {0}}, /* 80 disy_p */
    {91, 2, {93, 92}}, /* 81 conj */
    {92, 3, {4, 93, 92}}, /* 82 conj_p */
    {92, 0, {0}}, /* 83 conj_p */
    {93, 2, {95, 94}}, /* 84 igualdad */
    {94, 3, {18, 95, 94}}, /* 85 igualdad_p */
    {94, 3, {2, 95, 94}}, /* 86 igualdad_p */
    {94, 0, {0}}, /* 87 igualdad_p */
    {95, 2, {97, 96}}, /* 88 relacion */
    {96, 3, {15, 97, 96}}, /* 89 relacion_p */
    {96, 3, {19, 97, 96}}, /* 90 relacion_p */
    {96, 3, {16, 97, 96}}, /* 91 relacion_p */
    {96, 3, {20, 97, 96}}, /* 92 relacion_p */
    {96, 0, {0}}, /* 93 relacion_p */
    {97, 2, {99, 98}}, /* 94 aditivo */
    {98, 3, {8, 99, 98}}, /* 95 aditivo_p */
    {98, 3, {10, 99, 98}}, /* 96 aditivo_p */
    {98, 0, {0}}, /* 97 aditivo_p */
    {99, 2, {101, 100}}, /* 98 multiplicativo */
    {100, 3, {7, 101, 100}}, /* 99 multiplicativo_p */
    {100, 3, {12, 101, 100}}, /* 100 multiplicativo_p */
    {100, 3, {3, 101, 100}}, /* 101 multiplicativo_p */
    {100, 0, {0}}, /* 102 multiplicativo_p */
    {101, 2, {10, 101}}, /* 103 unario */
    {101, 2, {1, 101}}, /* 104 unario */
    {101, 1, {102}}, /* 105 unario */
    {102, 2, {104, 103}}, /* 106 potencia */
    {103, 2, {32, 101}}, /* 107 potencia_p */
    {103, 0, {0}}, /* 108 potencia_p */
    {104, 3, {5, 88, 6}}, /* 109 primario */
    {104, 1, {105}}, /* 110 primario */
    {104, 1, {43}}, /* 111 primario */
    {104, 1, {45}}, /* 112 primario */
    {104, 1, {40}}, /* 113 primario */
    {105, 2, {41, 106}}, /* 114 acceso */
    {106, 4, {30, 85, 31, 107}}, /* 115 acceso_p */
    {106, 0, {0}}, /* 116 acceso_p */
    {107, 3, {30, 88, 31}}, /* 117 indice */
    {107, 0, {0}}, /* 118 indice */
};
