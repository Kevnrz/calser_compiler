Vi tope : int = 3 ;

F es_par [ int n ] : bool r ;
    r = n % 2 == 0 ;
    E ;
.

F total [ Vi int v [3] ] : int s ;
    S [ i , 0 , tope , 1 ] ;
        C [ es_par [ v [ i ] ] & ! ( v [ i ] < 0 ) ] ;
            s = s + v [ i ] ;
        .
    .
    E ;
.

F main [] : void ;
    Vi datos : int [3] = [ 2 , 5 , 8 ] ;
    Vm copia : int [3] ;
    Vm t : int ;
    Vm ok : bool = verdadero ;
    copia = datos ;
    t = total [ datos ] ;
    ok = t > 9 | ok ;
    E ;
.
