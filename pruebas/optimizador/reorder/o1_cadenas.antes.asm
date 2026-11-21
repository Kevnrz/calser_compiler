function f (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  sw rz, 20(rz)
  sw rz, 24(rz)
  sw rz, 28(rz)
  sw rz, 32(rz)
  lw v4, 4(rz)
  lw v5, 8(rz)
  add v0, v4, v5
  sw v0, 20(rz)
  lw v6, 20(rz)
  lw v7, 20(rz)
  add v1, v6, v7
  sw v1, 24(rz)
  lw v8, 12(rz)
  lw v9, 16(rz)
  add v2, v8, v9
  sw v2, 28(rz)
  lw v10, 28(rz)
  lw v11, 28(rz)
  add v3, v10, v11
  sw v3, 32(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v0, 1
  sw v0, 4(rz)
  mov v1, 2
  sw v1, 8(rz)
  mov v2, 3
  sw v2, 12(rz)
  mov v3, 4
  sw v3, 16(rz)
  jas rl, .L_func_f
  pseudo.exit

