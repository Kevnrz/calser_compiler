function f (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors []
  lw v0, 8(rz)
  saddi v1, v0, 1
  sw v1, 4(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  mov v16, 1
  sw v16, 60(rz)
  mov v17, 2
  sw v17, 64(rz)
  mov v18, 3
  sw v18, 68(rz)
  mov v19, 4
  sw v19, 72(rz)
  sw rz, 76(rz)
  sw rz, 80(rz)
  sw rz, 84(rz)
  sw rz, 88(rz)
  sw rz, 92(rz)
  sw rz, 96(rz)
  sw rz, 100(rz)
  sw rz, 104(rz)
  sw rz, 108(rz)
  mov v20, 3
  mov v21, 4
  pseudo.mul v0, v20, v21
  mov v22, 7
  mov v23, 76
  add v24, v23, v0
  sw v22, 0(v24)
  mov v25, 3
  mov v26, 4
  pseudo.mul v1, v25, v26
  mov v27, 76
  add v28, v27, v1
  lw v2, 0(v28)
  sw v2, 92(rz)
  jef B2
B2: successors [B4, B3]
  lw v3, 100(rz)
  mov v29, 4
  slt v4, v3, v29
  beq v4, rz, B4
B3: successors [B2]
  lw v5, 96(rz)
  lw v6, 100(rz)
  mov v30, 4
  pseudo.mul v7, v6, v30
  mov v31, 60
  add v32, v31, v7
  lw v8, 0(v32)
  add v9, v5, v8
  sw v9, 96(rz)
  lw v10, 100(rz)
  saddi v11, v10, 1
  sw v11, 100(rz)
  jef B2
B4: successors []
  mov v33, 2
  mov v34, 4
  pseudo.mul v12, v33, v34
  mov v35, 60
  add v36, v35, v12
  lw v13, 0(v36)
  sw v13, 8(rz)
  jas rl, .L_func_f
  lw v14, 4(rz)
  sw v14, 104(rz)
  mov v37, 5
  sw v37, 8(rz)
  jas rl, .L_func_f
  lw v15, 4(rz)
  sw v15, 108(rz)
  pseudo.exit

