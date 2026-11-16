function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  lw v6, 4(rz)
  lw v7, 0(rz)
  add v0, v6, v7
  sw v0, 8(rz)
  lw v8, 4(rz)
  saddi v1, v8, 1
  sw v1, 16(rz)
  lw v9, 8(rz)
  sw v9, 24(rz)
  sw rz, 28(rz)
  sw rz, 32(rz)
  sw rz, 36(rz)
  mov v2, 12
  mov v3, 28
  mov v4, 0
  jef B2
B2: successors [B4, B3]
  mov v10, 12
  bge v4, v10, B4
B3: successors [B2]
  add v11, v2, v4
  lw v5, 0(v11)
  add v12, v3, v4
  sw v5, 0(v12)
  saddi v4, v4, 4
  jef B2
B4: successors []
  pseudo.exit

