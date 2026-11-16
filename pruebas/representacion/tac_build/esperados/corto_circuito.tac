initial_data bytes=4
  @00000000 = 00000000

function toca (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t0 = g + 1
  g = t0
  r = 1
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B3, B2]
  v = 1
  f = 0
  t0 = 0
  if t0 == 0 goto B3
B2: successors [B3]
  t1 = call toca, 0
  t0 = t1
  goto B3
B3: successors [B5, B4]
  r1 = t0
  g1 = g
  t2 = 1
  if t2 != 0 goto B5
B4: successors [B5]
  t3 = call toca, 0
  t2 = t3
  goto B5
B5: successors [B7, B6]
  r2 = t2
  g2 = g
  t4 = 1
  if t4 == 0 goto B7
B6: successors [B7]
  t5 = call toca, 0
  t4 = t5
  goto B7
B7: successors [B9, B8]
  r3 = t4
  g3 = g
  t6 = 0
  if t6 != 0 goto B9
B8: successors [B9]
  t7 = call toca, 0
  t6 = t7
  goto B9
B9: successors [B11, B10]
  r4 = t6
  g4 = g
  t8 = f
  if t8 == 0 goto B11
B10: successors [B11]
  t8 = f
  goto B11
B11: successors [B13, B12]
  and_ff = t8
  t9 = f
  if t9 == 0 goto B13
B12: successors [B13]
  t9 = v
  goto B13
B13: successors [B15, B14]
  and_fv = t9
  t10 = v
  if t10 == 0 goto B15
B14: successors [B15]
  t10 = f
  goto B15
B15: successors [B17, B16]
  and_vf = t10
  t11 = v
  if t11 == 0 goto B17
B16: successors [B17]
  t11 = v
  goto B17
B17: successors [B19, B18]
  and_vv = t11
  t12 = f
  if t12 != 0 goto B19
B18: successors [B19]
  t12 = f
  goto B19
B19: successors [B21, B20]
  or_ff = t12
  t13 = f
  if t13 != 0 goto B21
B20: successors [B21]
  t13 = v
  goto B21
B21: successors [B23, B22]
  or_fv = t13
  t14 = v
  if t14 != 0 goto B23
B22: successors [B23]
  t14 = f
  goto B23
B23: successors [B25, B24]
  or_vf = t14
  t15 = v
  if t15 != 0 goto B25
B24: successors [B25]
  t15 = v
  goto B25
B25: successors [B27, B26]
  or_vv = t15
  t16 = f
  if t16 == 0 goto B27
B26: successors [B27]
  t16 = f
  goto B27
B27: successors [B29, B28]
  t17 = t16
  if t17 != 0 goto B29
B28: successors [B29]
  t17 = v
  goto B29
B29: successors [B33, B30]
  mix1 = t17
  t18 = v
  if t18 != 0 goto B33
B30: successors [B32, B31]
  t19 = f
  if t19 == 0 goto B32
B31: successors [B32]
  t19 = f
  goto B32
B32: successors [B33]
  t18 = t19
  goto B33
B33: successors [B35, B34]
  mix2 = t18
  x = 0
  en_c = 0
  n = 0
  t20 = x > 0
  t21 = t20
  if t21 == 0 goto B35
B34: successors [B35]
  t22 = call toca, 0
  t21 = t22
  goto B35
B35: successors [B37, B36]
  if t21 == 0 goto B37
B36: successors [B38]
  en_c = 1
  goto B38
B37: successors [B38]
  goto B38
B38: successors [B39]
  g5 = g
  goto B39
B39: successors [B41, B40]
  t23 = n < 2
  t24 = t23
  if t24 == 0 goto B41
B40: successors [B41]
  t25 = call toca, 0
  t24 = t25
  goto B41
B41: successors [B43, B42]
  if t24 == 0 goto B43
B42: successors [B39]
  t26 = n + 1
  n = t26
  goto B39
B43: successors []
  return

