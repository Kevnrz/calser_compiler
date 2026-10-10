# Prueba de campos de memoria: globales, bool, 2D, varios parámetros, bloques

Vm contador : int ;
Vi limite : int = 10 ;
Vm bandera : bool = verdadero ;
Vi tabla : int [2][3] = [ [ 1 , 2 , 3 ] , [ 4 , 5 , 6 ] ] ;

F maximo [ Vi int m [2][3] , int f , Vm bool b ] : int mayor ;
    S [ i , 0 , 3 , 1 ] ;
        C [ m [ f ] [ i ] > mayor ] ;
            Vm nuevo : int = m [ f ] [ i ] ;
            mayor = nuevo ;
        .
    .
    E ;
.

F main [] : void ;
    Vm r : int ;
    Vm ok : bool ;
    Vm v : int [4] ;
    r = maximo [ tabla , 1 , bandera ] ;
    L [ r > 0 ] ;
        Vm paso : int = 1 ;
        r = r - paso ;
    .
    E ;
.