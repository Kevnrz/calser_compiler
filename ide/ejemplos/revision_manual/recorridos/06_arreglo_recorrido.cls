# Recorrer un arreglo de cinco elementos, sumar y hallar el mayor.
# Esperado (arreglos y matrices por filas): datos = [3, -2, 7, 0, 4], total = 12, mayor = 7

F main [] : void ;
    Vi datos : int [5] = [3, -2, 7, 0, 4] ;
    Vm total : int = 0 ;
    Vm mayor : int = datos[0] ;
    S [ i, 0, 5, 1 ] ;
        total = total + datos[i] ;
        C [ datos[i] > mayor ] ;
            mayor = datos[i] ;
        .
    .
    E ;
.
