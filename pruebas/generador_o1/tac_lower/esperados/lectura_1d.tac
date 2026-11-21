function f (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors []
  lw v1, 8(rz)
  saddi v0, v1, 1
  sw v0, 4(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  mov v12, 1
  sw v12, 60(rz)
  mov v13, 2
  sw v13, 64(rz)
  mov v14, 3
  sw v14, 68(rz)
  mov v15, 4
  sw v15, 72(rz)
  sw rz, 76(rz)
  sw rz, 80(rz)
  sw rz, 84(rz)
  sw rz, 88(rz)
  sw rz, 92(rz)
  sw rz, 96(rz)
  sw rz, 100(rz)
  sw rz, 104(rz)
  sw rz, 108(rz)
  mov v16, 3
  mov v17, 2
  sll v0, v16, v17
  mov v18, 7
  mov v19, 76
  add v20, v19, v0
  sw v18, 0(v20)
  mov v21, 3
  mov v22, 2
  sll v1, v21, v22
  mov v23, 76
  add v24, v23, v1
  lw v2, 0(v24)
  sw v2, 92(rz)
  jef B2
B2: successors [B4, B3]
  lw v25, 100(rz)
  mov v26, 4
  slt v3, v25, v26
  beq v3, rz, B4
B3: successors [B2]
  lw v27, 100(rz)
  mov v28, 2
  sll v4, v27, v28
  mov v29, 60
  add v30, v29, v4
  lw v5, 0(v30)
  lw v31, 96(rz)
  add v6, v31, v5
  sw v6, 96(rz)
  lw v32, 100(rz)
  saddi v7, v32, 1
  sw v7, 100(rz)
  jef B2
B4: successors []
  mov v33, 2
  mov v34, 2
  sll v8, v33, v34
  mov v35, 60
  add v36, v35, v8
  lw v9, 0(v36)
  sw v9, 8(rz)
  jas rl, .L_func_f
  lw v10, 4(rz)
  sw v10, 104(rz)
  mov v37, 5
  sw v37, 8(rz)
  jas rl, .L_func_f
  lw v11, 4(rz)
  sw v11, 108(rz)
  pseudo.exit

