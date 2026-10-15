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
  { mov t0, 5896 | mov t1, 15 | nopi | nopi }                                   # 0010  linea 62
  { nopi | nopi | nopi | nopi }                                                 # 0011
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0012
  { nopi | nopi | nopi | nopi }                                                 # 0013
  { saddi t0, t0, 10485 | nopi | nopi | nopi }                                  # 0014
  { nopi | nopi | nopi | nopi }                                                 # 0015
  { sub t0, rz, t0 | nopi | nopi | nopi }                                       # 0016
  { nopi | nopi | nopi | nopi }                                                 # 0017
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0018
  { nopi | nopi | nopi | nopi }                                                 # 0019
  { nopi | nopi | nopi | nopi }                                                 # 0020
  { nopi | auth t6 | nopi | nopi }                                              # 0021
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0022  linea 63
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_preparar_boveda | nopi }           # 0023
  { nopi | nopi | nopi | nopi }                                                 # 0024
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0025
  { nopi | nopi | nopi | nopi }                                                 # 0026
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0027
  { nopi | nopi | nopi | lw t0, 0(rz) }                                         # 0028  linea 64
  { nopi | nopi | nopi | nopi }                                                 # 0029
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0030
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 4(rz) }                              # 0031
  { nopi | nopi | nopi | nopi }                                                 # 0032
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0033
  { saddi rx, rx, 4 | mov t0, 128 | nopi | nopi }                               # 0034
  { nopi | nopi | nopi | nopi }                                                 # 0035
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0036
  { saddi rx, rx, 4 | mov t0, 16 | nopi | nopi }                                # 0037
  { nopi | nopi | nopi | nopi }                                                 # 0038
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0039
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0040
  { nopi | nopi | nopi | nopi }                                                 # 0041
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0042
  { nopi | nopi | nopi | nopi }                                                 # 0043
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0044
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0045
  { nopi | nopi | nopi | sw t0, 224(rz) }                                       # 0046
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0047
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0048
  { nopi | nopi | nopi | sw t0, 220(rz) }                                       # 0049
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0050
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0051
  { nopi | nopi | nopi | sw t0, 216(rz) }                                       # 0052
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0053
  { nopi | nopi | nopi | nopi }                                                 # 0054
  { nopi | nopi | nopi | sw t0, 212(rz) }                                       # 0055
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0056
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_cifrar_archivo | nopi }            # 0057
  { nopi | nopi | nopi | nopi }                                                 # 0058
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0059
  { nopi | nopi | nopi | nopi }                                                 # 0060
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0061
  { nopi | nopi | nopi | lw t0, 208(rz) }                                       # 0062
  { nopi | nopi | nopi | nopi }                                                 # 0063
  { nopi | nopi | nopi | sw t0, 372(rz) }                                       # 0064
  { nopi | nopi | nopi | lw t0, 4(rz) }                                         # 0065  linea 65
  { nopi | nopi | nopi | nopi }                                                 # 0066
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0067
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 8(rz) }                              # 0068
  { nopi | nopi | nopi | nopi }                                                 # 0069
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0070
  { saddi rx, rx, 4 | mov t0, 128 | nopi | nopi }                               # 0071
  { nopi | nopi | nopi | nopi }                                                 # 0072
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0073
  { saddi rx, rx, 4 | mov t0, 16 | nopi | nopi }                                # 0074
  { nopi | nopi | nopi | nopi }                                                 # 0075
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0076
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0077
  { nopi | nopi | nopi | nopi }                                                 # 0078
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0079
  { nopi | nopi | nopi | nopi }                                                 # 0080
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0081
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0082
  { nopi | nopi | nopi | sw t0, 308(rz) }                                       # 0083
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0084
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0085
  { nopi | nopi | nopi | sw t0, 304(rz) }                                       # 0086
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0087
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0088
  { nopi | nopi | nopi | sw t0, 300(rz) }                                       # 0089
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0090
  { nopi | nopi | nopi | nopi }                                                 # 0091
  { nopi | nopi | nopi | sw t0, 296(rz) }                                       # 0092
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0093
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_descifrar_archivo | nopi }         # 0094
  { nopi | nopi | nopi | nopi }                                                 # 0095
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0096
  { nopi | nopi | nopi | nopi }                                                 # 0097
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0098
  { nopi | nopi | nopi | lw t0, 292(rz) }                                       # 0099
  { nopi | nopi | nopi | nopi }                                                 # 0100
  { nopi | nopi | nopi | sw t0, 376(rz) }                                       # 0101
  { nopi | nopi | nopi | nopi }                                                 # 0102
  { nopi | logout | nopi | nopi }                                               # 0103  linea 66
  { nopi | nopi | jef fin_main | nopi }                                         # 0104  linea 67
  { nopi | nopi | nopi | nopi }                                                 # 0105
.L_func_bloques_de:                                                             # bundle 106
  { nopi | nopi | nopi | sw rz, 16(rz) }                                        # 0106  linea 5
  { nopi | nopi | nopi | nopi }                                                 # 0107
.L_body_bloques_de:                                                             # bundle 108
  { mov r11, 1 | mov r7, 0 | nopi | lw t0, 20(rz) }                             # 0108  linea 6
  { nopi | nopi | nopi | lw t1, 28(rz) }                                        # 0109
  { nopi | nopi | nopi | lw t2, 24(rz) }                                        # 0110
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0111
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0112
  { nopi | nopi | nopi | nopi }                                                 # 0113
__ccom_arith_0_mul:                                                             # bundle 114
  { nopi | nopi | beq r9, r0, __ccom_arith_2_end | nopi }                       # 0114
  { nopi | nopi | nopi | nopi }                                                 # 0115
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0116
  { nopi | nopi | nopi | nopi }                                                 # 0117
  { nopi | nopi | beq r10, r0, __ccom_arith_1_skip | nopi }                     # 0118
  { nopi | nopi | nopi | nopi }                                                 # 0119
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0120
  { nopi | nopi | nopi | nopi }                                                 # 0121
__ccom_arith_1_skip:                                                            # bundle 122
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_0_mul | nopi }         # 0122
  { nopi | nopi | nopi | nopi }                                                 # 0123
__ccom_arith_2_end:                                                             # bundle 124
  { sub t0, t0, t3 | nopi | nopi | nopi }                                       # 0124
  { nopi | nopi | nopi | nopi }                                                 # 0125
  { nopi | nopi | nopi | sw t0, 32(rz) }                                        # 0126
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0127  linea 7
  { nopi | nopi | nopi | lw t1, 24(rz) }                                        # 0128
  { nopi | nopi | nopi | nopi }                                                 # 0129
  { slt t0, t0, t1 | nopi | nopi | nopi }                                       # 0130
  { nopi | nopi | nopi | nopi }                                                 # 0131
  { nopi | nopi | beq t0, rz, .L_if_next_0 | nopi }                             # 0132
  { nopi | nopi | nopi | nopi }                                                 # 0133
  { nopi | nopi | nopi | lw t0, 32(rz) }                                        # 0134  linea 8
  { nopi | nopi | nopi | nopi }                                                 # 0135
  { nopi | nopi | jef .L_if_end_1 | sw t0, 16(rz) }                             # 0136  linea 7  slots 0=0,1=0,2=7,3=8
  { nopi | nopi | nopi | nopi }                                                 # 0137
