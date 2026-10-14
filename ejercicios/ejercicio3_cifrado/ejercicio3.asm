# ccom: ejercicios/ejercicio3_cifrado/ejercicio3.cls
  { mov rx, 428 | nopi | nopi | nopi }                                          # 0000
  { nopi | nopi | nopi | nopi }                                                 # 0001
.L_func_main:                                                                   # bundle 2
.L_body_main:                                                                   # bundle 2
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0002  linea 60
  { nopi | nopi | nopi | nopi }                                                 # 0003
  { nopi | nopi | nopi | sw t0, 372(rz) }                                       # 0004
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0005  linea 61
  { nopi | nopi | nopi | nopi }                                                 # 0006
  { nopi | nopi | nopi | sw t0, 376(rz) }                                       # 0007
  { nopi | nopi | nopi | nopi }                                                 # 0008
  { nopi | nopi | nopi | nopi }                                                 # 0009
  { mov t0, 37 | mov t1, 15 | nopi | nopi }                                     # 0010  linea 62
  { nopi | nopi | nopi | nopi }                                                 # 0011
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0012
  { nopi | nopi | nopi | nopi }                                                 # 0013
  { saddi t0, t0, 22151 | nopi | nopi | nopi }                                  # 0014
  { nopi | nopi | nopi | nopi }                                                 # 0015
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0016
  { nopi | nopi | nopi | nopi }                                                 # 0017
  { nopi | nopi | nopi | nopi }                                                 # 0018
  { nopi | auth t6 | nopi | nopi }                                              # 0019
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0020  linea 63
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_preparar_boveda | nopi }           # 0021
  { nopi | nopi | nopi | nopi }                                                 # 0022
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0023
  { nopi | nopi | nopi | nopi }                                                 # 0024
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0025
  { nopi | nopi | nopi | lw t0, 0(rz) }                                         # 0026  linea 64
  { nopi | nopi | nopi | nopi }                                                 # 0027
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0028
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 4(rz) }                              # 0029
  { nopi | nopi | nopi | nopi }                                                 # 0030
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0031
  { saddi rx, rx, 4 | mov t0, 128 | nopi | nopi }                               # 0032
  { nopi | nopi | nopi | nopi }                                                 # 0033
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0034
  { saddi rx, rx, 4 | mov t0, 16 | nopi | nopi }                                # 0035
  { nopi | nopi | nopi | nopi }                                                 # 0036
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0037
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0038
  { nopi | nopi | nopi | nopi }                                                 # 0039
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0040
  { nopi | nopi | nopi | nopi }                                                 # 0041
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0042
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0043
  { nopi | nopi | nopi | sw t0, 224(rz) }                                       # 0044
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0045
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0046
  { nopi | nopi | nopi | sw t0, 220(rz) }                                       # 0047
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0048
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0049
  { nopi | nopi | nopi | sw t0, 216(rz) }                                       # 0050
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0051
  { nopi | nopi | nopi | nopi }                                                 # 0052
  { nopi | nopi | nopi | sw t0, 212(rz) }                                       # 0053
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0054
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_cifrar_archivo | nopi }            # 0055
  { nopi | nopi | nopi | nopi }                                                 # 0056
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0057
  { nopi | nopi | nopi | nopi }                                                 # 0058
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0059
  { nopi | nopi | nopi | lw t0, 208(rz) }                                       # 0060
  { nopi | nopi | nopi | nopi }                                                 # 0061
  { nopi | nopi | nopi | sw t0, 372(rz) }                                       # 0062
  { nopi | nopi | nopi | lw t0, 4(rz) }                                         # 0063  linea 65
  { nopi | nopi | nopi | nopi }                                                 # 0064
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0065
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 8(rz) }                              # 0066
  { nopi | nopi | nopi | nopi }                                                 # 0067
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0068
  { saddi rx, rx, 4 | mov t0, 128 | nopi | nopi }                               # 0069
  { nopi | nopi | nopi | nopi }                                                 # 0070
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0071
  { saddi rx, rx, 4 | mov t0, 16 | nopi | nopi }                                # 0072
  { nopi | nopi | nopi | nopi }                                                 # 0073
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0074
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0075
  { nopi | nopi | nopi | nopi }                                                 # 0076
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0077
  { nopi | nopi | nopi | nopi }                                                 # 0078
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0079
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0080
  { nopi | nopi | nopi | sw t0, 308(rz) }                                       # 0081
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0082
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0083
  { nopi | nopi | nopi | sw t0, 304(rz) }                                       # 0084
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0085
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0086
  { nopi | nopi | nopi | sw t0, 300(rz) }                                       # 0087
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0088
  { nopi | nopi | nopi | nopi }                                                 # 0089
  { nopi | nopi | nopi | sw t0, 296(rz) }                                       # 0090
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0091
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_descifrar_archivo | nopi }         # 0092
  { nopi | nopi | nopi | nopi }                                                 # 0093
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0094
  { nopi | nopi | nopi | nopi }                                                 # 0095
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0096
  { nopi | nopi | nopi | lw t0, 292(rz) }                                       # 0097
  { nopi | nopi | nopi | nopi }                                                 # 0098
  { nopi | nopi | nopi | sw t0, 376(rz) }                                       # 0099
  { nopi | nopi | nopi | nopi }                                                 # 0100
  { nopi | logout | nopi | nopi }                                               # 0101  linea 66
  { nopi | nopi | jef fin_main | nopi }                                         # 0102  linea 67
  { nopi | nopi | nopi | nopi }                                                 # 0103
.L_func_bloques_de:                                                             # bundle 104
  { nopi | nopi | nopi | sw rz, 16(rz) }                                        # 0104  linea 5
  { nopi | nopi | nopi | nopi }                                                 # 0105
.L_body_bloques_de:                                                             # bundle 106
  { mov r11, 1 | mov r7, 0 | nopi | lw t0, 20(rz) }                             # 0106  linea 6
  { nopi | nopi | nopi | lw t1, 28(rz) }                                        # 0107
  { nopi | nopi | nopi | lw t2, 24(rz) }                                        # 0108
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0109
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0110
  { nopi | nopi | nopi | nopi }                                                 # 0111
