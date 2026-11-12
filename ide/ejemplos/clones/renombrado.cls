# Clon Tipo II: se renombran variables y cambian constantes.
F main [] : void ;
    Vm suma : int = 2 ;
    Vm indice : int = 1 ;
    L [ indice < 8 ] ;
        suma = suma + indice ;
        indice = indice + 2 ;
    .
    E ;
.