.L_if_next_0:                                                                   # bundle 138
  { nopi | nopi | nopi | lw t0, 24(rz) }                                        # 0138  linea 10
  { nopi | nopi | nopi | nopi }                                                 # 0139
  { nopi | nopi | nopi | sw t0, 16(rz) }                                        # 0140
  { nopi | nopi | nopi | nopi }                                                 # 0141
.L_if_end_1:                                                                    # bundle 142
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0142  linea 12
  { nopi | nopi | nopi | nopi }                                                 # 0143
.L_func_direccion:                                                              # bundle 144
  { nopi | nopi | nopi | sw rz, 88(rz) }                                        # 0144  linea 15
  { nopi | nopi | nopi | nopi }                                                 # 0145
.L_body_direccion:                                                              # bundle 146
  { mov r11, 1 | mov r7, 0 | nopi | lw t0, 92(rz) }                             # 0146  linea 16
  { nopi | nopi | nopi | lw t1, 100(rz) }                                       # 0147
  { nopi | nopi | nopi | lw t2, 96(rz) }                                        # 0148
  { add r8, r5, r0 | nopi | nopi | nopi }                                       # 0149
  { add r9, r6, r0 | nopi | nopi | nopi }                                       # 0150
  { nopi | nopi | nopi | nopi }                                                 # 0151
__ccom_arith_3_mul:                                                             # bundle 152
  { nopi | nopi | beq r9, r0, __ccom_arith_5_end | nopi }                       # 0152
  { nopi | nopi | nopi | nopi }                                                 # 0153
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0154
  { nopi | nopi | nopi | nopi }                                                 # 0155
  { nopi | nopi | beq r10, r0, __ccom_arith_4_skip | nopi }                     # 0156
  { nopi | nopi | nopi | nopi }                                                 # 0157
  { add r7, r7, r8 | nopi | nopi | nopi }                                       # 0158
  { nopi | nopi | nopi | nopi }                                                 # 0159
__ccom_arith_4_skip:                                                            # bundle 160
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_3_mul | nopi }         # 0160
  { nopi | nopi | nopi | nopi }                                                 # 0161
__ccom_arith_5_end:                                                             # bundle 162
  { mov t1, 8 | mov r11, 1 | nopi | nopi }                                      # 0162
  { add r8, r7, r0 | mov r6, 0 | nopi | nopi }                                  # 0163
  { add r9, r5, r0 | nopi | nopi | nopi }                                       # 0164
  { nopi | nopi | nopi | nopi }                                                 # 0165
__ccom_arith_6_mul:                                                             # bundle 166
  { nopi | nopi | beq r9, r0, __ccom_arith_8_end | nopi }                       # 0166
  { nopi | nopi | nopi | nopi }                                                 # 0167
  { and r10, r9, r11 | nopi | nopi | nopi }                                     # 0168
  { nopi | nopi | nopi | nopi }                                                 # 0169
  { nopi | nopi | beq r10, r0, __ccom_arith_7_skip | nopi }                     # 0170
  { nopi | nopi | nopi | nopi }                                                 # 0171
  { add r6, r6, r8 | nopi | nopi | nopi }                                       # 0172
  { nopi | nopi | nopi | nopi }                                                 # 0173
__ccom_arith_7_skip:                                                            # bundle 174
  { sll r8, r8, r11 | srl r9, r9, r11 | jef __ccom_arith_6_mul | nopi }         # 0174
  { nopi | nopi | nopi | nopi }                                                 # 0175
__ccom_arith_8_end:                                                             # bundle 176
  { add t0, t0, t2 | nopi | nopi | nopi }                                       # 0176
  { nopi | nopi | nopi | nopi }                                                 # 0177
  { nopi | nopi | jefR rz, rl | sw t0, 88(rz) }                                 # 0178  linea 17  slots 0=0,1=0,2=17,3=16
  { nopi | nopi | nopi | nopi }                                                 # 0179
