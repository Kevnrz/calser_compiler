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
  t0 = mutable + constante
  dinamica = t0
  t1 = mutable + 1
  datos[4] = t1
  local = dinamica
  copia[0] = 0
  copia[4] = 0
  copia[8] = 0
  t2 = address datos
  t3 = address copia
  t4 = 0
  goto B2
B2: successors [B4, B3]
  if t4 >= 12 goto B4
B3: successors [B2]
  t5 = t2[t4]
  t3[t4] = t5
  t4 = t4 + 4
  goto B2
B4: successors []
  return