__ccom_arith_0_mul:                                                             # bundle 112
  { nopi | nopi | beq r9, r0, __ccom_arith_2_end | nopi }                       # 0112
  { nopi | nopi | nopi | nopi }                                                 # 0113
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0114
  { nopi | nopi | nopi | nopi }                                                 # 0115
  { nopi | nopi | beq r10, r0, __ccom_arith_1_skip | nopi }                     # 0116
  { nopi | nopi | nopi | nopi }                                                 # 0117
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0118
  { nopi | nopi | nopi | nopi }                                                 # 0119
__ccom_arith_1_skip:                                                            # bundle 120
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_0_mul | nopi }         # 0120
  { nopi | nopi | nopi | nopi }                                                 # 0121
__ccom_arith_2_end:                                                             # bundle 122
  { sub t0, t0, t3 | nopi | nopi | nopi }                                       # 0122
  { nopi | nopi | nopi | nopi }                                                 # 0123
  { nopi | nopi | nopi | sw t0, 32(rz) }                                        # 0124
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0125  linea 7
  { nopi | nopi | nopi | lw t1, 24(rz) }                                        # 0126
  { nopi | nopi | nopi | nopi }                                                 # 0127
  { slt t0, t0, t1 | nopi | nopi | nopi }                                       # 0128
  { nopi | nopi | nopi | nopi }                                                 # 0129
  { nopi | nopi | beq t0, rz, .L_if_next_0 | nopi }                             # 0130
  { nopi | nopi | nopi | nopi }                                                 # 0131
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0132  linea 8
  { nopi | nopi | nopi | nopi }                                                 # 0133
  { nopi | nopi | jef .L_if_end_1 | sw t0, 16(rz) }                             # 0134  linea 7  slots 0=0,1=0,2=7,3=8
  { nopi | nopi | nopi | nopi }                                                 # 0135
.L_if_next_0:                                                                   # bundle 136
  { nopi | nopi | nopi | lw t0, 24(rz) }                                        # 0136  linea 10
  { nopi | nopi | nopi | nopi }                                                 # 0137
  { nopi | nopi | nopi | sw t0, 16(rz) }                                        # 0138
  { nopi | nopi | nopi | nopi }                                                 # 0139
.L_if_end_1:                                                                    # bundle 140
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0140  linea 12
  { nopi | nopi | nopi | nopi }                                                 # 0141
.L_func_direccion:                                                              # bundle 142
  { nopi | nopi | nopi | sw rz, 88(rz) }                                        # 0142  linea 15
  { nopi | nopi | nopi | nopi }                                                 # 0143
.L_body_direccion:                                                              # bundle 144
  { mov r11, 1 | mov r7, 0 | nopi | lw t0, 92(rz) }                             # 0144  linea 16
  { nopi | nopi | nopi | lw t1, 100(rz) }                                       # 0145
  { nopi | nopi | nopi | lw t2, 96(rz) }                                        # 0146
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0147
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0148
  { nopi | nopi | nopi | nopi }                                                 # 0149
__ccom_arith_3_mul:                                                             # bundle 150
  { nopi | nopi | beq r9, r0, __ccom_arith_5_end | nopi }                       # 0150
  { nopi | nopi | nopi | nopi }                                                 # 0151
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0152
  { nopi | nopi | nopi | nopi }                                                 # 0153
  { nopi | nopi | beq r10, r0, __ccom_arith_4_skip | nopi }                     # 0154
  { nopi | nopi | nopi | nopi }                                                 # 0155
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0156
  { nopi | nopi | nopi | nopi }                                                 # 0157
__ccom_arith_4_skip:                                                            # bundle 158
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_3_mul | nopi }         # 0158
  { nopi | nopi | nopi | nopi }                                                 # 0159
__ccom_arith_5_end:                                                             # bundle 160
  { mov t1, 8 | mov r11, 1 | nopi | nopi }                                      # 0160
  { add r8, r7, r0 | mov r6, 0 | nopi | nopi }                                  # 0161
  { add r9, r5, r0 | nopi | nopi | nopi }                                       # 0162
  { nopi | nopi | nopi | nopi }                                                 # 0163
__ccom_arith_6_mul:                                                             # bundle 164
  { nopi | nopi | beq r9, r0, __ccom_arith_8_end | nopi }                       # 0164
  { nopi | nopi | nopi | nopi }                                                 # 0165
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0166
  { nopi | nopi | nopi | nopi }                                                 # 0167
  { nopi | nopi | beq r10, r0, __ccom_arith_7_skip | nopi }                     # 0168
  { nopi | nopi | nopi | nopi }                                                 # 0169
  { add r6, r6, r8 | nopi | nopi | nopi }                                       # 0170
  { nopi | nopi | nopi | nopi }                                                 # 0171
__ccom_arith_7_skip:                                                            # bundle 172
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_6_mul | nopi }         # 0172
  { nopi | nopi | nopi | nopi }                                                 # 0173
__ccom_arith_8_end:                                                             # bundle 174
  { add t0, t0, t2 | nopi | nopi | nopi }                                       # 0174
  { nopi | nopi | nopi | nopi }                                                 # 0175
  { nopi | nopi | jefR rz, rl | sw t0, 88(rz) }                                 # 0176  linea 17  slots 0=0,1=0,2=17,3=16
  { nopi | nopi | nopi | nopi }                                                 # 0177
