initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B3, B2]
  t0 = neg 3
  x = t0
  y = 2
  t1 = x + y
  a = t1
  t2 = a - x
  a = t2
  t3 = x * y
  a = t3
  t4 = a / y
  a = t4
  t5 = a % y
  a = t5
  t6 = a ^ y
  a = t6
  t7 = x < y
  b = t7
  t8 = x <= y
  b = t8
  t9 = x > y
  b = t9
  t10 = x >= y
  b = t10
  t11 = x == y
  b = t11
  t12 = x != y
  b = t12
  t13 = not b
  b = t13
  t14 = b
  if t14 == 0 goto B3
B2: successors [B3]
  t14 = 1
  goto B3
B3: successors [B5, B4]
  t15 = t14
  if t15 != 0 goto B5
B4: successors [B5]
  t15 = 0
  goto B5
B5: successors []
  b = t15
  return

