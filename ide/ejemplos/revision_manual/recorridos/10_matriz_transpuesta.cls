# Transponer una matriz 2x3 para obtener una 3x2.
# Esperado (arreglos y matrices por filas): original = [1, 2, 3, 4, 5, 6], transpuesta = [1, 4, 2, 5, 3, 6]

F main [] : void ;
    Vi original : int [2][3] = [[1, 2, 3], [4, 5, 6]] ;
    Vm transpuesta : int [3][2] ;
    S [ fila, 0, 2, 1 ] ;
        S [ columna, 0, 3, 1 ] ;
            transpuesta[columna][fila] = original[fila][columna] ;
        .
    .
    E ;
.
