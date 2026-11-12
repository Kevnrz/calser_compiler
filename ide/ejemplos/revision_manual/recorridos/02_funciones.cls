# Llamadas adelantadas y anidadas; los argumentos usan corchetes.
# Esperado (arreglos y matrices por filas): resultado = 14

F main [] : void ;
    Vm resultado : int ;
    resultado = sumar[duplicar[3], duplicar[4]] ;
    E ;
.
F sumar [ int a, int b ] : int suma ;
    suma = a + b ;
    E ;
.
F duplicar [ int n ] : int doble ;
    doble = n * 2 ;
    E ;
.
