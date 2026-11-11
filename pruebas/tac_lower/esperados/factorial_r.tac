function factorial (entry B0)
B0: successors [B1]
  sw rz, 4(rz)
  jef B1
B1: successors [B3, B2]
  lw v0, 8(rz)
  mov v8, 1
  sle v1, v0, v8
  beq v1, rz, B3
B2: successors []
  lw v2, 12(rz)
  sw v2, 4(rz)
  jefR rz, rl
B3: successors [B1]
  lw v3, 8(rz)
  mov v9, 1
  sub v4, v3, v9
  lw v5, 12(rz)
  lw v6, 8(rz)
  pseudo.mul v7, v5, v6
  sw v4, 8(rz)
  sw v7, 12(rz)
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

