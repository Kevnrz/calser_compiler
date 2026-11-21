# ccom: pruebas/optimizador/spill/s2_lazo.cls
  { mov rx, 228 | nopi | nopi | nopi }                                    # 0000
  { nopi | nopi | nopi | nopi }                                           # 0001
.L_func_main:                                                             # bundle 2
.L_o1_5:                                                                  # bundle 2
  { nopi | nopi | jef .L_o1_6 | nopi }                                    # 0002  linea 13
  { nopi | nopi | nopi | nopi }                                           # 0003
.L_o1_6:                                                                  # bundle 4
  { mov t1, 1 | mov t0, 2 | nopi | nopi }                                 # 0004  linea 14
  { mov t11, 1 | mov t10, 2 | nopi | nopi }                               # 0005
  { mov t9, 1 | mov t8, 2 | nopi | sw t1, 8(rz) }                         # 0006
  { mov t7, 1 | mov t6, 2 | nopi | sw t0, 12(rz) }                        # 0007
  { mov t5, 1 | mov t4, 2 | nopi | sw t11, 16(rz) }                       # 0008
  { mov t3, 1 | mov t2, 2 | nopi | sw t10, 20(rz) }                       # 0009
  { mov t1, 1 | mov t0, 2 | nopi | sw t9, 24(rz) }                        # 0010
  { mov t11, 1 | mov t10, 2 | nopi | sw t8, 28(rz) }                      # 0011
  { mov t9, 1 | mov t8, 2 | nopi | sw t7, 32(rz) }                        # 0012
  { mov t7, 1 | nopi | nopi | sw t6, 36(rz) }                             # 0013
  { mov t6, 2 | nopi | nopi | sw t5, 40(rz) }                             # 0014
  { mov t5, 1 | nopi | nopi | sw t4, 44(rz) }                             # 0015
  { mov t4, 2 | nopi | nopi | sw t3, 48(rz) }                             # 0016
  { mov t3, 1 | nopi | nopi | sw t2, 52(rz) }                             # 0017
  { mov t2, 2 | nopi | nopi | sw t1, 56(rz) }                             # 0018
  { mov t1, 1 | nopi | nopi | sw t0, 60(rz) }                             # 0019
  { mov t0, 2 | nopi | nopi | sw t11, 64(rz) }                            # 0020
  { nopi | nopi | nopi | sw t10, 68(rz) }                                 # 0021
  { nopi | nopi | nopi | sw t9, 72(rz) }                                  # 0022
  { nopi | nopi | nopi | sw t8, 76(rz) }                                  # 0023
  { nopi | nopi | nopi | sw t7, 80(rz) }                                  # 0024
  { nopi | nopi | nopi | sw t6, 84(rz) }                                  # 0025
  { nopi | nopi | nopi | sw t5, 88(rz) }                                  # 0026
  { nopi | nopi | nopi | sw t4, 92(rz) }                                  # 0027
  { nopi | nopi | nopi | sw t3, 96(rz) }                                  # 0028
  { nopi | nopi | nopi | sw t2, 100(rz) }                                 # 0029
  { nopi | nopi | nopi | sw t1, 104(rz) }                                 # 0030
  { nopi | nopi | nopi | sw t0, 108(rz) }                                 # 0031
  { nopi | nopi | nopi | sw rl, 0(rx) }                                   # 0032
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_f | nopi }                   # 0033
  { nopi | nopi | nopi | nopi }                                           # 0034
  { saddi rx, rx, -4 | nopi | nopi | nopi }                               # 0035
  { nopi | nopi | nopi | nopi }                                           # 0036
  { nopi | nopi | jef fin_main | lw rl, 0(rx) }                           # 0037  linea 15  slots 0=0,1=0,2=15,3=14
  { nopi | nopi | nopi | nopi }                                           # 0038
.L_func_f:                                                                # bundle 39
.L_o1_0:                                                                  # bundle 39
  { nopi | nopi | jef .L_o1_1 | nopi }                                    # 0039  linea 3
  { nopi | nopi | nopi | nopi }                                           # 0040
.L_o1_1:                                                                  # bundle 41
  { mov t11, 3 | nopi | nopi | sw rz, 112(rz) }                           # 0041  linea 6
  { nopi | nopi | nopi | nopi }                                           # 0042
  { nopi | nopi | jef .L_o1_2 | sw t11, 116(rz) }                         # 0043
  { nopi | nopi | nopi | nopi }                                           # 0044
