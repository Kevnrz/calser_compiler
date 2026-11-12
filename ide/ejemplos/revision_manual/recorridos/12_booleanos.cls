# Corto circuito: las dos primeras expresiones no llaman a tocar.
# Esperado (arreglos y matrices por filas): conjuncion = 0, disyuncion = 1, antes = 0, evaluada = 1, despues = 1

Vm llamadas : int = 0 ;
F tocar [] : bool retorno ;
    llamadas = llamadas + 1 ;
    retorno = verdadero ;
    E ;
.
F main [] : void ;
    Vm conjuncion : bool = falso & tocar[] ;
    Vm disyuncion : bool = verdadero | tocar[] ;
    Vm antes : int = llamadas ;
    Vm evaluada : bool = verdadero & tocar[] ;
    Vm despues : int = llamadas ;
    E ;
.
