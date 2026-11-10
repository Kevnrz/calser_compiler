initial_data bytes=0

function identity (entry B0)
B0: successors [B1]
  result = 0
  goto B1
B1: successors []
  t0 = x
  result = t0
  return result

function add (entry B0)
B0: successors [B1]
  result = 0
  goto B1
B1: successors []
  t0 = a
  t1 = b
  t2 = t0 + t1
  result = t2
  return result

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  param 10, 0
  t0 = call identity, 1
  param 20, 0
  t1 = call identity, 1
  param t0, 0
  param t1, 1
  t2 = call add, 2
  sum = t2
  return

