# L mientras; S con fin exclusivo y paso positivo o negativo.
# Esperado (arreglos y matrices por filas): suma_l = 15, suma_s = 15, descendente = 15, contador = 6

F main [] : void ;
    Vm contador : int = 1 ;
    Vm suma_l : int = 0 ;
    L [ contador <= 5 ] ;
        suma_l = suma_l + contador ;
        contador = contador + 1 ;
    .
    Vm suma_s : int = 0 ;
    S [ i, 1, 6, 1 ] ;
        suma_s = suma_s + i ;
    .
    Vm descendente : int = 0 ;
    S [ j, 5, 0, -1 ] ;
        descendente = descendente + j ;
    .
    E ;
.