.L_o1_2:                                                                  # bundle 45
  { nopi | nopi | nopi | lw t7, 112(rz) }                                 # 0045
  { nopi | nopi | nopi | lw t1, 116(rz) }                                 # 0046
  { nopi | nopi | nopi | nopi }                                           # 0047
  { nopi | nopi | bge t7, t1, .L_o1_4 | nopi }                            # 0048
  { nopi | nopi | nopi | nopi }                                           # 0049
.L_o1_3:                                                                  # bundle 50
  { mov r9, 1 | mov r7, 0 | nopi | lw t10, 8(rz) }                        # 0050  linea 7
  { nopi | nopi | nopi | lw t4, 12(rz) }                                  # 0051
  { add r4, r14, r0 | nopi | nopi | nopi }                                # 0052
  { add r5, r8, r0 | nopi | nopi | nopi }                                 # 0053
  { nopi | nopi | nopi | nopi }                                           # 0054
__ccom_arith_0_mul:                                                       # bundle 55
  { nopi | nopi | beq r5, r0, __ccom_arith_2_end | nopi }                 # 0055
  { nopi | nopi | nopi | nopi }                                           # 0056
  { and r6, r5, r9 | nopi | nopi | nopi }                                 # 0057
  { nopi | nopi | nopi | nopi }                                           # 0058
  { nopi | nopi | beq r6, r0, __ccom_arith_1_skip | nopi }                # 0059
  { nopi | nopi | nopi | nopi }                                           # 0060
  { add r7, r7, r4 | nopi | nopi | nopi }                                 # 0061
  { nopi | nopi | nopi | nopi }                                           # 0062
__ccom_arith_1_skip:                                                      # bundle 63
  { sll r4, r4, r9 | srl r5, r5, r9 | jef __ccom_arith_0_mul | nopi }     # 0063
  { nopi | nopi | nopi | nopi }                                           # 0064
__ccom_arith_2_end:                                                       # bundle 65
  { mov r9, 1 | mov r5, 0 | nopi | lw t6, 0(rz) }                         # 0065
  { nopi | nopi | nopi | nopi }                                           # 0066
  { add rg, t6, t3 | nopi | nopi | nopi }                                 # 0067
  { nopi | nopi | nopi | nopi }                                           # 0068
  { nopi | nopi | nopi | sw rg, 224(rz) }                                 # 0069
  { nopi | nopi | nopi | lw t9, 16(rz) }                                  # 0070
  { nopi | nopi | nopi | lw t0, 20(rz) }                                  # 0071
  { add r6, r13, r0 | nopi | nopi | nopi }                                # 0072
  { add r7, r4, r0 | nopi | nopi | nopi }                                 # 0073
  { nopi | nopi | nopi | nopi }                                           # 0074
__ccom_arith_3_mul:                                                       # bundle 75
  { nopi | nopi | beq r7, r0, __ccom_arith_5_end | nopi }                 # 0075
  { nopi | nopi | nopi | nopi }                                           # 0076
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0077
  { nopi | nopi | nopi | nopi }                                           # 0078
  { nopi | nopi | beq r8, r0, __ccom_arith_4_skip | nopi }                # 0079
  { nopi | nopi | nopi | nopi }                                           # 0080
  { add r5, r5, r6 | nopi | nopi | nopi }                                 # 0081
  { nopi | nopi | nopi | nopi }                                           # 0082
__ccom_arith_4_skip:                                                      # bundle 83
  { sll r6, r6, r9 | srl r7, r7, r9 | jef __ccom_arith_3_mul | nopi }     # 0083
  { nopi | nopi | nopi | nopi }                                           # 0084
__ccom_arith_5_end:                                                       # bundle 85
  { mov r9, 1 | mov r4, 0 | nopi | sw t1, 220(rz) }                       # 0085
  { nopi | nopi | nopi | lw t2, 24(rz) }                                  # 0086
  { nopi | nopi | nopi | lw t8, 28(rz) }                                  # 0087
  { add r5, r6, r0 | nopi | nopi | nopi }                                 # 0088
  { add r7, r12, r0 | nopi | nopi | nopi }                                # 0089
  { nopi | nopi | nopi | nopi }                                           # 0090
