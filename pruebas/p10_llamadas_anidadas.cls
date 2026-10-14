F identity [int x] : int result ;
    result = x ;
    E ;
    .

F add [int a, int b] : int result ;
    result = a + b ;
    E ;
    .

F main [] : void ;
    Vm sum : int = add[identity[10], identity[20]] ;
    E ;
.