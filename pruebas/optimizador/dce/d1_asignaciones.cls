Vm g : int = 0 ;

F tocar [ int k ] : void ;
    g = g + k ;
    E ;
.

F calcular [ int a ] : int r ;
    Vm x : int = a * 2 ;
    Vm y : int = a + 1 ;
    tocar [ a ] ;
    r = y ;
    E ;
.

F main [] : void ;
    Vm v : int ;
    v = calcular [ 3 ] ;
    E ;
.
