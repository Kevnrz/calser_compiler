Vm salida : int = 0 ;

F f [ int a , int b ] : int r ;
    Vm x : int ;
    x = a + b ;
    r = x * 2 ;
    x = a - b ;
    r = r + x ;
    E ;
.

F main [] : void ;
    salida = f [ 5 , 3 ] ;
    E ;
.
