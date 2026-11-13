initial_data bytes=0

function nada (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = a
  t1 = t0 > 0
  if t1 == 0 goto B3
B2: successors [B4]
  r = 9
  goto B4
B3: successors [B4]
  goto B4
B4: successors []
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x = 0
  y = 0
  param 5, 0
  t0 = call nada, 1
  x = t0
  t1 = neg 1
  param t1, 0
  t2 = call nada, 1
  y = t2
  return

