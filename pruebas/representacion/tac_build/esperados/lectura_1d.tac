initial_data bytes=0

function f (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t0 = a + 1
  r = t0
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  v[0] = 1
  v[4] = 2
  v[8] = 3
  v[12] = 4
  w[0] = 0
  w[4] = 0
  w[8] = 0
  w[12] = 0
  y = 0
  s = 0
  i = 0
  z = 0
  q = 0
  t0 = 3 * 4
  w[t0] = 7
  t1 = 3 * 4
  t2 = w[t1]
  y = t2
  goto B2
B2: successors [B4, B3]
  t3 = i < 4
  if t3 == 0 goto B4
B3: successors [B2]
  t4 = i * 4
  t5 = v[t4]
  t6 = s + t5
  s = t6
  t7 = i + 1
  i = t7
  goto B2
B4: successors []
  t8 = 2 * 4
  t9 = v[t8]
  param t9, 0
  t10 = call f, 1
  z = t10
  param 5, 0
  t11 = call f, 1
  q = t11
  return

