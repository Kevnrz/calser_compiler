function tocar (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  jefR rz, rl

function f (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  lw v5, 68(rz)
  lw v6, 72(rz)
  lw v7, 68(rz)
  add v0, v5, v6
  sw v7, 12(rz)
  sw v0, 76(rz)
  jas rl, .L_func_tocar
  lw v8, 76(rz)
  mov v10, 2
  mov v13, 2
  lw v9, 72(rz)
  sll v2, rz, v10
  sll v3, rz, v13
  add v1, v8, v9
  add v12, rz, v2
  add v14, rz, v3
  sw v1, 76(rz)
  lw v11, 76(rz)
  sw v11, 0(v12)
  lw v4, 0(v14)
  sw v4, 76(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v0, 8
  mov v1, 2
  sw v0, 68(rz)
  sw v1, 72(rz)
  jas rl, .L_func_f
  pseudo.exit

