# ccom: ejercicios/ejercicio3_cifrado/ejercicio3.cls
  { mov rx, 428 | nopi | nopi | nopi }                                          # 0000
  { nopi | nopi | nopi | nopi }                                                 # 0001
.L_func_main:                                                                   # bundle 2
.L_body_main:                                                                   # bundle 2
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0002  linea 60
  { nopi | nopi | nopi | nopi }                                                 # 0003
  { nopi | nopi | nopi | sw t0, 372(rz) }                                       # 0004
  { nopi | nopi | nopi | nopi }                                                 # 0005
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0006  linea 61
  { nopi | nopi | nopi | nopi }                                                 # 0007
  { nopi | nopi | nopi | sw t0, 376(rz) }                                       # 0008
  { nopi | nopi | nopi | nopi }                                                 # 0009
  { nopi | nopi | nopi | nopi }                                                 # 0010
  { mov t0, 37 | mov t1, 15 | nopi | nopi }                                     # 0011  linea 62
  { nopi | nopi | nopi | nopi }                                                 # 0012
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0013
  { nopi | nopi | nopi | nopi }                                                 # 0014
  { saddi t0, t0, 22151 | nopi | nopi | nopi }                                  # 0015
  { nopi | nopi | nopi | nopi }                                                 # 0016
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0017
  { nopi | nopi | nopi | nopi }                                                 # 0018
  { nopi | nopi | nopi | nopi }                                                 # 0019
  { nopi | auth t6 | nopi | nopi }                                              # 0020
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0021  linea 63
  { nopi | nopi | nopi | nopi }                                                 # 0022
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0023
  { nopi | nopi | nopi | nopi }                                                 # 0024
  { nopi | nopi | jas rl, .L_func_preparar_boveda | nopi }                      # 0025
  { nopi | nopi | nopi | nopi }                                                 # 0026
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0027
  { nopi | nopi | nopi | nopi }                                                 # 0028
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0029
  { nopi | nopi | nopi | nopi }                                                 # 0030
  { nopi | nopi | nopi | lw t0, 0(rz) }                                         # 0031  linea 64
  { nopi | nopi | nopi | nopi }                                                 # 0032
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0033
  { nopi | nopi | nopi | nopi }                                                 # 0034
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0035
  { nopi | nopi | nopi | nopi }                                                 # 0036
  { nopi | nopi | nopi | lw t0, 4(rz) }                                         # 0037
  { nopi | nopi | nopi | nopi }                                                 # 0038
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0039
  { nopi | nopi | nopi | nopi }                                                 # 0040
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0041
  { nopi | nopi | nopi | nopi }                                                 # 0042
  { mov t0, 128 | nopi | nopi | nopi }                                          # 0043
  { nopi | nopi | nopi | nopi }                                                 # 0044
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0045
  { nopi | nopi | nopi | nopi }                                                 # 0046
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0047
  { nopi | nopi | nopi | nopi }                                                 # 0048
  { mov t0, 16 | nopi | nopi | nopi }                                           # 0049
  { nopi | nopi | nopi | nopi }                                                 # 0050
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0051
  { nopi | nopi | nopi | nopi }                                                 # 0052
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0053
  { nopi | nopi | nopi | nopi }                                                 # 0054
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0055
  { nopi | nopi | nopi | nopi }                                                 # 0056
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0057
  { nopi | nopi | nopi | nopi }                                                 # 0058
  { nopi | nopi | nopi | sw t0, 224(rz) }                                       # 0059
  { nopi | nopi | nopi | nopi }                                                 # 0060
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0061
  { nopi | nopi | nopi | nopi }                                                 # 0062
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0063
  { nopi | nopi | nopi | nopi }                                                 # 0064
  { nopi | nopi | nopi | sw t0, 220(rz) }                                       # 0065
  { nopi | nopi | nopi | nopi }                                                 # 0066
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0067
  { nopi | nopi | nopi | nopi }                                                 # 0068
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0069
  { nopi | nopi | nopi | nopi }                                                 # 0070
  { nopi | nopi | nopi | sw t0, 216(rz) }                                       # 0071
  { nopi | nopi | nopi | nopi }                                                 # 0072
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0073
  { nopi | nopi | nopi | nopi }                                                 # 0074
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0075
  { nopi | nopi | nopi | nopi }                                                 # 0076
  { nopi | nopi | nopi | sw t0, 212(rz) }                                       # 0077
  { nopi | nopi | nopi | nopi }                                                 # 0078
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0079
  { nopi | nopi | nopi | nopi }                                                 # 0080
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0081
  { nopi | nopi | nopi | nopi }                                                 # 0082
  { nopi | nopi | jas rl, .L_func_cifrar_archivo | nopi }                       # 0083
  { nopi | nopi | nopi | nopi }                                                 # 0084
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0085
  { nopi | nopi | nopi | nopi }                                                 # 0086
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0087
  { nopi | nopi | nopi | nopi }                                                 # 0088
  { nopi | nopi | nopi | lw t0, 208(rz) }                                       # 0089
  { nopi | nopi | nopi | nopi }                                                 # 0090
  { nopi | nopi | nopi | sw t0, 372(rz) }                                       # 0091
  { nopi | nopi | nopi | nopi }                                                 # 0092
  { nopi | nopi | nopi | lw t0, 4(rz) }                                         # 0093  linea 65
  { nopi | nopi | nopi | nopi }                                                 # 0094
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0095
  { nopi | nopi | nopi | nopi }                                                 # 0096
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0097
  { nopi | nopi | nopi | nopi }                                                 # 0098
  { nopi | nopi | nopi | lw t0, 8(rz) }                                         # 0099
  { nopi | nopi | nopi | nopi }                                                 # 0100
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0101
  { nopi | nopi | nopi | nopi }                                                 # 0102
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0103
  { nopi | nopi | nopi | nopi }                                                 # 0104
  { mov t0, 128 | nopi | nopi | nopi }                                          # 0105
  { nopi | nopi | nopi | nopi }                                                 # 0106
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0107
  { nopi | nopi | nopi | nopi }                                                 # 0108
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0109
  { nopi | nopi | nopi | nopi }                                                 # 0110
  { mov t0, 16 | nopi | nopi | nopi }                                           # 0111
  { nopi | nopi | nopi | nopi }                                                 # 0112
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0113
  { nopi | nopi | nopi | nopi }                                                 # 0114
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0115
  { nopi | nopi | nopi | nopi }                                                 # 0116
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0117
  { nopi | nopi | nopi | nopi }                                                 # 0118
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0119
  { nopi | nopi | nopi | nopi }                                                 # 0120
  { nopi | nopi | nopi | sw t0, 308(rz) }                                       # 0121
  { nopi | nopi | nopi | nopi }                                                 # 0122
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0123
  { nopi | nopi | nopi | nopi }                                                 # 0124
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0125
  { nopi | nopi | nopi | nopi }                                                 # 0126
  { nopi | nopi | nopi | sw t0, 304(rz) }                                       # 0127
  { nopi | nopi | nopi | nopi }                                                 # 0128
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0129
  { nopi | nopi | nopi | nopi }                                                 # 0130
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0131
  { nopi | nopi | nopi | nopi }                                                 # 0132
  { nopi | nopi | nopi | sw t0, 300(rz) }                                       # 0133
  { nopi | nopi | nopi | nopi }                                                 # 0134
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0135
  { nopi | nopi | nopi | nopi }                                                 # 0136
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0137
  { nopi | nopi | nopi | nopi }                                                 # 0138
  { nopi | nopi | nopi | sw t0, 296(rz) }                                       # 0139
  { nopi | nopi | nopi | nopi }                                                 # 0140
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0141
  { nopi | nopi | nopi | nopi }                                                 # 0142
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0143
  { nopi | nopi | nopi | nopi }                                                 # 0144
  { nopi | nopi | jas rl, .L_func_descifrar_archivo | nopi }                    # 0145
  { nopi | nopi | nopi | nopi }                                                 # 0146
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0147
  { nopi | nopi | nopi | nopi }                                                 # 0148
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0149
  { nopi | nopi | nopi | nopi }                                                 # 0150
  { nopi | nopi | nopi | lw t0, 292(rz) }                                       # 0151
  { nopi | nopi | nopi | nopi }                                                 # 0152
  { nopi | nopi | nopi | sw t0, 376(rz) }                                       # 0153
  { nopi | nopi | nopi | nopi }                                                 # 0154
  { nopi | logout | nopi | nopi }                                               # 0155  linea 66
  { nopi | nopi | jef fin_main | nopi }                                         # 0156  linea 67
  { nopi | nopi | nopi | nopi }                                                 # 0157
.L_func_bloques_de:                                                             # bundle 158
  { nopi | nopi | nopi | sw rz, 16(rz) }                                        # 0158  linea 5
  { nopi | nopi | nopi | nopi }                                                 # 0159
.L_body_bloques_de:                                                             # bundle 160
  { nopi | nopi | nopi | lw t0, 20(rz) }                                        # 0160  linea 6
  { nopi | nopi | nopi | nopi }                                                 # 0161
  { nopi | nopi | nopi | lw t1, 28(rz) }                                        # 0162
  { nopi | nopi | nopi | nopi }                                                 # 0163
  { nopi | nopi | nopi | lw t2, 24(rz) }                                        # 0164
  { nopi | nopi | nopi | nopi }                                                 # 0165
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0166
  { nopi | nopi | nopi | nopi }                                                 # 0167
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0168
  { nopi | nopi | nopi | nopi }                                                 # 0169
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0170
  { nopi | nopi | nopi | nopi }                                                 # 0171
  { mov r7, 0 | nopi | nopi | nopi }                                            # 0172
  { nopi | nopi | nopi | nopi }                                                 # 0173
