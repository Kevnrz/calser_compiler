function suma (entry B0)
B0: successors [B1]
  sw rz, 28(rz)
  jef B1
B1: successors [B3, B2]
  lw v0, 36(rz)
  mov v12, 3
  sle v1, v12, v0
  beq v1, rz, B3
B2: successors []
  lw v2, 40(rz)
  sw v2, 28(rz)
  jefR rz, rl
B3: successors [B1]
  mov v3, 12
  lw v4, 36(rz)
  saddi v5, v4, 1
  lw v6, 40(rz)
  lw v7, 36(rz)
  mov v13, 4
  pseudo.mul v8, v7, v13
  lw v9, 32(rz)
  add v14, v9, v8
  lw v10, 0(v14)
  add v11, v6, v10
  sw v3, 32(rz)
  sw v5, 36(rz)
  sw v11, 40(rz)
  jef B1
B4: successors []
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  sw rz, 92(rz)
  mov v0, 0
  sw v0, 32(rz)
  sw rz, 36(rz)
  sw rz, 40(rz)
  jas rl, .L_func_suma
  lw v1, 28(rz)
  sw v1, 92(rz)
  pseudo.exit

