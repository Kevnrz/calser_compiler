function suma (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors [B3, B2]
  mov v7, 10
  sw v7, 20(rz)
  mov v8, 20
  sw v8, 24(rz)
  mov v9, 30
  sw v9, 28(rz)
  lw v10, 12(rz)
  mov v11, 3
  sle v0, v11, v10
  beq v0, rz, B3
B2: successors []
  lw v12, 16(rz)
  sw v12, 4(rz)
  jefR rz, rl
B3: successors [B1]
  mov v1, 20
  lw v13, 12(rz)
  saddi v2, v13, 1
  lw v14, 12(rz)
  mov v15, 4
  pseudo.mul v3, v14, v15
  lw v4, 8(rz)
  add v16, v4, v3
  lw v5, 0(v16)
  lw v17, 16(rz)
  add v6, v17, v5
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
  mov v2, 2
  sw v2, 80(rz)
  mov v3, 4
  sw v3, 84(rz)
  mov v4, 6
  sw v4, 88(rz)
  sw rz, 92(rz)
  mov v0, 80
  sw v0, 8(rz)
  sw rz, 12(rz)
  sw rz, 16(rz)
  jas rl, .L_func_suma
  lw v1, 4(rz)
  sw v1, 92(rz)
  pseudo.exit

