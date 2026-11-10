initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B3, B4]
  x = 0
  t0 = x
  t1 = t0 == 0
  if t1 == 0 goto B3
B4: successors [B6, B7]
  t2 = x
  t3 = t2 < 1
  if t3 == 0 goto B6
B7: successors [B5]
  x = 1
  goto B5
B6: successors [B8, B9]
  t4 = x
  t5 = t4 > 1
  if t5 == 0 goto B8
B9: successors [B5]
  x = 2
  goto B5
B8: successors [B5]
  x = 3
  goto B5
B5: successors [B2]
  goto B2
B3: successors [B10, B11]
  t6 = x
  t7 = t6 == 1
  if t7 == 0 goto B10
B11: successors [B2]
  x = 4
  goto B2
B10: successors [B2]
  x = 5
  goto B2
B2: successors []
  return

