initial_data bytes=4
  @00000000 = 00000000

function f (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  a = 1
  a = 2
  g = 1
  g = 2
  r = 1
  r = 2
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x = 1
  t0 = 8 / x
  y = t0
  x = 2
  param 8, 0
  param 2, 1
  call f, 2
  return

