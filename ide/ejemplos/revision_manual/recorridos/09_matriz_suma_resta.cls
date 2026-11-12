# Sumar y restar matrices 2x2 elemento por elemento.
# Esperado (arreglos y matrices por filas): suma = [1, 3, 2, 6], resta = [1, -7, 4, 2]

F main [] : void ;
    Vi a : int [2][2] = [[1, -2], [3, 4]] ;
    Vi b : int [2][2] = [[0, 5], [-1, 2]] ;
    Vm suma : int [2][2] ;
    Vm resta : int [2][2] ;
    S [ i, 0, 2, 1 ] ;
        S [ j, 0, 2, 1 ] ;
            suma[i][j] = a[i][j] + b[i][j] ;
            resta[i][j] = a[i][j] - b[i][j] ;
        .
    .
    E ;
.
