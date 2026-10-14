Vm g : int = 0 ;

F toca [] : bool r ;
    g = g + 1 ;
    r = verdadero ;
    E ;
.

F main [] : void ;
    Vm v : bool = verdadero ;
    Vm f : bool = falso ;
    Vm r1 : bool = falso & toca [ ] ;
    Vm g1 : int = g ;
    Vm r2 : bool = verdadero | toca [ ] ;
    Vm g2 : int = g ;
    Vm r3 : bool = verdadero & toca [ ] ;
    Vm g3 : int = g ;
    Vm r4 : bool = falso | toca [ ] ;
    Vm g4 : int = g ;
    Vm and_ff : bool = f & f ;
    Vm and_fv : bool = f & v ;
    Vm and_vf : bool = v & f ;
    Vm and_vv : bool = v & v ;
    Vm or_ff : bool = f | f ;
    Vm or_fv : bool = f | v ;
    Vm or_vf : bool = v | f ;
    Vm or_vv : bool = v | v ;
    Vm mix1 : bool = f & f | v ;
    Vm mix2 : bool = v | f & f ;
    Vm x : int = 0 ;
    Vm en_c : int = 0 ;
    Vm n : int = 0 ;
    C [ x > 0 & toca [ ] ] ;
        en_c = 1 ;
    .
    Vm g5 : int = g ;
    L [ n < 2 & toca [ ] ] ;
        n = n + 1 ;
    .
    E ;
.
