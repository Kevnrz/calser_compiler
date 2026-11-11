function identity (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors []
  lw v0, 8(rz)
  sw v0, 4(rz)
  jefR rz, rl

function add (entry B0)
B0: successors [B1]
  sw rz, 64(rz)
  jef B1
B1: successors []
  lw v0, 68(rz)
  lw v1, 72(rz)
  add v2, v0, v1
  sw v2, 64(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v3, 10
  sw v3, 8(rz)
  jas rl, .L_func_identity
  lw v0, 4(rz)
  mov v4, 20
  sw v4, 8(rz)
  jas rl, .L_func_identity
  lw v1, 4(rz)
  sw v0, 68(rz)
  sw v1, 72(rz)
  jas rl, .L_func_add
  lw v2, 64(rz)
  sw v2, 124(rz)
  pseudo.exit

