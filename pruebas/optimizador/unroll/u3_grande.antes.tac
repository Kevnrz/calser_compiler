initial_data bytes=4
  @00000000 = 00000000

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2]
  i = 0
  $limite_S0 = 100
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

