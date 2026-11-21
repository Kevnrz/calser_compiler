# ccom: pruebas/optimizador/spill/s1_expresion.cls
  { mov rx, 220 | nopi | nopi | nopi }                                    # 0000
  { nopi | nopi | nopi | nopi }                                           # 0001
.L_func_main:                                                             # bundle 2
.L_o1_2:                                                                  # bundle 2
  { nopi | nopi | jef .L_o1_3 | nopi }                                    # 0002  linea 9
  { nopi | nopi | nopi | nopi }                                           # 0003
.L_o1_3:                                                                  # bundle 4
  { mov t1, 1 | mov t0, 2 | nopi | nopi }                                 # 0004  linea 10
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
  { nopi | nopi | jef fin_main | lw rl, 0(rx) }                           # 0037  linea 11  slots 0=0,1=0,2=11,3=10
  { nopi | nopi | nopi | nopi }                                           # 0038
.L_func_f:                                                                # bundle 39
.L_o1_0:                                                                  # bundle 39
  { nopi | nopi | jef .L_o1_1 | sw rz, 4(rz) }                            # 0039  linea 1
  { nopi | nopi | nopi | nopi }                                           # 0040
.L_o1_1:                                                                  # bundle 41
  { mov r9, 1 | mov r4, 0 | nopi | lw t4, 8(rz) }                         # 0041  linea 4
  { nopi | nopi | nopi | lw t6, 12(rz) }                                  # 0042
  { add r5, r8, r0 | nopi | nopi | nopi }                                 # 0043
  { add r6, r10, r0 | nopi | nopi | nopi }                                # 0044
  { nopi | nopi | nopi | nopi }                                           # 0045
__ccom_arith_0_mul:                                                       # bundle 46
  { nopi | nopi | beq r6, r0, __ccom_arith_2_end | nopi }                 # 0046
  { nopi | nopi | nopi | nopi }                                           # 0047
  { and r7, r6, r9 | nopi | nopi | nopi }                                 # 0048
  { nopi | nopi | nopi | nopi }                                           # 0049
  { nopi | nopi | beq r7, r0, __ccom_arith_1_skip | nopi }                # 0050
  { nopi | nopi | nopi | nopi }                                           # 0051
  { add r4, r4, r5 | nopi | nopi | nopi }                                 # 0052
  { nopi | nopi | nopi | nopi }                                           # 0053
__ccom_arith_1_skip:                                                      # bundle 54
  { sll r5, r5, r9 | srl r6, r6, r9 | jef __ccom_arith_0_mul | nopi }     # 0054
  { nopi | nopi | nopi | nopi }                                           # 0055
__ccom_arith_2_end:                                                       # bundle 56
  { mov r9, 1 | mov r5, 0 | nopi | sw t0, 216(rz) }                       # 0056
  { nopi | nopi | nopi | lw t9, 16(rz) }                                  # 0057
  { nopi | nopi | nopi | lw t0, 20(rz) }                                  # 0058
  { add r6, r13, r0 | nopi | nopi | nopi }                                # 0059
  { add r7, r4, r0 | nopi | nopi | nopi }                                 # 0060
  { nopi | nopi | nopi | nopi }                                           # 0061
__ccom_arith_3_mul:                                                       # bundle 62
  { nopi | nopi | beq r7, r0, __ccom_arith_5_end | nopi }                 # 0062
  { nopi | nopi | nopi | nopi }                                           # 0063
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0064
  { nopi | nopi | nopi | nopi }                                           # 0065
  { nopi | nopi | beq r8, r0, __ccom_arith_4_skip | nopi }                # 0066
  { nopi | nopi | nopi | nopi }                                           # 0067
  { add r5, r5, r6 | nopi | nopi | nopi }                                 # 0068
  { nopi | nopi | nopi | nopi }                                           # 0069
