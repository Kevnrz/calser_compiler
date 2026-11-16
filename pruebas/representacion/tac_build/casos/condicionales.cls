F main [] : void ;
    Vm x : int = 0 ;
    C [ x == 0 ] ;
        C [ x < 1 ] ;
            x = 1 ;
        A [ x > 1 ] ;
            x = 2 ;
        A ;
            x = 3 ;
        .
    A [ x == 1 ] ;
        x = 4 ;
    A ;
        x = 5 ;
    .
    E ;
.
