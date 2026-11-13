initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B8, B2]
  x = 0
  t0 = x
  t1 = t0 == 0
  if t1 == 0 goto B8
B2: successors [B4, B3]
  t2 = x
  t3 = t2 < 1
  if t3 == 0 goto B4
B3: successors [B7]
  x = 1
  goto B7
B4: successors [B6, B5]
  t4 = x
  t5 = t4 > 1
  if t5 == 0 goto B6
B5: successors [B7]
  x = 2
  goto B7
B6: successors [B7]
  x = 3
  goto B7
B7: successors [B11]
  goto B11
B8: successors [B10, B9]
  t6 = x
  t7 = t6 == 1
  if t7 == 0 goto B10
B9: successors [B11]
  x = 4
  goto B11
B10: successors [B11]
  x = 5
  goto B11
B11: successors []
  return

