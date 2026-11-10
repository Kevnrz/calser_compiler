initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B4]
  t0 = n
  t1 = t0 <= 1
  if t1 == 0 goto B3
B4: successors []
  t2 = acc
  r = t2
  return r
B3: successors [B1]
  t3 = n
  t4 = t3 - 1
  t5 = acc
  t6 = n
  t7 = t5 * t6
  n = t4
  acc = t7
  goto B1
B2: successors []
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  f = 0
  param 3, 0
  param 1, 1
  t0 = call factorial, 2
  f = t0
  return

