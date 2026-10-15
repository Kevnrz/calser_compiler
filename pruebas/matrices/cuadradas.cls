# Operaciones con ciclos e indices: los resultados no comparten los operandos.
F sumar [ Vi int izq [2][2], Vi int der [2][2], int suma [2][2], int resta [2][2] ] : void ;
    S [ i, 0, 2, 1 ] ;
        S [ j, 0, 2, 1 ] ;
            suma[i][j] = izq[i][j] + der[i][j] ;
            resta[i][j] = izq[i][j] - der[i][j] ;
        .
    .
    E ;
.

F multiplicar [ Vi int izq [2][2], Vi int der [2][2], int producto [2][2] ] : void ;
    S [ i, 0, 2, 1 ] ;
        S [ j, 0, 2, 1 ] ;
            producto[i][j] = 0 ;
            S [ k, 0, 2, 1 ] ;
                producto[i][j] = producto[i][j] + izq[i][k] * der[k][j] ;
            .
        .
    .
    E ;
.

F main [] : void ;
    Vi a : int [2][2] = [ [1, -2], [3, 4] ] ;
    Vi b : int [2][2] = [ [0, 5], [-1, 2] ] ;
    Vi identidad : int [2][2] = [ [1, 0], [0, 1] ] ;
    Vi cero : int [2][2] = [ [0, 0], [0, 0] ] ;
    Vm sumado : int [2][2] ;
    Vm restado : int [2][2] ;
    Vm ab : int [2][2] ;
    Vm ba : int [2][2] ;
    Vm ai : int [2][2] ;
    Vm ia : int [2][2] ;
    Vm az : int [2][2] ;
    Vm za : int [2][2] ;
    sumar [ a, b, sumado, restado ] ;
    multiplicar [ a, b, ab ] ;
    multiplicar [ b, a, ba ] ;
    multiplicar [ a, identidad, ai ] ;
    multiplicar [ identidad, a, ia ] ;
    multiplicar [ a, cero, az ] ;
    multiplicar [ cero, a, za ] ;
    # Reusar la salida obliga a reiniciar cada acumulador.
    multiplicar [ a, b, ab ] ;
    E ;
.
