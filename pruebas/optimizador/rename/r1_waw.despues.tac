initial_data bytes=4
  @00000000 = 00000000

function f (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t4 = 0
  t0 = a + b
  t5 = t0
  t1 = t5 * 2
  r = t1
  t2 = a - b
  x = t2
  t3 = r + x
  r = t3
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  param 5, 0
  param 3, 1
  t0 = call f, 2
  salida = t0
  return

