Vi origen : int = 4096 ;
Vi cifrado : int = 8192 ;
Vi vuelta : int = 12288 ;

F bloques_de [ int total , int tam , int i ] : int b ;
    Vm resto : int = total - i * tam ;
    C [ resto < tam ] ;
        b = resto ;
    A ;
        b = tam ;
    .
    E ;
.

F direccion [ int base , int tam , int i ] : int d ;
    d = base + i * tam * 8 ;
    E ;
.

F preparar_boveda [] : void ;
    cargar_llave [ 0 , 0 , 19114957 ] ;
    cargar_llave [ 0 , 1 , -1985229329 ] ;
    cargar_llave [ 0 , 2 , 324508639 ] ;
    cargar_llave [ 0 , 3 , 610839776 ] ;
    cargar_llave [ 1 , 0 , 286331153 ] ;
    cargar_llave [ 1 , 1 , 572662306 ] ;
    cargar_llave [ 1 , 2 , 858993459 ] ;
    cargar_llave [ 1 , 3 , 1145324612 ] ;
    sellar [ ] ;
    E ;
.

F cifrar_archivo [ int desde , int hacia , int total , int tam ] : int n ;
    n = ( total + tam - 1 ) / tam ;
    S [ i , 0 , n , 1 ] ;
        Vm b : int = bloques_de [ total , tam , i ] ;
        C [ i % 2 == 0 ] ;
            cifrar [ direccion [ desde , tam , i ] , direccion [ hacia , tam , i ] , b , 0 ] ;
        A ;
            cifrar [ direccion [ desde , tam , i ] , direccion [ hacia , tam , i ] , b , 1 ] ;
        .
    .
    E ;
.

F descifrar_archivo [ int desde , int hacia , int total , int tam ] : int n ;
    n = ( total + tam - 1 ) / tam ;
    S [ i , 0 , n , 1 ] ;
        Vm b : int = bloques_de [ total , tam , i ] ;
        C [ i % 2 == 0 ] ;
            descifrar [ direccion [ desde , tam , i ] , direccion [ hacia , tam , i ] , b , 0 ] ;
        A ;
            descifrar [ direccion [ desde , tam , i ] , direccion [ hacia , tam , i ] , b , 1 ] ;
        .
    .
    E ;
.

F main [] : void ;
    Vm n1 : int ;
    Vm n2 : int ;
    autenticar [ -193210613 ] ;
    preparar_boveda [ ] ;
    n1 = cifrar_archivo [ origen , cifrado , 128 , 16 ] ;
    n2 = descifrar_archivo [ cifrado , vuelta , 128 , 16 ] ;
    cerrar_sesion [ ] ;
    E ;
.
