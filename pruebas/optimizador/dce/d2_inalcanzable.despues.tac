initial_data bytes=4
  @00000000 = 00000000

function f (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B3, B2]
  t0 = a == b
  if t0 == 0 goto B3
B2: successors []
  g = 1
  return
B3: successors []
  g = 2
  return

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  param 8, 0
  param 2, 1
  call f, 2
  return

