function f (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B3, B2]
  lw v7, 4(rz)
  lw v8, 8(rz)
  lw v9, 4(rz)
  add v0, v7, v8
  lw v10, 8(rz)
  sw v0, 12(rz)
  sub v1, v9, v10
  lw v11, 12(rz)
  sw v1, 16(rz)
  lw v12, 16(rz)
  slt v2, v12, v11
  beq v2, rz, B3
B2: successors [B4]
  lw v13, 4(rz)
  lw v14, 8(rz)
  saddi v3, v13, 1
  saddi v4, v14, 2
  sw v3, 12(rz)
  sw v4, 16(rz)
  jef B4
B3: successors [B4]
  lw v15, 4(rz)
  lw v16, 8(rz)
  saddi v5, v15, 3
  saddi v6, v16, 4
  sw v5, 12(rz)
  sw v6, 16(rz)
  jef B4
B4: successors []
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v0, 8
  mov v1, 2
  sw v0, 4(rz)
  sw v1, 8(rz)
  jas rl, .L_func_f
  pseudo.exit