__ccom_arith_6_mul:                                                       # bundle 91
  { nopi | nopi | beq r7, r0, __ccom_arith_8_end | nopi }                 # 0091
  { nopi | nopi | nopi | nopi }                                           # 0092
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0093
  { nopi | nopi | nopi | nopi }                                           # 0094
  { nopi | nopi | beq r8, r0, __ccom_arith_7_skip | nopi }                # 0095
  { nopi | nopi | nopi | nopi }                                           # 0096
  { add r4, r4, r5 | nopi | nopi | nopi }                                 # 0097
  { nopi | nopi | nopi | nopi }                                           # 0098
__ccom_arith_7_skip:                                                      # bundle 99
  { sll r5, r5, r9 | srl r7, r7, r9 | jef __ccom_arith_6_mul | nopi }     # 0099
  { nopi | nopi | nopi | nopi }                                           # 0100
__ccom_arith_8_end:                                                       # bundle 101
  { mov r8, 1 | mov r15, 0 | nopi | sw t0, 216(rz) }                      # 0101
  { nopi | nopi | nopi | lw t5, 32(rz) }                                  # 0102
  { nopi | nopi | nopi | lw t3, 36(rz) }                                  # 0103
  { add r4, r9, r0 | nopi | nopi | nopi }                                 # 0104
  { add r5, r7, r0 | nopi | nopi | nopi }                                 # 0105
  { nopi | nopi | nopi | nopi }                                           # 0106
__ccom_arith_9_mul:                                                       # bundle 107
  { nopi | nopi | beq r5, r0, __ccom_arith_11_end | nopi }                # 0107
  { nopi | nopi | nopi | nopi }                                           # 0108
  { and r6, r5, r8 | nopi | nopi | nopi }                                 # 0109
  { nopi | nopi | nopi | nopi }                                           # 0110
  { nopi | nopi | beq r6, r0, __ccom_arith_10_skip | nopi }               # 0111
  { nopi | nopi | nopi | nopi }                                           # 0112
  { add r15, r15, r4 | nopi | nopi | nopi }                               # 0113
  { nopi | nopi | nopi | nopi }                                           # 0114
__ccom_arith_10_skip:                                                     # bundle 115
  { sll r4, r4, r8 | srl r5, r5, r8 | jef __ccom_arith_9_mul | nopi }     # 0115
  { nopi | nopi | nopi | nopi }                                           # 0116
__ccom_arith_11_end:                                                      # bundle 117
  { mov r8, 1 | mov r14, 0 | nopi | lw t7, 40(rz) }                       # 0117
  { nopi | nopi | nopi | lw t1, 44(rz) }                                  # 0118
  { add r4, r11, r0 | nopi | nopi | nopi }                                # 0119
  { add r6, r5, r0 | nopi | nopi | nopi }                                 # 0120
  { nopi | nopi | nopi | nopi }                                           # 0121
__ccom_arith_12_mul:                                                      # bundle 122
  { nopi | nopi | beq r6, r0, __ccom_arith_14_end | nopi }                # 0122
  { nopi | nopi | nopi | nopi }                                           # 0123
  { and r7, r6, r8 | nopi | nopi | nopi }                                 # 0124
  { nopi | nopi | nopi | nopi }                                           # 0125
  { nopi | nopi | beq r7, r0, __ccom_arith_13_skip | nopi }               # 0126
  { nopi | nopi | nopi | nopi }                                           # 0127
  { add r14, r14, r4 | nopi | nopi | nopi }                               # 0128
  { nopi | nopi | nopi | nopi }                                           # 0129
__ccom_arith_13_skip:                                                     # bundle 130
  { sll r4, r4, r8 | srl r6, r6, r8 | jef __ccom_arith_12_mul | nopi }    # 0130
  { nopi | nopi | nopi | nopi }                                           # 0131
__ccom_arith_14_end:                                                      # bundle 132
  { nopi | nopi | nopi | lw t4, 48(rz) }                                  # 0132
  { nopi | nopi | nopi | lw t6, 52(rz) }                                  # 0133
  { nopi | nopi | nopi | nopi }                                           # 0134
  { nopi | nopi | nopi | nopi }                                           # 0135
  { mov r7, 1 | add r4, r8, r0 | nopi | sw r14, 160(rz) }                 # 0136
  { add r5, r10, r0 | mov r13, 0 | nopi | nopi }                          # 0137
  { nopi | nopi | nopi | nopi }                                           # 0138