.L_func_preparar_boveda:                                                        # bundle 178
.L_body_preparar_boveda:                                                        # bundle 178
  { nopi | nopi | nopi | nopi }                                                 # 0178
  { mov t0, 583 | mov t1, 15 | nopi | nopi }                                    # 0179  linea 21
  { nopi | nopi | nopi | nopi }                                                 # 0180
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0181
  { nopi | nopi | nopi | nopi }                                                 # 0182
  { saddi t0, t0, 11213 | nopi | nopi | nopi }                                  # 0183
  { nopi | nopi | nopi | nopi }                                                 # 0184
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0185
  { nopi | nopi | nopi | nopi }                                                 # 0186
  { nopi | nopi | nopi | nopi }                                                 # 0187
  { nopi | kv_write t6, 0, 0 | nopi | nopi }                                    # 0188
  { nopi | nopi | nopi | nopi }                                                 # 0189
  { mov t0, 60584 | mov t1, 15 | nopi | nopi }                                  # 0190  linea 22
  { nopi | nopi | nopi | nopi }                                                 # 0191
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0192
  { nopi | nopi | nopi | nopi }                                                 # 0193
  { saddi t0, t0, 12817 | nopi | nopi | nopi }                                  # 0194
  { nopi | nopi | nopi | nopi }                                                 # 0195
  { sub t0, rz, t0 | nopi | nopi | nopi }                                       # 0196
  { nopi | nopi | nopi | nopi }                                                 # 0197
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0198
  { nopi | nopi | nopi | nopi }                                                 # 0199
  { nopi | nopi | nopi | nopi }                                                 # 0200
  { nopi | kv_write t6, 0, 1 | nopi | nopi }                                    # 0201
  { nopi | nopi | nopi | nopi }                                                 # 0202
  { mov t0, 9903 | mov t1, 15 | nopi | nopi }                                   # 0203  linea 23
  { nopi | nopi | nopi | nopi }                                                 # 0204
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0205
  { nopi | nopi | nopi | nopi }                                                 # 0206
  { saddi t0, t0, 7135 | nopi | nopi | nopi }                                   # 0207
  { nopi | nopi | nopi | nopi }                                                 # 0208
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0209
  { nopi | nopi | nopi | nopi }                                                 # 0210
  { nopi | nopi | nopi | nopi }                                                 # 0211
  { nopi | kv_write t6, 0, 2 | nopi | nopi }                                    # 0212
  { nopi | nopi | nopi | nopi }                                                 # 0213
  { mov t0, 18641 | mov t1, 15 | nopi | nopi }                                  # 0214  linea 24
  { nopi | nopi | nopi | nopi }                                                 # 0215
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0216
  { nopi | nopi | nopi | nopi }                                                 # 0217
  { saddi t0, t0, 11488 | nopi | nopi | nopi }                                  # 0218
  { nopi | nopi | nopi | nopi }                                                 # 0219
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0220
  { nopi | nopi | nopi | nopi }                                                 # 0221
  { nopi | nopi | nopi | nopi }                                                 # 0222
  { nopi | kv_write t6, 0, 3 | nopi | nopi }                                    # 0223
  { nopi | nopi | nopi | nopi }                                                 # 0224
  { mov t0, 8738 | mov t1, 15 | nopi | nopi }                                   # 0225  linea 25
  { nopi | nopi | nopi | nopi }                                                 # 0226
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0227
  { nopi | nopi | nopi | nopi }                                                 # 0228
  { saddi t0, t0, 4369 | nopi | nopi | nopi }                                   # 0229
  { nopi | nopi | nopi | nopi }                                                 # 0230
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0231
  { nopi | nopi | nopi | nopi }                                                 # 0232
  { nopi | nopi | nopi | nopi }                                                 # 0233
  { nopi | kv_write t6, 1, 0 | nopi | nopi }                                    # 0234
  { nopi | nopi | nopi | nopi }                                                 # 0235
  { mov t0, 17476 | mov t1, 15 | nopi | nopi }                                  # 0236  linea 26
  { nopi | nopi | nopi | nopi }                                                 # 0237
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0238
  { nopi | nopi | nopi | nopi }                                                 # 0239
  { saddi t0, t0, 8738 | nopi | nopi | nopi }                                   # 0240
  { nopi | nopi | nopi | nopi }                                                 # 0241
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0242
  { nopi | nopi | nopi | nopi }                                                 # 0243
  { nopi | nopi | nopi | nopi }                                                 # 0244
  { nopi | kv_write t6, 1, 1 | nopi | nopi }                                    # 0245
  { nopi | nopi | nopi | nopi }                                                 # 0246
  { mov t0, 26214 | mov t1, 15 | nopi | nopi }                                  # 0247  linea 27
  { nopi | nopi | nopi | nopi }                                                 # 0248
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0249
  { nopi | nopi | nopi | nopi }                                                 # 0250
  { saddi t0, t0, 13107 | nopi | nopi | nopi }                                  # 0251
  { nopi | nopi | nopi | nopi }                                                 # 0252
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0253
  { nopi | nopi | nopi | nopi }                                                 # 0254
  { nopi | nopi | nopi | nopi }                                                 # 0255
  { nopi | kv_write t6, 1, 2 | nopi | nopi }                                    # 0256
  { nopi | nopi | nopi | nopi }                                                 # 0257
  { mov t0, 34952 | mov t1, 15 | nopi | nopi }                                  # 0258  linea 28
  { nopi | nopi | nopi | nopi }                                                 # 0259
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0260
  { nopi | nopi | nopi | nopi }                                                 # 0261
  { saddi t0, t0, 17476 | nopi | nopi | nopi }                                  # 0262
  { nopi | nopi | nopi | nopi }                                                 # 0263
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0264
  { nopi | nopi | nopi | nopi }                                                 # 0265
  { nopi | nopi | nopi | nopi }                                                 # 0266
  { nopi | kv_write t6, 1, 3 | nopi | nopi }                                    # 0267
  { nopi | kv_lock | nopi | nopi }                                              # 0268  linea 29
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0269  linea 30
  { nopi | nopi | nopi | nopi }                                                 # 0270
.L_func_cifrar_archivo:                                                         # bundle 271
  { nopi | nopi | nopi | sw rz, 208(rz) }                                       # 0271  linea 33
  { nopi | nopi | nopi | nopi }                                                 # 0272
.L_body_cifrar_archivo:                                                         # bundle 273
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0273  linea 34
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0274
  { nopi | nopi | nopi | nopi }                                                 # 0275
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0276
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0277
  { nopi | nopi | nopi | nopi }                                                 # 0278
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0279
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0280
  { nopi | nopi | nopi | nopi }                                                 # 0281
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0282
  { nopi | nopi | nopi | nopi }                                                 # 0283
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0284
  { nopi | nopi | nopi | nopi }                                                 # 0285
  { nopi | nopi | bge r7, r0, __ccom_arith_9_apos | nopi }                      # 0286
  { nopi | nopi | nopi | nopi }                                                 # 0287
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0288
  { nopi | nopi | nopi | nopi }                                                 # 0289
