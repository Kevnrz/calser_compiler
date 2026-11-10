initial_data bytes=0

function suma (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B4]
  t0 = i
  t1 = t0 >= 3
  if t1 == 0 goto B3
B4: successors []
  t2 = acc
  r = t2
  return r
B3: successors [B1]
  t3 = v
  t4 = i
  t5 = t4 + 1
  t6 = acc
  t7 = i
  t8 = t7 * 4
  t9 = v
  t10 = t9[t8]
  t11 = t6 + t10
  v = t3
  i = t5
  acc = t11
  goto B1
B2: successors []
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  a[0] = 4
  a[4] = 5
  a[8] = 6
  s = 0
  t0 = address a
  param t0, 0
  param 0, 1
  param 0, 2
  t1 = call suma, 3
  s = t1
  return

