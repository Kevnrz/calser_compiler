Vi constante : int = 3 ;
Vm mutable : int = 4 ;
Vm dinamica : int = mutable + constante ;
Vm datos : int [3] = [ constante, mutable + 1, 7 ] ;
F main [] : void ;
    Vm local : int = dinamica ;
    Vm copia : int [3] ;
    copia = datos ;
    E ;
.
