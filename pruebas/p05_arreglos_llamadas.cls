F suma_fila [ Vi int m [2][3] , int f ] : int s ;
    S [ j , 0 , 3 , 1 ] ;
        s = s + m [ f ] [ j ] ;
    .
    E ;
.

F poner [ Vm int v [3] , int k ] : void ;
    v [ k ] = 7 ;
    E ;
.

F main [] : void ;
    Vm t : int [2][3] = [ [ 1 , 2 , 3 ] , [ 4 , 5 , 6 ] ] ;
    Vm w : int [3] ;
    Vm r : int ;
    r = suma_fila [ t , 1 ] ;
    poner [ w , 2 ] ;
    w [ 0 ] = t [ 1 ] [ 2 ] ;
    E ;
.