.L_func_preparar_boveda:                                                        # bundle 180
.L_body_preparar_boveda:                                                        # bundle 180
  { nopi | nopi | nopi | nopi }                                                 # 0180
  { mov t0, 583 | mov t1, 15 | nopi | nopi }                                    # 0181  linea 21
  { nopi | nopi | nopi | nopi }                                                 # 0182
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0183
  { nopi | nopi | nopi | nopi }                                                 # 0184
  { saddi t0, t0, 11213 | nopi | nopi | nopi }                                  # 0185
  { nopi | nopi | nopi | nopi }                                                 # 0186
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0187
  { nopi | nopi | nopi | nopi }                                                 # 0188
  { nopi | nopi | nopi | nopi }                                                 # 0189
  { nopi | kv_write t6, 0, 0 | nopi | nopi }                                    # 0190
  { nopi | nopi | nopi | nopi }                                                 # 0191
  { mov t0, 60584 | mov t1, 15 | nopi | nopi }                                  # 0192  linea 22
  { nopi | nopi | nopi | nopi }                                                 # 0193
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0194
  { nopi | nopi | nopi | nopi }                                                 # 0195
  { saddi t0, t0, 12817 | nopi | nopi | nopi }                                  # 0196
  { nopi | nopi | nopi | nopi }                                                 # 0197
  { sub t0, rz, t0 | nopi | nopi | nopi }                                       # 0198
  { nopi | nopi | nopi | nopi }                                                 # 0199
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0200
  { nopi | nopi | nopi | nopi }                                                 # 0201
  { nopi | nopi | nopi | nopi }                                                 # 0202
  { nopi | kv_write t6, 0, 1 | nopi | nopi }                                    # 0203
  { nopi | nopi | nopi | nopi }                                                 # 0204
  { mov t0, 9903 | mov t1, 15 | nopi | nopi }                                   # 0205  linea 23
  { nopi | nopi | nopi | nopi }                                                 # 0206
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0207
  { nopi | nopi | nopi | nopi }                                                 # 0208
  { saddi t0, t0, 7135 | nopi | nopi | nopi }                                   # 0209
  { nopi | nopi | nopi | nopi }                                                 # 0210
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0211
  { nopi | nopi | nopi | nopi }                                                 # 0212
  { nopi | nopi | nopi | nopi }                                                 # 0213
  { nopi | kv_write t6, 0, 2 | nopi | nopi }                                    # 0214
  { nopi | nopi | nopi | nopi }                                                 # 0215
  { mov t0, 18641 | mov t1, 15 | nopi | nopi }                                  # 0216  linea 24
  { nopi | nopi | nopi | nopi }                                                 # 0217
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0218
  { nopi | nopi | nopi | nopi }                                                 # 0219
  { saddi t0, t0, 11488 | nopi | nopi | nopi }                                  # 0220
  { nopi | nopi | nopi | nopi }                                                 # 0221
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0222
  { nopi | nopi | nopi | nopi }                                                 # 0223
  { nopi | nopi | nopi | nopi }                                                 # 0224
  { nopi | kv_write t6, 0, 3 | nopi | nopi }                                    # 0225
  { nopi | nopi | nopi | nopi }                                                 # 0226
  { mov t0, 8738 | mov t1, 15 | nopi | nopi }                                   # 0227  linea 25
  { nopi | nopi | nopi | nopi }                                                 # 0228
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0229
  { nopi | nopi | nopi | nopi }                                                 # 0230
  { saddi t0, t0, 4369 | nopi | nopi | nopi }                                   # 0231
  { nopi | nopi | nopi | nopi }                                                 # 0232
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0233
  { nopi | nopi | nopi | nopi }                                                 # 0234
  { nopi | nopi | nopi | nopi }                                                 # 0235
  { nopi | kv_write t6, 1, 0 | nopi | nopi }                                    # 0236
  { nopi | nopi | nopi | nopi }                                                 # 0237
  { mov t0, 17476 | mov t1, 15 | nopi | nopi }                                  # 0238  linea 26
  { nopi | nopi | nopi | nopi }                                                 # 0239
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0240
  { nopi | nopi | nopi | nopi }                                                 # 0241
  { saddi t0, t0, 8738 | nopi | nopi | nopi }                                   # 0242
  { nopi | nopi | nopi | nopi }                                                 # 0243
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0244
  { nopi | nopi | nopi | nopi }                                                 # 0245
  { nopi | nopi | nopi | nopi }                                                 # 0246
  { nopi | kv_write t6, 1, 1 | nopi | nopi }                                    # 0247
  { nopi | nopi | nopi | nopi }                                                 # 0248
  { mov t0, 26214 | mov t1, 15 | nopi | nopi }                                  # 0249  linea 27
  { nopi | nopi | nopi | nopi }                                                 # 0250
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0251
  { nopi | nopi | nopi | nopi }                                                 # 0252
  { saddi t0, t0, 13107 | nopi | nopi | nopi }                                  # 0253
  { nopi | nopi | nopi | nopi }                                                 # 0254
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0255
  { nopi | nopi | nopi | nopi }                                                 # 0256
  { nopi | nopi | nopi | nopi }                                                 # 0257
  { nopi | kv_write t6, 1, 2 | nopi | nopi }                                    # 0258
  { nopi | nopi | nopi | nopi }                                                 # 0259
  { mov t0, 34952 | mov t1, 15 | nopi | nopi }                                  # 0260  linea 28
  { nopi | nopi | nopi | nopi }                                                 # 0261
  { sll t0, t0, t1 | nopi | nopi | nopi }                                       # 0262
  { nopi | nopi | nopi | nopi }                                                 # 0263
  { saddi t0, t0, 17476 | nopi | nopi | nopi }                                  # 0264
  { nopi | nopi | nopi | nopi }                                                 # 0265
  { saddi t6, t0, 0 | nopi | nopi | nopi }                                      # 0266
  { nopi | nopi | nopi | nopi }                                                 # 0267
  { nopi | nopi | nopi | nopi }                                                 # 0268
  { nopi | kv_write t6, 1, 3 | nopi | nopi }                                    # 0269
  { nopi | kv_lock | nopi | nopi }                                              # 0270  linea 29
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0271  linea 30
  { nopi | nopi | nopi | nopi }                                                 # 0272
.L_func_cifrar_archivo:                                                         # bundle 273
  { nopi | nopi | nopi | sw rz, 208(rz) }                                       # 0273  linea 33
  { nopi | nopi | nopi | nopi }                                                 # 0274
.L_body_cifrar_archivo:                                                         # bundle 275
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0275  linea 34
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0276
  { nopi | nopi | nopi | nopi }                                                 # 0277
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0278
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0279
  { nopi | nopi | nopi | nopi }                                                 # 0280
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0281
  { nopi | nopi | nopi | lw t1, 224(rz) }                                       # 0282
  { nopi | nopi | nopi | nopi }                                                 # 0283
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0284
  { nopi | nopi | nopi | nopi }                                                 # 0285
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0286
  { nopi | nopi | nopi | nopi }                                                 # 0287
  { nopi | nopi | bge r7, r0, __ccom_arith_9_apos | nopi }                      # 0288
  { nopi | nopi | nopi | nopi }                                                 # 0289
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0290
  { nopi | nopi | nopi | nopi }                                                 # 0291
__ccom_arith_9_apos:                                                            # bundle 292
  { nopi | nopi | bge r8, r0, __ccom_arith_10_bpos | nopi }                     # 0292
  { nopi | nopi | nopi | nopi }                                                 # 0293
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0294
  { nopi | nopi | nopi | nopi }                                                 # 0295
__ccom_arith_10_bpos:                                                           # bundle 296
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0296
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0297
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0298
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0299
  { nopi | nopi | nopi | nopi }                                                 # 0300
__ccom_arith_11_div:                                                            # bundle 301
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0301
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0302
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0303
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0304
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0305
  { nopi | nopi | nopi | nopi }                                                 # 0306
  { nopi | nopi | blt r13, r12, __ccom_arith_12_skip | nopi }                   # 0307
  { nopi | nopi | nopi | nopi }                                                 # 0308
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0309
  { nopi | nopi | nopi | nopi }                                                 # 0310
__ccom_arith_12_skip:                                                           # bundle 311
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0311
  { nopi | nopi | nopi | nopi }                                                 # 0312
  { nopi | nopi | bne r10, r0, __ccom_arith_11_div | nopi }                     # 0313
  { nopi | nopi | nopi | nopi }                                                 # 0314
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0315
  { nopi | nopi | nopi | nopi }                                                 # 0316
  { nopi | nopi | bge r13, r0, __ccom_arith_13_end | nopi }                     # 0317
  { nopi | nopi | nopi | nopi }                                                 # 0318
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0319
  { nopi | nopi | nopi | nopi }                                                 # 0320
