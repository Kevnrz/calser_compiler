initial_data bytes=4
  @00000000 = 00000000

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  i = 0
  $limite_S0 = 4
  t1 = suma + i
  suma = t1
  t5 = i + 1
  t2 = suma + t5
  suma = t2
  t6 = i + 2
  t3 = suma + t6
  suma = t3
  t7 = i + 3
  t4 = suma + t7
  suma = t4
  i = t7 + 1
  return

