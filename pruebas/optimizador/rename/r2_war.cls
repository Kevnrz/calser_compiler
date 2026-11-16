Vm g : int = 0 ;

F f [ int a , int b ] : void ;
    Vm x : int = a ;
    g = x ;
    x = b ;
    g = g + x ;
    x = a + b ;
    g = g + x ;
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
