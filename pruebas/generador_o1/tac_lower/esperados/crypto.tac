function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v2, 5896
  mov v3, 15
  sll v2, v2, v3
  saddi v2, v2, 10485
  sub v0, rz, v2
  pseudo.auth v0
  mov v4, 583
  mov v5, 15
  sll v4, v4, v5
  saddi v4, v4, 11213
  pseudo.kv_write v4, 0, 0
  mov v6, 60584
  mov v7, 15
  sll v6, v6, v7
  saddi v6, v6, 12817
  sub v1, rz, v6
  pseudo.kv_write v1, 0, 1
  mov v8, 9903
  mov v9, 15
  sll v8, v8, v9
  saddi v8, v8, 7135
  pseudo.kv_write v8, 0, 2
  mov v10, 18641
  mov v11, 15
  sll v10, v10, v11
  saddi v10, v10, 11488
  pseudo.kv_write v10, 0, 3
  pseudo.kv_lock
  lw v12, 0(rz)
  lw v13, 4(rz)
  mov v14, 128
  pseudo.cifrar v12, v13, v14, 0
  lw v15, 4(rz)
  lw v16, 8(rz)
  mov v17, 128
  pseudo.descifrar v15, v16, v17, 0
  pseudo.logout
  lw v18, 0(rz)
  lw v19, 12(rz)
  mov v20, 1
  pseudo.cifrar v18, v19, v20, 0
  mov v21, 999
  pseudo.auth v21
  mov v22, 5
  pseudo.sanc v22
  sw rz, 16(rz)
  pseudo.exit

