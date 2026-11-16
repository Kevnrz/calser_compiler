function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  sw rz, 0(rz)
  mov v1, 5
  sw v1, 4(rz)
  sw rz, 8(rz)
  jef B2
B2: successors [B4, B3]
  lw v2, 4(rz)
  lw v3, 8(rz)
  bge v3, v2, B4
B3: successors [B2]
  lw v4, 0(rz)
  lw v5, 4(rz)
  add v0, v4, v5
  sw v0, 0(rz)
  lw v6, 4(rz)
  saddi v7, v6, -1
  sw v7, 4(rz)
  jef B2
B4: successors []
  pseudo.exit