__ccom_arith_9_apos:                                                            # bundle 290
  { nopi | nopi | bge r8, r0, __ccom_arith_10_bpos | nopi }                     # 0290
  { nopi | nopi | nopi | nopi }                                                 # 0291
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0292
  { nopi | nopi | nopi | nopi }                                                 # 0293
__ccom_arith_10_bpos:                                                           # bundle 294
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0294
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0295
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0296
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0297
  { nopi | nopi | nopi | nopi }                                                 # 0298
__ccom_arith_11_div:                                                            # bundle 299
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0299
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0300
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0301
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0302
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0303
  { nopi | nopi | nopi | nopi }                                                 # 0304
  { nopi | nopi | blt r13, r12, __ccom_arith_12_skip | nopi }                   # 0305
  { nopi | nopi | nopi | nopi }                                                 # 0306
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0307
  { nopi | nopi | nopi | nopi }                                                 # 0308
__ccom_arith_12_skip:                                                           # bundle 309
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0309
  { nopi | nopi | nopi | nopi }                                                 # 0310
  { nopi | nopi | bne r10, r0, __ccom_arith_11_div | nopi }                     # 0311
  { nopi | nopi | nopi | nopi }                                                 # 0312
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0313
  { nopi | nopi | nopi | nopi }                                                 # 0314
  { nopi | nopi | bge r13, r0, __ccom_arith_13_end | nopi }                     # 0315
  { nopi | nopi | nopi | nopi }                                                 # 0316
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0317
  { nopi | nopi | nopi | nopi }                                                 # 0318
__ccom_arith_13_end:                                                            # bundle 319
  { mov t0, 0 | nopi | nopi | sw t2, 208(rz) }                                  # 0319  linea 35  slots 0=35,1=0,2=0,3=34
  { nopi | nopi | nopi | lw t1, 208(rz) }                                       # 0320
  { nopi | nopi | nopi | sw t0, 228(rz) }                                       # 0321
  { nopi | nopi | nopi | sw t1, 232(rz) }                                       # 0322
  { nopi | nopi | nopi | nopi }                                                 # 0323
.L_for_start_2:                                                                 # bundle 324
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0324
  { nopi | nopi | nopi | lw t1, 232(rz) }                                       # 0325
  { nopi | nopi | nopi | nopi }                                                 # 0326
  { nopi | nopi | bge t0, t1, .L_for_end_4 | nopi }                             # 0327
  { nopi | nopi | nopi | nopi }                                                 # 0328
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0329  linea 36
  { nopi | nopi | nopi | nopi }                                                 # 0330
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0331
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0332
  { nopi | nopi | nopi | nopi }                                                 # 0333
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0334
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0335
  { nopi | nopi | nopi | nopi }                                                 # 0336
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0337
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0338
  { nopi | nopi | nopi | nopi }                                                 # 0339
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0340
  { nopi | nopi | nopi | nopi }                                                 # 0341
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0342
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0343
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0344
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0345
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0346
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0347
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0348
  { nopi | nopi | nopi | nopi }                                                 # 0349
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0350
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0351
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_bloques_de | nopi }                # 0352
  { nopi | nopi | nopi | nopi }                                                 # 0353
  { saddi rx, rx, -4 | mov t1, 2 | nopi | nopi }                                # 0354  slots 0=36,1=37,2=0,3=0
  { nopi | nopi | nopi | nopi }                                                 # 0355
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0356
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0357
  { nopi | nopi | nopi | nopi }                                                 # 0358
  { nopi | nopi | nopi | sw t0, 236(rz) }                                       # 0359
  { nopi | nopi | beq r5, r0, .L_arith_error | lw t0, 228(rz) }                 # 0360  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0361
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0362
  { nopi | nopi | nopi | nopi }                                                 # 0363
  { nopi | nopi | bge r7, r0, __ccom_arith_14_apos | nopi }                     # 0364
  { nopi | nopi | nopi | nopi }                                                 # 0365
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0366
  { nopi | nopi | nopi | nopi }                                                 # 0367
__ccom_arith_14_apos:                                                           # bundle 368
  { nopi | nopi | bge r8, r0, __ccom_arith_15_bpos | nopi }                     # 0368
  { nopi | nopi | nopi | nopi }                                                 # 0369
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0370
  { nopi | nopi | nopi | nopi }                                                 # 0371
__ccom_arith_15_bpos:                                                           # bundle 372
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0372
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0373
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0374
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0375
  { nopi | nopi | nopi | nopi }                                                 # 0376
__ccom_arith_16_div:                                                            # bundle 377
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0377
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0378
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0379
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0380
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0381
  { nopi | nopi | nopi | nopi }                                                 # 0382
  { nopi | nopi | blt r13, r12, __ccom_arith_17_skip | nopi }                   # 0383
  { nopi | nopi | nopi | nopi }                                                 # 0384
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0385
  { nopi | nopi | nopi | nopi }                                                 # 0386
__ccom_arith_17_skip:                                                           # bundle 387
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0387
  { nopi | nopi | nopi | nopi }                                                 # 0388
  { nopi | nopi | bne r10, r0, __ccom_arith_16_div | nopi }                     # 0389
  { nopi | nopi | nopi | nopi }                                                 # 0390
  { add r6, r9, r0 | nopi | bge r4, r0, __ccom_arith_18_end | nopi }            # 0391
  { nopi | nopi | nopi | nopi }                                                 # 0392
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0393
  { nopi | nopi | nopi | nopi }                                                 # 0394
