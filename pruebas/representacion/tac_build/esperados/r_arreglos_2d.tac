initial_data bytes=32
  @00000000 = 00000001
  @00000004 = 00000002
  @00000008 = 00000003
  @0000000c = 00000004
  @00000010 = 0000000a
  @00000014 = 00000014
  @00000018 = 0000001e
  @0000001c = 00000028

function suma (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors [B3, B2]
  t0 = i >= 2
  if t0 == 0 goto B3
B2: successors []
  r = acc
  return r
B3: successors [B1]
  t1 = address b
  t2 = i + 1
  t3 = i * 2
  t4 = t3 + 0
  t5 = t4 * 4
  t6 = v
  t7 = t6[t5]
  t8 = acc + t7
  t9 = i * 2
  t10 = t9 + 1
  t11 = t10 * 4
  t12 = v
  t13 = t12[t11]
  t14 = t8 + t13
  v = t1
  i = t2
  acc = t14
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

