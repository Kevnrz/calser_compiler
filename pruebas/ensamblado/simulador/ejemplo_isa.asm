# Ejemplo oficial de la ISA actualizada, sección 11.1.
# En la copia revisada está en docs/isa.md.
# El salto condicional al propio bundle debe continuar, no finalizar.
inicio:
  { mov r4, 0 | mov r5, 3 | nopi | nopi }
  { nopi | nopi | nopi | nopi }
lazo:
  { saddi r4, r4, 1 | nopi | blt r4, r5, lazo | nopi }
