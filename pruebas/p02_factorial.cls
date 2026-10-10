F factorial [ int n ] : int res ;
    C [ n < 0 ] ;
        E ;
    .
    res = 1 ;
    Vm i : int = 2 ;
    L [ i <= n ] ;
        res = res * i ;
        i = i + 1 ;
    .
    E ;
.

F main [] : void ;
    Vi x : int = 5 ;
    Vm f : int ;
    f = factorial [ x ] ;
    E ;
.