# C y A: ejecutar tanto la rama verdadera como la alternativa.
# Esperado (arreglos y matrices por filas): negativo = -1, cero = 0, positivo = 1

F clasificar [ int numero ] : int signo ;
    C [ numero < 0 ] ;
        signo = -1 ;
    A ;
        C [ numero == 0 ] ;
            signo = 0 ;
        A ;
            signo = 1 ;
        .
    .
    E ;
.
F main [] : void ;
    Vm negativo : int = clasificar[-7] ;
    Vm cero : int = clasificar[0] ;
    Vm positivo : int = clasificar[8] ;
    E ;
.