__ccom_arith_13_end:                                                            # bundle 321
  { mov t0, 0 | nopi | nopi | sw t2, 208(rz) }                                  # 0321  linea 35  slots 0=35,1=0,2=0,3=34
  { nopi | nopi | nopi | lw t1, 208(rz) }                                       # 0322
  { nopi | nopi | nopi | sw t0, 228(rz) }                                       # 0323
  { nopi | nopi | nopi | sw t1, 232(rz) }                                       # 0324
  { nopi | nopi | nopi | nopi }                                                 # 0325
.L_for_start_2:                                                                 # bundle 326
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0326
  { nopi | nopi | nopi | lw t1, 232(rz) }                                       # 0327
  { nopi | nopi | nopi | nopi }                                                 # 0328
  { nopi | nopi | bge t0, t1, .L_for_end_4 | nopi }                             # 0329
  { nopi | nopi | nopi | nopi }                                                 # 0330
  { nopi | nopi | nopi | lw t0, 220(rz) }                                       # 0331  linea 36
  { nopi | nopi | nopi | nopi }                                                 # 0332
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0333
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0334
  { nopi | nopi | nopi | nopi }                                                 # 0335
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0336
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0337
  { nopi | nopi | nopi | nopi }                                                 # 0338
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0339
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0340
  { nopi | nopi | nopi | nopi }                                                 # 0341
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0342
  { nopi | nopi | nopi | nopi }                                                 # 0343
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0344
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0345
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0346
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0347
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0348
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0349
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0350
  { nopi | nopi | nopi | nopi }                                                 # 0351
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0352
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0353
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_bloques_de | nopi }                # 0354
  { nopi | nopi | nopi | nopi }                                                 # 0355
  { saddi rx, rx, -4 | mov t1, 2 | nopi | nopi }                                # 0356  slots 0=36,1=37,2=0,3=0
  { nopi | nopi | nopi | nopi }                                                 # 0357
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0358
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0359
  { nopi | nopi | nopi | nopi }                                                 # 0360
  { nopi | nopi | nopi | sw t0, 236(rz) }                                       # 0361
  { nopi | nopi | beq r5, r0, .L_arith_error | lw t0, 228(rz) }                 # 0362  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0363
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0364
  { nopi | nopi | nopi | nopi }                                                 # 0365
  { nopi | nopi | bge r7, r0, __ccom_arith_14_apos | nopi }                     # 0366
  { nopi | nopi | nopi | nopi }                                                 # 0367
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0368
  { nopi | nopi | nopi | nopi }                                                 # 0369
__ccom_arith_14_apos:                                                           # bundle 370
  { nopi | nopi | bge r8, r0, __ccom_arith_15_bpos | nopi }                     # 0370
  { nopi | nopi | nopi | nopi }                                                 # 0371
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0372
  { nopi | nopi | nopi | nopi }                                                 # 0373
__ccom_arith_15_bpos:                                                           # bundle 374
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0374
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0375
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0376
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0377
  { nopi | nopi | nopi | nopi }                                                 # 0378
__ccom_arith_16_div:                                                            # bundle 379
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0379
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0380
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0381
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0382
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0383
  { nopi | nopi | nopi | nopi }                                                 # 0384
  { nopi | nopi | blt r13, r12, __ccom_arith_17_skip | nopi }                   # 0385
  { nopi | nopi | nopi | nopi }                                                 # 0386
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0387
  { nopi | nopi | nopi | nopi }                                                 # 0388
__ccom_arith_17_skip:                                                           # bundle 389
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0389
  { nopi | nopi | nopi | nopi }                                                 # 0390
  { nopi | nopi | bne r10, r0, __ccom_arith_16_div | nopi }                     # 0391
  { nopi | nopi | nopi | nopi }                                                 # 0392
  { add r6, r9, r0 | nopi | bge r4, r0, __ccom_arith_18_end | nopi }            # 0393
  { nopi | nopi | nopi | nopi }                                                 # 0394
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0395
  { nopi | nopi | nopi | nopi }                                                 # 0396
__ccom_arith_18_end:                                                            # bundle 397
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0397
  { nopi | nopi | nopi | nopi }                                                 # 0398
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 0399
  { nopi | nopi | nopi | nopi }                                                 # 0400
  { nopi | nopi | beq t2, rz, .L_if_next_5 | nopi }                             # 0401
  { nopi | nopi | nopi | nopi }                                                 # 0402
  { nopi | nopi | nopi | nopi }                                                 # 0403
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0404  linea 38
  { nopi | nopi | nopi | nopi }                                                 # 0405
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0406
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0407
  { nopi | nopi | nopi | nopi }                                                 # 0408
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0409
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0410
  { nopi | nopi | nopi | nopi }                                                 # 0411
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0412
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0413
  { nopi | nopi | nopi | nopi }                                                 # 0414
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0415
  { nopi | nopi | nopi | nopi }                                                 # 0416
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0417
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0418
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0419
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0420
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0421
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0422
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0423
  { nopi | nopi | nopi | nopi }                                                 # 0424
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0425
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0426
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0427
  { nopi | nopi | nopi | nopi }                                                 # 0428
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0429
  { nopi | nopi | nopi | nopi }                                                 # 0430
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0431
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0432
  { nopi | nopi | nopi | nopi }                                                 # 0433
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0434
  { nopi | nopi | nopi | nopi }                                                 # 0435
  { nopi | nopi | nopi | nopi }                                                 # 0436
  { nopi | nopi | nopi | nopi }                                                 # 0437
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0438
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0439
  { nopi | nopi | nopi | nopi }                                                 # 0440
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0441
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0442
  { nopi | nopi | nopi | nopi }                                                 # 0443
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0444
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0445
  { nopi | nopi | nopi | nopi }                                                 # 0446
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0447
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0448
  { nopi | nopi | nopi | nopi }                                                 # 0449
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0450
  { nopi | nopi | nopi | nopi }                                                 # 0451
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0452
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0453
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0454
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0455
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0456
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0457
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0458
  { nopi | nopi | nopi | nopi }                                                 # 0459
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0460
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0461
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0462
  { nopi | nopi | nopi | nopi }                                                 # 0463
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0464
  { nopi | nopi | nopi | nopi }                                                 # 0465
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0466
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0467
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0468
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0469
  { nopi | nopi | nopi | nopi }                                                 # 0470
  { nopi | nopi | nopi | nopi }                                                 # 0471
  { nopi | nopi | nopi | nopi }                                                 # 0472
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0473
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0474
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0475
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0476
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 244(rz) }                            # 0477
  { nopi | nopi | nopi | nopi }                                                 # 0478
  { nopi | nopi | nopi | nopi }                                                 # 0479
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0480
  { nopi | nopi | nopi | nopi }                                                 # 0481
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_1 | nopi }                    # 0482
.L_crypto_cifra_1:                                                              # bundle 483
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0483
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0484
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0485
  { nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }                                # 0486
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0487
  { nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }                                # 0488
  { nopi | nopi | nopi | nopi }                                                 # 0489
  { nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }                                # 0490
  { nopi | nopi | nopi | nopi }                                                 # 0491
  { nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }                                # 0492
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0493
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_1 | sw t1, 4(t3) }     # 0494
.L_crypto_fin_cifra_1:                                                          # bundle 495
  { nopi | nopi | nopi | nopi }                                                 # 0495
  { nopi | nopi | jef .L_if_end_6 | nopi }                                      # 0496  linea 37
  { nopi | nopi | nopi | nopi }                                                 # 0497
