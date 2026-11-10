initial_data bytes=0

function doble (entry B0)
B0: successors [B1]
  s = 0
  goto B1
B1: successors [B2]
  loc[0] = 0
  loc[4] = 0
  loc[8] = 0
  t0 = a
  t1 = address loc
  t2 = 0
  goto B2
B2: successors [B4, B3]
  if t2 >= 12 goto B4
B3: successors [B2]
  t3 = t0[t2]
  t1[t2] = t3
  t2 = t2 + 4
  goto B2
B4: successors [B5]
  t4 = address loc
  t5 = b
  t6 = 0
  goto B5
B5: successors [B7, B6]
  if t6 >= 12 goto B7
B6: successors [B5]
  t7 = t4[t6]
  t5[t6] = t7
  t6 = t6 + 4
  goto B5
B7: successors []
  t8 = 0 * 4
  t9 = loc[t8]
  t10 = 1 * 4
  t11 = loc[t10]
  t12 = t9 + t11
  t13 = 2 * 4
  t14 = loc[t13]
  t15 = t12 + t14
  s = t15
  return s

function fuera (entry B0)
B0: successors [B1]
  s = 0
  goto B1
B1: successors []
  t0 = r
  t1 = q
  param t1, 0
  param t0, 1
  t2 = call doble, 2
  s = t2
  return s

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x[0] = 4
  x[4] = 5
  x[8] = 6
  y[0] = 0
  y[4] = 0
  y[8] = 0
  t = 0
  t0 = address y
  t1 = address x
  param t1, 0
  param t0, 1
  t2 = call fuera, 2
  t = t2
  return

