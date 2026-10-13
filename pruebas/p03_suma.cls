# Suma los elementos de un arreglo de 5 enteros

F suma [ Vi int v [5] ] : int total ;
    S [ i , 0 , 5 , 1 ] ;
        total = total + v [ i ] ;
    .
    E ;
.

F main [] : void ;
    Vi datos : int [5] = [ 1 , 2 , 3 , 4 , 5 ] ;
    Vm r : int ;
    r = suma [ datos ] ;
    E ;
.