__ccom_arith_4_skip:                                                      # bundle 70
  { sll r6, r6, r9 | srl r7, r7, r9 | jef __ccom_arith_3_mul | nopi }     # 0070
  { nopi | nopi | nopi | nopi }                                           # 0071
__ccom_arith_5_end:                                                       # bundle 72
  { mov r9, 1 | mov r4, 0 | nopi | sw t1, 212(rz) }                       # 0072
  { nopi | nopi | nopi | lw t2, 24(rz) }                                  # 0073
  { nopi | nopi | nopi | lw t8, 28(rz) }                                  # 0074
  { add r5, r6, r0 | nopi | nopi | nopi }                                 # 0075
  { add r7, r12, r0 | nopi | nopi | nopi }                                # 0076
  { nopi | nopi | nopi | nopi }                                           # 0077
__ccom_arith_6_mul:                                                       # bundle 78
  { nopi | nopi | beq r7, r0, __ccom_arith_8_end | nopi }                 # 0078
  { nopi | nopi | nopi | nopi }                                           # 0079
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0080
  { nopi | nopi | nopi | nopi }                                           # 0081
  { nopi | nopi | beq r8, r0, __ccom_arith_7_skip | nopi }                # 0082
  { nopi | nopi | nopi | nopi }                                           # 0083
  { add r4, r4, r5 | nopi | nopi | nopi }                                 # 0084
  { nopi | nopi | nopi | nopi }                                           # 0085
__ccom_arith_7_skip:                                                      # bundle 86
  { sll r5, r5, r9 | srl r7, r7, r9 | jef __ccom_arith_6_mul | nopi }     # 0086
  { nopi | nopi | nopi | nopi }                                           # 0087
__ccom_arith_8_end:                                                       # bundle 88
  { mov r8, 1 | mov r15, 0 | nopi | sw t0, 208(rz) }                      # 0088
  { nopi | nopi | nopi | lw t5, 32(rz) }                                  # 0089
  { nopi | nopi | nopi | lw t3, 36(rz) }                                  # 0090
  { add r4, r9, r0 | nopi | nopi | nopi }                                 # 0091
  { add r5, r7, r0 | nopi | nopi | nopi }                                 # 0092
  { nopi | nopi | nopi | nopi }                                           # 0093
__ccom_arith_9_mul:                                                       # bundle 94
  { nopi | nopi | beq r5, r0, __ccom_arith_11_end | nopi }                # 0094
  { nopi | nopi | nopi | nopi }                                           # 0095
  { and r6, r5, r8 | nopi | nopi | nopi }                                 # 0096
  { nopi | nopi | nopi | nopi }                                           # 0097
  { nopi | nopi | beq r6, r0, __ccom_arith_10_skip | nopi }               # 0098
  { nopi | nopi | nopi | nopi }                                           # 0099
  { add r15, r15, r4 | nopi | nopi | nopi }                               # 0100
  { nopi | nopi | nopi | nopi }                                           # 0101
__ccom_arith_10_skip:                                                     # bundle 102
  { sll r4, r4, r8 | srl r5, r5, r8 | jef __ccom_arith_9_mul | nopi }     # 0102
  { nopi | nopi | nopi | nopi }                                           # 0103
__ccom_arith_11_end:                                                      # bundle 104
  { mov r8, 1 | mov r14, 0 | nopi | lw t7, 40(rz) }                       # 0104
  { nopi | nopi | nopi | lw t1, 44(rz) }                                  # 0105
  { add r4, r11, r0 | nopi | nopi | nopi }                                # 0106
  { add r6, r5, r0 | nopi | nopi | nopi }                                 # 0107
  { nopi | nopi | nopi | nopi }                                           # 0108
