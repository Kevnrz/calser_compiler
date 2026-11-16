initial_data bytes=8
  @00000000 = 00000000
  @00000004 = 00000000

function f (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  t0 = a / 4
  x = t0
  t1 = a / b
  y = t1
  autenticar [a]
  t2 = 0 * 4
  datos[t2] = a
  return

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  param 8, 0
  param 2, 1
  call f, 2
  return

