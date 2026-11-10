initial_data bytes=4
  @00000000 = 00000000

function toca (entry B0)
B0: successors [B1]
  r = 0
  goto B1
B1: successors []
  t0 = g
  t1 = t0 + 1
  g = t1
  r = 1
  return r

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors [B2, B3]
  v = 1
  f = 0
  t0 = 0
  if t0 == 0 goto B2
B3: successors [B2]
  t1 = call toca, 0
  t0 = t1
  goto B2
B2: successors [B4, B5]
  r1 = t0
  t2 = g
  g1 = t2
  t3 = 1
  if t3 != 0 goto B4
B5: successors [B4]
  t4 = call toca, 0
  t3 = t4
  goto B4
B4: successors [B6, B7]
  r2 = t3
  t5 = g
  g2 = t5
  t6 = 1
  if t6 == 0 goto B6
B7: successors [B6]
  t7 = call toca, 0
  t6 = t7
  goto B6
B6: successors [B8, B9]
  r3 = t6
  t8 = g
  g3 = t8
  t9 = 0
  if t9 != 0 goto B8
B9: successors [B8]
  t10 = call toca, 0
  t9 = t10
  goto B8
B8: successors [B10, B11]
  r4 = t9
  t11 = g
  g4 = t11
  t12 = f
  t13 = t12
  if t13 == 0 goto B10
B11: successors [B10]
  t14 = f
  t13 = t14
  goto B10
B10: successors [B12, B13]
  and_ff = t13
  t15 = f
  t16 = t15
  if t16 == 0 goto B12
B13: successors [B12]
  t17 = v
  t16 = t17
  goto B12
B12: successors [B14, B15]
  and_fv = t16
  t18 = v
  t19 = t18
  if t19 == 0 goto B14
B15: successors [B14]
  t20 = f
  t19 = t20
  goto B14
B14: successors [B16, B17]
  and_vf = t19
  t21 = v
  t22 = t21
  if t22 == 0 goto B16
B17: successors [B16]
  t23 = v
  t22 = t23
  goto B16
B16: successors [B18, B19]
  and_vv = t22
  t24 = f
  t25 = t24
  if t25 != 0 goto B18
B19: successors [B18]
  t26 = f
  t25 = t26
  goto B18
B18: successors [B20, B21]
  or_ff = t25
  t27 = f
  t28 = t27
  if t28 != 0 goto B20
B21: successors [B20]
  t29 = v
  t28 = t29
  goto B20
B20: successors [B22, B23]
  or_fv = t28
  t30 = v
  t31 = t30
  if t31 != 0 goto B22
B23: successors [B22]
  t32 = f
  t31 = t32
  goto B22
B22: successors [B24, B25]
  or_vf = t31
  t33 = v
  t34 = t33
  if t34 != 0 goto B24
B25: successors [B24]
  t35 = v
  t34 = t35
  goto B24
B24: successors [B26, B27]
  or_vv = t34
  t36 = f
  t37 = t36
  if t37 == 0 goto B26
B27: successors [B26]
  t38 = f
  t37 = t38
  goto B26
B26: successors [B28, B29]
  t39 = t37
  if t39 != 0 goto B28
B29: successors [B28]
  t40 = v
  t39 = t40
  goto B28
B28: successors [B30, B31]
  mix1 = t39
  t41 = v
  t42 = t41
  if t42 != 0 goto B30
B31: successors [B32, B33]
  t43 = f
  t44 = t43
  if t44 == 0 goto B32
B33: successors [B32]
  t45 = f
  t44 = t45
  goto B32
B32: successors [B30]
  t42 = t44
  goto B30
B30: successors [B35, B36]
  mix2 = t42
  x = 0
  en_c = 0
  n = 0
  t46 = x
  t47 = t46 > 0
  t48 = t47
  if t48 == 0 goto B35
B36: successors [B35]
  t49 = call toca, 0
  t48 = t49
  goto B35
B35: successors [B37, B38]
  if t48 == 0 goto B37
B38: successors [B34]
  en_c = 1
  goto B34
B37: successors [B34]
  goto B34
B34: successors [B39]
  t50 = g
  g5 = t50
  goto B39
B39: successors [B40, B41]
  t51 = n
  t52 = t51 < 2
  t53 = t52
  if t53 == 0 goto B40
B41: successors [B40]
  t54 = call toca, 0
  t53 = t54
  goto B40
B40: successors [B42, B43]
  if t53 == 0 goto B42
B43: successors [B39]
  t55 = n
  t56 = t55 + 1
  n = t56
  goto B39
B42: successors []
  return

