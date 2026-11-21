F f [ int a , int b ] : void ;
    Vm p : int = a + b ;
    Vm q : int = a - b ;
    C [ p > q ] ;
        p = a + 1 ; q = b + 2 ;
    A ;
        p = a + 3 ; q = b + 4 ;
    .
    E ;
.

F main [] : void ;
    f [ 8 , 2 ] ;
    E ;
.
