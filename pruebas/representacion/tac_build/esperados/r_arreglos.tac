initial_data bytes=24
  @00000000 = 00000001
  @00000004 = 00000002
  @00000008 = 00000003
  @0000000c = 0000000a
  @00000010 = 00000014
  @00000014 = 0000001e

function suma (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = i >= 3
  if t0 == 0 goto B3
B2: successors []
  r = acc
  return r
B3: successors [B1]
  t1 = address b
  t2 = i + 1
  t3 = i * 4
  t4 = v
  t5 = t4[t3]
  t6 = acc + t5
  v = t1
  i = t2
  acc = t6
  goto B1
B4: successors []
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  s = 0
  t0 = address a
  param t0, 0
  param 0, 1
  param 0, 2
  t1 = call suma, 3
  s = t1
  return