__ccom_arith_12_mul:                                                      # bundle 109
  { nopi | nopi | beq r6, r0, __ccom_arith_14_end | nopi }                # 0109
  { nopi | nopi | nopi | nopi }                                           # 0110
  { and r7, r6, r8 | nopi | nopi | nopi }                                 # 0111
  { nopi | nopi | nopi | nopi }                                           # 0112
  { nopi | nopi | beq r7, r0, __ccom_arith_13_skip | nopi }               # 0113
  { nopi | nopi | nopi | nopi }                                           # 0114
  { add r14, r14, r4 | nopi | nopi | nopi }                               # 0115
  { nopi | nopi | nopi | nopi }                                           # 0116
__ccom_arith_13_skip:                                                     # bundle 117
  { sll r4, r4, r8 | srl r6, r6, r8 | jef __ccom_arith_12_mul | nopi }    # 0117
  { nopi | nopi | nopi | nopi }                                           # 0118
__ccom_arith_14_end:                                                      # bundle 119
  { nopi | nopi | nopi | lw t4, 48(rz) }                                  # 0119
  { nopi | nopi | nopi | lw t6, 52(rz) }                                  # 0120
  { nopi | nopi | nopi | nopi }                                           # 0121
  { nopi | nopi | nopi | nopi }                                           # 0122
  { mov r7, 1 | add r4, r8, r0 | nopi | sw r14, 152(rz) }                 # 0123
  { add r5, r10, r0 | mov r13, 0 | nopi | nopi }                          # 0124
  { nopi | nopi | nopi | nopi }                                           # 0125
__ccom_arith_15_mul:                                                      # bundle 126
  { nopi | nopi | beq r5, r0, __ccom_arith_17_end | nopi }                # 0126
  { nopi | nopi | nopi | nopi }                                           # 0127
  { and r6, r5, r7 | nopi | nopi | nopi }                                 # 0128
  { nopi | nopi | nopi | nopi }                                           # 0129
  { nopi | nopi | beq r6, r0, __ccom_arith_16_skip | nopi }               # 0130
  { nopi | nopi | nopi | nopi }                                           # 0131
  { add r13, r13, r4 | nopi | nopi | nopi }                               # 0132
  { nopi | nopi | nopi | nopi }                                           # 0133
__ccom_arith_16_skip:                                                     # bundle 134
  { sll r4, r4, r7 | srl r5, r5, r7 | jef __ccom_arith_15_mul | nopi }    # 0134
  { nopi | nopi | nopi | nopi }                                           # 0135
__ccom_arith_17_end:                                                      # bundle 136
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0136
  { nopi | nopi | nopi | lw t0, 56(rz) }                                  # 0137
  { nopi | nopi | nopi | lw t2, 60(rz) }                                  # 0138
  { nopi | nopi | nopi | nopi }                                           # 0139
  { nopi | nopi | nopi | nopi }                                           # 0140
  { mov r9, 1 | add r5, r4, r0 | nopi | sw r13, 148(rz) }                 # 0141
  { add r7, r6, r0 | mov r12, 0 | nopi | sw r14, 152(rz) }                # 0142
  { nopi | nopi | nopi | nopi }                                           # 0143
__ccom_arith_18_mul:                                                      # bundle 144
  { nopi | nopi | beq r7, r0, __ccom_arith_20_end | nopi }                # 0144
  { nopi | nopi | nopi | nopi }                                           # 0145
  { and r8, r7, r9 | nopi | nopi | nopi }                                 # 0146
  { nopi | nopi | nopi | nopi }                                           # 0147
  { nopi | nopi | beq r8, r0, __ccom_arith_19_skip | nopi }               # 0148
  { nopi | nopi | nopi | nopi }                                           # 0149
  { add r12, r12, r5 | nopi | nopi | nopi }                               # 0150
  { nopi | nopi | nopi | nopi }                                           # 0151
__ccom_arith_19_skip:                                                     # bundle 152
  { sll r5, r5, r9 | srl r7, r7, r9 | jef __ccom_arith_18_mul | nopi }    # 0152
  { nopi | nopi | nopi | nopi }                                           # 0153
