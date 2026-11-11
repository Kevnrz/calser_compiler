# Factorial recursivo con R y caso base.
# Esperado (arreglos y matrices por filas): resultado = 120, base = 1

F factorial [ int n, int acumulador ] : int valor ;
    C [ n <= 1 ] ;
        valor = acumulador ;
        E ;
    A ;
        R factorial[n - 1, acumulador * n] ;
    .
.
F main [] : void ;
    Vm resultado : int = factorial[5, 1] ;
    Vm base : int = factorial[0, 1] ;
    E ;
.