__ccom_arith_15_mul:                                                      # bundle 139
  { nopi | nopi | beq r5, r0, __ccom_arith_17_end | nopi }                # 0139
  { nopi | nopi | nopi | nopi }                                           # 0140
  { and r6, r5, r7 | nopi | nopi | nopi }                                 # 0141
  { nopi | nopi | nopi | nopi }                                           # 0142
  { nopi | nopi | beq r6, r0, __ccom_arith_16_skip | nopi }               # 0143
  { nopi | nopi | nopi | nopi }                                           # 0144
  { add r13, r13, r4 | nopi | nopi | nopi }                               # 0145
  { nopi | nopi | nopi | nopi }                                           # 0146
__ccom_arith_16_skip:                                                     # bundle 147
  { sll r4, r4, r7 | srl r5, r5, r7 | jef __ccom_arith_15_mul | nopi }    # 0147
  { nopi | nopi | nopi | nopi }                                           # 0148
__ccom_arith_17_end:                                                      # bundle 149
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0149
  { nopi | nopi | nopi | lw t0, 56(rz) }                                  # 0150
  { nopi | nopi | nopi | lw t2, 60(rz) }                                  # 0151
  { nopi | nopi | nopi | nopi }                                           # 0152
  { nopi | nopi | nopi | nopi }                                           # 0153
  { mov r9, 1 | add r5, r4, r0 | nopi | sw r13, 156(rz) }                 # 0154
  { add r7, r6, r0 | mov r12, 0 | nopi | sw r14, 160(rz) }                # 0155
  { nopi | nopi | nopi | nopi }                                           # 0156
__ccom_arith_18_mul:                                                      # bundle 157
  { nopi | nopi | beq r7, r0, __ccom_arith_20_end | nopi }                # 0157
  { nopi | nopi | nopi | nopi }                                           # 0158
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0159
  { nopi | nopi | nopi | nopi }                                           # 0160
  { nopi | nopi | beq r8, r0, __ccom_arith_19_skip | nopi }               # 0161
  { nopi | nopi | nopi | nopi }                                           # 0162
  { add r12, r12, r5 | nopi | nopi | nopi }                               # 0163
  { nopi | nopi | nopi | nopi }                                           # 0164
__ccom_arith_19_skip:                                                     # bundle 165
  { sll r5, r5, r9 | srl r7, r7, r9 | jef __ccom_arith_18_mul | nopi }    # 0165
  { nopi | nopi | nopi | nopi }                                           # 0166
__ccom_arith_20_end:                                                      # bundle 167
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0167
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0168
  { nopi | nopi | nopi | lw t5, 64(rz) }                                  # 0169  linea 8
  { nopi | nopi | nopi | lw t3, 68(rz) }                                  # 0170
  { nopi | nopi | nopi | nopi }                                           # 0171
  { nopi | nopi | nopi | nopi }                                           # 0172
  { mov r8, 1 | add r4, r9, r0 | nopi | sw r12, 152(rz) }                 # 0173
  { add r5, r7, r0 | mov r11, 0 | nopi | sw r13, 156(rz) }                # 0174
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0175
  { nopi | nopi | nopi | nopi }                                           # 0176
__ccom_arith_21_mul:                                                      # bundle 177
  { nopi | nopi | beq r5, r0, __ccom_arith_23_end | nopi }                # 0177
  { nopi | nopi | nopi | nopi }                                           # 0178
  { and r6, r5, r8 | nopi | nopi | nopi }                                 # 0179
  { nopi | nopi | nopi | nopi }                                           # 0180
  { nopi | nopi | beq r6, r0, __ccom_arith_22_skip | nopi }               # 0181
  { nopi | nopi | nopi | nopi }                                           # 0182
  { add r11, r11, r4 | nopi | nopi | nopi }                               # 0183
  { nopi | nopi | nopi | nopi }                                           # 0184
__ccom_arith_22_skip:                                                     # bundle 185
  { sll r4, r4, r8 | srl r5, r5, r8 | jef __ccom_arith_21_mul | nopi }    # 0185
  { nopi | nopi | nopi | nopi }                                           # 0186
__ccom_arith_23_end:                                                      # bundle 187
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0187
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0188
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0189
  { nopi | nopi | nopi | lw t1, 72(rz) }                                  # 0190
  { nopi | nopi | nopi | lw t4, 76(rz) }                                  # 0191
  { nopi | nopi | nopi | nopi }                                           # 0192
  { nopi | nopi | nopi | nopi }                                           # 0193
  { mov r9, 1 | add r4, r5, r0 | nopi | sw r11, 148(rz) }                 # 0194
  { add r6, r8, r0 | mov r10, 0 | nopi | sw r12, 152(rz) }                # 0195
  { nopi | nopi | nopi | sw r13, 156(rz) }                                # 0196
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0197
  { nopi | nopi | nopi | nopi }                                           # 0198
