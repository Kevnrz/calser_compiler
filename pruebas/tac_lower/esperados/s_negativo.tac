function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B2]
  sw rz, 0(rz)
  mov v3, 5
  sw v3, 4(rz)
  sw rz, 8(rz)
  jef B2
B2: successors [B4, B3]
  lw v4, 4(rz)
  lw v5, 8(rz)
  bge v5, v4, B4
B3: successors [B2]
  lw v0, 0(rz)
  lw v1, 4(rz)
  add v2, v0, v1
  sw v2, 0(rz)
  lw v6, 4(rz)
  saddi v7, v6, -1
  sw v7, 4(rz)
  jef B2
B4: successors []
  pseudo.exit

