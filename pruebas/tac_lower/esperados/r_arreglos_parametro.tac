function suma (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors [B3, B2]
  lw v0, 12(rz)
  mov v12, 3
  sle v1, v12, v0
  beq v1, rz, B3
B2: successors []
  lw v2, 16(rz)
  sw v2, 4(rz)
  jefR rz, rl
B3: successors [B1]
  lw v3, 8(rz)
  lw v4, 12(rz)
  saddi v5, v4, 1
  lw v6, 16(rz)
  lw v7, 12(rz)
  mov v13, 4
  pseudo.mul v8, v7, v13
  lw v9, 8(rz)
  add v14, v9, v8
  lw v10, 0(v14)
  add v11, v6, v10
  sw v3, 8(rz)
  sw v5, 12(rz)
  sw v11, 16(rz)
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