__ccom_arith_24_mul:                                                      # bundle 199
  { nopi | nopi | beq r6, r0, __ccom_arith_26_end | nopi }                # 0199
  { nopi | nopi | nopi | nopi }                                           # 0200
  { and r7, r6, r9 | nopi | nopi | nopi }                                 # 0201
  { nopi | nopi | nopi | nopi }                                           # 0202
  { nopi | nopi | beq r7, r0, __ccom_arith_25_skip | nopi }               # 0203
  { nopi | nopi | nopi | nopi }                                           # 0204
  { add r10, r10, r4 | nopi | nopi | nopi }                               # 0205
  { nopi | nopi | nopi | nopi }                                           # 0206
__ccom_arith_25_skip:                                                     # bundle 207
  { sll r4, r4, r9 | srl r6, r6, r9 | jef __ccom_arith_24_mul | nopi }    # 0207
  { nopi | nopi | nopi | nopi }                                           # 0208
__ccom_arith_26_end:                                                      # bundle 209
  { nopi | nopi | nopi | lw r11, 148(rz) }                                # 0209
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0210
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0211
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0212
  { nopi | nopi | nopi | lw t0, 80(rz) }                                  # 0213
  { nopi | nopi | nopi | lw t2, 84(rz) }                                  # 0214
  { nopi | nopi | nopi | nopi }                                           # 0215
  { nopi | nopi | nopi | nopi }                                           # 0216
  { add r5, r4, r0 | add r7, r6, r0 | nopi | sw r10, 144(rz) }            # 0217
  { mov r10, 1 | mov r9, 0 | nopi | sw r11, 148(rz) }                     # 0218
  { nopi | nopi | nopi | sw r12, 152(rz) }                                # 0219
  { nopi | nopi | nopi | sw r13, 156(rz) }                                # 0220
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0221
  { nopi | nopi | nopi | nopi }                                           # 0222
__ccom_arith_27_mul:                                                      # bundle 223
  { nopi | nopi | beq r7, r0, __ccom_arith_29_end | nopi }                # 0223
  { nopi | nopi | nopi | nopi }                                           # 0224
  { and r8, r7, r10 | nopi | nopi | nopi }                                # 0225
  { nopi | nopi | nopi | nopi }                                           # 0226
  { nopi | nopi | beq r8, r0, __ccom_arith_28_skip | nopi }               # 0227
  { nopi | nopi | nopi | nopi }                                           # 0228
  { add r9, r9, r5 | nopi | nopi | nopi }                                 # 0229
  { nopi | nopi | nopi | nopi }                                           # 0230
__ccom_arith_28_skip:                                                     # bundle 231
  { sll r5, r5, r10 | srl r7, r7, r10 | jef __ccom_arith_27_mul | nopi }  # 0231
  { nopi | nopi | nopi | nopi }                                           # 0232
__ccom_arith_29_end:                                                      # bundle 233
  { nopi | nopi | nopi | lw r10, 144(rz) }                                # 0233
  { nopi | nopi | nopi | lw r11, 148(rz) }                                # 0234
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0235
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0236
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0237
  { nopi | nopi | nopi | lw t3, 88(rz) }                                  # 0238
  { nopi | nopi | nopi | lw t1, 92(rz) }                                  # 0239
  { nopi | nopi | nopi | nopi }                                           # 0240
  { nopi | nopi | nopi | nopi }                                           # 0241
  { add r4, r7, r0 | add r6, r5, r0 | nopi | sw r9, 140(rz) }             # 0242
  { mov r8, 0 | nopi | nopi | sw r10, 144(rz) }                           # 0243
  { mov r10, 1 | nopi | nopi | sw r11, 148(rz) }                          # 0244
  { nopi | nopi | nopi | sw r12, 152(rz) }                                # 0245
  { nopi | nopi | nopi | sw r13, 156(rz) }                                # 0246
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0247
  { nopi | nopi | nopi | nopi }                                           # 0248
__ccom_arith_30_mul:                                                      # bundle 249
  { nopi | nopi | beq r6, r0, __ccom_arith_32_end | nopi }                # 0249
  { nopi | nopi | nopi | nopi }                                           # 0250
  { and r9, r6, r10 | nopi | nopi | nopi }                                # 0251
  { nopi | nopi | nopi | nopi }                                           # 0252
  { nopi | nopi | beq r9, r0, __ccom_arith_31_skip | nopi }               # 0253
  { nopi | nopi | nopi | nopi }                                           # 0254
  { add r8, r8, r4 | nopi | nopi | nopi }                                 # 0255
  { nopi | nopi | nopi | nopi }                                           # 0256
