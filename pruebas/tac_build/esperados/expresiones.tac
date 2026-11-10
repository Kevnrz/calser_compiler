initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2, B3]
  t0 = neg 3
  x = t0
  y = 2
  t1 = x
  t2 = y
  t3 = t1 + t2
  a = t3
  t4 = a
  t5 = x
  t6 = t4 - t5
  a = t6
  t7 = x
  t8 = y
  t9 = t7 * t8
  a = t9
  t10 = a
  t11 = y
  t12 = t10 / t11
  a = t12
  t13 = a
  t14 = y
  t15 = t13 % t14
  a = t15
  t16 = a
  t17 = y
  t18 = t16 ^ t17
  a = t18
  t19 = x
  t20 = y
  t21 = t19 < t20
  b = t21
  t22 = x
  t23 = y
  t24 = t22 <= t23
  b = t24
  t25 = x
  t26 = y
  t27 = t25 > t26
  b = t27
  t28 = x
  t29 = y
  t30 = t28 >= t29
  b = t30
  t31 = x
  t32 = y
  t33 = t31 == t32
  b = t33
  t34 = x
  t35 = y
  t36 = t34 != t35
  b = t36
  t37 = b
  t38 = not t37
  b = t38
  t39 = b
  t40 = t39
  if t40 == 0 goto B2
B3: successors [B2]
  t40 = 1
  goto B2
B2: successors [B4, B5]
  t41 = t40
  if t41 != 0 goto B4
B5: successors [B4]
  t41 = 0
  goto B4
B4: successors []
  b = t41
  return

