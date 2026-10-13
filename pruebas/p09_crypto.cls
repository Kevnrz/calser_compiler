Vi origen: int = 4096 ;
Vi cifrado: int = 8192 ;
Vi vuelta: int = 12288 ;
Vi negado: int = 16384 ;

F main [] : void ;
    autenticar [ 1234567 ] ;
    cargar_llave [ 0 , 0 , 19114957 ] ;
    cargar_llave [ 0 , 1 , -1985229329 ] ;
    cargar_llave [ 0 , 2 , 324508639 ] ;
    cargar_llave [ 0 , 3 , 610839776 ] ;
    sellar [ ] ;
    cifrar [ origen , cifrado , 128 , 0 ] ;
    descifrar [ cifrado , vuelta , 128 , 0 ] ;
    cerrar_sesion [ ] ;
    cifrar [ origen , negado , 1 , 0 ] ;
    autenticar [ 999 ] ;
    cambiar_ancla [ 5 ] ;
    Vm cifrar_total : int = 0 ;
    E ;
.
