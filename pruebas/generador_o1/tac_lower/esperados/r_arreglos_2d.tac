function suma (entry B0)
B0: successors [B1]
  sw rz, 36(rz)
  jef B1
B1: successors [B3, B2]
  lw v15, 44(rz)
  mov v16, 2
  sle v0, v16, v15
  beq v0, rz, B3
B2: successors []
  lw v17, 48(rz)
  sw v17, 36(rz)
  jefR rz, rl
B3: successors [B1]
  mov v1, 16
  lw v18, 44(rz)
  saddi v2, v18, 1
  lw v19, 44(rz)
  mov v20, 2
  pseudo.mul v3, v19, v20
  saddi v4, v3, 0
  mov v21, 4
  pseudo.mul v5, v4, v21
  lw v6, 40(rz)
  add v22, v6, v5
  lw v7, 0(v22)
  lw v23, 48(rz)
  add v8, v23, v7
  lw v24, 44(rz)
  mov v25, 2
  pseudo.mul v9, v24, v25
  saddi v10, v9, 1
  mov v26, 4
  pseudo.mul v11, v10, v26
  lw v12, 40(rz)
  add v27, v12, v11
  lw v13, 0(v27)
  add v14, v8, v13
  sw v1, 40(rz)
  sw v2, 44(rz)
  sw v14, 48(rz)
  jef B1
B4: successors []
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  sw rz, 100(rz)
  mov v0, 0
  sw v0, 40(rz)
  sw rz, 44(rz)
  sw rz, 48(rz)
  jas rl, .L_func_suma
  lw v1, 36(rz)
  sw v1, 100(rz)
  pseudo.exit