__ccom_arith_0_mul:                                                             # bundle 174
  { nopi | nopi | beq r9, r0, __ccom_arith_2_end | nopi }                       # 0174
  { nopi | nopi | nopi | nopi }                                                 # 0175
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0176
  { nopi | nopi | nopi | nopi }                                                 # 0177
  { nopi | nopi | beq r10, r0, __ccom_arith_1_skip | nopi }                     # 0178
  { nopi | nopi | nopi | nopi }                                                 # 0179
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0180
  { nopi | nopi | nopi | nopi }                                                 # 0181
__ccom_arith_1_skip:                                                            # bundle 182
  { sll r8, r8, r11 | nopi | nopi | nopi }                                      # 0182
  { nopi | nopi | nopi | nopi }                                                 # 0183
  { srl r9, r9, r11 | nopi | nopi | nopi }                                      # 0184
  { nopi | nopi | nopi | nopi }                                                 # 0185
  { nopi | nopi | jef __ccom_arith_0_mul | nopi }                               # 0186
  { nopi | nopi | nopi | nopi }                                                 # 0187
__ccom_arith_2_end:                                                             # bundle 188
  { sub t0, t0, t3 | nopi | nopi | nopi }                                       # 0188
  { nopi | nopi | nopi | nopi }                                                 # 0189
  { nopi | nopi | nopi | sw t0, 32(rz) }                                        # 0190
  { nopi | nopi | nopi | nopi }                                                 # 0191
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0192  linea 7
  { nopi | nopi | nopi | nopi }                                                 # 0193
  { nopi | nopi | nopi | lw t1, 24(rz) }                                        # 0194
  { nopi | nopi | nopi | nopi }                                                 # 0195
  { slt t0, t0, t1 | nopi | nopi | nopi }                                       # 0196
  { nopi | nopi | nopi | nopi }                                                 # 0197
  { nopi | nopi | beq t0, rz, .L_if_next_0 | nopi }                             # 0198
  { nopi | nopi | nopi | nopi }                                                 # 0199
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0200  linea 8
  { nopi | nopi | nopi | nopi }                                                 # 0201
  { nopi | nopi | nopi | sw t0, 16(rz) }                                        # 0202
  { nopi | nopi | nopi | nopi }                                                 # 0203
  { nopi | nopi | jef .L_if_end_1 | nopi }                                      # 0204  linea 7
  { nopi | nopi | nopi | nopi }                                                 # 0205
.L_if_next_0:                                                                   # bundle 206
  { nopi | nopi | nopi | lw t0, 24(rz) }                                        # 0206  linea 10
  { nopi | nopi | nopi | nopi }                                                 # 0207
  { nopi | nopi | nopi | sw t0, 16(rz) }                                        # 0208
  { nopi | nopi | nopi | nopi }                                                 # 0209
.L_if_end_1:                                                                    # bundle 210
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0210  linea 12
  { nopi | nopi | nopi | nopi }                                                 # 0211
.L_func_direccion:                                                              # bundle 212
  { nopi | nopi | nopi | sw rz, 88(rz) }                                        # 0212  linea 15
  { nopi | nopi | nopi | nopi }                                                 # 0213
.L_body_direccion:                                                              # bundle 214
  { nopi | nopi | nopi | lw t0, 92(rz) }                                        # 0214  linea 16
  { nopi | nopi | nopi | nopi }                                                 # 0215
  { nopi | nopi | nopi | lw t1, 100(rz) }                                       # 0216
  { nopi | nopi | nopi | nopi }                                                 # 0217
  { nopi | nopi | nopi | lw t2, 96(rz) }                                        # 0218
  { nopi | nopi | nopi | nopi }                                                 # 0219
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0220
  { nopi | nopi | nopi | nopi }                                                 # 0221
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0222
  { nopi | nopi | nopi | nopi }                                                 # 0223
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0224
  { nopi | nopi | nopi | nopi }                                                 # 0225
  { mov r7, 0 | nopi | nopi | nopi }                                            # 0226
  { nopi | nopi | nopi | nopi }                                                 # 0227
__ccom_arith_3_mul:                                                             # bundle 228
  { nopi | nopi | beq r9, r0, __ccom_arith_5_end | nopi }                       # 0228
  { nopi | nopi | nopi | nopi }                                                 # 0229
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0230
  { nopi | nopi | nopi | nopi }                                                 # 0231
  { nopi | nopi | beq r10, r0, __ccom_arith_4_skip | nopi }                     # 0232
  { nopi | nopi | nopi | nopi }                                                 # 0233
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0234
  { nopi | nopi | nopi | nopi }                                                 # 0235
__ccom_arith_4_skip:                                                            # bundle 236
  { sll r8, r8, r11 | nopi | nopi | nopi }                                      # 0236
  { nopi | nopi | nopi | nopi }                                                 # 0237
  { srl r9, r9, r11 | nopi | nopi | nopi }                                      # 0238
  { nopi | nopi | nopi | nopi }                                                 # 0239
  { nopi | nopi | jef __ccom_arith_3_mul | nopi }                               # 0240
  { nopi | nopi | nopi | nopi }                                                 # 0241
__ccom_arith_5_end:                                                             # bundle 242
  { mov t1, 8 | nopi | nopi | nopi }                                            # 0242
  { nopi | nopi | nopi | nopi }                                                 # 0243
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0244
  { nopi | nopi | nopi | nopi }                                                 # 0245
  { add r8, r7, r0 | nopi | nopi | nopi }                                       # 0246
  { nopi | nopi | nopi | nopi }                                                 # 0247
  { add r9, r5, r0 | nopi | nopi | nopi }                                       # 0248
  { nopi | nopi | nopi | nopi }                                                 # 0249
  { mov r6, 0 | nopi | nopi | nopi }                                            # 0250
  { nopi | nopi | nopi | nopi }                                                 # 0251
__ccom_arith_6_mul:                                                             # bundle 252
  { nopi | nopi | beq r9, r0, __ccom_arith_8_end | nopi }                       # 0252
  { nopi | nopi | nopi | nopi }                                                 # 0253
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0254
  { nopi | nopi | nopi | nopi }                                                 # 0255
  { nopi | nopi | beq r10, r0, __ccom_arith_7_skip | nopi }                     # 0256
  { nopi | nopi | nopi | nopi }                                                 # 0257
  { add r6, r6, r8 | nopi | nopi | nopi }                                       # 0258
  { nopi | nopi | nopi | nopi }                                                 # 0259
__ccom_arith_7_skip:                                                            # bundle 260
  { sll r8, r8, r11 | nopi | nopi | nopi }                                      # 0260
  { nopi | nopi | nopi | nopi }                                                 # 0261
  { srl r9, r9, r11 | nopi | nopi | nopi }                                      # 0262
  { nopi | nopi | nopi | nopi }                                                 # 0263
  { nopi | nopi | jef __ccom_arith_6_mul | nopi }                               # 0264
  { nopi | nopi | nopi | nopi }                                                 # 0265
__ccom_arith_8_end:                                                             # bundle 266
  { add t0, t0, t2 | nopi | nopi | nopi }                                       # 0266
  { nopi | nopi | nopi | nopi }                                                 # 0267
  { nopi | nopi | nopi | sw t0, 88(rz) }                                        # 0268
  { nopi | nopi | nopi | nopi }                                                 # 0269
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0270  linea 17
  { nopi | nopi | nopi | nopi }                                                 # 0271
