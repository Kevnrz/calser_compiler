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
  t2 = g
  g1 = t2
  t3 = 1
  if t3 != 0 goto B5
B4: successors [B5]
  t4 = call toca, 0
  t3 = t4
  goto B5
B5: successors [B7, B6]
  r2 = t3
  t5 = g
  g2 = t5
  t6 = 1
  if t6 == 0 goto B7
B6: successors [B7]
  t7 = call toca, 0
  t6 = t7
  goto B7
B7: successors [B9, B8]
  r3 = t6
  t8 = g
  g3 = t8
  t9 = 0
  if t9 != 0 goto B9
B8: successors [B9]
  t10 = call toca, 0
  t9 = t10
  goto B9
B9: successors [B11, B10]
  r4 = t9
  t11 = g
  g4 = t11
  t12 = f
  t13 = t12
  if t13 == 0 goto B11
B10: successors [B11]
  t14 = f
  t13 = t14
  goto B11
B11: successors [B13, B12]
  and_ff = t13
  t15 = f
  t16 = t15
  if t16 == 0 goto B13
B12: successors [B13]
  t17 = v
  t16 = t17
  goto B13
B13: successors [B15, B14]
  and_fv = t16
  t18 = v
  t19 = t18
  if t19 == 0 goto B15
B14: successors [B15]
  t20 = f
  t19 = t20
  goto B15
B15: successors [B17, B16]
  and_vf = t19
  t21 = v
  t22 = t21
  if t22 == 0 goto B17
B16: successors [B17]
  t23 = v
  t22 = t23
  goto B17
B17: successors [B19, B18]
  and_vv = t22
  t24 = f
  t25 = t24
  if t25 != 0 goto B19
B18: successors [B19]
  t26 = f
  t25 = t26
  goto B19
B19: successors [B21, B20]
  or_ff = t25
  t27 = f
  t28 = t27
  if t28 != 0 goto B21
B20: successors [B21]
  t29 = v
  t28 = t29
  goto B21
B21: successors [B23, B22]
  or_fv = t28
  t30 = v
  t31 = t30
  if t31 != 0 goto B23
B22: successors [B23]
  t32 = f
  t31 = t32
  goto B23
B23: successors [B25, B24]
  or_vf = t31
  t33 = v
  t34 = t33
  if t34 != 0 goto B25
B24: successors [B25]
  t35 = v
  t34 = t35
  goto B25
B25: successors [B27, B26]
  or_vv = t34
  t36 = f
  t37 = t36
  if t37 == 0 goto B27
B26: successors [B27]
  t38 = f
  t37 = t38
  goto B27
B27: successors [B29, B28]
  t39 = t37
  if t39 != 0 goto B29
B28: successors [B29]
  t40 = v
  t39 = t40
  goto B29
B29: successors [B33, B30]
  mix1 = t39
  t41 = v
  t42 = t41
  if t42 != 0 goto B33
B30: successors [B32, B31]
  t43 = f
  t44 = t43
  if t44 == 0 goto B32
B31: successors [B32]
  t45 = f
  t44 = t45
  goto B32
B32: successors [B33]
  t42 = t44
  goto B33
B33: successors [B35, B34]
  mix2 = t42
  x = 0
  en_c = 0
  n = 0
  t46 = x
  t47 = t46 > 0
  t48 = t47
  if t48 == 0 goto B35
B34: successors [B35]
  t49 = call toca, 0
  t48 = t49
  goto B35
B35: successors [B37, B36]
  if t48 == 0 goto B37
B36: successors [B38]
  en_c = 1
  goto B38
B37: successors [B38]
  goto B38
B38: successors [B39]
  t50 = g
  g5 = t50
  goto B39
B39: successors [B41, B40]
  t51 = n
  t52 = t51 < 2
  t53 = t52
  if t53 == 0 goto B41
B40: successors [B41]
  t54 = call toca, 0
  t53 = t54
  goto B41
B41: successors [B43, B42]
  if t53 == 0 goto B43
B42: successors [B39]
  t55 = n
  t56 = t55 + 1
  n = t56
  goto B39
B43: successors []
  return