__ccom_arith_18_end:                                                            # bundle 395
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0395
  { nopi | nopi | nopi | nopi }                                                 # 0396
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 0397
  { nopi | nopi | nopi | nopi }                                                 # 0398
  { nopi | nopi | beq t2, rz, .L_if_next_5 | nopi }                             # 0399
  { nopi | nopi | nopi | nopi }                                                 # 0400
  { nopi | nopi | nopi | nopi }                                                 # 0401
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0402  linea 38
  { nopi | nopi | nopi | nopi }                                                 # 0403
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0404
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0405
  { nopi | nopi | nopi | nopi }                                                 # 0406
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0407
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0408
  { nopi | nopi | nopi | nopi }                                                 # 0409
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0410
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0411
  { nopi | nopi | nopi | nopi }                                                 # 0412
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0413
  { nopi | nopi | nopi | nopi }                                                 # 0414
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0415
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0416
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0417
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0418
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0419
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0420
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0421
  { nopi | nopi | nopi | nopi }                                                 # 0422
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0423
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0424
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0425
  { nopi | nopi | nopi | nopi }                                                 # 0426
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0427
  { nopi | nopi | nopi | nopi }                                                 # 0428
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0429
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0430
  { nopi | nopi | nopi | nopi }                                                 # 0431
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0432
  { nopi | nopi | nopi | nopi }                                                 # 0433
  { nopi | nopi | nopi | nopi }                                                 # 0434
  { nopi | nopi | nopi | nopi }                                                 # 0435
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0436
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0437
  { nopi | nopi | nopi | nopi }                                                 # 0438
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0439
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0440
  { nopi | nopi | nopi | nopi }                                                 # 0441
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0442
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0443
  { nopi | nopi | nopi | nopi }                                                 # 0444
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0445
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0446
  { nopi | nopi | nopi | nopi }                                                 # 0447
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0448
  { nopi | nopi | nopi | nopi }                                                 # 0449
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0450
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0451
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0452
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0453
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0454
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0455
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0456
  { nopi | nopi | nopi | nopi }                                                 # 0457
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0458
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0459
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0460
  { nopi | nopi | nopi | nopi }                                                 # 0461
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0462
  { nopi | nopi | nopi | nopi }                                                 # 0463
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0464
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0465
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0466
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0467
  { nopi | nopi | nopi | nopi }                                                 # 0468
  { nopi | nopi | nopi | nopi }                                                 # 0469
  { nopi | nopi | nopi | nopi }                                                 # 0470
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0471
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0472
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0473
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0474
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 244(rz) }                            # 0475
  { nopi | nopi | nopi | nopi }                                                 # 0476
  { nopi | nopi | nopi | nopi }                                                 # 0477
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0478
  { nopi | nopi | nopi | nopi }                                                 # 0479
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_1 | nopi }                    # 0480
.L_crypto_cifra_1:                                                              # bundle 481
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0481
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0482
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0483
  { nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }                                # 0484
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0485
  { nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }                                # 0486
  { nopi | nopi | nopi | nopi }                                                 # 0487
  { nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }                                # 0488
  { nopi | nopi | nopi | nopi }                                                 # 0489
  { nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }                                # 0490
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0491
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_1 | sw t1, 4(t3) }     # 0492
.L_crypto_fin_cifra_1:                                                          # bundle 493
  { nopi | nopi | nopi | nopi }                                                 # 0493
  { nopi | nopi | jef .L_if_end_6 | nopi }                                      # 0494  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0495
.L_if_next_5:                                                                   # bundle 496
  { nopi | nopi | nopi | nopi }                                                 # 0496
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0497  linea 40
  { nopi | nopi | nopi | nopi }                                                 # 0498
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0499
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0500
  { nopi | nopi | nopi | nopi }                                                 # 0501
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0502
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0503
  { nopi | nopi | nopi | nopi }                                                 # 0504
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0505
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0506
  { nopi | nopi | nopi | nopi }                                                 # 0507
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0508
  { nopi | nopi | nopi | nopi }                                                 # 0509
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0510
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0511
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0512
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0513
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0514
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0515
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0516
  { nopi | nopi | nopi | nopi }                                                 # 0517
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0518
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0519
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0520
  { nopi | nopi | nopi | nopi }                                                 # 0521
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0522
  { nopi | nopi | nopi | nopi }                                                 # 0523
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0524
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0525
  { nopi | nopi | nopi | nopi }                                                 # 0526
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0527
  { nopi | nopi | nopi | nopi }                                                 # 0528
  { nopi | nopi | nopi | nopi }                                                 # 0529
  { nopi | nopi | nopi | nopi }                                                 # 0530
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0531
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0532
  { nopi | nopi | nopi | nopi }                                                 # 0533
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0534
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0535
  { nopi | nopi | nopi | nopi }                                                 # 0536
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0537
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0538
  { nopi | nopi | nopi | nopi }                                                 # 0539
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0540
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0541
  { nopi | nopi | nopi | nopi }                                                 # 0542
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0543
  { nopi | nopi | nopi | nopi }                                                 # 0544
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0545
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0546
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0547
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0548
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0549
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0550
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0551
  { nopi | nopi | nopi | nopi }                                                 # 0552
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0553
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0554
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0555
  { nopi | nopi | nopi | nopi }                                                 # 0556
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0557
  { nopi | nopi | nopi | nopi }                                                 # 0558
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0559
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0560
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0561
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0562
  { nopi | nopi | nopi | nopi }                                                 # 0563
  { nopi | nopi | nopi | nopi }                                                 # 0564
  { nopi | nopi | nopi | nopi }                                                 # 0565
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0566
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0567
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0568
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0569
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 244(rz) }                            # 0570
  { nopi | nopi | nopi | nopi }                                                 # 0571
  { nopi | nopi | nopi | nopi }                                                 # 0572
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0573
  { nopi | nopi | nopi | nopi }                                                 # 0574
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_2 | nopi }                    # 0575
.L_crypto_cifra_2:                                                              # bundle 576
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0576
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0577
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0578
  { nopi | feis t0, t0, t1, 1, 0 | nopi | nopi }                                # 0579
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0580
  { nopi | feis t1, t1, t0, 1, 1 | nopi | nopi }                                # 0581
  { nopi | nopi | nopi | nopi }                                                 # 0582
  { nopi | feis t0, t0, t1, 1, 2 | nopi | nopi }                                # 0583
  { nopi | nopi | nopi | nopi }                                                 # 0584
  { nopi | feis t1, t1, t0, 1, 3 | nopi | nopi }                                # 0585
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0586
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_2 | sw t1, 4(t3) }     # 0587
.L_crypto_fin_cifra_2:                                                          # bundle 588
  { nopi | nopi | nopi | nopi }                                                 # 0588