.L_if_next_5:                                                                   # bundle 498
  { nopi | nopi | nopi | nopi }                                                 # 0498
  { nopi | nopi | nopi | lw t0, 212(rz) }                                       # 0499  linea 40
  { nopi | nopi | nopi | nopi }                                                 # 0500
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0501
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0502
  { nopi | nopi | nopi | nopi }                                                 # 0503
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0504
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0505
  { nopi | nopi | nopi | nopi }                                                 # 0506
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0507
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0508
  { nopi | nopi | nopi | nopi }                                                 # 0509
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0510
  { nopi | nopi | nopi | nopi }                                                 # 0511
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0512
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0513
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0514
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0515
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0516
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0517
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0518
  { nopi | nopi | nopi | nopi }                                                 # 0519
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0520
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0521
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0522
  { nopi | nopi | nopi | nopi }                                                 # 0523
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0524
  { nopi | nopi | nopi | nopi }                                                 # 0525
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0526
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0527
  { nopi | nopi | nopi | nopi }                                                 # 0528
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0529
  { nopi | nopi | nopi | nopi }                                                 # 0530
  { nopi | nopi | nopi | nopi }                                                 # 0531
  { nopi | nopi | nopi | nopi }                                                 # 0532
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0533
  { nopi | nopi | nopi | lw t0, 216(rz) }                                       # 0534
  { nopi | nopi | nopi | nopi }                                                 # 0535
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0536
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 224(rz) }                            # 0537
  { nopi | nopi | nopi | nopi }                                                 # 0538
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0539
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 228(rz) }                            # 0540
  { nopi | nopi | nopi | nopi }                                                 # 0541
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0542
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0543
  { nopi | nopi | nopi | nopi }                                                 # 0544
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0545
  { nopi | nopi | nopi | nopi }                                                 # 0546
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0547
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0548
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0549
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0550
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0551
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0552
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0553
  { nopi | nopi | nopi | nopi }                                                 # 0554
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0555
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0556
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0557
  { nopi | nopi | nopi | nopi }                                                 # 0558
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0559
  { nopi | nopi | nopi | nopi }                                                 # 0560
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0561
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0562
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0563
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0564
  { nopi | nopi | nopi | nopi }                                                 # 0565
  { nopi | nopi | nopi | nopi }                                                 # 0566
  { nopi | nopi | nopi | nopi }                                                 # 0567
  { nopi | nopi | nopi | sw t2, 240(rz) }                                       # 0568
  { nopi | nopi | nopi | sw t3, 244(rz) }                                       # 0569
  { nopi | nopi | nopi | lw t0, 236(rz) }                                       # 0570
  { nopi | nopi | nopi | lw t2, 240(rz) }                                       # 0571
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 244(rz) }                            # 0572
  { nopi | nopi | nopi | nopi }                                                 # 0573
  { nopi | nopi | nopi | nopi }                                                 # 0574
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0575
  { nopi | nopi | nopi | nopi }                                                 # 0576
  { nopi | nopi | bge rz, t5, .L_crypto_fin_cifra_2 | nopi }                    # 0577
.L_crypto_cifra_2:                                                              # bundle 578
  { nopi | nopi | nopi | lw t0, 0(t2) }                                         # 0578
  { nopi | nopi | nopi | lw t1, 4(t2) }                                         # 0579
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0580
  { nopi | feis t0, t0, t1, 1, 0 | nopi | nopi }                                # 0581
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0582
  { nopi | feis t1, t1, t0, 1, 1 | nopi | nopi }                                # 0583
  { nopi | nopi | nopi | nopi }                                                 # 0584
  { nopi | feis t0, t0, t1, 1, 2 | nopi | nopi }                                # 0585
  { nopi | nopi | nopi | nopi }                                                 # 0586
  { nopi | feis t1, t1, t0, 1, 3 | nopi | nopi }                                # 0587
  { nopi | nopi | nopi | sw t0, 0(t3) }                                         # 0588
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_cifra_2 | sw t1, 4(t3) }     # 0589
.L_crypto_fin_cifra_2:                                                          # bundle 590
  { nopi | nopi | nopi | nopi }                                                 # 0590
.L_if_end_6:                                                                    # bundle 591
.L_for_update_3:                                                                # bundle 591
  { nopi | nopi | nopi | lw t0, 228(rz) }                                       # 0591  linea 35
  { nopi | nopi | nopi | nopi }                                                 # 0592
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 0593
  { nopi | nopi | nopi | nopi }                                                 # 0594
  { nopi | nopi | jef .L_for_start_2 | sw t0, 228(rz) }                         # 0595
  { nopi | nopi | nopi | nopi }                                                 # 0596
.L_for_end_4:                                                                   # bundle 597
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0597  linea 43
  { nopi | nopi | nopi | nopi }                                                 # 0598
.L_func_descifrar_archivo:                                                      # bundle 599
  { nopi | nopi | nopi | sw rz, 292(rz) }                                       # 0599  linea 46
  { nopi | nopi | nopi | nopi }                                                 # 0600
.L_body_descifrar_archivo:                                                      # bundle 601
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0601  linea 47
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0602
  { nopi | nopi | nopi | nopi }                                                 # 0603
  { add t0, t0, t1 | nopi | nopi | nopi }                                       # 0604
  { mov t1, 1 | nopi | nopi | nopi }                                            # 0605
  { nopi | nopi | nopi | nopi }                                                 # 0606
  { sub t0, t0, t1 | nopi | nopi | nopi }                                       # 0607
  { nopi | nopi | nopi | lw t1, 308(rz) }                                       # 0608
  { nopi | nopi | nopi | nopi }                                                 # 0609
  { nopi | nopi | beq r5, r0, .L_arith_error | nopi }                           # 0610
  { nopi | nopi | nopi | nopi }                                                 # 0611
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0612
  { nopi | nopi | nopi | nopi }                                                 # 0613
  { nopi | nopi | bge r7, r0, __ccom_arith_19_apos | nopi }                     # 0614
  { nopi | nopi | nopi | nopi }                                                 # 0615
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0616
  { nopi | nopi | nopi | nopi }                                                 # 0617
__ccom_arith_19_apos:                                                           # bundle 618
  { nopi | nopi | bge r8, r0, __ccom_arith_20_bpos | nopi }                     # 0618
  { nopi | nopi | nopi | nopi }                                                 # 0619
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0620
  { nopi | nopi | nopi | nopi }                                                 # 0621