.L_func_preparar_boveda:                                                        # bundle 272
.L_body_preparar_boveda:                                                        # bundle 272
  { nopi | nopi | nopi | nopi }                                                 # 0272
  { mov t0, 583 | mov t1, 15 | nopi | nopi }                                    # 0273  linea 21
  { nopi | nopi | nopi | nopi }                                                 # 0274
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0275
  { nopi | nopi | nopi | nopi }                                                 # 0276
  { saddi t0, t0, 11213 | nopi | nopi | nopi }                                  # 0277
  { nopi | nopi | nopi | nopi }                                                 # 0278
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0279
  { nopi | nopi | nopi | nopi }                                                 # 0280
  { nopi | nopi | nopi | nopi }                                                 # 0281
  { nopi | kv_write t6, 0, 0 | nopi | nopi }                                    # 0282
  { nopi | nopi | nopi | nopi }                                                 # 0283
  { mov t0, 60584 | mov t1, 15 | nopi | nopi }                                  # 0284  linea 22
  { nopi | nopi | nopi | nopi }                                                 # 0285
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0286
  { nopi | nopi | nopi | nopi }                                                 # 0287
  { saddi t0, t0, 12817 | nopi | nopi | nopi }                                  # 0288
  { nopi | nopi | nopi | nopi }                                                 # 0289
  { sub t0, rz, t0 | nopi | nopi | nopi }                                       # 0290
  { nopi | nopi | nopi | nopi }                                                 # 0291
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0292
  { nopi | nopi | nopi | nopi }                                                 # 0293
  { nopi | nopi | nopi | nopi }                                                 # 0294
  { nopi | kv_write t6, 0, 1 | nopi | nopi }                                    # 0295
  { nopi | nopi | nopi | nopi }                                                 # 0296
  { mov t0, 9903 | mov t1, 15 | nopi | nopi }                                   # 0297  linea 23
  { nopi | nopi | nopi | nopi }                                                 # 0298
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0299
  { nopi | nopi | nopi | nopi }                                                 # 0300
  { saddi t0, t0, 7135 | nopi | nopi | nopi }                                   # 0301
  { nopi | nopi | nopi | nopi }                                                 # 0302
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0303
  { nopi | nopi | nopi | nopi }                                                 # 0304
  { nopi | nopi | nopi | nopi }                                                 # 0305
  { nopi | kv_write t6, 0, 2 | nopi | nopi }                                    # 0306
  { nopi | nopi | nopi | nopi }                                                 # 0307
  { mov t0, 18641 | mov t1, 15 | nopi | nopi }                                  # 0308  linea 24
  { nopi | nopi | nopi | nopi }                                                 # 0309
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0310
  { nopi | nopi | nopi | nopi }                                                 # 0311
  { saddi t0, t0, 11488 | nopi | nopi | nopi }                                  # 0312
  { nopi | nopi | nopi | nopi }                                                 # 0313
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0314
  { nopi | nopi | nopi | nopi }                                                 # 0315
  { nopi | nopi | nopi | nopi }                                                 # 0316
  { nopi | kv_write t6, 0, 3 | nopi | nopi }                                    # 0317
  { nopi | nopi | nopi | nopi }                                                 # 0318
  { mov t0, 8738 | mov t1, 15 | nopi | nopi }                                   # 0319  linea 25
  { nopi | nopi | nopi | nopi }                                                 # 0320
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0321
  { nopi | nopi | nopi | nopi }                                                 # 0322
  { saddi t0, t0, 4369 | nopi | nopi | nopi }                                   # 0323
  { nopi | nopi | nopi | nopi }                                                 # 0324
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0325
  { nopi | nopi | nopi | nopi }                                                 # 0326
  { nopi | nopi | nopi | nopi }                                                 # 0327
  { nopi | kv_write t6, 1, 0 | nopi | nopi }                                    # 0328
  { nopi | nopi | nopi | nopi }                                                 # 0329
  { mov t0, 17476 | mov t1, 15 | nopi | nopi }                                  # 0330  linea 26
  { nopi | nopi | nopi | nopi }                                                 # 0331
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0332
  { nopi | nopi | nopi | nopi }                                                 # 0333
  { saddi t0, t0, 8738 | nopi | nopi | nopi }                                   # 0334
  { nopi | nopi | nopi | nopi }                                                 # 0335
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0336
  { nopi | nopi | nopi | nopi }                                                 # 0337
  { nopi | nopi | nopi | nopi }                                                 # 0338
  { nopi | kv_write t6, 1, 1 | nopi | nopi }                                    # 0339
  { nopi | nopi | nopi | nopi }                                                 # 0340
  { mov t0, 26214 | mov t1, 15 | nopi | nopi }                                  # 0341  linea 27
  { nopi | nopi | nopi | nopi }                                                 # 0342
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0343
  { nopi | nopi | nopi | nopi }                                                 # 0344
  { saddi t0, t0, 13107 | nopi | nopi | nopi }                                  # 0345
  { nopi | nopi | nopi | nopi }                                                 # 0346
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0347
  { nopi | nopi | nopi | nopi }                                                 # 0348
  { nopi | nopi | nopi | nopi }                                                 # 0349
  { nopi | kv_write t6, 1, 2 | nopi | nopi }                                    # 0350
  { nopi | nopi | nopi | nopi }                                                 # 0351
  { mov t0, 34952 | mov t1, 15 | nopi | nopi }                                  # 0352  linea 28
  { nopi | nopi | nopi | nopi }                                                 # 0353
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0354
  { nopi | nopi | nopi | nopi }                                                 # 0355
  { saddi t0, t0, 17476 | nopi | nopi | nopi }                                  # 0356
  { nopi | nopi | nopi | nopi }                                                 # 0357
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0358
  { nopi | nopi | nopi | nopi }                                                 # 0359
  { nopi | nopi | nopi | nopi }                                                 # 0360
  { nopi | kv_write t6, 1, 3 | nopi | nopi }                                    # 0361
  { nopi | kv_lock | nopi | nopi }                                              # 0362  linea 29
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0363  linea 30
  { nopi | nopi | nopi | nopi }                                                 # 0364
.L_func_cifrar_archivo:                                                         # bundle 365
  { nopi | nopi | nopi | sw rz, 208(rz) }                                       # 0365  linea 33
  { nopi | nopi | nopi | nopi }                                                 # 0366
.L_body_cifrar_archivo:                                                         # bundle 367
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0367  linea 34
  { nopi | nopi | nopi | nopi }                                                 # 0368
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0369
  { nopi | nopi | nopi | nopi }                                                 # 0370
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0371
  { nopi | nopi | nopi | nopi }                                                 # 0372
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0373
  { nopi | nopi | nopi | nopi }                                                 # 0374
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0375
  { nopi | nopi | nopi | nopi }                                                 # 0376
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0377
  { nopi | nopi | nopi | nopi }                                                 # 0378
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0379
  { nopi | nopi | nopi | nopi }                                                 # 0380
  { add r7, r4, r0 | nopi | nopi | nopi }                                       # 0381
  { nopi | nopi | nopi | nopi }                                                 # 0382
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0383
  { nopi | nopi | nopi | nopi }                                                 # 0384
  { nopi | nopi | bge r7, r0, __ccom_arith_9_apos | nopi }                      # 0385
  { nopi | nopi | nopi | nopi }                                                 # 0386
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0387
  { nopi | nopi | nopi | nopi }                                                 # 0388
__ccom_arith_9_apos:                                                            # bundle 389
  { nopi | nopi | bge r8, r0, __ccom_arith_10_bpos | nopi }                     # 0389
  { nopi | nopi | nopi | nopi }                                                 # 0390
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0391
  { nopi | nopi | nopi | nopi }                                                 # 0392
__ccom_arith_10_bpos:                                                           # bundle 393
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0393
  { nopi | nopi | nopi | nopi }                                                 # 0394
  { mov r10, 31 | nopi | nopi | nopi }                                          # 0395
  { nopi | nopi | nopi | nopi }                                                 # 0396
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0397
  { nopi | nopi | nopi | nopi }                                                 # 0398
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0399
  { nopi | nopi | nopi | nopi }                                                 # 0400
  { mov r9, 0 | nopi | nopi | nopi }                                            # 0401
  { nopi | nopi | nopi | nopi }                                                 # 0402
  { mov r6, 0 | nopi | nopi | nopi }                                            # 0403
  { nopi | nopi | nopi | nopi }                                                 # 0404
__ccom_arith_11_div:                                                            # bundle 405
  { slt r12, r7, r0 | nopi | nopi | nopi }                                      # 0405
  { nopi | nopi | nopi | nopi }                                                 # 0406
  { sll r7, r7, r11 | nopi | nopi | nopi }                                      # 0407
  { nopi | nopi | nopi | nopi }                                                 # 0408
  { sll r9, r9, r11 | nopi | nopi | nopi }                                      # 0409
  { nopi | nopi | nopi | nopi }                                                 # 0410
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0411
  { nopi | nopi | nopi | nopi }                                                 # 0412
  { sll r6, r6, r11 | nopi | nopi | nopi }                                      # 0413
  { nopi | nopi | nopi | nopi }                                                 # 0414
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0415
  { nopi | nopi | nopi | nopi }                                                 # 0416
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0417
  { nopi | nopi | nopi | nopi }                                                 # 0418
  { nopi | nopi | blt r13, r12, __ccom_arith_12_skip | nopi }                   # 0419
  { nopi | nopi | nopi | nopi }                                                 # 0420
  { sub r9, r9, r8 | nopi | nopi | nopi }                                       # 0421
  { nopi | nopi | nopi | nopi }                                                 # 0422
  { saddi r6, r6, 1 | nopi | nopi | nopi }                                      # 0423
  { nopi | nopi | nopi | nopi }                                                 # 0424
__ccom_arith_12_skip:                                                           # bundle 425
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0425
  { nopi | nopi | nopi | nopi }                                                 # 0426
  { nopi | nopi | bne r10, r0, __ccom_arith_11_div | nopi }                     # 0427
  { nopi | nopi | nopi | nopi }                                                 # 0428
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0429
  { nopi | nopi | nopi | nopi }                                                 # 0430
  { nopi | nopi | bge r13, r0, __ccom_arith_13_end | nopi }                     # 0431
  { nopi | nopi | nopi | nopi }                                                 # 0432
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0433
  { nopi | nopi | nopi | nopi }                                                 # 0434
__ccom_arith_13_end:                                                            # bundle 435
  { nopi | nopi | nopi | sw t2, 208(rz) }                                       # 0435
  { nopi | nopi | nopi | nopi }                                                 # 0436
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0437  linea 35
  { nopi | nopi | nopi | nopi }                                                 # 0438
  { nopi | nopi | nopi | lw t1, 208(rz) }                                       # 0439
  { nopi | nopi | nopi | nopi }                                                 # 0440
  { nopi | nopi | nopi | sw t0, 228(rz) }                                       # 0441
  { nopi | nopi | nopi | nopi }                                                 # 0442
  { nopi | nopi | nopi | sw t1, 232(rz) }                                       # 0443
  { nopi | nopi | nopi | nopi }                                                 # 0444
.L_for_start_2:                                                                 # bundle 445
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0445
  { nopi | nopi | nopi | nopi }                                                 # 0446
  { nopi | nopi | nopi | lw t1, 232(rz) }                                       # 0447
  { nopi | nopi | nopi | nopi }                                                 # 0448
  { nopi | nopi | bge t0, t1, .L_for_end_4 | nopi }                             # 0449
  { nopi | nopi | nopi | nopi }                                                 # 0450
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0451  linea 36
  { nopi | nopi | nopi | nopi }                                                 # 0452
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0453
  { nopi | nopi | nopi | nopi }                                                 # 0454
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0455
  { nopi | nopi | nopi | nopi }                                                 # 0456
  { nopi | nopi | nopi | lw t0, 224(rz) }                                       # 0457
  { nopi | nopi | nopi | nopi }                                                 # 0458
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0459
  { nopi | nopi | nopi | nopi }                                                 # 0460
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0461
  { nopi | nopi | nopi | nopi }                                                 # 0462
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0463
  { nopi | nopi | nopi | nopi }                                                 # 0464
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0465
  { nopi | nopi | nopi | nopi }                                                 # 0466
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0467
  { nopi | nopi | nopi | nopi }                                                 # 0468
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0469
  { nopi | nopi | nopi | nopi }                                                 # 0470
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0471
  { nopi | nopi | nopi | nopi }                                                 # 0472
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0473
  { nopi | nopi | nopi | nopi }                                                 # 0474
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0475
  { nopi | nopi | nopi | nopi }                                                 # 0476
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0477
  { nopi | nopi | nopi | nopi }                                                 # 0478
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0479
  { nopi | nopi | nopi | nopi }                                                 # 0480
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0481
  { nopi | nopi | nopi | nopi }                                                 # 0482
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0483
  { nopi | nopi | nopi | nopi }                                                 # 0484
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0485
  { nopi | nopi | nopi | nopi }                                                 # 0486
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0487
  { nopi | nopi | nopi | nopi }                                                 # 0488
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0489
  { nopi | nopi | nopi | nopi }                                                 # 0490
  { nopi | nopi | jas rl, .L_func_bloques_de | nopi }                           # 0491
  { nopi | nopi | nopi | nopi }                                                 # 0492
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0493
  { nopi | nopi | nopi | nopi }                                                 # 0494
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0495
  { nopi | nopi | nopi | nopi }                                                 # 0496
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0497
  { nopi | nopi | nopi | nopi }                                                 # 0498
  { nopi | nopi | nopi | sw t0, 236(rz) }                                       # 0499
  { nopi | nopi | nopi | nopi }                                                 # 0500
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0501  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0502
  { mov t1, 2 | nopi | nopi | nopi }                                            # 0503
  { nopi | nopi | nopi | nopi }                                                 # 0504
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0505
  { nopi | nopi | nopi | nopi }                                                 # 0506
  { add r7, r4, r0 | nopi | nopi | nopi }                                       # 0507
  { nopi | nopi | nopi | nopi }                                                 # 0508
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0509
  { nopi | nopi | nopi | nopi }                                                 # 0510
  { nopi | nopi | bge r7, r0, __ccom_arith_14_apos | nopi }                     # 0511
  { nopi | nopi | nopi | nopi }                                                 # 0512
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0513
  { nopi | nopi | nopi | nopi }                                                 # 0514