__ccom_arith_31_skip:                                                     # bundle 257
  { sll r4, r4, r10 | srl r6, r6, r10 | jef __ccom_arith_30_mul | nopi }  # 0257
  { nopi | nopi | nopi | nopi }                                           # 0258
__ccom_arith_32_end:                                                      # bundle 259
  { nopi | nopi | nopi | lw r9, 140(rz) }                                 # 0259
  { nopi | nopi | nopi | lw r10, 144(rz) }                                # 0260
  { nopi | nopi | nopi | lw r11, 148(rz) }                                # 0261
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0262
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0263
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0264
  { nopi | nopi | nopi | lw t0, 96(rz) }                                  # 0265
  { nopi | nopi | nopi | lw t2, 100(rz) }                                 # 0266
  { nopi | nopi | nopi | nopi }                                           # 0267
  { nopi | nopi | nopi | nopi }                                           # 0268
  { add r5, r4, r0 | mov r7, 0 | nopi | sw r8, 136(rz) }                  # 0269
  { add r8, r6, r0 | nopi | nopi | sw r9, 140(rz) }                       # 0270
  { nopi | nopi | nopi | sw r10, 144(rz) }                                # 0271
  { mov r10, 1 | nopi | nopi | sw r11, 148(rz) }                          # 0272
  { nopi | nopi | nopi | sw r12, 152(rz) }                                # 0273
  { nopi | nopi | nopi | sw r13, 156(rz) }                                # 0274
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0275
  { nopi | nopi | nopi | nopi }                                           # 0276
__ccom_arith_33_mul:                                                      # bundle 277
  { nopi | nopi | beq r8, r0, __ccom_arith_35_end | nopi }                # 0277
  { nopi | nopi | nopi | nopi }                                           # 0278
  { and r9, r8, r10 | nopi | nopi | nopi }                                # 0279
  { nopi | nopi | nopi | nopi }                                           # 0280
  { nopi | nopi | beq r9, r0, __ccom_arith_34_skip | nopi }               # 0281
  { nopi | nopi | nopi | nopi }                                           # 0282
  { add r7, r7, r5 | nopi | nopi | nopi }                                 # 0283
  { nopi | nopi | nopi | nopi }                                           # 0284
__ccom_arith_34_skip:                                                     # bundle 285
  { sll r5, r5, r10 | srl r8, r8, r10 | jef __ccom_arith_33_mul | nopi }  # 0285
  { nopi | nopi | nopi | nopi }                                           # 0286
__ccom_arith_35_end:                                                      # bundle 287
  { nopi | nopi | nopi | lw r8, 136(rz) }                                 # 0287
  { nopi | nopi | nopi | lw r9, 140(rz) }                                 # 0288
  { nopi | nopi | nopi | lw r10, 144(rz) }                                # 0289
  { nopi | nopi | nopi | lw r11, 148(rz) }                                # 0290
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0291
  { nopi | nopi | nopi | lw r13, 156(rz) }                                # 0292
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0293
  { nopi | nopi | nopi | lw t1, 104(rz) }                                 # 0294
  { nopi | nopi | nopi | lw t0, 108(rz) }                                 # 0295
  { nopi | nopi | nopi | nopi }                                           # 0296
  { nopi | nopi | nopi | nopi }                                           # 0297
  { mov r6, 0 | nopi | nopi | sw r7, 132(rz) }                            # 0298
  { add r7, r5, r0 | nopi | nopi | sw r8, 136(rz) }                       # 0299
  { add r8, r4, r0 | nopi | nopi | sw r9, 140(rz) }                       # 0300
  { nopi | nopi | nopi | sw r10, 144(rz) }                                # 0301
  { mov r10, 1 | nopi | nopi | sw r11, 148(rz) }                          # 0302
  { nopi | nopi | nopi | sw r12, 152(rz) }                                # 0303
  { nopi | nopi | nopi | sw r13, 156(rz) }                                # 0304
  { nopi | nopi | nopi | sw r14, 160(rz) }                                # 0305
  { nopi | nopi | nopi | nopi }                                           # 0306
