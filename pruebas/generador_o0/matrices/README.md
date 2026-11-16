# Operaciones matriciales

Los programas usan ciclos e índices existentes en Calser. No agregan operadores
entre arreglos. Los resultados se leen de la memoria final del simulador y se
comparan elemento por elemento con valores esperados independientes.

```sh
make check-matrices
```

Con `ccom` ya construido también se puede ejecutar:

```sh
python3 pruebas/matrices/probar.py
```

- `cuadradas.cls`: suma, resta, productos AB y BA distintos, identidad y cero
  a ambos lados, negativos y reutilización del destino del producto. Usa funciones
  con parámetros de arreglos y comprueba que las entradas no cambien.
- `rectangulares.cls`: 2×3 por 3×2 (resultado 2×2) y el orden inverso
  (resultado 3×3), con tres ciclos anidados y valores negativos.

Cada programa se compila con y sin el organizador de bundles. Ambos deben
producir los mismos valores esperados, terminar normalmente y no presentar
avisos de separación. Las salidas se guardan en una carpeta temporal que se
elimina al terminar. `make check` incluye esta suite.

Los valores elegidos no desbordan enteros de 32 bits. Estos casos comprueban
los algoritmos indicados; no implican validación de dimensiones en ejecución,
soporte de aritmética matricial directa ni seguridad ante destinos que se
superpongan con las entradas.