__ccom_arith_14_apos:                                                           # bundle 515
  { nopi | nopi | bge r8, r0, __ccom_arith_15_bpos | nopi }                     # 0515
  { nopi | nopi | nopi | nopi }                                                 # 0516
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0517
  { nopi | nopi | nopi | nopi }                                                 # 0518
__ccom_arith_15_bpos:                                                           # bundle 519
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0519
  { nopi | nopi | nopi | nopi }                                                 # 0520
  { mov r10, 31 | nopi | nopi | nopi }                                          # 0521
  { nopi | nopi | nopi | nopi }                                                 # 0522
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0523
  { nopi | nopi | nopi | nopi }                                                 # 0524
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0525
  { nopi | nopi | nopi | nopi }                                                 # 0526
  { mov r9, 0 | nopi | nopi | nopi }                                            # 0527
  { nopi | nopi | nopi | nopi }                                                 # 0528
  { mov r6, 0 | nopi | nopi | nopi }                                            # 0529
  { nopi | nopi | nopi | nopi }                                                 # 0530
__ccom_arith_16_div:                                                            # bundle 531
  { slt r12, r7, r0 | nopi | nopi | nopi }                                      # 0531
  { nopi | nopi | nopi | nopi }                                                 # 0532
  { sll r7, r7, r11 | nopi | nopi | nopi }                                      # 0533
  { nopi | nopi | nopi | nopi }                                                 # 0534
  { sll r9, r9, r11 | nopi | nopi | nopi }                                      # 0535
  { nopi | nopi | nopi | nopi }                                                 # 0536
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0537
  { nopi | nopi | nopi | nopi }                                                 # 0538
  { sll r6, r6, r11 | nopi | nopi | nopi }                                      # 0539
  { nopi | nopi | nopi | nopi }                                                 # 0540
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0541
  { nopi | nopi | nopi | nopi }                                                 # 0542
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0543
  { nopi | nopi | nopi | nopi }                                                 # 0544
  { nopi | nopi | blt r13, r12, __ccom_arith_17_skip | nopi }                   # 0545
  { nopi | nopi | nopi | nopi }                                                 # 0546
  { sub r9, r9, r8 | nopi | nopi | nopi }                                       # 0547
  { nopi | nopi | nopi | nopi }                                                 # 0548
  { saddi r6, r6, 1 | nopi | nopi | nopi }                                      # 0549
  { nopi | nopi | nopi | nopi }                                                 # 0550
__ccom_arith_17_skip:                                                           # bundle 551
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0551
  { nopi | nopi | nopi | nopi }                                                 # 0552
  { nopi | nopi | bne r10, r0, __ccom_arith_16_div | nopi }                     # 0553
  { nopi | nopi | nopi | nopi }                                                 # 0554
  { add r6, r9, r0 | nopi | nopi | nopi }                                       # 0555
  { nopi | nopi | nopi | nopi }                                                 # 0556
  { nopi | nopi | bge r4, r0, __ccom_arith_18_end | nopi }                      # 0557
  { nopi | nopi | nopi | nopi }                                                 # 0558
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0559
  { nopi | nopi | nopi | nopi }                                                 # 0560
__ccom_arith_18_end:                                                            # bundle 561
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0561
  { nopi | nopi | nopi | nopi }                                                 # 0562
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 0563
  { nopi | nopi | nopi | nopi }                                                 # 0564
  { nopi | nopi | beq t2, rz, .L_if_next_5 | nopi }                             # 0565
  { nopi | nopi | nopi | nopi }                                                 # 0566
  { nopi | nopi | nopi | nopi }                                                 # 0567
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0568  linea 38
  { nopi | nopi | nopi | nopi }                                                 # 0569
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0570
  { nopi | nopi | nopi | nopi }                                                 # 0571
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0572
  { nopi | nopi | nopi | nopi }                                                 # 0573
  { nopi | nopi | nopi | lw t0, 224(rz) }                                       # 0574
  { nopi | nopi | nopi | nopi }                                                 # 0575
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0576
  { nopi | nopi | nopi | nopi }                                                 # 0577
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0578
  { nopi | nopi | nopi | nopi }                                                 # 0579
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0580
  { nopi | nopi | nopi | nopi }                                                 # 0581
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0582
  { nopi | nopi | nopi | nopi }                                                 # 0583
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0584
  { nopi | nopi | nopi | nopi }                                                 # 0585
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0586
  { nopi | nopi | nopi | nopi }                                                 # 0587
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0588
  { nopi | nopi | nopi | nopi }                                                 # 0589
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0590
  { nopi | nopi | nopi | nopi }                                                 # 0591
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0592
  { nopi | nopi | nopi | nopi }                                                 # 0593
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0594
  { nopi | nopi | nopi | nopi }                                                 # 0595
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0596
  { nopi | nopi | nopi | nopi }                                                 # 0597
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0598
  { nopi | nopi | nopi | nopi }                                                 # 0599
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0600
  { nopi | nopi | nopi | nopi }                                                 # 0601
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0602
  { nopi | nopi | nopi | nopi }                                                 # 0603
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0604
  { nopi | nopi | nopi | nopi }                                                 # 0605
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0606
  { nopi | nopi | nopi | nopi }                                                 # 0607
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 0608
  { nopi | nopi | nopi | nopi }                                                 # 0609
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0610
  { nopi | nopi | nopi | nopi }                                                 # 0611
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0612
  { nopi | nopi | nopi | nopi }                                                 # 0613
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0614
  { nopi | nopi | nopi | nopi }                                                 # 0615
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0616
  { nopi | nopi | nopi | nopi }                                                 # 0617
  { nopi | nopi | nopi | nopi }                                                 # 0618
  { nopi | nopi | nopi | nopi }                                                 # 0619
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0620
  { nopi | nopi | nopi | nopi }                                                 # 0621
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0622
  { nopi | nopi | nopi | nopi }                                                 # 0623
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0624
  { nopi | nopi | nopi | nopi }                                                 # 0625
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0626
  { nopi | nopi | nopi | nopi }                                                 # 0627
  { nopi | nopi | nopi | lw t0, 224(rz) }                                       # 0628
  { nopi | nopi | nopi | nopi }                                                 # 0629
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0630
  { nopi | nopi | nopi | nopi }                                                 # 0631
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0632
  { nopi | nopi | nopi | nopi }                                                 # 0633
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0634
  { nopi | nopi | nopi | nopi }                                                 # 0635
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0636
  { nopi | nopi | nopi | nopi }                                                 # 0637
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0638
  { nopi | nopi | nopi | nopi }                                                 # 0639
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0640
  { nopi | nopi | nopi | nopi }                                                 # 0641
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0642
  { nopi | nopi | nopi | nopi }                                                 # 0643
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0644
  { nopi | nopi | nopi | nopi }                                                 # 0645
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0646
  { nopi | nopi | nopi | nopi }                                                 # 0647
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0648
  { nopi | nopi | nopi | nopi }                                                 # 0649
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0650
  { nopi | nopi | nopi | nopi }                                                 # 0651
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0652
  { nopi | nopi | nopi | nopi }                                                 # 0653
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0654
  { nopi | nopi | nopi | nopi }                                                 # 0655
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0656
  { nopi | nopi | nopi | nopi }                                                 # 0657
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0658
  { nopi | nopi | nopi | nopi }                                                 # 0659
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0660
  { nopi | nopi | nopi | nopi }                                                 # 0661
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 0662
  { nopi | nopi | nopi | nopi }                                                 # 0663
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0664
  { nopi | nopi | nopi | nopi }                                                 # 0665
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0666
  { nopi | nopi | nopi | nopi }                                                 # 0667
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0668
  { nopi | nopi | nopi | nopi }                                                 # 0669
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0670
  { nopi | nopi | nopi | nopi }                                                 # 0671
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0672
  { nopi | nopi | nopi | nopi }                                                 # 0673
  { nopi | nopi | nopi | nopi }                                                 # 0674
  { nopi | nopi | nopi | nopi }                                                 # 0675
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0676
  { nopi | nopi | nopi | nopi }                                                 # 0677
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0678
  { nopi | nopi | nopi | nopi }                                                 # 0679
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0680
  { nopi | nopi | nopi | nopi }                                                 # 0681
  { saddi t5, t0, 0 | nopi | nopi | nopi }                                      # 0682
  { nopi | nopi | nopi | nopi }                                                 # 0683
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0684
  { nopi | nopi | nopi | nopi }                                                 # 0685
  { nopi | nopi | nopi | lw t3, 244(rz) }                                       # 0686
  { nopi | nopi | nopi | nopi }                                                 # 0687
  { nopi | nopi | nopi | nopi }                                                 # 0688
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0689
  { nopi | nopi | nopi | nopi }                                                 # 0690
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_1 | nopi }                    # 0691
.L_crypto_cifra_1:                                                              # bundle 692
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0692
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0693
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0694
  { nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }                                # 0695
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0696
  { nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }                                # 0697
  { nopi | nopi | nopi | nopi }                                                 # 0698
  { nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }                                # 0699
  { nopi | nopi | nopi | nopi }                                                 # 0700
  { nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }                                # 0701
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0702
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_1 | sw t1, 4(t3) }     # 0703
.L_crypto_fin_cifra_1:                                                          # bundle 704
  { nopi | nopi | nopi | nopi }                                                 # 0704
  { nopi | nopi | jef .L_if_end_6 | nopi }                                      # 0705  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0706
