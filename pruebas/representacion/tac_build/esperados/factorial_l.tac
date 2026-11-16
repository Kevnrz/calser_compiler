initial_data bytes=0

function factorial (entry B0)
B0: successors [B1]
  res = 0
  goto B1
B1: successors [B3, B2]
  t0 = n < 0
  if t0 == 0 goto B3
B2: successors []
  return res
B3: successors [B4]
  goto B4
B4: successors [B5]
  res = 1
  i = 2
  goto B5
B5: successors [B7, B6]
  t1 = i <= n
  if t1 == 0 goto B7
B6: successors [B5]
  t2 = res * i
  res = t2
  t3 = i + 1
  i = t3
  goto B5
B7: successors []
  return res

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  x = 5
  f = 0
  param x, 0
  t0 = call factorial, 1
  f = t0
  return

