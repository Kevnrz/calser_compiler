function suma (entry B0)
B0: successors [B1]
  sw rz, 36(rz)
  jef B1
B1: successors [B3, B2]
  lw v0, 44(rz)
  mov v21, 2
  sle v1, v21, v0
  beq v1, rz, B3
B2: successors []
  lw v2, 48(rz)
  sw v2, 36(rz)
  jefR rz, rl
B3: successors [B1]
  mov v3, 16
  lw v4, 44(rz)
  saddi v5, v4, 1
  lw v6, 48(rz)
  lw v7, 44(rz)
  mov v22, 2
  pseudo.mul v8, v7, v22
  saddi v9, v8, 0
  mov v23, 4
  pseudo.mul v10, v9, v23
  lw v11, 40(rz)
  add v24, v11, v10
  lw v12, 0(v24)
  add v13, v6, v12
  lw v14, 44(rz)
  mov v25, 2
  pseudo.mul v15, v14, v25
  saddi v16, v15, 1
  mov v26, 4
  pseudo.mul v17, v16, v26
  lw v18, 40(rz)
  add v27, v18, v17
  lw v19, 0(v27)
  add v20, v13, v19
  sw v3, 40(rz)
  sw v5, 44(rz)
  sw v20, 48(rz)
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