__ccom_arith_36_mul:                                                      # bundle 307
  { nopi | nopi | beq r8, r0, __ccom_arith_38_end | nopi }                # 0307
  { nopi | nopi | nopi | nopi }                                           # 0308
  { and r9, r8, r10 | nopi | nopi | nopi }                                # 0309
  { nopi | nopi | nopi | nopi }                                           # 0310
  { nopi | nopi | beq r9, r0, __ccom_arith_37_skip | nopi }               # 0311
  { nopi | nopi | nopi | nopi }                                           # 0312
  { add r6, r6, r7 | nopi | nopi | nopi }                                 # 0313
  { nopi | nopi | nopi | nopi }                                           # 0314
__ccom_arith_37_skip:                                                     # bundle 315
  { sll r7, r7, r10 | srl r8, r8, r10 | jef __ccom_arith_36_mul | nopi }  # 0315
  { nopi | nopi | nopi | nopi }                                           # 0316
__ccom_arith_38_end:                                                      # bundle 317
  { nopi | nopi | nopi | lw r7, 132(rz) }                                 # 0317
  { nopi | nopi | nopi | lw r8, 136(rz) }                                 # 0318
  { add t1, t3, t2 | nopi | nopi | lw r9, 140(rz) }                       # 0319
  { nopi | nopi | nopi | lw r10, 144(rz) }                                # 0320
  { add t0, t4, t1 | nopi | nopi | lw r11, 148(rz) }                      # 0321
  { nopi | nopi | nopi | lw r12, 152(rz) }                                # 0322
  { add t1, t5, t0 | nopi | nopi | lw r13, 156(rz) }                      # 0323
  { nopi | nopi | nopi | lw r14, 160(rz) }                                # 0324
  { add t0, t6, t1 | nopi | nopi | lw rg, 216(rz) }                       # 0325  slots 0=8,1=0,2=0,3=7
  { nopi | nopi | nopi | nopi }                                           # 0326
  { add t3, t7, t0 | nopi | nopi | nopi }                                 # 0327
  { nopi | nopi | nopi | nopi }                                           # 0328
  { add t2, t8, t3 | nopi | nopi | nopi }                                 # 0329  linea 7
  { nopi | nopi | nopi | nopi }                                           # 0330
  { add t1, t9, t2 | nopi | nopi | nopi }                                 # 0331
  { nopi | nopi | nopi | nopi }                                           # 0332
  { add t0, t10, t1 | nopi | nopi | nopi }                                # 0333
  { nopi | nopi | nopi | nopi }                                           # 0334
  { add t5, t11, t0 | nopi | nopi | nopi }                                # 0335
  { nopi | nopi | nopi | nopi }                                           # 0336
  { add t4, rg, t5 | nopi | nopi | nopi }                                 # 0337
  { nopi | nopi | nopi | lw rg, 220(rz) }                                 # 0338
  { nopi | nopi | nopi | nopi }                                           # 0339
  { add t3, rg, t4 | nopi | nopi | nopi }                                 # 0340
  { nopi | nopi | nopi | lw rg, 224(rz) }                                 # 0341
  { nopi | nopi | nopi | nopi }                                           # 0342
  { add t2, rg, t3 | nopi | nopi | nopi }                                 # 0343
  { nopi | nopi | nopi | nopi }                                           # 0344
  { nopi | nopi | nopi | sw t2, 0(rz) }                                   # 0345
  { nopi | nopi | nopi | lw t1, 112(rz) }                                 # 0346  linea 6
  { nopi | nopi | nopi | nopi }                                           # 0347
  { saddi t0, t1, 1 | nopi | nopi | nopi }                                # 0348
  { nopi | nopi | nopi | nopi }                                           # 0349
  { nopi | nopi | jef .L_o1_2 | sw t0, 112(rz) }                          # 0350
  { nopi | nopi | nopi | nopi }                                           # 0351
.L_o1_4:                                                                  # bundle 352
  { nopi | nopi | jefR rz, rl | nopi }                                    # 0352  linea 10
  { nopi | nopi | nopi | nopi }                                           # 0353
fin_main:                                                                 # bundle 354
  { nopi | nopi | jef fin_main | nopi }                                   # 0354  linea -