.L_if_end_6:                                                                    # bundle 589
.L_for_update_3:                                                                # bundle 589
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0589  linea 35
  { nopi | nopi | nopi | nopi }                                                 # 0590
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 0591
  { nopi | nopi | nopi | nopi }                                                 # 0592
  { nopi | nopi | jef .L_for_start_2 | sw t0, 228(rz) }                         # 0593
  { nopi | nopi | nopi | nopi }                                                 # 0594
.L_for_end_4:                                                                   # bundle 595
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0595  linea 43
  { nopi | nopi | nopi | nopi }                                                 # 0596
.L_func_descifrar_archivo:                                                      # bundle 597
  { nopi | nopi | nopi | sw rz, 292(rz) }                                       # 0597  linea 46
  { nopi | nopi | nopi | nopi }                                                 # 0598
.L_body_descifrar_archivo:                                                      # bundle 599
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0599  linea 47
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0600
  { nopi | nopi | nopi | nopi }                                                 # 0601
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0602
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0603
  { nopi | nopi | nopi | nopi }                                                 # 0604
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0605
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0606
  { nopi | nopi | nopi | nopi }                                                 # 0607
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0608
  { nopi | nopi | nopi | nopi }                                                 # 0609
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0610
  { nopi | nopi | nopi | nopi }                                                 # 0611
  { nopi | nopi | bge r7, r0, __ccom_arith_19_apos | nopi }                     # 0612
  { nopi | nopi | nopi | nopi }                                                 # 0613
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0614
  { nopi | nopi | nopi | nopi }                                                 # 0615
__ccom_arith_19_apos:                                                           # bundle 616
  { nopi | nopi | bge r8, r0, __ccom_arith_20_bpos | nopi }                     # 0616
  { nopi | nopi | nopi | nopi }                                                 # 0617
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0618
  { nopi | nopi | nopi | nopi }                                                 # 0619
__ccom_arith_20_bpos:                                                           # bundle 620
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0620
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0621
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0622
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0623
  { nopi | nopi | nopi | nopi }                                                 # 0624
__ccom_arith_21_div:                                                            # bundle 625
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0625
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0626
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0627
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0628
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0629
  { nopi | nopi | nopi | nopi }                                                 # 0630
  { nopi | nopi | blt r13, r12, __ccom_arith_22_skip | nopi }                   # 0631
  { nopi | nopi | nopi | nopi }                                                 # 0632
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0633
  { nopi | nopi | nopi | nopi }                                                 # 0634
__ccom_arith_22_skip:                                                           # bundle 635
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0635
  { nopi | nopi | nopi | nopi }                                                 # 0636
  { nopi | nopi | bne r10, r0, __ccom_arith_21_div | nopi }                     # 0637
  { nopi | nopi | nopi | nopi }                                                 # 0638
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0639
  { nopi | nopi | nopi | nopi }                                                 # 0640
  { nopi | nopi | bge r13, r0, __ccom_arith_23_end | nopi }                     # 0641
  { nopi | nopi | nopi | nopi }                                                 # 0642
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0643
  { nopi | nopi | nopi | nopi }                                                 # 0644
__ccom_arith_23_end:                                                            # bundle 645
  { mov t0, 0 | nopi | nopi | sw t2, 292(rz) }                                  # 0645  linea 48  slots 0=48,1=0,2=0,3=47
  { nopi | nopi | nopi | lw t1, 292(rz) }                                       # 0646
  { nopi | nopi | nopi | sw t0, 312(rz) }                                       # 0647
  { nopi | nopi | nopi | sw t1, 316(rz) }                                       # 0648
  { nopi | nopi | nopi | nopi }                                                 # 0649
.L_for_start_7:                                                                 # bundle 650
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0650
  { nopi | nopi | nopi | lw t1, 316(rz) }                                       # 0651
  { nopi | nopi | nopi | nopi }                                                 # 0652
  { nopi | nopi | bge t0, t1, .L_for_end_9 | nopi }                             # 0653
  { nopi | nopi | nopi | nopi }                                                 # 0654
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0655  linea 49
  { nopi | nopi | nopi | nopi }                                                 # 0656
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0657
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0658
  { nopi | nopi | nopi | nopi }                                                 # 0659
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0660
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0661
  { nopi | nopi | nopi | nopi }                                                 # 0662
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0663
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0664
  { nopi | nopi | nopi | nopi }                                                 # 0665
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0666
  { nopi | nopi | nopi | nopi }                                                 # 0667
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0668
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0669
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0670
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0671
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0672
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0673
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0674
  { nopi | nopi | nopi | nopi }                                                 # 0675
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0676
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0677
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_bloques_de | nopi }                # 0678
  { nopi | nopi | nopi | nopi }                                                 # 0679
  { saddi rx, rx, -4 | mov t1, 2 | nopi | nopi }                                # 0680  slots 0=49,1=50,2=0,3=0
  { nopi | nopi | nopi | nopi }                                                 # 0681
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0682
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0683
  { nopi | nopi | nopi | nopi }                                                 # 0684
  { nopi | nopi | nopi | sw t0, 320(rz) }                                       # 0685
  { nopi | nopi | beq r5, r0, .L_arith_error | lw t0, 312(rz) }                 # 0686  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 0687
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0688
  { nopi | nopi | nopi | nopi }                                                 # 0689
  { nopi | nopi | bge r7, r0, __ccom_arith_24_apos | nopi }                     # 0690
  { nopi | nopi | nopi | nopi }                                                 # 0691
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0692
  { nopi | nopi | nopi | nopi }                                                 # 0693
