# Copiar un arreglo completo y modificar solo el destino desde una funcion.
# Esperado (arreglos y matrices por filas): original = [1, -2, 3, 4], copia = [3, -6, 9, 12]

F escalar [ int destino [4], int factor ] : void ;
    S [ i, 0, 4, 1 ] ;
        destino[i] = destino[i] * factor ;
    .
    E ;
.
F main [] : void ;
    Vi original : int [4] = [1, -2, 3, 4] ;
    Vm copia : int [4] ;
    copia = original ;
    escalar[copia, 3] ;
    E ;
.
