function suma (entry B0)
B0: successors [B1]
  sw rz, 28(rz)
  jef B1
B1: successors [B3, B2]
  lw v7, 36(rz)
  mov v8, 3
  sle v0, v8, v7
  beq v0, rz, B3
B2: successors []
  lw v9, 40(rz)
  sw v9, 28(rz)
  jefR rz, rl
B3: successors [B1]
  mov v1, 12
  lw v10, 36(rz)
  saddi v2, v10, 1
  lw v11, 36(rz)
  mov v12, 2
  sll v3, v11, v12
  lw v4, 32(rz)
  add v13, v4, v3
  lw v5, 0(v13)
  lw v14, 40(rz)
  add v6, v14, v5
  sw v1, 32(rz)
  sw v2, 36(rz)
  sw v6, 40(rz)
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

