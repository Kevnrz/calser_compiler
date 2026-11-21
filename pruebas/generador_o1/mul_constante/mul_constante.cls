Vm positivos : int [ 10 ] ;
Vm negativos : int [ 10 ] ;
Vm vector : int [ 4 ] ;
Vm matriz : int [ 2 ][ 3 ] ;
F main [] : void ;
 Vm x : int = 7 ;
 positivos [ 0 ] = x * 0 ;
 positivos [ 1 ] = x * 1 ;
 positivos [ 2 ] = x * 2 ;
 positivos [ 3 ] = x * 4 ;
 positivos [ 4 ] = x * 3 ;
 positivos [ 5 ] = x * 10 ;
 positivos [ 6 ] = x * -1 ;
 positivos [ 7 ] = x * -5 ;
 positivos [ 8 ] = 4 * x ;
 x = -7 ;
 negativos [ 0 ] = x * 0 ;
 negativos [ 1 ] = x * 1 ;
 negativos [ 2 ] = x * 2 ;
 negativos [ 3 ] = x * 4 ;
 negativos [ 4 ] = x * 3 ;
 negativos [ 5 ] = x * 10 ;
 negativos [ 6 ] = x * -1 ;
 negativos [ 7 ] = x * -5 ;
 negativos [ 8 ] = 4 * x ;
 S [ i , 0 , 4 , 1 ] ;
  vector [ i ] = i * 3 ;
 .
 S [ fila , 0 , 2 , 1 ] ;
  S [ columna , 0 , 3 , 1 ] ;
   matriz [ fila ][ columna ] = fila * 10 + columna ;
  .
 .
 E ;
.
