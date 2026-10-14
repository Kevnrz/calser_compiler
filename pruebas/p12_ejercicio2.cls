Vi m : int [3][4] = [ [  4,  8, 15, 16 ],
                      [ 23, 42,  7,  1 ],
                      [  9,  3,  5,  2 ] ] ;

F buscar [ Vi int mat [3][4], int x, int pos [2] ] : bool hallado ;
    pos[0] = -1 ;
    pos[1] = -1 ;
    S [ i, 0, 3, 1 ] ;
        S [ j, 0, 4, 1 ] ;
            C [ mat[i][j] == x ] ;
                pos[0] = i ;
                pos[1] = j ;
                hallado = verdadero ;
                E ;
            .
        .
    .
    E ;
.

F main [] : void ;
    Vm p : int [2] ;
    Vm ok : bool ;
    ok = buscar [ m, 42, p ] ;
    E ;
.
