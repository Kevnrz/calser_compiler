initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = n <= 1
  if t0 == 0 goto B3
B2: successors []
  r = acc
  return r
B3: successors [B1]
  t1 = n - 1
  t2 = acc * n
  n = t1
  acc = t2
  goto B1
B4: successors []
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