__ccom_arith_20_bpos:                                                           # bundle 622
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0622
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0623
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0624
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0625
  { nopi | nopi | nopi | nopi }                                                 # 0626
__ccom_arith_21_div:                                                            # bundle 627
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0627
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0628
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0629
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0630
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0631
  { nopi | nopi | nopi | nopi }                                                 # 0632
  { nopi | nopi | blt r13, r12, __ccom_arith_22_skip | nopi }                   # 0633
  { nopi | nopi | nopi | nopi }                                                 # 0634
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0635
  { nopi | nopi | nopi | nopi }                                                 # 0636
__ccom_arith_22_skip:                                                           # bundle 637
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0637
  { nopi | nopi | nopi | nopi }                                                 # 0638
  { nopi | nopi | bne r10, r0, __ccom_arith_21_div | nopi }                     # 0639
  { nopi | nopi | nopi | nopi }                                                 # 0640
  { xor r13, r4, r5 | nopi | nopi | nopi }                                      # 0641
  { nopi | nopi | nopi | nopi }                                                 # 0642
  { nopi | nopi | bge r13, r0, __ccom_arith_23_end | nopi }                     # 0643
  { nopi | nopi | nopi | nopi }                                                 # 0644
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0645
  { nopi | nopi | nopi | nopi }                                                 # 0646
__ccom_arith_23_end:                                                            # bundle 647
  { mov t0, 0 | nopi | nopi | sw t2, 292(rz) }                                  # 0647  linea 48  slots 0=48,1=0,2=0,3=47
  { nopi | nopi | nopi | lw t1, 292(rz) }                                       # 0648
  { nopi | nopi | nopi | sw t0, 312(rz) }                                       # 0649
  { nopi | nopi | nopi | sw t1, 316(rz) }                                       # 0650
  { nopi | nopi | nopi | nopi }                                                 # 0651
.L_for_start_7:                                                                 # bundle 652
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0652
  { nopi | nopi | nopi | lw t1, 316(rz) }                                       # 0653
  { nopi | nopi | nopi | nopi }                                                 # 0654
  { nopi | nopi | bge t0, t1, .L_for_end_9 | nopi }                             # 0655
  { nopi | nopi | nopi | nopi }                                                 # 0656
  { nopi | nopi | nopi | lw t0, 304(rz) }                                       # 0657  linea 49
  { nopi | nopi | nopi | nopi }                                                 # 0658
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0659
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0660
  { nopi | nopi | nopi | nopi }                                                 # 0661
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0662
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0663
  { nopi | nopi | nopi | nopi }                                                 # 0664
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0665
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0666
  { nopi | nopi | nopi | nopi }                                                 # 0667
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0668
  { nopi | nopi | nopi | nopi }                                                 # 0669
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0670
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0671
  { nopi | nopi | nopi | sw t0, 28(rz) }                                        # 0672
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0673
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0674
  { nopi | nopi | nopi | sw t0, 24(rz) }                                        # 0675
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0676
  { nopi | nopi | nopi | nopi }                                                 # 0677
  { nopi | nopi | nopi | sw t0, 20(rz) }                                        # 0678
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0679
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_bloques_de | nopi }                # 0680
  { nopi | nopi | nopi | nopi }                                                 # 0681
  { saddi rx, rx, -4 | mov t1, 2 | nopi | nopi }                                # 0682  slots 0=49,1=50,2=0,3=0
  { nopi | nopi | nopi | nopi }                                                 # 0683
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0684
  { nopi | nopi | nopi | lw t0, 16(rz) }                                        # 0685
  { nopi | nopi | nopi | nopi }                                                 # 0686
  { nopi | nopi | nopi | sw t0, 320(rz) }                                       # 0687
  { nopi | nopi | beq r5, r0, .L_arith_error | lw t0, 312(rz) }                 # 0688  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 0689
  { add r7, r4, r0 | add r8, r5, r0 | nopi | nopi }                             # 0690
  { nopi | nopi | nopi | nopi }                                                 # 0691
  { nopi | nopi | bge r7, r0, __ccom_arith_24_apos | nopi }                     # 0692
  { nopi | nopi | nopi | nopi }                                                 # 0693
  { sub r7, r0, r7 | nopi | nopi | nopi }                                       # 0694
  { nopi | nopi | nopi | nopi }                                                 # 0695
__ccom_arith_24_apos:                                                           # bundle 696
  { nopi | nopi | bge r8, r0, __ccom_arith_25_bpos | nopi }                     # 0696
  { nopi | nopi | nopi | nopi }                                                 # 0697
  { sub r8, r0, r8 | nopi | nopi | nopi }                                       # 0698
  { nopi | nopi | nopi | nopi }                                                 # 0699
__ccom_arith_25_bpos:                                                           # bundle 700
  { mov r11, 1 | mov r10, 31 | nopi | nopi }                                    # 0700
  { mov r9, 0 | mov r6, 0 | nopi | nopi }                                       # 0701
  { sll r14, r11, r10 | nopi | nopi | nopi }                                    # 0702
  { mov r10, 32 | nopi | nopi | nopi }                                          # 0703
  { nopi | nopi | nopi | nopi }                                                 # 0704
__ccom_arith_26_div:                                                            # bundle 705
  { slt r12, r7, r0 | sll r9, r9, r11 | nopi | nopi }                           # 0705
  { sll r7, r7, r11 | sll r6, r6, r11 | nopi | nopi }                           # 0706
  { or r9, r9, r12 | nopi | nopi | nopi }                                       # 0707
  { xor r12, r8, r14 | nopi | nopi | nopi }                                     # 0708
  { xor r13, r9, r14 | nopi | nopi | nopi }                                     # 0709
  { nopi | nopi | nopi | nopi }                                                 # 0710
  { nopi | nopi | blt r13, r12, __ccom_arith_27_skip | nopi }                   # 0711
  { nopi | nopi | nopi | nopi }                                                 # 0712
  { sub r9, r9, r8 | saddi r6, r6, 1 | nopi | nopi }                            # 0713
  { nopi | nopi | nopi | nopi }                                                 # 0714
__ccom_arith_27_skip:                                                           # bundle 715
  { saddi r10, r10, -1 | nopi | nopi | nopi }                                   # 0715
  { nopi | nopi | nopi | nopi }                                                 # 0716
  { nopi | nopi | bne r10, r0, __ccom_arith_26_div | nopi }                     # 0717
  { nopi | nopi | nopi | nopi }                                                 # 0718
  { add r6, r9, r0 | nopi | bge r4, r0, __ccom_arith_28_end | nopi }            # 0719
  { nopi | nopi | nopi | nopi }                                                 # 0720
  { sub r6, r0, r6 | nopi | nopi | nopi }                                       # 0721
  { nopi | nopi | nopi | nopi }                                                 # 0722
