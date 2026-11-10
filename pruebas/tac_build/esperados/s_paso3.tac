initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  x = 0
  i = 0
  $limite_S0 = 10
  goto B2
B2: successors [B3, B4]
  if i >= $limite_S0 goto B3
B4: successors [B2]
  t0 = x
  t1 = i
  t2 = t0 + t1
  x = t2
  i = i + 3
  goto B2
B3: successors []
  return

