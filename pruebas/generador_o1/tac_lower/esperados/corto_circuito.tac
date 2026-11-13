function toca (entry B0)
B0: successors [B1]
  sw rz, 8(rz)
  jef B1
B1: successors []
  lw v0, 0(rz)
  saddi v1, v0, 1
  sw v1, 0(rz)
  mov v2, 1
  sw v2, 8(rz)
  jefR rz, rl

function main (entry B0)
B0: successors [B1]
  jef B1
B1: successors [B3, B2]
  mov v57, 1
  sw v57, 60(rz)
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
  lw v2, 0(rz)
  sw v2, 72(rz)
  mov v3, 1
  bne v3, rz, B5
B4: successors [B5]
  jas rl, .L_func_toca
  lw v4, 8(rz)
  add v3, v4, rz
  jef B5
B5: successors [B7, B6]
  sw v3, 76(rz)
  lw v5, 0(rz)
  sw v5, 80(rz)
  mov v6, 1
  beq v6, rz, B7
B6: successors [B7]
  jas rl, .L_func_toca
  lw v7, 8(rz)
  add v6, v7, rz
  jef B7
B7: successors [B9, B8]
  sw v6, 84(rz)
  lw v8, 0(rz)
  sw v8, 88(rz)
  mov v9, 0
  bne v9, rz, B9
B8: successors [B9]
  jas rl, .L_func_toca
  lw v10, 8(rz)
  add v9, v10, rz
  jef B9
B9: successors [B11, B10]
  sw v9, 92(rz)
  lw v11, 0(rz)
  sw v11, 96(rz)
  lw v12, 64(rz)
  add v13, v12, rz
  beq v13, rz, B11
B10: successors [B11]
  lw v14, 64(rz)
  add v13, v14, rz
  jef B11
B11: successors [B13, B12]
  sw v13, 100(rz)
  lw v15, 64(rz)
  add v16, v15, rz
  beq v16, rz, B13
B12: successors [B13]
  lw v17, 60(rz)
  add v16, v17, rz
  jef B13
B13: successors [B15, B14]
  sw v16, 104(rz)
  lw v18, 60(rz)
  add v19, v18, rz
  beq v19, rz, B15
B14: successors [B15]
  lw v20, 64(rz)
  add v19, v20, rz
  jef B15
B15: successors [B17, B16]
  sw v19, 108(rz)
  lw v21, 60(rz)
  add v22, v21, rz
  beq v22, rz, B17
B16: successors [B17]
  lw v23, 60(rz)
  add v22, v23, rz
  jef B17
B17: successors [B19, B18]
  sw v22, 112(rz)
  lw v24, 64(rz)
  add v25, v24, rz
  bne v25, rz, B19
B18: successors [B19]
  lw v26, 64(rz)
  add v25, v26, rz
  jef B19
B19: successors [B21, B20]
  sw v25, 116(rz)
  lw v27, 64(rz)
  add v28, v27, rz
  bne v28, rz, B21
B20: successors [B21]
  lw v29, 60(rz)
  add v28, v29, rz
  jef B21
B21: successors [B23, B22]
  sw v28, 120(rz)
  lw v30, 60(rz)
  add v31, v30, rz
  bne v31, rz, B23
B22: successors [B23]
  lw v32, 64(rz)
  add v31, v32, rz
  jef B23
B23: successors [B25, B24]
  sw v31, 124(rz)
  lw v33, 60(rz)
  add v34, v33, rz
  bne v34, rz, B25
B24: successors [B25]
  lw v35, 60(rz)
  add v34, v35, rz
  jef B25
B25: successors [B27, B26]
  sw v34, 128(rz)
  lw v36, 64(rz)
  add v37, v36, rz
  beq v37, rz, B27
B26: successors [B27]
  lw v38, 64(rz)
  add v37, v38, rz
  jef B27
B27: successors [B29, B28]
  add v39, v37, rz
  bne v39, rz, B29
B28: successors [B29]
  lw v40, 60(rz)
  add v39, v40, rz
  jef B29
B29: successors [B33, B30]
  sw v39, 132(rz)
  lw v41, 60(rz)
  add v42, v41, rz
  bne v42, rz, B33
B30: successors [B32, B31]
  lw v43, 64(rz)
  add v44, v43, rz
  beq v44, rz, B32
B31: successors [B32]
  lw v45, 64(rz)
  add v44, v45, rz
  jef B32
B32: successors [B33]
  add v42, v44, rz
  jef B33
B33: successors [B35, B34]
  sw v42, 136(rz)
  sw rz, 140(rz)
  sw rz, 144(rz)
  sw rz, 148(rz)
  lw v46, 140(rz)
  slt v47, rz, v46
  add v48, v47, rz
  beq v48, rz, B35
B34: successors [B35]
  jas rl, .L_func_toca
  lw v49, 8(rz)
  add v48, v49, rz
  jef B35
B35: successors [B37, B36]
  beq v48, rz, B37
B36: successors [B38]
  mov v58, 1
  sw v58, 144(rz)
  jef B38
B37: successors [B38]
  jef B38
B38: successors [B39]
  lw v50, 0(rz)
  sw v50, 152(rz)
  jef B39
B39: successors [B41, B40]
  lw v51, 148(rz)
  mov v59, 2
  slt v52, v51, v59
  add v53, v52, rz
  beq v53, rz, B41
B40: successors [B41]
  jas rl, .L_func_toca
  lw v54, 8(rz)
  add v53, v54, rz
  jef B41
B41: successors [B43, B42]
  beq v53, rz, B43
B42: successors [B39]
  lw v55, 148(rz)
  saddi v56, v55, 1
  sw v56, 148(rz)
  jef B39
B43: successors []
  pseudo.exit

