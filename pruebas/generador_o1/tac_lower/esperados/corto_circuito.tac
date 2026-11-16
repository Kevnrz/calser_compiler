function toca (entry B0)
B0: successors [B1]
  sw rz, 8(rz)
  jef B1
B1: successors []
  lw v1, 0(rz)
  saddi v0, v1, 1
  sw v0, 0(rz)
  mov v2, 1
  sw v2, 8(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B3, B2]
  mov v27, 1
  sw v27, 60(rz)
  sw rz, 64(rz)
  mov v0, 0
  beq v0, rz, B3
B2: successors [B3]
  jas rl, .L_func_toca
  lw v1, 8(rz)
  add v0, v1, rz
  jef B3
B3: successors [B5, B4]
  sw v0, 68(rz)
  lw v28, 0(rz)
  sw v28, 72(rz)
  mov v2, 1
  bne v2, rz, B5
B4: successors [B5]
  jas rl, .L_func_toca
  lw v3, 8(rz)
  add v2, v3, rz
  jef B5
B5: successors [B7, B6]
  sw v2, 76(rz)
  lw v29, 0(rz)
  sw v29, 80(rz)
  mov v4, 1
  beq v4, rz, B7
B6: successors [B7]
  jas rl, .L_func_toca
  lw v5, 8(rz)
  add v4, v5, rz
  jef B7
B7: successors [B9, B8]
  sw v4, 84(rz)
  lw v30, 0(rz)
  sw v30, 88(rz)
  mov v6, 0
  bne v6, rz, B9
B8: successors [B9]
  jas rl, .L_func_toca
  lw v7, 8(rz)
  add v6, v7, rz
  jef B9
B9: successors [B11, B10]
  sw v6, 92(rz)
  lw v31, 0(rz)
  sw v31, 96(rz)
  lw v8, 64(rz)
  beq v8, rz, B11
B10: successors [B11]
  lw v8, 64(rz)
  jef B11
B11: successors [B13, B12]
  sw v8, 100(rz)
  lw v9, 64(rz)
  beq v9, rz, B13
B12: successors [B13]
  lw v9, 60(rz)
  jef B13
B13: successors [B15, B14]
  sw v9, 104(rz)
  lw v10, 60(rz)
  beq v10, rz, B15
B14: successors [B15]
  lw v10, 64(rz)
  jef B15
B15: successors [B17, B16]
  sw v10, 108(rz)
  lw v11, 60(rz)
  beq v11, rz, B17
B16: successors [B17]
  lw v11, 60(rz)
  jef B17
B17: successors [B19, B18]
  sw v11, 112(rz)
  lw v12, 64(rz)
  bne v12, rz, B19
B18: successors [B19]
  lw v12, 64(rz)
  jef B19
B19: successors [B21, B20]
  sw v12, 116(rz)
  lw v13, 64(rz)
  bne v13, rz, B21
B20: successors [B21]
  lw v13, 60(rz)
  jef B21
B21: successors [B23, B22]
  sw v13, 120(rz)
  lw v14, 60(rz)
  bne v14, rz, B23
B22: successors [B23]
  lw v14, 64(rz)
  jef B23
B23: successors [B25, B24]
  sw v14, 124(rz)
  lw v15, 60(rz)
  bne v15, rz, B25
B24: successors [B25]
  lw v15, 60(rz)
  jef B25
B25: successors [B27, B26]
  sw v15, 128(rz)
  lw v16, 64(rz)
  beq v16, rz, B27
B26: successors [B27]
  lw v16, 64(rz)
  jef B27
B27: successors [B29, B28]
  add v17, v16, rz
  bne v17, rz, B29
B28: successors [B29]
  lw v17, 60(rz)
  jef B29
B29: successors [B33, B30]
  sw v17, 132(rz)
  lw v18, 60(rz)
  bne v18, rz, B33
B30: successors [B32, B31]
  lw v19, 64(rz)
  beq v19, rz, B32
B31: successors [B32]
  lw v19, 64(rz)
  jef B32
B32: successors [B33]
  add v18, v19, rz
  jef B33
B33: successors [B35, B34]
  sw v18, 136(rz)
  sw rz, 140(rz)
  sw rz, 144(rz)
  sw rz, 148(rz)
  lw v32, 140(rz)
  slt v20, rz, v32
  add v21, v20, rz
  beq v21, rz, B35
B34: successors [B35]
  jas rl, .L_func_toca
  lw v22, 8(rz)
  add v21, v22, rz
  jef B35
B35: successors [B37, B36]
  beq v21, rz, B37
B36: successors [B38]
  mov v33, 1
  sw v33, 144(rz)
  jef B38
B37: successors [B38]
  jef B38
B38: successors [B39]
  lw v34, 0(rz)
  sw v34, 152(rz)
  jef B39
B39: successors [B41, B40]
  lw v35, 148(rz)
  mov v36, 2
  slt v23, v35, v36
  add v24, v23, rz
  beq v24, rz, B41
B40: successors [B41]
  jas rl, .L_func_toca
  lw v25, 8(rz)
  add v24, v25, rz
  jef B41
B41: successors [B43, B42]
  beq v24, rz, B43
B42: successors [B39]
  lw v37, 148(rz)
  saddi v26, v37, 1
  sw v26, 148(rz)
  jef B39
B43: successors []
  pseudo.exit