.L_arith_error:                                                           # bundle 355
  { nopi | nopi | jef .L_arith_error | nopi }                             # 0355

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_o1_5                          2
#   .L_o1_6                          4
#   .L_func_f                       39
#   .L_o1_0                         39
#   .L_o1_1                         41
#   .L_o1_2                         45
#   .L_o1_3                         50
#   __ccom_arith_0_mul              55  (interna)
#   __ccom_arith_1_skip             63  (interna)
#   __ccom_arith_2_end              65  (interna)
#   __ccom_arith_3_mul              75  (interna)
#   __ccom_arith_4_skip             83  (interna)
#   __ccom_arith_5_end              85  (interna)
#   __ccom_arith_6_mul              91  (interna)
#   __ccom_arith_7_skip             99  (interna)
#   __ccom_arith_8_end             101  (interna)
#   __ccom_arith_9_mul             107  (interna)
#   __ccom_arith_10_skip           115  (interna)
#   __ccom_arith_11_end            117  (interna)
#   __ccom_arith_12_mul            122  (interna)
#   __ccom_arith_13_skip           130  (interna)
#   __ccom_arith_14_end            132  (interna)
#   __ccom_arith_15_mul            139  (interna)
#   __ccom_arith_16_skip           147  (interna)
#   __ccom_arith_17_end            149  (interna)
#   __ccom_arith_18_mul            157  (interna)
#   __ccom_arith_19_skip           165  (interna)
#   __ccom_arith_20_end            167  (interna)
#   __ccom_arith_21_mul            177  (interna)
#   __ccom_arith_22_skip           185  (interna)
#   __ccom_arith_23_end            187  (interna)
#   __ccom_arith_24_mul            199  (interna)
#   __ccom_arith_25_skip           207  (interna)
#   __ccom_arith_26_end            209  (interna)
#   __ccom_arith_27_mul            223  (interna)
#   __ccom_arith_28_skip           231  (interna)
#   __ccom_arith_29_end            233  (interna)
#   __ccom_arith_30_mul            249  (interna)
#   __ccom_arith_31_skip           257  (interna)
#   __ccom_arith_32_end            259  (interna)
#   __ccom_arith_33_mul            277  (interna)
#   __ccom_arith_34_skip           285  (interna)
#   __ccom_arith_35_end            287  (interna)
#   __ccom_arith_36_mul            307  (interna)
#   __ccom_arith_37_skip           315  (interna)
#   __ccom_arith_38_end            317  (interna)
#   .L_o1_4                        352
#   fin_main                       354
#   .L_arith_error                 355

# Variables (direccion de datos)
#
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x00A8 size=48 temp_base=0x00A8
#       f : void (function) params=[a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7, a8, b8, a9, b9, a10, b10, a11, b11, a12, b12, a13, b13] area=0x0004 size=164 temp_base=0x0078
#       g : int width=4 addr=0x0000 line=1
#
#   Scope 1: FUNCTION f
#       b13 : int (parameter) width=4 addr=0x006C line=5
#       a13 : int (parameter) width=4 addr=0x0068 line=5
#       b12 : int (parameter) width=4 addr=0x0064 line=5
#       a12 : int (parameter) width=4 addr=0x0060 line=5
#       b11 : int (parameter) width=4 addr=0x005C line=5
#       a11 : int (parameter) width=4 addr=0x0058 line=5
#       b10 : int (parameter) width=4 addr=0x0054 line=4
#       a10 : int (parameter) width=4 addr=0x0050 line=4
#       b9 : int (parameter) width=4 addr=0x004C line=4
#       a9 : int (parameter) width=4 addr=0x0048 line=4
#       b8 : int (parameter) width=4 addr=0x0044 line=4
#       a8 : int (parameter) width=4 addr=0x0040 line=4
#       b7 : int (parameter) width=4 addr=0x003C line=4
#       a7 : int (parameter) width=4 addr=0x0038 line=4
#       b6 : int (parameter) width=4 addr=0x0034 line=4
#       a6 : int (parameter) width=4 addr=0x0030 line=4
#       b5 : int (parameter) width=4 addr=0x002C line=3
#       a5 : int (parameter) width=4 addr=0x0028 line=3
#       b4 : int (parameter) width=4 addr=0x0024 line=3
#       a4 : int (parameter) width=4 addr=0x0020 line=3
#       b3 : int (parameter) width=4 addr=0x001C line=3
#       a3 : int (parameter) width=4 addr=0x0018 line=3
#       b2 : int (parameter) width=4 addr=0x0014 line=3
#       a2 : int (parameter) width=4 addr=0x0010 line=3
#       b1 : int (parameter) width=4 addr=0x000C line=3
#       a1 : int (parameter) width=4 addr=0x0008 line=3
#
#   Scope 2: S f
#       i : int width=4 addr=0x0070 line=6
#
#   Scope 3: FUNCTION main
#
#   =======================
