function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  lw v0, 4(rz)
  lw v1, 0(rz)
  add v2, v0, v1
  sw v2, 8(rz)
  lw v3, 4(rz)
  saddi v4, v3, 1
  sw v4, 16(rz)
  lw v5, 8(rz)
  sw v5, 24(rz)
  sw rz, 28(rz)
  sw rz, 32(rz)
  sw rz, 36(rz)
  mov v6, 12
  mov v7, 28
  mov v8, 0
  jef B2
B2: successors [B4, B3]
  mov v10, 12
  bge v8, v10, B4
B3: successors [B2]
  add v11, v6, v8
  lw v9, 0(v11)
  add v12, v7, v8
  sw v9, 0(v12)
  saddi v8, v8, 4
  jef B2
B4: successors []
  pseudo.exit

