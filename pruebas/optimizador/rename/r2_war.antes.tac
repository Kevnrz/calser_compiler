initial_data bytes=4
  @00000000 = 00000000

function f (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x = a
  g = x
  x = b
  t0 = g + x
  g = t0
  t1 = a + b
  x = t1
  t2 = g + x
  g = t2
  return

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  param 8, 0
  param 2, 1
  call f, 2
  return

