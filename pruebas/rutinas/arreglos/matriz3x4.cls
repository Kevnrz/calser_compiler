# Ejercicio 2: buscar 6; posicion lineal 5, fila 1, columna 1.
# Para ejecucion completa necesita el generador de Kevin y las fases 5/6.
F buscar [ Vi int m [3][4] , int x ] : int posicion ;
    posicion = -1 ;
    Vm i : int = 0 ;
    L [ i < 3 ] ;
        Vm j : int = 0 ;
        L [ j < 4 ] ;
            C [ m [ i ] [ j ] == x ] ;
                posicion = i * 4 + j ;
                E ;
            .
            j = j + 1 ;
        .
        i = i + 1 ;
    .
    E ;
.
F main [] : void ;
    Vi matriz : int [3][4] = [ [ 1 , 2 , 3 , 4 ] , [ 5 , 6 , 7 , 8 ] , [ 9 , 10 , 11 , 12 ] ] ;
    Vm posicion : int ;
    Vm fila : int ;
    Vm columna : int ;
    posicion = buscar [ matriz , 6 ] ;
    fila = posicion / 4 ;
    columna = posicion % 4 ;
    E ;
.