.L_if_next_5:                                                                   # bundle 707
  { nopi | nopi | nopi | nopi }                                                 # 0707
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0708  linea 40
  { nopi | nopi | nopi | nopi }                                                 # 0709
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0710
  { nopi | nopi | nopi | nopi }                                                 # 0711
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0712
  { nopi | nopi | nopi | nopi }                                                 # 0713
  { nopi | nopi | nopi | lw t0, 224(rz) }                                       # 0714
  { nopi | nopi | nopi | nopi }                                                 # 0715
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0716
  { nopi | nopi | nopi | nopi }                                                 # 0717
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0718
  { nopi | nopi | nopi | nopi }                                                 # 0719
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0720
  { nopi | nopi | nopi | nopi }                                                 # 0721
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0722
  { nopi | nopi | nopi | nopi }                                                 # 0723
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0724
  { nopi | nopi | nopi | nopi }                                                 # 0725
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0726
  { nopi | nopi | nopi | nopi }                                                 # 0727
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0728
  { nopi | nopi | nopi | nopi }                                                 # 0729
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0730
  { nopi | nopi | nopi | nopi }                                                 # 0731
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0732
  { nopi | nopi | nopi | nopi }                                                 # 0733
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0734
  { nopi | nopi | nopi | nopi }                                                 # 0735
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0736
  { nopi | nopi | nopi | nopi }                                                 # 0737
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0738
  { nopi | nopi | nopi | nopi }                                                 # 0739
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0740
  { nopi | nopi | nopi | nopi }                                                 # 0741
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0742
  { nopi | nopi | nopi | nopi }                                                 # 0743
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0744
  { nopi | nopi | nopi | nopi }                                                 # 0745
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0746
  { nopi | nopi | nopi | nopi }                                                 # 0747
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 0748
  { nopi | nopi | nopi | nopi }                                                 # 0749
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0750
  { nopi | nopi | nopi | nopi }                                                 # 0751
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0752
  { nopi | nopi | nopi | nopi }                                                 # 0753
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0754
  { nopi | nopi | nopi | nopi }                                                 # 0755
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0756
  { nopi | nopi | nopi | nopi }                                                 # 0757
  { nopi | nopi | nopi | nopi }                                                 # 0758
  { nopi | nopi | nopi | nopi }                                                 # 0759
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0760
  { nopi | nopi | nopi | nopi }                                                 # 0761
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0762
  { nopi | nopi | nopi | nopi }                                                 # 0763
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0764
  { nopi | nopi | nopi | nopi }                                                 # 0765
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0766
  { nopi | nopi | nopi | nopi }                                                 # 0767
  { nopi | nopi | nopi | lw t0, 224(rz) }                                       # 0768
  { nopi | nopi | nopi | nopi }                                                 # 0769
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0770
  { nopi | nopi | nopi | nopi }                                                 # 0771
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0772
  { nopi | nopi | nopi | nopi }                                                 # 0773
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0774
  { nopi | nopi | nopi | nopi }                                                 # 0775
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0776
  { nopi | nopi | nopi | nopi }                                                 # 0777
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0778
  { nopi | nopi | nopi | nopi }                                                 # 0779
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0780
  { nopi | nopi | nopi | nopi }                                                 # 0781
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0782
  { nopi | nopi | nopi | nopi }                                                 # 0783
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0784
  { nopi | nopi | nopi | nopi }                                                 # 0785
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0786
  { nopi | nopi | nopi | nopi }                                                 # 0787
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0788
  { nopi | nopi | nopi | nopi }                                                 # 0789
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0790
  { nopi | nopi | nopi | nopi }                                                 # 0791
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0792
  { nopi | nopi | nopi | nopi }                                                 # 0793
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0794
  { nopi | nopi | nopi | nopi }                                                 # 0795
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0796
  { nopi | nopi | nopi | nopi }                                                 # 0797
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0798
  { nopi | nopi | nopi | nopi }                                                 # 0799
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0800
  { nopi | nopi | nopi | nopi }                                                 # 0801
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 0802
  { nopi | nopi | nopi | nopi }                                                 # 0803
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0804
  { nopi | nopi | nopi | nopi }                                                 # 0805
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0806
  { nopi | nopi | nopi | nopi }                                                 # 0807
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0808
  { nopi | nopi | nopi | nopi }                                                 # 0809
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0810
  { nopi | nopi | nopi | nopi }                                                 # 0811
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0812
  { nopi | nopi | nopi | nopi }                                                 # 0813
  { nopi | nopi | nopi | nopi }                                                 # 0814
  { nopi | nopi | nopi | nopi }                                                 # 0815
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0816
  { nopi | nopi | nopi | nopi }                                                 # 0817
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0818
  { nopi | nopi | nopi | nopi }                                                 # 0819
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0820
  { nopi | nopi | nopi | nopi }                                                 # 0821
  { saddi t5, t0, 0 | nopi | nopi | nopi }                                      # 0822
  { nopi | nopi | nopi | nopi }                                                 # 0823
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0824
  { nopi | nopi | nopi | nopi }                                                 # 0825
  { nopi | nopi | nopi | lw t3, 244(rz) }                                       # 0826
  { nopi | nopi | nopi | nopi }                                                 # 0827
  { nopi | nopi | nopi | nopi }                                                 # 0828
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0829
  { nopi | nopi | nopi | nopi }                                                 # 0830
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_2 | nopi }                    # 0831
.L_crypto_cifra_2:                                                              # bundle 832
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0832
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0833
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0834
  { nopi | feis t0, t0, t1, 1, 0 | nopi | nopi }                                # 0835
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0836
  { nopi | feis t1, t1, t0, 1, 1 | nopi | nopi }                                # 0837
  { nopi | nopi | nopi | nopi }                                                 # 0838
  { nopi | feis t0, t0, t1, 1, 2 | nopi | nopi }                                # 0839
  { nopi | nopi | nopi | nopi }                                                 # 0840
  { nopi | feis t1, t1, t0, 1, 3 | nopi | nopi }                                # 0841
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0842
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_2 | sw t1, 4(t3) }     # 0843
.L_crypto_fin_cifra_2:                                                          # bundle 844
  { nopi | nopi | nopi | nopi }                                                 # 0844
.L_if_end_6:                                                                    # bundle 845
.L_for_update_3:                                                                # bundle 845
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0845  linea 35
  { nopi | nopi | nopi | nopi }                                                 # 0846
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 0847
  { nopi | nopi | nopi | nopi }                                                 # 0848
  { nopi | nopi | nopi | sw t0, 228(rz) }                                       # 0849
  { nopi | nopi | nopi | nopi }                                                 # 0850
  { nopi | nopi | jef .L_for_start_2 | nopi }                                   # 0851
  { nopi | nopi | nopi | nopi }                                                 # 0852
.L_for_end_4:                                                                   # bundle 853
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0853  linea 43
  { nopi | nopi | nopi | nopi }                                                 # 0854
.L_func_descifrar_archivo:                                                      # bundle 855
  { nopi | nopi | nopi | sw rz, 292(rz) }                                       # 0855  linea 46
  { nopi | nopi | nopi | nopi }                                                 # 0856
.L_body_descifrar_archivo:                                                      # bundle 857
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0857  linea 47
  { nopi | nopi | nopi | nopi }                                                 # 0858
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0859
  { nopi | nopi | nopi | nopi }                                                 # 0860
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0861
  { nopi | nopi | nopi | nopi }                                                 # 0862
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0863
  { nopi | nopi | nopi | nopi }                                                 # 0864
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0865
  { nopi | nopi | nopi | nopi }                                                 # 0866
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0867
  { nopi | nopi | nopi | nopi }                                                 # 0868
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0869
  { nopi | nopi | nopi | nopi }                                                 # 0870
  { add r7, r4, r0 | nopi | nopi | nopi }                                       # 0871
  { nopi | nopi | nopi | nopi }                                                 # 0872
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0873
  { nopi | nopi | nopi | nopi }                                                 # 0874
  { nopi | nopi | bge r7, r0, __ccom_arith_19_apos | nopi }                     # 0875
  { nopi | nopi | nopi | nopi }                                                 # 0876
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0877
  { nopi | nopi | nopi | nopi }                                                 # 0878
__ccom_arith_19_apos:                                                           # bundle 879
  { nopi | nopi | bge r8, r0, __ccom_arith_20_bpos | nopi }                     # 0879
  { nopi | nopi | nopi | nopi }                                                 # 0880
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0881
  { nopi | nopi | nopi | nopi }                                                 # 0882
__ccom_arith_20_bpos:                                                           # bundle 883
  { mov r11, 1 | nopi | nopi | nopi }                                           # 0883
  { nopi | nopi | nopi | nopi }                                                 # 0884
  { mov r10, 31 | nopi | nopi | nopi }                                          # 0885
  { nopi | nopi | nopi | nopi }                                                 # 0886
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0887
  { nopi | nopi | nopi | nopi }                                                 # 0888
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0889
  { nopi | nopi | nopi | nopi }                                                 # 0890
  { mov r9, 0 | nopi | nopi | nopi }                                            # 0891
  { nopi | nopi | nopi | nopi }                                                 # 0892
  { mov r6, 0 | nopi | nopi | nopi }                                            # 0893
  { nopi | nopi | nopi | nopi }                                                 # 0894
