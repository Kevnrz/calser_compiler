# Ejecutor opcional de pruebas

`tools/probar.py` reúne las suites existentes sin incorporar pruebas al
compilador. Requiere Python 3.9 o posterior y no abre ventanas ni navegadores.
Sin argumentos corre todas las suites; también se pueden elegir varias:

```sh
python3 tools/probar.py --help
python3 tools/probar.py --listar
python3 tools/probar.py matrices tiempo ayuda
python3 tools/probar.py --detalle matrices
python3 tools/probar.py --detener-al-fallar
```

Por defecto prepara los binarios necesarios en una sola llamada a Make y luego
ejecuta directamente las pruebas con el mismo Python que inició el script.
Para usar binarios ya construidos, sin invocar Make:

```sh
python3 tools/probar.py --sin-compilar matrices programas
```

En ese modo los binarios deben existir y estar actualizados. Las suites siguen
necesitando sus auxiliares de `build/` cuando corresponda. Los errores de una
suite no impiden ejecutar las siguientes, salvo con `--detener-al-fallar`.
Una suite con varios comandos se detiene al fallar uno de ellos.

Al finalizar muestra cuántas suites aprobaron, fallaron o fueron omitidas y el
tiempo total. Son conteos de suites, no de sus casos internos. Por defecto solo
muestra los logs de comandos fallidos; `--detalle` muestra también los exitosos.
`--timeout SEGUNDOS` fija el límite por comando, incluida la construcción
(300 segundos por defecto). Si falla la construcción no se ejecutan las suites.

## Un archivo concreto

```sh
python3 tools/probar.py --caso pruebas/p02_factorial.cls
python3 tools/probar.py --caso pruebas/e01_no_declarado.cls --codigo-esperado 1
```

`--caso` compila con `-s -m` y compara el código de salida con 0, o con 1 si se
indica `--codigo-esperado 1`. Esto permite verificar aceptación o rechazo, pero
no valida el texto exacto del diagnóstico ni el resultado del programa en el
simulador. Para inspeccionar tablas y diagnósticos usar `--detalle`.
Las salidas de ese caso van a un directorio temporal y se eliminan al terminar.
No se combina con nombres de suites.

El script funciona desde cualquier directorio: localiza el repositorio a partir
de su propia ruta. Las rutas de `--caso` se interpretan desde el directorio donde
lo invocaste. Por ejemplo:

```sh
python3 /home/saddygr/Documentos/GitHub/calser_compiler/tools/probar.py --sin-compilar matrices
```

## Suites y archivos generados

`--listar` muestra aritmética, arreglos, crypto, AST, ensamblador, programas,
ayuda, tiempo, matrices, etiquetas, simulador, bundles, salidas, aceptación de
bundles y las pruebas del propio ejecutor. Estas últimas usan comandos simulados
para comprobar selección, fallos y resumen; no vuelven a lanzar todas las suites.

La suite `validar-bundles` conserva sus reportes en `build/validacion_bundles/`,
como la herramienta original. La construcción escribe los binarios en `build/`
y `ccom`; el resto de las salidas temporales mantiene el manejo de cada suite.
No se invoca `pruebas/correr_pruebas.sh` ni se sobrescribe `pruebas/resultados.txt`.

Códigos de salida del ejecutor: 0 si todo lo seleccionado aprobó (también para
ayuda/listado), 1 ante fallo de construcción o pruebas, 2 por argumentos
incorrectos y 130 si se interrumpe con Ctrl+C.

`make check` sigue disponible e incluye las pruebas del ejecutor mediante
`make check-runner`. El uso del script es opcional durante la presentación.
