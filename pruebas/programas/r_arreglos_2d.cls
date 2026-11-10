Vi a : int [2][2] = [ [ 1 , 2 ] , [ 3 , 4 ] ] ;
Vi b : int [2][2] = [ [ 10 , 20 ] , [ 30 , 40 ] ] ;

F suma [ Vi int v [2][2] , int i , int acc ] : int r ;
    C [ i >= 2 ] ;
        r = acc ;
        E ;
    A ;
        R suma [ b , i + 1 , acc + v [ i ][ 0 ] + v [ i ][ 1 ] ] ;
    .
.

F main [] : void ;
    Vm s : int ;
    s = suma [ a , 0 , 0 ] ;
    E ;
.
