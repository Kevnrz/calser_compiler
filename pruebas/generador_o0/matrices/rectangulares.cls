# 2x3 por 3x2 produce 2x2; el orden inverso produce 3x3.
F main [] : void ;
    Vi a : int [2][3] = [ [1, -2, 3], [0, 4, -1] ] ;
    Vi b : int [3][2] = [ [2, 0], [-1, 5], [3, -2] ] ;
    Vm ab : int [2][2] ;
    Vm ba : int [3][3] ;
    S [ i, 0, 2, 1 ] ;
        S [ j, 0, 2, 1 ] ;
            ab[i][j] = 0 ;
            S [ k, 0, 3, 1 ] ;
                ab[i][j] = ab[i][j] + a[i][k] * b[k][j] ;
            .
        .
    .
    S [ fila, 0, 3, 1 ] ;
        S [ columna, 0, 3, 1 ] ;
            ba[fila][columna] = 0 ;
            S [ interior, 0, 2, 1 ] ;
                ba[fila][columna] = ba[fila][columna] + b[fila][interior] * a[interior][columna] ;
            .
        .
    .
    E ;
.