__ccom_arith_24_apos:                                                           # bundle 694
  { nopi | nopi | bge r8, r0, __ccom_arith_25_bpos | nopi }                     # 0694
  { nopi | nopi | nopi | nopi }                                                 # 0695
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0696
  { nopi | nopi | nopi | nopi }                                                 # 0697
__ccom_arith_25_bpos:                                                           # bundle 698
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0698
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0699
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0700
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0701
  { nopi | nopi | nopi | nopi }                                                 # 0702
__ccom_arith_26_div:                                                            # bundle 703
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0703
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0704
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0705
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0706
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0707
  { nopi | nopi | nopi | nopi }                                                 # 0708
  { nopi | nopi | blt r13, r12, __ccom_arith_27_skip | nopi }                   # 0709
  { nopi | nopi | nopi | nopi }                                                 # 0710
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0711
  { nopi | nopi | nopi | nopi }                                                 # 0712
__ccom_arith_27_skip:                                                           # bundle 713
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0713
  { nopi | nopi | nopi | nopi }                                                 # 0714
  { nopi | nopi | bne r10, r0, __ccom_arith_26_div | nopi }                     # 0715
  { nopi | nopi | nopi | nopi }                                                 # 0716
  { add r6, r9, r0 | nopi | bge r4, r0, __ccom_arith_28_end | nopi }            # 0717
  { nopi | nopi | nopi | nopi }                                                 # 0718
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0719
  { nopi | nopi | nopi | nopi }                                                 # 0720
__ccom_arith_28_end:                                                            # bundle 721
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0721
  { nopi | nopi | nopi | nopi }                                                 # 0722
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 0723
  { nopi | nopi | nopi | nopi }                                                 # 0724
  { nopi | nopi | beq t2, rz, .L_if_next_10 | nopi }                            # 0725
  { nopi | nopi | nopi | nopi }                                                 # 0726
  { nopi | nopi | nopi | nopi }                                                 # 0727
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 0728  linea 51
  { nopi | nopi | nopi | nopi }                                                 # 0729
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0730
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0731
  { nopi | nopi | nopi | nopi }                                                 # 0732
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0733
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0734
  { nopi | nopi | nopi | nopi }                                                 # 0735
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0736
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0737
  { nopi | nopi | nopi | nopi }                                                 # 0738
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0739
  { nopi | nopi | nopi | nopi }                                                 # 0740
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0741
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0742
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0743
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0744
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0745
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0746
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0747
  { nopi | nopi | nopi | nopi }                                                 # 0748
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0749
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0750
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0751
  { nopi | nopi | nopi | nopi }                                                 # 0752
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0753
  { nopi | nopi | nopi | nopi }                                                 # 0754
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0755
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0756
  { nopi | nopi | nopi | nopi }                                                 # 0757
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0758
  { nopi | nopi | nopi | nopi }                                                 # 0759
  { nopi | nopi | nopi | nopi }                                                 # 0760
  { nopi | nopi | nopi | nopi }                                                 # 0761
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0762
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 0763
  { nopi | nopi | nopi | nopi }                                                 # 0764
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0765
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0766
  { nopi | nopi | nopi | nopi }                                                 # 0767
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0768
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0769
  { nopi | nopi | nopi | nopi }                                                 # 0770
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0771
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0772
  { nopi | nopi | nopi | nopi }                                                 # 0773
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0774
  { nopi | nopi | nopi | nopi }                                                 # 0775
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0776
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0777
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0778
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0779
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0780
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0781
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0782
  { nopi | nopi | nopi | nopi }                                                 # 0783
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0784
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0785
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0786
  { nopi | nopi | nopi | nopi }                                                 # 0787
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0788
  { nopi | nopi | nopi | nopi }                                                 # 0789
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0790
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0791
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0792
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0793
  { nopi | nopi | nopi | nopi }                                                 # 0794
  { nopi | nopi | nopi | nopi }                                                 # 0795
  { nopi | nopi | nopi | nopi }                                                 # 0796
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0797
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 0798
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 0799
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0800
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 328(rz) }                            # 0801
  { nopi | nopi | nopi | nopi }                                                 # 0802
  { nopi | nopi | nopi | nopi }                                                 # 0803
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0804
  { nopi | nopi | nopi | nopi }                                                 # 0805
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_3 | nopi }                 # 0806
.L_crypto_descifra_3:                                                           # bundle 807
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 0807
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 0808
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0809
  { nopi | feis t0, t0, t1, 0, 3 | nopi | nopi }                                # 0810
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0811
  { nopi | feis t1, t1, t0, 0, 2 | nopi | nopi }                                # 0812
  { nopi | nopi | nopi | nopi }                                                 # 0813
  { nopi | feis t0, t0, t1, 0, 1 | nopi | nopi }                                # 0814
  { nopi | nopi | nopi | nopi }                                                 # 0815
  { nopi | feis t1, t1, t0, 0, 0 | nopi | nopi }                                # 0816
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 0817
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_3 | sw t1, 0(t3) }  # 0818
.L_crypto_fin_descifra_3:                                                       # bundle 819
  { nopi | nopi | nopi | nopi }                                                 # 0819
  { nopi | nopi | jef .L_if_end_11 | nopi }                                     # 0820  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 0821
