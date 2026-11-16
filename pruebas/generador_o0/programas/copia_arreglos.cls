F doble [ Vi int a [3] , int b [3] ] : int s ;
    Vm loc : int [3] ;
    loc = a ;
    b = loc ;
    s = loc [ 0 ] + loc [ 1 ] + loc [ 2 ] ;
    E ;
.

F fuera [ Vi int q [3] , int r [3] ] : int s ;
    s = doble [ q , r ] ;
    E ;
.

F main [] : void ;
    Vi x : int [3] = [ 4 , 5 , 6 ] ;
    Vm y : int [3] ;
    Vm t : int ;
    t = fuera [ x , y ] ;
    E ;
.