__ccom_arith_21_div:                                                            # bundle 895
  { slt r12, r7, r0 | nopi | nopi | nopi }                                      # 0895
  { nopi | nopi | nopi | nopi }                                                 # 0896
  { sll r7, r7, r11 | nopi | nopi | nopi }                                      # 0897
  { nopi | nopi | nopi | nopi }                                                 # 0898
  { sll r9, r9, r11 | nopi | nopi | nopi }                                      # 0899
  { nopi | nopi | nopi | nopi }                                                 # 0900
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0901
  { nopi | nopi | nopi | nopi }                                                 # 0902
  { sll r6, r6, r11 | nopi | nopi | nopi }                                      # 0903
  { nopi | nopi | nopi | nopi }                                                 # 0904
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0905
  { nopi | nopi | nopi | nopi }                                                 # 0906
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0907
  { nopi | nopi | nopi | nopi }                                                 # 0908
  { nopi | nopi | blt r13, r12, __ccom_arith_22_skip | nopi }                   # 0909
  { nopi | nopi | nopi | nopi }                                                 # 0910
  { sub r9, r9, r8 | nopi | nopi | nopi }                                       # 0911
  { nopi | nopi | nopi | nopi }                                                 # 0912
  { saddi r6, r6, 1 | nopi | nopi | nopi }                                      # 0913
  { nopi | nopi | nopi | nopi }                                                 # 0914
__ccom_arith_22_skip:                                                           # bundle 915
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0915
  { nopi | nopi | nopi | nopi }                                                 # 0916
  { nopi | nopi | bne r10, r0, __ccom_arith_21_div | nopi }                     # 0917
  { nopi | nopi | nopi | nopi }                                                 # 0918
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0919
  { nopi | nopi | nopi | nopi }                                                 # 0920
  { nopi | nopi | bge r13, r0, __ccom_arith_23_end | nopi }                     # 0921
  { nopi | nopi | nopi | nopi }                                                 # 0922
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0923
  { nopi | nopi | nopi | nopi }                                                 # 0924
__ccom_arith_23_end:                                                            # bundle 925
  { nopi | nopi | nopi | sw t2, 292(rz) }                                       # 0925
  { nopi | nopi | nopi | nopi }                                                 # 0926
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0927  linea 48
  { nopi | nopi | nopi | nopi }                                                 # 0928
  { nopi | nopi | nopi | lw t1, 292(rz) }                                       # 0929
  { nopi | nopi | nopi | nopi }                                                 # 0930
  { nopi | nopi | nopi | sw t0, 312(rz) }                                       # 0931
  { nopi | nopi | nopi | nopi }                                                 # 0932
  { nopi | nopi | nopi | sw t1, 316(rz) }                                       # 0933
  { nopi | nopi | nopi | nopi }                                                 # 0934
.L_for_start_7:                                                                 # bundle 935
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0935
  { nopi | nopi | nopi | nopi }                                                 # 0936
  { nopi | nopi | nopi | lw t1, 316(rz) }                                       # 0937
  { nopi | nopi | nopi | nopi }                                                 # 0938
  { nopi | nopi | bge t0, t1, .L_for_end_9 | nopi }                             # 0939
  { nopi | nopi | nopi | nopi }                                                 # 0940
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0941  linea 49
  { nopi | nopi | nopi | nopi }                                                 # 0942
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0943
  { nopi | nopi | nopi | nopi }                                                 # 0944
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0945
  { nopi | nopi | nopi | nopi }                                                 # 0946
  { nopi | nopi | nopi | lw t0, 308(rz) }                                       # 0947
  { nopi | nopi | nopi | nopi }                                                 # 0948
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0949
  { nopi | nopi | nopi | nopi }                                                 # 0950
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0951
  { nopi | nopi | nopi | nopi }                                                 # 0952
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0953
  { nopi | nopi | nopi | nopi }                                                 # 0954
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0955
  { nopi | nopi | nopi | nopi }                                                 # 0956
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0957
  { nopi | nopi | nopi | nopi }                                                 # 0958
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0959
  { nopi | nopi | nopi | nopi }                                                 # 0960
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0961
  { nopi | nopi | nopi | nopi }                                                 # 0962
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0963
  { nopi | nopi | nopi | nopi }                                                 # 0964
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0965
  { nopi | nopi | nopi | nopi }                                                 # 0966
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0967
  { nopi | nopi | nopi | nopi }                                                 # 0968
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0969
  { nopi | nopi | nopi | nopi }                                                 # 0970
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0971
  { nopi | nopi | nopi | nopi }                                                 # 0972
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0973
  { nopi | nopi | nopi | nopi }                                                 # 0974
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0975
  { nopi | nopi | nopi | nopi }                                                 # 0976
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0977
  { nopi | nopi | nopi | nopi }                                                 # 0978
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0979
  { nopi | nopi | nopi | nopi }                                                 # 0980
  { nopi | nopi | jas rl, .L_func_bloques_de | nopi }                           # 0981
  { nopi | nopi | nopi | nopi }                                                 # 0982
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0983
  { nopi | nopi | nopi | nopi }                                                 # 0984
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0985
  { nopi | nopi | nopi | nopi }                                                 # 0986
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0987
  { nopi | nopi | nopi | nopi }                                                 # 0988
  { nopi | nopi | nopi | sw t0, 320(rz) }                                       # 0989
  { nopi | nopi | nopi | nopi }                                                 # 0990
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0991  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 0992
  { mov t1, 2 | nopi | nopi | nopi }                                            # 0993
  { nopi | nopi | nopi | nopi }                                                 # 0994
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0995
  { nopi | nopi | nopi | nopi }                                                 # 0996
  { add r7, r4, r0 | nopi | nopi | nopi }                                       # 0997
  { nopi | nopi | nopi | nopi }                                                 # 0998
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0999
  { nopi | nopi | nopi | nopi }                                                 # 1000
  { nopi | nopi | bge r7, r0, __ccom_arith_24_apos | nopi }                     # 1001
  { nopi | nopi | nopi | nopi }                                                 # 1002
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 1003
  { nopi | nopi | nopi | nopi }                                                 # 1004
__ccom_arith_24_apos:                                                           # bundle 1005
  { nopi | nopi | bge r8, r0, __ccom_arith_25_bpos | nopi }                     # 1005
  { nopi | nopi | nopi | nopi }                                                 # 1006
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 1007
  { nopi | nopi | nopi | nopi }                                                 # 1008
__ccom_arith_25_bpos:                                                           # bundle 1009
  { mov r11, 1 | nopi | nopi | nopi }                                           # 1009
  { nopi | nopi | nopi | nopi }                                                 # 1010
  { mov r10, 31 | nopi | nopi | nopi }                                          # 1011
  { nopi | nopi | nopi | nopi }                                                 # 1012
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 1013
  { nopi | nopi | nopi | nopi }                                                 # 1014
  { mov r10, 32 | nopi | nopi | nopi }                                          # 1015
  { nopi | nopi | nopi | nopi }                                                 # 1016
  { mov r9, 0 | nopi | nopi | nopi }                                            # 1017
  { nopi | nopi | nopi | nopi }                                                 # 1018
  { mov r6, 0 | nopi | nopi | nopi }                                            # 1019
  { nopi | nopi | nopi | nopi }                                                 # 1020
__ccom_arith_26_div:                                                            # bundle 1021
  { slt r12, r7, r0 | nopi | nopi | nopi }                                      # 1021
  { nopi | nopi | nopi | nopi }                                                 # 1022
  { sll r7, r7, r11 | nopi | nopi | nopi }                                      # 1023
  { nopi | nopi | nopi | nopi }                                                 # 1024
  { sll r9, r9, r11 | nopi | nopi | nopi }                                      # 1025
  { nopi | nopi | nopi | nopi }                                                 # 1026
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 1027
  { nopi | nopi | nopi | nopi }                                                 # 1028
  { sll r6, r6, r11 | nopi | nopi | nopi }                                      # 1029
  { nopi | nopi | nopi | nopi }                                                 # 1030
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 1031
  { nopi | nopi | nopi | nopi }                                                 # 1032
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 1033
  { nopi | nopi | nopi | nopi }                                                 # 1034
  { nopi | nopi | blt r13, r12, __ccom_arith_27_skip | nopi }                   # 1035
  { nopi | nopi | nopi | nopi }                                                 # 1036
  { sub r9, r9, r8 | nopi | nopi | nopi }                                       # 1037
  { nopi | nopi | nopi | nopi }                                                 # 1038
  { saddi r6, r6, 1 | nopi | nopi | nopi }                                      # 1039
  { nopi | nopi | nopi | nopi }                                                 # 1040
__ccom_arith_27_skip:                                                           # bundle 1041
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 1041
  { nopi | nopi | nopi | nopi }                                                 # 1042
  { nopi | nopi | bne r10, r0, __ccom_arith_26_div | nopi }                     # 1043
  { nopi | nopi | nopi | nopi }                                                 # 1044
  { add r6, r9, r0 | nopi | nopi | nopi }                                       # 1045
  { nopi | nopi | nopi | nopi }                                                 # 1046
  { nopi | nopi | bge r4, r0, __ccom_arith_28_end | nopi }                      # 1047
  { nopi | nopi | nopi | nopi }                                                 # 1048
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 1049
  { nopi | nopi | nopi | nopi }                                                 # 1050
