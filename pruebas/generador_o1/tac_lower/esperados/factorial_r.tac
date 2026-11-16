function factorial (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors [B3, B2]
  lw v3, 8(rz)
  mov v4, 1
  sle v0, v3, v4
  beq v0, rz, B3
B2: successors []
  lw v5, 12(rz)
  sw v5, 4(rz)
  jefR rz, rl
B3: successors [B1]
  lw v6, 8(rz)
  mov v7, 1
  sub v1, v6, v7
  lw v8, 12(rz)
  lw v9, 8(rz)
  pseudo.mul v2, v8, v9
  sw v1, 8(rz)
  sw v2, 12(rz)
  jef B1
B4: successors []
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  sw rz, 64(rz)
  mov v1, 3
  sw v1, 8(rz)
  mov v2, 1
  sw v2, 12(rz)
  jas rl, .L_func_factorial
  lw v0, 4(rz)
  sw v0, 64(rz)
  pseudo.exit

