# Ejemplo propio: no es el ejemplo de isa.md §11 (esa sección falta).
# Calcula 2 + 2, almacena el resultado y termina sin riesgos RAW.
inicio:
{ mov t0, 2 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ add t0, t0, t0 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | nopi | nopi | sw t0, 0(rz) }
fin_main: { nopi | nopi | jef fin_main | nopi }