__ccom_arith_20_end:                                                      # bundle 154
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0154
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0155
  { nopi | nopi | nopi | lw t5, 64(rz) }                                  # 0156  linea 5
  { nopi | nopi | nopi | lw t3, 68(rz) }                                  # 0157
  { nopi | nopi | nopi | nopi }                                           # 0158
  { nopi | nopi | nopi | nopi }                                           # 0159
  { mov r8, 1 | add r4, r9, r0 | nopi | sw r12, 144(rz) }                 # 0160
  { add r5, r7, r0 | mov r11, 0 | nopi | sw r13, 148(rz) }                # 0161
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0162
  { nopi | nopi | nopi | nopi }                                           # 0163
__ccom_arith_21_mul:                                                      # bundle 164
  { nopi | nopi | beq r5, r0, __ccom_arith_23_end | nopi }                # 0164
  { nopi | nopi | nopi | nopi }                                           # 0165
  { and r6, r5, r8 | nopi | nopi | nopi }                                 # 0166
  { nopi | nopi | nopi | nopi }                                           # 0167
  { nopi | nopi | beq r6, r0, __ccom_arith_22_skip | nopi }               # 0168
  { nopi | nopi | nopi | nopi }                                           # 0169
  { add r11, r11, r4 | nopi | nopi | nopi }                               # 0170
  { nopi | nopi | nopi | nopi }                                           # 0171
__ccom_arith_22_skip:                                                     # bundle 172
  { sll r4, r4, r8 | srl r5, r5, r8 | jef __ccom_arith_21_mul | nopi }    # 0172
  { nopi | nopi | nopi | nopi }                                           # 0173
__ccom_arith_23_end:                                                      # bundle 174
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0174
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0175
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0176
  { nopi | nopi | nopi | lw t1, 72(rz) }                                  # 0177
  { nopi | nopi | nopi | lw t4, 76(rz) }                                  # 0178
  { nopi | nopi | nopi | nopi }                                           # 0179
  { nopi | nopi | nopi | nopi }                                           # 0180
  { mov r9, 1 | add r4, r5, r0 | nopi | sw r11, 140(rz) }                 # 0181
  { add r6, r8, r0 | mov r10, 0 | nopi | sw r12, 144(rz) }                # 0182
  { nopi | nopi | nopi | sw r13, 148(rz) }                                # 0183
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0184
  { nopi | nopi | nopi | nopi }                                           # 0185
__ccom_arith_24_mul:                                                      # bundle 186
  { nopi | nopi | beq r6, r0, __ccom_arith_26_end | nopi }                # 0186
  { nopi | nopi | nopi | nopi }                                           # 0187
  { and r7, r6, r9 | nopi | nopi | nopi }                                 # 0188
  { nopi | nopi | nopi | nopi }                                           # 0189
  { nopi | nopi | beq r7, r0, __ccom_arith_25_skip | nopi }               # 0190
  { nopi | nopi | nopi | nopi }                                           # 0191
  { add r10, r10, r4 | nopi | nopi | nopi }                               # 0192
  { nopi | nopi | nopi | nopi }                                           # 0193
__ccom_arith_25_skip:                                                     # bundle 194
  { sll r4, r4, r9 | srl r6, r6, r9 | jef __ccom_arith_24_mul | nopi }    # 0194
  { nopi | nopi | nopi | nopi }                                           # 0195
__ccom_arith_26_end:                                                      # bundle 196
  { nopi | nopi | nopi | lw r11, 140(rz) }                                # 0196
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0197
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0198
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0199
  { nopi | nopi | nopi | lw t0, 80(rz) }                                  # 0200
  { nopi | nopi | nopi | lw t2, 84(rz) }                                  # 0201
  { nopi | nopi | nopi | nopi }                                           # 0202
  { nopi | nopi | nopi | nopi }                                           # 0203
  { add r5, r4, r0 | add r7, r6, r0 | nopi | sw r10, 136(rz) }            # 0204
  { mov r10, 1 | mov r9, 0 | nopi | sw r11, 140(rz) }                     # 0205
  { nopi | nopi | nopi | sw r12, 144(rz) }                                # 0206
  { nopi | nopi | nopi | sw r13, 148(rz) }                                # 0207
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0208
  { nopi | nopi | nopi | nopi }                                           # 0209
