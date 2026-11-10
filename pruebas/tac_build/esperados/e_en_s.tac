initial_data bytes=0

function buscar (entry B0)
B0: successors [B1]
  pos = 0
  goto B1
B1: successors [B2]
  t0 = neg 1
  pos = t0
  i = 0
  $limite_S0 = 5
  goto B2
B2: successors [B3, B4]
  if i >= $limite_S0 goto B3
B4: successors [B6, B7]
  t1 = i
  t2 = t1 * 4
  t3 = v
  t4 = t3[t2]
  t5 = x
  t6 = t4 == t5
  if t6 == 0 goto B6
B7: successors []
  t7 = i
  pos = t7
  return pos
B6: successors [B5]
  goto B5
B5: successors [B2]
  i = i + 1
  goto B2
B3: successors []
  return pos

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  datos[0] = 4
  datos[4] = 8
  datos[8] = 15
  datos[12] = 16
  datos[16] = 23
  a = 0
  b = 0
  c = 0
  t0 = address datos
  param t0, 0
  param 15, 1
  t1 = call buscar, 2
  a = t1
  t2 = address datos
  param t2, 0
  param 99, 1
  t3 = call buscar, 2
  b = t3
  k = 0
  $limite_S1 = 3
  goto B2
B2: successors [B3, B4]
  if k >= $limite_S1 goto B3
B4: successors [B2]
  t4 = c
  t5 = a
  t6 = t4 + t5
  c = t6
  k = k + 1
  goto B2
B3: successors []
  return

