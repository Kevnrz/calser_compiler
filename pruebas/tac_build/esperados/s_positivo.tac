initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  x = 0
  i = 0
  $limite_S0 = 5
  goto B2
B2: successors [B4, B3]
  if i >= $limite_S0 goto B4
B3: successors [B2]
  t0 = x
  t1 = i
  t2 = t0 + t1
  x = t2
  i = i + 1
  goto B2
B4: successors []
  return

