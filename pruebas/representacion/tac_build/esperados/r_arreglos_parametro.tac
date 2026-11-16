initial_data bytes=0

function suma (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = i >= 3
  if t0 == 0 goto B3
B2: successors []
  r = acc
  return r
B3: successors [B1]
  t1 = v
  t2 = i + 1
  t3 = i * 4
  t4 = v
  t5 = t4[t3]
  t6 = acc + t5
  v = t1
  i = t2
  acc = t6
  goto B1
B4: successors []
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

