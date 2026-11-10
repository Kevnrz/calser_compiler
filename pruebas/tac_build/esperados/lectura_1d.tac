initial_data bytes=0

function f (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t0 = a
  t1 = t0 + 1
  r = t1
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
  t3 = i
  t4 = t3 < 4
  if t4 == 0 goto B4
B3: successors [B2]
  t5 = s
  t6 = i
  t7 = t6 * 4
  t8 = v[t7]
  t9 = t5 + t8
  s = t9
  t10 = i
  t11 = t10 + 1
  i = t11
  goto B2
B4: successors []
  t12 = 2 * 4
  t13 = v[t12]
  param t13, 0
  t14 = call f, 1
  z = t14
  param 5, 0
  t15 = call f, 1
  q = t15
  return

