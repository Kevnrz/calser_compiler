initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  x = 0
  i = 0
  $limite_S0 = 3
  goto B2
B2: successors [B3, B4]
  if i >= $limite_S0 goto B3
B4: successors [B5]
  j = 0
  $limite_S1 = 4
  goto B5
B5: successors [B6, B7]
  if j >= $limite_S1 goto B6
B7: successors [B5]
  t0 = x
  t1 = i
  t2 = j
  t3 = t1 * t2
  t4 = t0 + t3
  x = t4
  j = j + 1
  goto B5
B6: successors [B2]
  i = i + 1
  goto B2
B3: successors []
  return