__ccom_arith_27_mul:                                                      # bundle 210
  { nopi | nopi | beq r7, r0, __ccom_arith_29_end | nopi }                # 0210
  { nopi | nopi | nopi | nopi }                                           # 0211
  { and r8, r7, r10 | nopi | nopi | nopi }                                # 0212
  { nopi | nopi | nopi | nopi }                                           # 0213
  { nopi | nopi | beq r8, r0, __ccom_arith_28_skip | nopi }               # 0214
  { nopi | nopi | nopi | nopi }                                           # 0215
  { add r9, r9, r5 | nopi | nopi | nopi }                                 # 0216
  { nopi | nopi | nopi | nopi }                                           # 0217
__ccom_arith_28_skip:                                                     # bundle 218
  { sll r5, r5, r10 | srl r7, r7, r10 | jef __ccom_arith_27_mul | nopi }  # 0218
  { nopi | nopi | nopi | nopi }                                           # 0219
__ccom_arith_29_end:                                                      # bundle 220
  { nopi | nopi | nopi | lw r10, 136(rz) }                                # 0220
  { nopi | nopi | nopi | lw r11, 140(rz) }                                # 0221
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0222
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0223
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0224
  { nopi | nopi | nopi | lw t3, 88(rz) }                                  # 0225
  { nopi | nopi | nopi | lw t1, 92(rz) }                                  # 0226
  { nopi | nopi | nopi | nopi }                                           # 0227
  { nopi | nopi | nopi | nopi }                                           # 0228
  { add r4, r7, r0 | add r6, r5, r0 | nopi | sw r9, 132(rz) }             # 0229
  { mov r8, 0 | nopi | nopi | sw r10, 136(rz) }                           # 0230
  { mov r10, 1 | nopi | nopi | sw r11, 140(rz) }                          # 0231
  { nopi | nopi | nopi | sw r12, 144(rz) }                                # 0232
  { nopi | nopi | nopi | sw r13, 148(rz) }                                # 0233
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0234
  { nopi | nopi | nopi | nopi }                                           # 0235
__ccom_arith_30_mul:                                                      # bundle 236
  { nopi | nopi | beq r6, r0, __ccom_arith_32_end | nopi }                # 0236
  { nopi | nopi | nopi | nopi }                                           # 0237
  { and r9, r6, r10 | nopi | nopi | nopi }                                # 0238
  { nopi | nopi | nopi | nopi }                                           # 0239
  { nopi | nopi | beq r9, r0, __ccom_arith_31_skip | nopi }               # 0240
  { nopi | nopi | nopi | nopi }                                           # 0241
  { add r8, r8, r4 | nopi | nopi | nopi }                                 # 0242
  { nopi | nopi | nopi | nopi }                                           # 0243
__ccom_arith_31_skip:                                                     # bundle 244
  { sll r4, r4, r10 | srl r6, r6, r10 | jef __ccom_arith_30_mul | nopi }  # 0244
  { nopi | nopi | nopi | nopi }                                           # 0245