__ccom_arith_28_end:                                                            # bundle 723
  { mov t0, 0 | nopi | nopi | nopi }                                            # 0723
  { nopi | nopi | nopi | nopi }                                                 # 0724
  { seq t2, t2, t0 | nopi | nopi | nopi }                                       # 0725
  { nopi | nopi | nopi | nopi }                                                 # 0726
  { nopi | nopi | beq t2, rz, .L_if_next_10 | nopi }                            # 0727
  { nopi | nopi | nopi | nopi }                                                 # 0728
  { nopi | nopi | nopi | nopi }                                                 # 0729
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 0730  linea 51
  { nopi | nopi | nopi | nopi }                                                 # 0731
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0732
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0733
  { nopi | nopi | nopi | nopi }                                                 # 0734
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0735
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0736
  { nopi | nopi | nopi | nopi }                                                 # 0737
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0738
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0739
  { nopi | nopi | nopi | nopi }                                                 # 0740
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0741
  { nopi | nopi | nopi | nopi }                                                 # 0742
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0743
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0744
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0745
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0746
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0747
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0748
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0749
  { nopi | nopi | nopi | nopi }                                                 # 0750
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0751
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0752
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0753
  { nopi | nopi | nopi | nopi }                                                 # 0754
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0755
  { nopi | nopi | nopi | nopi }                                                 # 0756
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0757
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0758
  { nopi | nopi | nopi | nopi }                                                 # 0759
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0760
  { nopi | nopi | nopi | nopi }                                                 # 0761
  { nopi | nopi | nopi | nopi }                                                 # 0762
  { nopi | nopi | nopi | nopi }                                                 # 0763
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0764
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 0765
  { nopi | nopi | nopi | nopi }                                                 # 0766
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0767
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0768
  { nopi | nopi | nopi | nopi }                                                 # 0769
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0770
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0771
  { nopi | nopi | nopi | nopi }                                                 # 0772
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0773
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0774
  { nopi | nopi | nopi | nopi }                                                 # 0775
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0776
  { nopi | nopi | nopi | nopi }                                                 # 0777
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0778
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0779
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0780
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0781
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0782
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0783
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0784
  { nopi | nopi | nopi | nopi }                                                 # 0785
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0786
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0787
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0788
  { nopi | nopi | nopi | nopi }                                                 # 0789
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0790
  { nopi | nopi | nopi | nopi }                                                 # 0791
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0792
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0793
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0794
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0795
  { nopi | nopi | nopi | nopi }                                                 # 0796
  { nopi | nopi | nopi | nopi }                                                 # 0797
  { nopi | nopi | nopi | nopi }                                                 # 0798
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0799
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 0800
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 0801
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0802
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 328(rz) }                            # 0803
  { nopi | nopi | nopi | nopi }                                                 # 0804
  { nopi | nopi | nopi | nopi }                                                 # 0805
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0806
  { nopi | nopi | nopi | nopi }                                                 # 0807
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_3 | nopi }                 # 0808
.L_crypto_descifra_3:                                                           # bundle 809
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 0809
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 0810
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0811
  { nopi | feis t0, t0, t1, 0, 3 | nopi | nopi }                                # 0812
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0813
  { nopi | feis t1, t1, t0, 0, 2 | nopi | nopi }                                # 0814
  { nopi | nopi | nopi | nopi }                                                 # 0815
  { nopi | feis t0, t0, t1, 0, 1 | nopi | nopi }                                # 0816
  { nopi | nopi | nopi | nopi }                                                 # 0817
  { nopi | feis t1, t1, t0, 0, 0 | nopi | nopi }                                # 0818
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 0819
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_3 | sw t1, 0(t3) }  # 0820
.L_crypto_fin_descifra_3:                                                       # bundle 821
  { nopi | nopi | nopi | nopi }                                                 # 0821
  { nopi | nopi | jef .L_if_end_11 | nopi }                                     # 0822  linea 50
  { nopi | nopi | nopi | nopi }                                                 # 0823
.L_if_next_10:                                                                  # bundle 824
  { nopi | nopi | nopi | nopi }                                                 # 0824
  { nopi | nopi | nopi | lw t0, 296(rz) }                                       # 0825  linea 53
  { nopi | nopi | nopi | nopi }                                                 # 0826
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0827
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0828
  { nopi | nopi | nopi | nopi }                                                 # 0829
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0830
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0831
  { nopi | nopi | nopi | nopi }                                                 # 0832
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0833
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0834
  { nopi | nopi | nopi | nopi }                                                 # 0835
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0836
  { nopi | nopi | nopi | nopi }                                                 # 0837
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0838
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0839
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0840
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0841
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0842
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0843
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0844
  { nopi | nopi | nopi | nopi }                                                 # 0845
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0846
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0847
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0848
  { nopi | nopi | nopi | nopi }                                                 # 0849
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0850
  { nopi | nopi | nopi | nopi }                                                 # 0851
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0852
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0853
  { nopi | nopi | nopi | nopi }                                                 # 0854
  { saddi t2, t0, 0 | nopi | nopi | nopi }                                      # 0855
  { nopi | nopi | nopi | nopi }                                                 # 0856
  { nopi | nopi | nopi | nopi }                                                 # 0857
  { nopi | nopi | nopi | nopi }                                                 # 0858
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0859
  { nopi | nopi | nopi | lw t0, 300(rz) }                                       # 0860
  { nopi | nopi | nopi | nopi }                                                 # 0861
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0862
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 308(rz) }                            # 0863
  { nopi | nopi | nopi | nopi }                                                 # 0864
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0865
  { saddi rx, rx, 4 | nopi | nopi | lw t0, 312(rz) }                            # 0866
  { nopi | nopi | nopi | nopi }                                                 # 0867
  { nopi | nopi | nopi | sw t0, 0(rx) }                                         # 0868
  { saddi rx, rx, 4 | nopi | nopi | nopi }                                      # 0869
  { nopi | nopi | nopi | nopi }                                                 # 0870
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0871
  { nopi | nopi | nopi | nopi }                                                 # 0872
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0873
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0874
  { nopi | nopi | nopi | sw t0, 100(rz) }                                       # 0875
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0876
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0877
  { nopi | nopi | nopi | sw t0, 96(rz) }                                        # 0878
  { nopi | nopi | nopi | lw t0, 0(rx) }                                         # 0879
  { nopi | nopi | nopi | nopi }                                                 # 0880
  { nopi | nopi | nopi | sw t0, 92(rz) }                                        # 0881
  { nopi | nopi | nopi | sw rl, 0(rx) }                                         # 0882
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_direccion | nopi }                 # 0883
  { nopi | nopi | nopi | nopi }                                                 # 0884
  { saddi rx, rx, -4 | nopi | nopi | nopi }                                     # 0885
  { nopi | nopi | nopi | nopi }                                                 # 0886
  { nopi | nopi | nopi | lw rl, 0(rx) }                                         # 0887
  { nopi | nopi | nopi | lw t0, 88(rz) }                                        # 0888
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0889
  { saddi t3, t0, 0 | nopi | nopi | nopi }                                      # 0890
  { nopi | nopi | nopi | nopi }                                                 # 0891
  { nopi | nopi | nopi | nopi }                                                 # 0892
  { nopi | nopi | nopi | nopi }                                                 # 0893
  { nopi | nopi | nopi | sw t2, 324(rz) }                                       # 0894
  { nopi | nopi | nopi | sw t3, 328(rz) }                                       # 0895
  { nopi | nopi | nopi | lw t0, 320(rz) }                                       # 0896
  { nopi | nopi | nopi | lw t2, 324(rz) }                                       # 0897
  { saddi t5, t0, 0 | nopi | nopi | lw t3, 328(rz) }                            # 0898
  { nopi | nopi | nopi | nopi }                                                 # 0899
  { nopi | nopi | nopi | nopi }                                                 # 0900
  { mov t4, 0 | nopi | nopi | nopi }                                            # 0901
  { nopi | nopi | nopi | nopi }                                                 # 0902
  { nopi | nopi | bge rz, t5, .L_crypto_fin_descifra_4 | nopi }                 # 0903
