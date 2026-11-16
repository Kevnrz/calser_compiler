initial_data bytes=4
  @00000000 = 00000000

function tocar (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  t0 = g + k
  g = t0
  return

function calcular (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t0 = a * 2
  x = t0
  t1 = a + 1
  y = t1
  param a, 0
  call tocar, 1
  r = y
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  v = 0
  param 3, 0
  t0 = call calcular, 1
  v = t0
  return