__ccom_arith_32_end:                                                      # bundle 246
  { nopi | nopi | nopi | lw r9, 132(rz) }                                 # 0246
  { nopi | nopi | nopi | lw r10, 136(rz) }                                # 0247
  { nopi | nopi | nopi | lw r11, 140(rz) }                                # 0248
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0249
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0250
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0251
  { nopi | nopi | nopi | lw t0, 96(rz) }                                  # 0252
  { nopi | nopi | nopi | lw t2, 100(rz) }                                 # 0253
  { nopi | nopi | nopi | nopi }                                           # 0254
  { nopi | nopi | nopi | nopi }                                           # 0255
  { add r5, r4, r0 | mov r7, 0 | nopi | sw r8, 128(rz) }                  # 0256
  { add r8, r6, r0 | nopi | nopi | sw r9, 132(rz) }                       # 0257
  { nopi | nopi | nopi | sw r10, 136(rz) }                                # 0258
  { mov r10, 1 | nopi | nopi | sw r11, 140(rz) }                          # 0259
  { nopi | nopi | nopi | sw r12, 144(rz) }                                # 0260
  { nopi | nopi | nopi | sw r13, 148(rz) }                                # 0261
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0262
  { nopi | nopi | nopi | nopi }                                           # 0263
__ccom_arith_33_mul:                                                      # bundle 264
  { nopi | nopi | beq r8, r0, __ccom_arith_35_end | nopi }                # 0264
  { nopi | nopi | nopi | nopi }                                           # 0265
  { and r9, r8, r10 | nopi | nopi | nopi }                                # 0266
  { nopi | nopi | nopi | nopi }                                           # 0267
  { nopi | nopi | beq r9, r0, __ccom_arith_34_skip | nopi }               # 0268
  { nopi | nopi | nopi | nopi }                                           # 0269
  { add r7, r7, r5 | nopi | nopi | nopi }                                 # 0270
  { nopi | nopi | nopi | nopi }                                           # 0271
__ccom_arith_34_skip:                                                     # bundle 272
  { sll r5, r5, r10 | srl r8, r8, r10 | jef __ccom_arith_33_mul | nopi }  # 0272
  { nopi | nopi | nopi | nopi }                                           # 0273
__ccom_arith_35_end:                                                      # bundle 274
  { nopi | nopi | nopi | lw r8, 128(rz) }                                 # 0274
  { nopi | nopi | nopi | lw r9, 132(rz) }                                 # 0275
  { nopi | nopi | nopi | lw r10, 136(rz) }                                # 0276
  { nopi | nopi | nopi | lw r11, 140(rz) }                                # 0277
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0278
  { nopi | nopi | nopi | lw r13, 148(rz) }                                # 0279
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0280
  { nopi | nopi | nopi | lw t1, 104(rz) }                                 # 0281
  { nopi | nopi | nopi | lw t0, 108(rz) }                                 # 0282
  { nopi | nopi | nopi | nopi }                                           # 0283
  { nopi | nopi | nopi | nopi }                                           # 0284
  { mov r6, 0 | nopi | nopi | sw r7, 124(rz) }                            # 0285
  { add r7, r5, r0 | nopi | nopi | sw r8, 128(rz) }                       # 0286
  { add r8, r4, r0 | nopi | nopi | sw r9, 132(rz) }                       # 0287
  { nopi | nopi | nopi | sw r10, 136(rz) }                                # 0288
  { mov r10, 1 | nopi | nopi | sw r11, 140(rz) }                          # 0289
  { nopi | nopi | nopi | sw r12, 144(rz) }                                # 0290
  { nopi | nopi | nopi | sw r13, 148(rz) }                                # 0291
  { nopi | nopi | nopi | sw r14, 152(rz) }                                # 0292
  { nopi | nopi | nopi | nopi }                                           # 0293
__ccom_arith_36_mul:                                                      # bundle 294
  { nopi | nopi | beq r8, r0, __ccom_arith_38_end | nopi }                # 0294
  { nopi | nopi | nopi | nopi }                                           # 0295
  { and r9, r8, r10 | nopi | nopi | nopi }                                # 0296
  { nopi | nopi | nopi | nopi }                                           # 0297
  { nopi | nopi | beq r9, r0, __ccom_arith_37_skip | nopi }               # 0298
  { nopi | nopi | nopi | nopi }                                           # 0299
  { add r6, r6, r7 | nopi | nopi | nopi }                                 # 0300
  { nopi | nopi | nopi | nopi }                                           # 0301
