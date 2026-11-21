Vm datos : int [ 2 ] ;

F tocar [ int k ] : void ;
    E ;
.

F f [ int a , int b ] : void ;
    Vm x : int = a + b ;
    tocar [ a ] ;
    x = x + b ;
    datos [ 0 ] = x ;
    x = datos [ 0 ] ;
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