__ccom_arith_28_end:                                                            # bundle 1051
  { mov t0, 0 | nopi | nopi | nopi }                                            # 1051
  { nopi | nopi | nopi | nopi }                                                 # 1052
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 1053
  { nopi | nopi | nopi | nopi }                                                 # 1054
  { nopi | nopi | beq t2, rz, .L_if_next_10 | nopi }                            # 1055
  { nopi | nopi | nopi | nopi }                                                 # 1056
  { nopi | nopi | nopi | nopi }                                                 # 1057
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 1058  linea 51
  { nopi | nopi | nopi | nopi }                                                 # 1059
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1060
  { nopi | nopi | nopi | nopi }                                                 # 1061
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1062
  { nopi | nopi | nopi | nopi }                                                 # 1063
  { nopi | nopi | nopi | lw t0, 308(rz) }                                       # 1064
  { nopi | nopi | nopi | nopi }                                                 # 1065
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1066
  { nopi | nopi | nopi | nopi }                                                 # 1067
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1068
  { nopi | nopi | nopi | nopi }                                                 # 1069
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 1070
  { nopi | nopi | nopi | nopi }                                                 # 1071
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1072
  { nopi | nopi | nopi | nopi }                                                 # 1073
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1074
  { nopi | nopi | nopi | nopi }                                                 # 1075
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1076
  { nopi | nopi | nopi | nopi }                                                 # 1077
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1078
  { nopi | nopi | nopi | nopi }                                                 # 1079
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 1080
  { nopi | nopi | nopi | nopi }                                                 # 1081
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1082
  { nopi | nopi | nopi | nopi }                                                 # 1083
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1084
  { nopi | nopi | nopi | nopi }                                                 # 1085
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 1086
  { nopi | nopi | nopi | nopi }                                                 # 1087
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1088
  { nopi | nopi | nopi | nopi }                                                 # 1089
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1090
  { nopi | nopi | nopi | nopi }                                                 # 1091
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 1092
  { nopi | nopi | nopi | nopi }                                                 # 1093
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 1094
  { nopi | nopi | nopi | nopi }                                                 # 1095
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1096
  { nopi | nopi | nopi | nopi }                                                 # 1097
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 1098
  { nopi | nopi | nopi | nopi }                                                 # 1099
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1100
  { nopi | nopi | nopi | nopi }                                                 # 1101
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 1102
  { nopi | nopi | nopi | nopi }                                                 # 1103
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 1104
  { nopi | nopi | nopi | nopi }                                                 # 1105
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 1106
  { nopi | nopi | nopi | nopi }                                                 # 1107
  { nopi | nopi | nopi | nopi }                                                 # 1108
  { nopi | nopi | nopi | nopi }                                                 # 1109
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 1110
  { nopi | nopi | nopi | nopi }                                                 # 1111
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 1112
  { nopi | nopi | nopi | nopi }                                                 # 1113
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1114
  { nopi | nopi | nopi | nopi }                                                 # 1115
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1116
  { nopi | nopi | nopi | nopi }                                                 # 1117
  { nopi | nopi | nopi | lw t0, 308(rz) }                                       # 1118
  { nopi | nopi | nopi | nopi }                                                 # 1119
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1120
  { nopi | nopi | nopi | nopi }                                                 # 1121
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1122
  { nopi | nopi | nopi | nopi }                                                 # 1123
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 1124
  { nopi | nopi | nopi | nopi }                                                 # 1125
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1126
  { nopi | nopi | nopi | nopi }                                                 # 1127
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1128
  { nopi | nopi | nopi | nopi }                                                 # 1129
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1130
  { nopi | nopi | nopi | nopi }                                                 # 1131
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1132
  { nopi | nopi | nopi | nopi }                                                 # 1133
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 1134
  { nopi | nopi | nopi | nopi }                                                 # 1135
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1136
  { nopi | nopi | nopi | nopi }                                                 # 1137
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1138
  { nopi | nopi | nopi | nopi }                                                 # 1139
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 1140
  { nopi | nopi | nopi | nopi }                                                 # 1141
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1142
  { nopi | nopi | nopi | nopi }                                                 # 1143
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1144
  { nopi | nopi | nopi | nopi }                                                 # 1145
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 1146
  { nopi | nopi | nopi | nopi }                                                 # 1147
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 1148
  { nopi | nopi | nopi | nopi }                                                 # 1149
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1150
  { nopi | nopi | nopi | nopi }                                                 # 1151
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 1152
  { nopi | nopi | nopi | nopi }                                                 # 1153
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1154
  { nopi | nopi | nopi | nopi }                                                 # 1155
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 1156
  { nopi | nopi | nopi | nopi }                                                 # 1157
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 1158
  { nopi | nopi | nopi | nopi }                                                 # 1159
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 1160
  { nopi | nopi | nopi | nopi }                                                 # 1161
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 1162
  { nopi | nopi | nopi | nopi }                                                 # 1163
  { nopi | nopi | nopi | nopi }                                                 # 1164
  { nopi | nopi | nopi | nopi }                                                 # 1165
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 1166
  { nopi | nopi | nopi | nopi }                                                 # 1167
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 1168
  { nopi | nopi | nopi | nopi }                                                 # 1169
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 1170
  { nopi | nopi | nopi | nopi }                                                 # 1171
  { saddi t5, t0, 0 | nopi | nopi | nopi }                                      # 1172
  { nopi | nopi | nopi | nopi }                                                 # 1173
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 1174
  { nopi | nopi | nopi | nopi }                                                 # 1175
  { nopi | nopi | nopi | lw t3, 328(rz) }                                       # 1176
  { nopi | nopi | nopi | nopi }                                                 # 1177
  { nopi | nopi | nopi | nopi }                                                 # 1178
  { mov t4, 0 | nopi | nopi | nopi }                                            # 1179
  { nopi | nopi | nopi | nopi }                                                 # 1180
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_3 | nopi }                 # 1181
.L_crypto_descifra_3:                                                           # bundle 1182
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 1182
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 1183
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 1184
  { nopi | feis t0, t0, t1, 0, 3 | nopi | nopi }                                # 1185
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 1186
  { nopi | feis t1, t1, t0, 0, 2 | nopi | nopi }                                # 1187
  { nopi | nopi | nopi | nopi }                                                 # 1188
  { nopi | feis t0, t0, t1, 0, 1 | nopi | nopi }                                # 1189
  { nopi | nopi | nopi | nopi }                                                 # 1190
  { nopi | feis t1, t1, t0, 0, 0 | nopi | nopi }                                # 1191
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 1192
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_3 | sw t1, 0(t3) }  # 1193
.L_crypto_fin_descifra_3:                                                       # bundle 1194
  { nopi | nopi | nopi | nopi }                                                 # 1194
  { nopi | nopi | jef .L_if_end_11 | nopi }                                     # 1195  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 1196
.L_if_next_10:                                                                  # bundle 1197
  { nopi | nopi | nopi | nopi }                                                 # 1197
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 1198  linea 53
  { nopi | nopi | nopi | nopi }                                                 # 1199
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1200
  { nopi | nopi | nopi | nopi }                                                 # 1201
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1202
  { nopi | nopi | nopi | nopi }                                                 # 1203
  { nopi | nopi | nopi | lw t0, 308(rz) }                                       # 1204
  { nopi | nopi | nopi | nopi }                                                 # 1205
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1206
  { nopi | nopi | nopi | nopi }                                                 # 1207
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1208
  { nopi | nopi | nopi | nopi }                                                 # 1209
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 1210
  { nopi | nopi | nopi | nopi }                                                 # 1211
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1212
  { nopi | nopi | nopi | nopi }                                                 # 1213
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1214
  { nopi | nopi | nopi | nopi }                                                 # 1215
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1216
  { nopi | nopi | nopi | nopi }                                                 # 1217
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1218
  { nopi | nopi | nopi | nopi }                                                 # 1219
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 1220
  { nopi | nopi | nopi | nopi }                                                 # 1221
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1222
  { nopi | nopi | nopi | nopi }                                                 # 1223
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1224
  { nopi | nopi | nopi | nopi }                                                 # 1225
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 1226
  { nopi | nopi | nopi | nopi }                                                 # 1227
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1228
  { nopi | nopi | nopi | nopi }                                                 # 1229
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1230
  { nopi | nopi | nopi | nopi }                                                 # 1231
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 1232
  { nopi | nopi | nopi | nopi }                                                 # 1233
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 1234
  { nopi | nopi | nopi | nopi }                                                 # 1235
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1236
  { nopi | nopi | nopi | nopi }                                                 # 1237
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 1238
  { nopi | nopi | nopi | nopi }                                                 # 1239
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1240
  { nopi | nopi | nopi | nopi }                                                 # 1241
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 1242
  { nopi | nopi | nopi | nopi }                                                 # 1243
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 1244
  { nopi | nopi | nopi | nopi }                                                 # 1245
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 1246
  { nopi | nopi | nopi | nopi }                                                 # 1247
  { nopi | nopi | nopi | nopi }                                                 # 1248
  { nopi | nopi | nopi | nopi }                                                 # 1249
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 1250
  { nopi | nopi | nopi | nopi }                                                 # 1251
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 1252
  { nopi | nopi | nopi | nopi }                                                 # 1253
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1254
  { nopi | nopi | nopi | nopi }                                                 # 1255
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1256
  { nopi | nopi | nopi | nopi }                                                 # 1257
  { nopi | nopi | nopi | lw t0, 308(rz) }                                       # 1258
  { nopi | nopi | nopi | nopi }                                                 # 1259
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1260
  { nopi | nopi | nopi | nopi }                                                 # 1261
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1262
  { nopi | nopi | nopi | nopi }                                                 # 1263
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 1264
  { nopi | nopi | nopi | nopi }                                                 # 1265
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 1266
  { nopi | nopi | nopi | nopi }                                                 # 1267
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1268
  { nopi | nopi | nopi | nopi }                                                 # 1269
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1270
  { nopi | nopi | nopi | nopi }                                                 # 1271
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1272
  { nopi | nopi | nopi | nopi }                                                 # 1273
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 1274
  { nopi | nopi | nopi | nopi }                                                 # 1275
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1276
  { nopi | nopi | nopi | nopi }                                                 # 1277
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1278
  { nopi | nopi | nopi | nopi }                                                 # 1279
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 1280
  { nopi | nopi | nopi | nopi }                                                 # 1281
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1282
  { nopi | nopi | nopi | nopi }                                                 # 1283
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 1284
  { nopi | nopi | nopi | nopi }                                                 # 1285
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 1286
  { nopi | nopi | nopi | nopi }                                                 # 1287
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 1288
  { nopi | nopi | nopi | nopi }                                                 # 1289
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 1290
  { nopi | nopi | nopi | nopi }                                                 # 1291
  { nopi | nopi | jas rl, .L_func_direccion | nopi }                            # 1292
  { nopi | nopi | nopi | nopi }                                                 # 1293
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 1294
  { nopi | nopi | nopi | nopi }                                                 # 1295
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 1296
  { nopi | nopi | nopi | nopi }                                                 # 1297
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 1298
  { nopi | nopi | nopi | nopi }                                                 # 1299
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 1300
  { nopi | nopi | nopi | nopi }                                                 # 1301
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 1302
  { nopi | nopi | nopi | nopi }                                                 # 1303
  { nopi | nopi | nopi | nopi }                                                 # 1304
  { nopi | nopi | nopi | nopi }                                                 # 1305
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 1306
  { nopi | nopi | nopi | nopi }                                                 # 1307
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 1308
  { nopi | nopi | nopi | nopi }                                                 # 1309
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 1310
  { nopi | nopi | nopi | nopi }                                                 # 1311
  { saddi t5, t0, 0 | nopi | nopi | nopi }                                      # 1312
  { nopi | nopi | nopi | nopi }                                                 # 1313
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 1314
  { nopi | nopi | nopi | nopi }                                                 # 1315
  { nopi | nopi | nopi | lw t3, 328(rz) }                                       # 1316
  { nopi | nopi | nopi | nopi }                                                 # 1317
  { nopi | nopi | nopi | nopi }                                                 # 1318
  { mov t4, 0 | nopi | nopi | nopi }                                            # 1319
  { nopi | nopi | nopi | nopi }                                                 # 1320
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_4 | nopi }                 # 1321
.L_crypto_descifra_4:                                                           # bundle 1322
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 1322
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 1323
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 1324
  { nopi | feis t0, t0, t1, 1, 3 | nopi | nopi }                                # 1325
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 1326
  { nopi | feis t1, t1, t0, 1, 2 | nopi | nopi }                                # 1327
  { nopi | nopi | nopi | nopi }                                                 # 1328
  { nopi | feis t0, t0, t1, 1, 1 | nopi | nopi }                                # 1329
  { nopi | nopi | nopi | nopi }                                                 # 1330
  { nopi | feis t1, t1, t0, 1, 0 | nopi | nopi }                                # 1331
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 1332
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_4 | sw t1, 0(t3) }  # 1333
.L_crypto_fin_descifra_4:                                                       # bundle 1334
  { nopi | nopi | nopi | nopi }                                                 # 1334
