initial_data bytes=24
  @00000000 = 00000003
  @00000004 = 00000004
  @00000008 = 00000000
  @0000000c = 00000003
  @00000010 = 00000000
  @00000014 = 00000007

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  t0 = mutable
  t1 = constante
  t2 = t0 + t1
  dinamica = t2
  t3 = mutable
  t4 = t3 + 1
  datos[4] = t4
  t5 = dinamica
  local = t5
  copia[0] = 0
  copia[4] = 0
  copia[8] = 0
  t6 = address datos
  t7 = address copia
  t8 = 0
  goto B2
B2: successors [B4, B3]
  if t8 >= 12 goto B4
B3: successors [B2]
  t9 = t6[t8]
  t7[t8] = t9
  t8 = t8 + 4
  goto B2
B4: successors []
  return