__ccom_arith_37_skip:                                                     # bundle 302
  { sll r7, r7, r10 | srl r8, r8, r10 | jef __ccom_arith_36_mul | nopi }  # 0302
  { nopi | nopi | nopi | nopi }                                           # 0303
__ccom_arith_38_end:                                                      # bundle 304
  { nopi | nopi | nopi | lw r7, 124(rz) }                                 # 0304
  { nopi | nopi | nopi | lw r8, 128(rz) }                                 # 0305
  { add t0, t3, t2 | nopi | nopi | lw r9, 132(rz) }                       # 0306
  { nopi | nopi | nopi | lw r10, 136(rz) }                                # 0307
  { add t1, t4, t0 | nopi | nopi | lw r11, 140(rz) }                      # 0308
  { nopi | nopi | nopi | lw r12, 144(rz) }                                # 0309
  { add t0, t5, t1 | nopi | nopi | lw r13, 148(rz) }                      # 0310
  { nopi | nopi | nopi | lw r14, 152(rz) }                                # 0311
  { add t3, t6, t0 | nopi | nopi | lw rg, 208(rz) }                       # 0312  slots 0=5,1=0,2=0,3=4
  { nopi | nopi | nopi | nopi }                                           # 0313
  { add t2, t7, t3 | nopi | nopi | nopi }                                 # 0314
  { nopi | nopi | nopi | nopi }                                           # 0315
  { add t1, t8, t2 | nopi | nopi | nopi }                                 # 0316  linea 4
  { nopi | nopi | nopi | nopi }                                           # 0317
  { add t0, t9, t1 | nopi | nopi | nopi }                                 # 0318
  { nopi | nopi | nopi | nopi }                                           # 0319
  { add t4, t10, t0 | nopi | nopi | nopi }                                # 0320
  { nopi | nopi | nopi | nopi }                                           # 0321
  { add t3, t11, t4 | nopi | nopi | nopi }                                # 0322
  { nopi | nopi | nopi | nopi }                                           # 0323
  { add t2, rg, t3 | nopi | nopi | nopi }                                 # 0324
  { nopi | nopi | nopi | lw rg, 212(rz) }                                 # 0325
  { nopi | nopi | nopi | nopi }                                           # 0326
  { add t1, rg, t2 | nopi | nopi | nopi }                                 # 0327
  { nopi | nopi | nopi | lw rg, 216(rz) }                                 # 0328
  { nopi | nopi | nopi | nopi }                                           # 0329
  { add t0, rg, t1 | nopi | nopi | nopi }                                 # 0330
  { nopi | nopi | nopi | nopi }                                           # 0331
  { nopi | nopi | jefR rz, rl | sw t0, 4(rz) }                            # 0332  linea 6  slots 0=0,1=0,2=6,3=4
  { nopi | nopi | nopi | nopi }                                           # 0333
fin_main:                                                                 # bundle 334
  { nopi | nopi | jef fin_main | nopi }                                   # 0334  linea -