.L_if_end_11:                                                                   # bundle 1335
.L_for_update_8:                                                                # bundle 1335
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 1335  linea 48
  { nopi | nopi | nopi | nopi }                                                 # 1336
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 1337
  { nopi | nopi | nopi | nopi }                                                 # 1338
  { nopi | nopi | nopi | sw t0, 312(rz) }                                       # 1339
  { nopi | nopi | nopi | nopi }                                                 # 1340
  { nopi | nopi | jef .L_for_start_7 | nopi }                                   # 1341
  { nopi | nopi | nopi | nopi }                                                 # 1342
.L_for_end_9:                                                                   # bundle 1343
  { nopi | nopi | jefR rz, rl | nopi }                                          # 1343  linea 56
  { nopi | nopi | nopi | nopi }                                                 # 1344
fin_main:                                                                       # bundle 1345
  { nopi | nopi | jef fin_main | nopi }                                         # 1345  linea -
.L_arith_error:                                                                 # bundle 1346
  { nopi | nopi | jef .L_arith_error | nopi }                                   # 1346
  { nopi | nopi | nopi | nopi }                                                 # 1347

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_bloques_de             158
#   .L_body_bloques_de             160
#   __ccom_arith_0_mul             174  (interna)
#   __ccom_arith_1_skip            182  (interna)
#   __ccom_arith_2_end             188  (interna)
#   .L_if_next_0                   206
#   .L_if_end_1                    210
#   .L_func_direccion              212
#   .L_body_direccion              214
#   __ccom_arith_3_mul             228  (interna)
#   __ccom_arith_4_skip            236  (interna)
#   __ccom_arith_5_end             242  (interna)
#   __ccom_arith_6_mul             252  (interna)
#   __ccom_arith_7_skip            260  (interna)
#   __ccom_arith_8_end             266  (interna)
#   .L_func_preparar_boveda        272
#   .L_body_preparar_boveda        272
#   .L_func_cifrar_archivo         365
#   .L_body_cifrar_archivo         367
#   __ccom_arith_9_apos            389  (interna)
#   __ccom_arith_10_bpos           393  (interna)
#   __ccom_arith_11_div            405  (interna)
#   __ccom_arith_12_skip           425  (interna)
#   __ccom_arith_13_end            435  (interna)
#   .L_for_start_2                 445
#   __ccom_arith_14_apos           515  (interna)
#   __ccom_arith_15_bpos           519  (interna)
#   __ccom_arith_16_div            531  (interna)
#   __ccom_arith_17_skip           551  (interna)
#   __ccom_arith_18_end            561  (interna)
#   .L_crypto_cifra_1              692  (interna)
#   .L_crypto_fin_cifra_1          704  (interna)
#   .L_if_next_5                   707
#   .L_crypto_cifra_2              832  (interna)
#   .L_crypto_fin_cifra_2          844  (interna)
#   .L_if_end_6                    845
#   .L_for_update_3                845
#   .L_for_end_4                   853
#   .L_func_descifrar_archivo      855
#   .L_body_descifrar_archivo      857
#   __ccom_arith_19_apos           879  (interna)
#   __ccom_arith_20_bpos           883  (interna)
#   __ccom_arith_21_div            895  (interna)
#   __ccom_arith_22_skip           915  (interna)
#   __ccom_arith_23_end            925  (interna)
#   .L_for_start_7                 935
#   __ccom_arith_24_apos          1005  (interna)
#   __ccom_arith_25_bpos          1009  (interna)
#   __ccom_arith_26_div           1021  (interna)
#   __ccom_arith_27_skip          1041  (interna)
#   __ccom_arith_28_end           1051  (interna)
#   .L_crypto_descifra_3          1182  (interna)
#   .L_crypto_fin_descifra_3      1194  (interna)
#   .L_if_next_10                 1197
#   .L_crypto_descifra_4          1322  (interna)
#   .L_crypto_fin_descifra_4      1334  (interna)
#   .L_if_end_11                  1335
#   .L_for_update_8               1335
#   .L_for_end_9                  1343
#   fin_main                      1345
#   .L_arith_error                1346

# Variables (direccion de datos)
#
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x0174 size=56 temp_base=0x017C
#       descifrar_archivo : int (function) params=[desde, hacia, total, tam] area=0x0120 size=84 temp_base=0x0144
#       cifrar_archivo : int (function) params=[desde, hacia, total, tam] area=0x00CC size=84 temp_base=0x00F0
#       preparar_boveda : void (function) params=[] area=0x0098 size=52 temp_base=0x009C
#       direccion : int (function) params=[base, tam, i] area=0x0054 size=68 temp_base=0x0068
#       bloques_de : int (function) params=[total, tam, i] area=0x000C size=72 temp_base=0x0024
#       vuelta : int width=4 addr=0x0008 line=3
#       cifrado : int width=4 addr=0x0004 line=2
#       origen : int width=4 addr=0x0000 line=1
#
#   Scope 1: FUNCTION bloques_de
#       resto : int width=4 addr=0x0020 line=6
#       i : int (parameter) width=4 addr=0x001C line=5
#       tam : int (parameter) width=4 addr=0x0018 line=5
#       total : int (parameter) width=4 addr=0x0014 line=5
#       b : int (return) width=4 addr=0x0010 line=5
#
#   Scope 2: C bloques_de
#
#   Scope 3: RESIDUAL bloques_de
#
#   Scope 4: FUNCTION direccion
#       i : int (parameter) width=4 addr=0x0064 line=15
#       tam : int (parameter) width=4 addr=0x0060 line=15
#       base : int (parameter) width=4 addr=0x005C line=15
#       d : int (return) width=4 addr=0x0058 line=15
#
#   Scope 5: FUNCTION preparar_boveda
#
#   Scope 6: FUNCTION cifrar_archivo
#       tam : int (parameter) width=4 addr=0x00E0 line=33
#       total : int (parameter) width=4 addr=0x00DC line=33
#       hacia : int (parameter) width=4 addr=0x00D8 line=33
#       desde : int (parameter) width=4 addr=0x00D4 line=33
#       n : int (return) width=4 addr=0x00D0 line=33
#
#   Scope 7: S cifrar_archivo
#       b : int width=4 addr=0x00EC line=36
#       i : int width=4 addr=0x00E4 line=35
#
#   Scope 8: C cifrar_archivo
#
#   Scope 9: RESIDUAL cifrar_archivo
#
#   Scope 10: FUNCTION descifrar_archivo
#       tam : int (parameter) width=4 addr=0x0134 line=46
#       total : int (parameter) width=4 addr=0x0130 line=46
#       hacia : int (parameter) width=4 addr=0x012C line=46
#       desde : int (parameter) width=4 addr=0x0128 line=46
#       n : int (return) width=4 addr=0x0124 line=46
#
#   Scope 11: S descifrar_archivo
#       b : int width=4 addr=0x0140 line=49
#       i : int width=4 addr=0x0138 line=48
#
#   Scope 12: C descifrar_archivo
#
#   Scope 13: RESIDUAL descifrar_archivo
#
#   Scope 14: FUNCTION main
#       n2 : int width=4 addr=0x0178 line=61
#       n1 : int width=4 addr=0x0174 line=60
#
#   =======================
