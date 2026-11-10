initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  res = 0
  goto B1
B1: successors [B3, B2]
  t0 = n
  t1 = t0 < 0
  if t1 == 0 goto B3
B2: successors []
  return res
B3: successors [B4]
  goto B4
B4: successors [B5]
  res = 1
  i = 2
  goto B5
B5: successors [B7, B6]
  t2 = i
  t3 = n
  t4 = t2 <= t3
  if t4 == 0 goto B7
B6: successors [B5]
  t5 = res
  t6 = i
  t7 = t5 * t6
  res = t7
  t8 = i
  t9 = t8 + 1
  i = t9
  goto B5
B7: successors []
  return res

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x = 5
  f = 0
  t0 = x
  param t0, 0
  t1 = call factorial, 1
  f = t1
  return

