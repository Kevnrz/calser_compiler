initial_data bytes=0

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B8, B2]
  x = 0
  t0 = x == 0
  if t0 == 0 goto B8
B2: successors [B4, B3]
  t1 = x < 1
  if t1 == 0 goto B4
B3: successors [B7]
  x = 1
  goto B7
B4: successors [B6, B5]
  t2 = x > 1
  if t2 == 0 goto B6
B5: successors [B7]
  x = 2
  goto B7
B6: successors [B7]
  x = 3
  goto B7
B7: successors [B11]
  goto B11
B8: successors [B10, B9]
  t3 = x == 1
  if t3 == 0 goto B10
B9: successors [B11]
  x = 4
  goto B11
B10: successors [B11]
  x = 5
  goto B11
B11: successors []
  return

