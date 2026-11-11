# Buscar con retorno temprano; -1 representa un elemento ausente.
# Esperado (arreglos y matrices por filas): encontrado = 2, ausente = -1

F buscar [ Vi int datos [5], int objetivo ] : int posicion ;
    posicion = -1 ;
    S [ i, 0, 5, 1 ] ;
        C [ datos[i] == objetivo ] ;
            posicion = i ;
            E ;
        .
    .
    E ;
.
F main [] : void ;
    Vi entrada : int [5] = [4, 8, 15, 16, 23] ;
    Vm encontrado : int = buscar[entrada, 15] ;
    Vm ausente : int = buscar[entrada, 99] ;
    E ;
.