.L_if_next_10:                                                                  # bundle 822
  { nopi | nopi | nopi | nopi }                                                 # 0822
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 0823  linea 53
  { nopi | nopi | nopi | nopi }                                                 # 0824
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0825
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0826
  { nopi | nopi | nopi | nopi }                                                 # 0827
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0828
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0829
  { nopi | nopi | nopi | nopi }                                                 # 0830
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0831
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0832
  { nopi | nopi | nopi | nopi }                                                 # 0833
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0834
  { nopi | nopi | nopi | nopi }                                                 # 0835
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0836
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0837
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0838
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0839
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0840
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0841
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0842
  { nopi | nopi | nopi | nopi }                                                 # 0843
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0844
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0845
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0846
  { nopi | nopi | nopi | nopi }                                                 # 0847
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0848
  { nopi | nopi | nopi | nopi }                                                 # 0849
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0850
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0851
  { nopi | nopi | nopi | nopi }                                                 # 0852
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0853
  { nopi | nopi | nopi | nopi }                                                 # 0854
  { nopi | nopi | nopi | nopi }                                                 # 0855
  { nopi | nopi | nopi | nopi }                                                 # 0856
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0857
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 0858
  { nopi | nopi | nopi | nopi }                                                 # 0859
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0860
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0861
  { nopi | nopi | nopi | nopi }                                                 # 0862
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0863
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0864
  { nopi | nopi | nopi | nopi }                                                 # 0865
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0866
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0867
  { nopi | nopi | nopi | nopi }                                                 # 0868
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0869
  { nopi | nopi | nopi | nopi }                                                 # 0870
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0871
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0872
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0873
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0874
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0875
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0876
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0877
  { nopi | nopi | nopi | nopi }                                                 # 0878
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0879
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0880
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0881
  { nopi | nopi | nopi | nopi }                                                 # 0882
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0883
  { nopi | nopi | nopi | nopi }                                                 # 0884
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0885
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0886
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0887
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0888
  { nopi | nopi | nopi | nopi }                                                 # 0889
  { nopi | nopi | nopi | nopi }                                                 # 0890
  { nopi | nopi | nopi | nopi }                                                 # 0891
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0892
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 0893
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 0894
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0895
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 328(rz) }                            # 0896
  { nopi | nopi | nopi | nopi }                                                 # 0897
  { nopi | nopi | nopi | nopi }                                                 # 0898
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0899
  { nopi | nopi | nopi | nopi }                                                 # 0900
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_4 | nopi }                 # 0901
.L_crypto_descifra_4:                                                           # bundle 902
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 0902
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 0903
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0904
  { nopi | feis t0, t0, t1, 1, 3 | nopi | nopi }                                # 0905
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0906
  { nopi | feis t1, t1, t0, 1, 2 | nopi | nopi }                                # 0907
  { nopi | nopi | nopi | nopi }                                                 # 0908
  { nopi | feis t0, t0, t1, 1, 1 | nopi | nopi }                                # 0909
  { nopi | nopi | nopi | nopi }                                                 # 0910
  { nopi | feis t1, t1, t0, 1, 0 | nopi | nopi }                                # 0911
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 0912
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_4 | sw t1, 0(t3) }  # 0913
.L_crypto_fin_descifra_4:                                                       # bundle 914
  { nopi | nopi | nopi | nopi }                                                 # 0914
.L_if_end_11:                                                                   # bundle 915
.L_for_update_8:                                                                # bundle 915
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0915  linea 48
  { nopi | nopi | nopi | nopi }                                                 # 0916
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 0917
  { nopi | nopi | nopi | nopi }                                                 # 0918
  { nopi | nopi | jef .L_for_start_7 | sw t0, 312(rz) }                         # 0919
  { nopi | nopi | nopi | nopi }                                                 # 0920
.L_for_end_9:                                                                   # bundle 921
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0921  linea 56
  { nopi | nopi | nopi | nopi }                                                 # 0922
fin_main:                                                                       # bundle 923
  { nopi | nopi | jef fin_main | nopi }                                         # 0923  linea -
.L_arith_error:                                                                 # bundle 924
  { nopi | nopi | jef .L_arith_error | nopi }                                   # 0924

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_bloques_de             104
#   .L_body_bloques_de             106
#   __ccom_arith_0_mul             112  (interna)
#   __ccom_arith_1_skip            120  (interna)
#   __ccom_arith_2_end             122  (interna)
#   .L_if_next_0                   136
#   .L_if_end_1                    140
#   .L_func_direccion              142
#   .L_body_direccion              144
#   __ccom_arith_3_mul             150  (interna)
#   __ccom_arith_4_skip            158  (interna)
#   __ccom_arith_5_end             160  (interna)
#   __ccom_arith_6_mul             164  (interna)
#   __ccom_arith_7_skip            172  (interna)
#   __ccom_arith_8_end             174  (interna)
#   .L_func_preparar_boveda        178
#   .L_body_preparar_boveda        178
#   .L_func_cifrar_archivo         271
#   .L_body_cifrar_archivo         273
#   __ccom_arith_9_apos            290  (interna)
#   __ccom_arith_10_bpos           294  (interna)
#   __ccom_arith_11_div            299  (interna)
#   __ccom_arith_12_skip           309  (interna)
#   __ccom_arith_13_end            319  (interna)
#   .L_for_start_2                 324
#   __ccom_arith_14_apos           368  (interna)
#   __ccom_arith_15_bpos           372  (interna)
#   __ccom_arith_16_div            377  (interna)
#   __ccom_arith_17_skip           387  (interna)
#   __ccom_arith_18_end            395  (interna)
#   .L_crypto_cifra_1              481  (interna)
#   .L_crypto_fin_cifra_1          493  (interna)
#   .L_if_next_5                   496
#   .L_crypto_cifra_2              576  (interna)
#   .L_crypto_fin_cifra_2          588  (interna)
#   .L_if_end_6                    589
#   .L_for_update_3                589
#   .L_for_end_4                   595
#   .L_func_descifrar_archivo      597
#   .L_body_descifrar_archivo      599
#   __ccom_arith_19_apos           616  (interna)
#   __ccom_arith_20_bpos           620  (interna)
#   __ccom_arith_21_div            625  (interna)
#   __ccom_arith_22_skip           635  (interna)
#   __ccom_arith_23_end            645  (interna)
#   .L_for_start_7                 650
#   __ccom_arith_24_apos           694  (interna)
#   __ccom_arith_25_bpos           698  (interna)
#   __ccom_arith_26_div            703  (interna)
#   __ccom_arith_27_skip           713  (interna)
#   __ccom_arith_28_end            721  (interna)
#   .L_crypto_descifra_3           807  (interna)
#   .L_crypto_fin_descifra_3       819  (interna)
#   .L_if_next_10                  822
#   .L_crypto_descifra_4           902  (interna)
#   .L_crypto_fin_descifra_4       914  (interna)
#   .L_if_end_11                   915
#   .L_for_update_8                915
#   .L_for_end_9                   921
#   fin_main                       923
#   .L_arith_error                 924

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
