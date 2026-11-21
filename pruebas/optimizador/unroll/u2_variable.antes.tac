initial_data bytes=8
  @00000000 = 00000000
  @00000004 = 00000006

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  i = 0
  $limite_S0 = n
  goto B2
B2: successors [B4, B3]
  if i >= $limite_S0 goto B4
B3: successors [B2]
  t0 = suma + i
  suma = t0
  i = i + 1
  goto B2
B4: successors []
  return

