F f [ int a ] : int r ;
    r = a + 1 ;
    E ;
.

F main [] : void ;
    Vm v : int [4] = [ 1 , 2 , 3 , 4 ] ;
    Vm w : int [4] ;
    Vm y : int ;
    Vm s : int = 0 ;
    Vm i : int = 0 ;
    Vm z : int ;
    Vm q : int ;
    w [3] = 7 ;
    y = w [3] ;
    L [ i < 4 ] ;
        s = s + v [ i ] ;
        i = i + 1 ;
    .
    z = f [ v [ 2 ] ] ;
    q = f [ 5 ] ;
    E ;
.
