function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v2, -65536
  mov v3, 15
  sll v2, v2, v3
  saddi v2, v2, 0
  sub v0, rz, v2
  sw v0, 0(rz)
  mov v4, 65535
  mov v5, 15
  sll v4, v4, v5
  saddi v4, v4, 32767
  sw v4, 4(rz)
  mov v6, 6
  mov v7, 15
  sll v6, v6, v7
  saddi v6, v6, 3392
  sw v6, 8(rz)
  mov v8, 4
  mov v9, 15
  sll v8, v8, v9
  saddi v8, v8, 1
  sub v1, rz, v8
  sw v1, 12(rz)
  pseudo.exit

