function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors []
  mov v8, 5896
  mov v9, 15
  sll v8, v8, v9
  saddi v8, v8, 10485
  sub v0, rz, v8
  pseudo.auth v0
  mov v10, 583
  mov v11, 15
  sll v10, v10, v11
  saddi v10, v10, 11213
  pseudo.kv_write v10, 0, 0
  mov v12, 60584
  mov v13, 15
  sll v12, v12, v13
  saddi v12, v12, 12817
  sub v1, rz, v12
  pseudo.kv_write v1, 0, 1
  mov v14, 9903
  mov v15, 15
  sll v14, v14, v15
  saddi v14, v14, 7135
  pseudo.kv_write v14, 0, 2
  mov v16, 18641
  mov v17, 15
  sll v16, v16, v17
  saddi v16, v16, 11488
  pseudo.kv_write v16, 0, 3
  pseudo.kv_lock
  lw v2, 0(rz)
  lw v3, 4(rz)
  mov v18, 128
  pseudo.cifrar v2, v3, v18, 0
  lw v4, 4(rz)
  lw v5, 8(rz)
  mov v19, 128
  pseudo.descifrar v4, v5, v19, 0
  pseudo.logout
  lw v6, 0(rz)
  lw v7, 12(rz)
  mov v20, 1
  pseudo.cifrar v6, v7, v20, 0
  mov v21, 999
  pseudo.auth v21
  mov v22, 5
  pseudo.sanc v22
  sw rz, 16(rz)
  pseudo.exit