.L_arith_error:                                                           # bundle 335
  { nopi | nopi | jef .L_arith_error | nopi }                             # 0335

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_o1_2                          2
#   .L_o1_3                          4
#   .L_func_f                       39
#   .L_o1_0                         39
#   .L_o1_1                         41
#   __ccom_arith_0_mul              46  (interna)
#   __ccom_arith_1_skip             54  (interna)
#   __ccom_arith_2_end              56  (interna)
#   __ccom_arith_3_mul              62  (interna)
#   __ccom_arith_4_skip             70  (interna)
#   __ccom_arith_5_end              72  (interna)
#   __ccom_arith_6_mul              78  (interna)
#   __ccom_arith_7_skip             86  (interna)
#   __ccom_arith_8_end              88  (interna)
#   __ccom_arith_9_mul              94  (interna)
#   __ccom_arith_10_skip           102  (interna)
#   __ccom_arith_11_end            104  (interna)
#   __ccom_arith_12_mul            109  (interna)
#   __ccom_arith_13_skip           117  (interna)
#   __ccom_arith_14_end            119  (interna)
#   __ccom_arith_15_mul            126  (interna)
#   __ccom_arith_16_skip           134  (interna)
#   __ccom_arith_17_end            136  (interna)
#   __ccom_arith_18_mul            144  (interna)
#   __ccom_arith_19_skip           152  (interna)
#   __ccom_arith_20_end            154  (interna)
#   __ccom_arith_21_mul            164  (interna)
#   __ccom_arith_22_skip           172  (interna)
#   __ccom_arith_23_end            174  (interna)
#   __ccom_arith_24_mul            186  (interna)
#   __ccom_arith_25_skip           194  (interna)
#   __ccom_arith_26_end            196  (interna)
#   __ccom_arith_27_mul            210  (interna)
#   __ccom_arith_28_skip           218  (interna)
#   __ccom_arith_29_end            220  (interna)
#   __ccom_arith_30_mul            236  (interna)
#   __ccom_arith_31_skip           244  (interna)
#   __ccom_arith_32_end            246  (interna)
#   __ccom_arith_33_mul            264  (interna)
#   __ccom_arith_34_skip           272  (interna)
#   __ccom_arith_35_end            274  (interna)
#   __ccom_arith_36_mul            294  (interna)
#   __ccom_arith_37_skip           302  (interna)
#   __ccom_arith_38_end            304  (interna)
#   fin_main                       334
#   .L_arith_error                 335

# Variables (direccion de datos)
#
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x00A0 size=48 temp_base=0x00A0
#       f : int (function) params=[a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7, a8, b8, a9, b9, a10, b10, a11, b11, a12, b12, a13, b13] area=0x0000 size=160 temp_base=0x0070
#
#   Scope 1: FUNCTION f
#       b13 : int (parameter) width=4 addr=0x006C line=3
#       a13 : int (parameter) width=4 addr=0x0068 line=3
#       b12 : int (parameter) width=4 addr=0x0064 line=3
#       a12 : int (parameter) width=4 addr=0x0060 line=3
#       b11 : int (parameter) width=4 addr=0x005C line=3
#       a11 : int (parameter) width=4 addr=0x0058 line=3
#       b10 : int (parameter) width=4 addr=0x0054 line=2
#       a10 : int (parameter) width=4 addr=0x0050 line=2
#       b9 : int (parameter) width=4 addr=0x004C line=2
#       a9 : int (parameter) width=4 addr=0x0048 line=2
#       b8 : int (parameter) width=4 addr=0x0044 line=2
#       a8 : int (parameter) width=4 addr=0x0040 line=2
#       b7 : int (parameter) width=4 addr=0x003C line=2
#       a7 : int (parameter) width=4 addr=0x0038 line=2
#       b6 : int (parameter) width=4 addr=0x0034 line=2
#       a6 : int (parameter) width=4 addr=0x0030 line=2
#       b5 : int (parameter) width=4 addr=0x002C line=1
#       a5 : int (parameter) width=4 addr=0x0028 line=1
#       b4 : int (parameter) width=4 addr=0x0024 line=1
#       a4 : int (parameter) width=4 addr=0x0020 line=1
#       b3 : int (parameter) width=4 addr=0x001C line=1
#       a3 : int (parameter) width=4 addr=0x0018 line=1
#       b2 : int (parameter) width=4 addr=0x0014 line=1
#       a2 : int (parameter) width=4 addr=0x0010 line=1
#       b1 : int (parameter) width=4 addr=0x000C line=1
#       a1 : int (parameter) width=4 addr=0x0008 line=1
#       r : int (return) width=4 addr=0x0004 line=3
#
#   Scope 2: FUNCTION main
#
#   =======================
