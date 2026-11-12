# Producto 2x3 por 3x2, con tres ciclos y acumulador por celda.
# Esperado (arreglos y matrices por filas): producto = [13, -16, -7, 22]

F main [] : void ;
    Vi a : int [2][3] = [[1, -2, 3], [0, 4, -1]] ;
    Vi b : int [3][2] = [[2, 0], [-1, 5], [3, -2]] ;
    Vm producto : int [2][2] ;
    S [ i, 0, 2, 1 ] ;
        S [ j, 0, 2, 1 ] ;
            producto[i][j] = 0 ;
            S [ k, 0, 3, 1 ] ;
                producto[i][j] = producto[i][j] + a[i][k] * b[k][j] ;
            .
        .
    .
    E ;
.
