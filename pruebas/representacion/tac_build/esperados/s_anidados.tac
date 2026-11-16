initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  x = 0
  i = 0
  $limite_S0 = 3
  goto B2
B2: successors [B7, B3]
  if i >= $limite_S0 goto B7
B3: successors [B4]
  j = 0
  $limite_S1 = 4
  goto B4
B4: successors [B6, B5]
  if j >= $limite_S1 goto B6
B5: successors [B4]
  t0 = i * j
  t1 = x + t0
  x = t1
  j = j + 1
  goto B4
B6: successors [B2]
  i = i + 1
  goto B2
B7: successors []
  return

