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
  t1 = i + 3
  if t1 >= $limite_S0 goto B4
B3: successors [B2]
  t2 = suma + i
  suma = t2
  t6 = i + 1
  t3 = suma + t6
  suma = t3
  t7 = i + 2
  t4 = suma + t7
  suma = t4
  t8 = i + 3
  t5 = suma + t8
  suma = t5
  i = t8 + 1
  goto B2
B4: successors [B6, B5]
  if i >= $limite_S0 goto B6
B5: successors [B4]
  t0 = suma + i
  suma = t0
  i = i + 1
  goto B4
B6: successors []
  return