.L_crypto_descifra_4:                                                           # bundle 904
  { nopi | nopi | nopi | lw t0, 4(t2) }                                         # 0904
  { nopi | nopi | nopi | lw t1, 0(t2) }                                         # 0905
  { saddi t2, t2, 8 | nopi | nopi | nopi }                                      # 0906
  { nopi | feis t0, t0, t1, 1, 3 | nopi | nopi }                                # 0907
  { saddi t4, t4, 1 | nopi | nopi | nopi }                                      # 0908
  { nopi | feis t1, t1, t0, 1, 2 | nopi | nopi }                                # 0909
  { nopi | nopi | nopi | nopi }                                                 # 0910
  { nopi | feis t0, t0, t1, 1, 1 | nopi | nopi }                                # 0911
  { nopi | nopi | nopi | nopi }                                                 # 0912
  { nopi | feis t1, t1, t0, 1, 0 | nopi | nopi }                                # 0913
  { nopi | nopi | nopi | sw t0, 4(t3) }                                         # 0914
  { saddi t3, t3, 8 | nopi | blt t4, t5, .L_crypto_descifra_4 | sw t1, 0(t3) }  # 0915
.L_crypto_fin_descifra_4:                                                       # bundle 916
  { nopi | nopi | nopi | nopi }                                                 # 0916
.L_if_end_11:                                                                   # bundle 917
.L_for_update_8:                                                                # bundle 917
  { nopi | nopi | nopi | lw t0, 312(rz) }                                       # 0917  linea 48
  { nopi | nopi | nopi | nopi }                                                 # 0918
  { saddi t0, t0, 1 | nopi | nopi | nopi }                                      # 0919
  { nopi | nopi | nopi | nopi }                                                 # 0920
  { nopi | nopi | jef .L_for_start_7 | sw t0, 312(rz) }                         # 0921
  { nopi | nopi | nopi | nopi }                                                 # 0922
.L_for_end_9:                                                                   # bundle 923
  { nopi | nopi | jefR rz, rl | nopi }                                          # 0923  linea 56
  { nopi | nopi | nopi | nopi }                                                 # 0924
fin_main:                                                                       # bundle 925
  { nopi | nopi | jef fin_main | nopi }                                         # 0925  linea -
.L_arith_error:                                                                 # bundle 926
  { nopi | nopi | jef .L_arith_error | nopi }                                   # 0926

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_bloques_de             106
#   .L_body_bloques_de             108
#   __ccom_arith_0_mul             114  (interna)
#   __ccom_arith_1_skip            122  (interna)
#   __ccom_arith_2_end             124  (interna)
#   .L_if_next_0                   138
#   .L_if_end_1                    142
#   .L_func_direccion              144
#   .L_body_direccion              146
#   __ccom_arith_3_mul             152  (interna)
#   __ccom_arith_4_skip            160  (interna)
#   __ccom_arith_5_end             162  (interna)
#   __ccom_arith_6_mul             166  (interna)
#   __ccom_arith_7_skip            174  (interna)
#   __ccom_arith_8_end             176  (interna)
#   .L_func_preparar_boveda        180
#   .L_body_preparar_boveda        180
#   .L_func_cifrar_archivo         273
#   .L_body_cifrar_archivo         275
#   __ccom_arith_9_apos            292  (interna)
#   __ccom_arith_10_bpos           296  (interna)
#   __ccom_arith_11_div            301  (interna)
#   __ccom_arith_12_skip           311  (interna)
#   __ccom_arith_13_end            321  (interna)
#   .L_for_start_2                 326
#   __ccom_arith_14_apos           370  (interna)
#   __ccom_arith_15_bpos           374  (interna)
#   __ccom_arith_16_div            379  (interna)
#   __ccom_arith_17_skip           389  (interna)
#   __ccom_arith_18_end            397  (interna)
#   .L_crypto_cifra_1              483  (interna)
#   .L_crypto_fin_cifra_1          495  (interna)
#   .L_if_next_5                   498
#   .L_crypto_cifra_2              578  (interna)
#   .L_crypto_fin_cifra_2          590  (interna)
#   .L_if_end_6                    591
#   .L_for_update_3                591
#   .L_for_end_4                   597
#   .L_func_descifrar_archivo      599
#   .L_body_descifrar_archivo      601
#   __ccom_arith_19_apos           618  (interna)
#   __ccom_arith_20_bpos           622  (interna)
#   __ccom_arith_21_div            627  (interna)
#   __ccom_arith_22_skip           637  (interna)
#   __ccom_arith_23_end            647  (interna)
#   .L_for_start_7                 652
#   __ccom_arith_24_apos           696  (interna)
#   __ccom_arith_25_bpos           700  (interna)
#   __ccom_arith_26_div            705  (interna)
#   __ccom_arith_27_skip           715  (interna)
#   __ccom_arith_28_end            723  (interna)
#   .L_crypto_descifra_3           809  (interna)
#   .L_crypto_fin_descifra_3       821  (interna)
#   .L_if_next_10                  824
#   .L_crypto_descifra_4           904  (interna)
#   .L_crypto_fin_descifra_4       916  (interna)
#   .L_if_end_11                   917
#   .L_for_update_8                917
#   .L_for_end_9                   923
#   fin_main                       925
#   .L_arith_error                 926

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
