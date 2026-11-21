function suma (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors [B3, B2]
  lw v7, 12(rz)
  mov v8, 3
  sle v0, v8, v7
  beq v0, rz, B3
B2: successors []
  lw v9, 16(rz)
  sw v9, 4(rz)
  jefR rz, rl
B3: successors [B1]
  lw v1, 8(rz)
  lw v10, 12(rz)
  saddi v2, v10, 1
  lw v11, 12(rz)
  mov v12, 2
  sll v3, v11, v12
  lw v4, 8(rz)
  add v13, v4, v3
  lw v5, 0(v13)
  lw v14, 16(rz)
  add v6, v14, v5
  sw v1, 8(rz)
  sw v2, 12(rz)
  sw v6, 16(rz)
  jef B1
B4: successors []
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v2, 4
  sw v2, 68(rz)
  mov v3, 5
  sw v3, 72(rz)
  mov v4, 6
  sw v4, 76(rz)
  sw rz, 80(rz)
  mov v0, 68
  sw v0, 8(rz)
  sw rz, 12(rz)
  sw rz, 16(rz)
  jas rl, .L_func_suma
  lw v1, 4(rz)
  sw v1, 80(rz)
  pseudo.exit

