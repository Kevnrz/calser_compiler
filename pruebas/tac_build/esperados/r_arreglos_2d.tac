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
  t0 = i
  t1 = t0 >= 2
  if t1 == 0 goto B3
B2: successors []
  t2 = acc
  r = t2
  return r
B3: successors [B1]
  t3 = address b
  t4 = i
  t5 = t4 + 1
  t6 = acc
  t7 = i
  t8 = t7 * 2
  t9 = t8 + 0
  t10 = t9 * 4
  t11 = v
  t12 = t11[t10]
  t13 = t6 + t12
  t14 = i
  t15 = t14 * 2
  t16 = t15 + 1
  t17 = t16 * 4
  t18 = v
  t19 = t18[t17]
  t20 = t13 + t19
  v = t3
  i = t5
  acc = t20
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

