initial_data bytes=0

function suma_fila (entry B0)
B0: successors [B1]
  s = 0
  goto B1
B1: successors [B2]
  j = 0
  $limite_S0 = 3
  goto B2
B2: successors [B4, B3]
  if j >= $limite_S0 goto B4
B3: successors [B2]
  t0 = f * 3
  t1 = t0 + j
  t2 = t1 * 4
  t3 = m
  t4 = t3[t2]
  t5 = s + t4
  s = t5
  j = j + 1
  goto B2
B4: successors []
  return s

function poner (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  t0 = k * 4
  t1 = v
  t1[t0] = 7
  return

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  t[0] = 1
  t[4] = 2
  t[8] = 3
  t[12] = 4
  t[16] = 5
  t[20] = 6
  w[0] = 0
  w[4] = 0
  w[8] = 0
  r = 0
  t0 = address t
  param t0, 0
  param 1, 1
  t1 = call suma_fila, 2
  r = t1
  t2 = address w
  param t2, 0
  param 2, 1
  call poner, 2
  t3 = 1 * 3
  t4 = t3 + 2
  t5 = t4 * 4
  t6 = t[t5]
  t7 = 0 * 4
  w[t7] = t6
  return

