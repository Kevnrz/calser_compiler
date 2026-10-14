# ccom: ejercicios/ejercicio2_matriz/ejercicio2.cls
  { mov rx, 192 | nopi | nopi | nopi }               # 0000
  { nopi | nopi | nopi | nopi }                      # 0001
.L_func_main:                                        # bundle 2
.L_body_main:                                        # bundle 2
  { mov t0, 0 | nopi | nopi | nopi }                 # 0002  linea 22
  { nopi | nopi | nopi | nopi }                      # 0003
  { nopi | nopi | nopi | sw t0, 132(rz) }            # 0004
  { nopi | nopi | nopi | nopi }                      # 0005
  { mov t0, 0 | nopi | nopi | nopi }                 # 0006
  { nopi | nopi | nopi | nopi }                      # 0007
  { nopi | nopi | nopi | sw t0, 136(rz) }            # 0008
  { nopi | nopi | nopi | nopi }                      # 0009
  { mov t0, 0 | nopi | nopi | nopi }                 # 0010  linea 23
  { nopi | nopi | nopi | nopi }                      # 0011
  { nopi | nopi | nopi | sw t0, 140(rz) }            # 0012
  { nopi | nopi | nopi | nopi }                      # 0013
  { mov t0, 42 | nopi | nopi | nopi }                # 0014  linea 24
  { nopi | nopi | nopi | nopi }                      # 0015
  { nopi | nopi | nopi | sw t0, 0(rx) }              # 0016
  { nopi | nopi | nopi | nopi }                      # 0017
  { saddi rx, rx, 4 | nopi | nopi | nopi }           # 0018
  { nopi | nopi | nopi | nopi }                      # 0019
  { mov r4, 132 | nopi | nopi | nopi }               # 0020
  { nopi | nopi | nopi | nopi }                      # 0021
  { nopi | nopi | nopi | sw r4, 64(r0) }             # 0022
  { nopi | nopi | nopi | nopi }                      # 0023
  { saddi rx, rx, -4 | nopi | nopi | nopi }          # 0024
  { nopi | nopi | nopi | nopi }                      # 0025
  { nopi | nopi | nopi | lw t0, 0(rx) }              # 0026
  { nopi | nopi | nopi | nopi }                      # 0027
  { nopi | nopi | nopi | sw t0, 60(rz) }             # 0028
  { nopi | nopi | nopi | nopi }                      # 0029
  { mov r4, 0 | nopi | nopi | nopi }                 # 0030
  { nopi | nopi | nopi | nopi }                      # 0031
  { nopi | nopi | nopi | sw r4, 56(r0) }             # 0032
  { nopi | nopi | nopi | nopi }                      # 0033
  { nopi | nopi | nopi | sw rl, 0(rx) }              # 0034
  { nopi | nopi | nopi | nopi }                      # 0035
  { saddi rx, rx, 4 | nopi | nopi | nopi }           # 0036
  { nopi | nopi | nopi | nopi }                      # 0037
  { nopi | nopi | jas rl, .L_func_buscar | nopi }    # 0038
  { nopi | nopi | nopi | nopi }                      # 0039
  { saddi rx, rx, -4 | nopi | nopi | nopi }          # 0040
  { nopi | nopi | nopi | nopi }                      # 0041
  { nopi | nopi | nopi | lw rl, 0(rx) }              # 0042
  { nopi | nopi | nopi | nopi }                      # 0043
  { nopi | nopi | nopi | lw t0, 52(rz) }             # 0044
  { nopi | nopi | nopi | nopi }                      # 0045
  { nopi | nopi | nopi | sw t0, 140(rz) }            # 0046
  { nopi | nopi | nopi | nopi }                      # 0047
  { nopi | nopi | jef fin_main | nopi }              # 0048  linea 25
  { nopi | nopi | nopi | nopi }                      # 0049
.L_func_buscar:                                      # bundle 50
  { nopi | nopi | nopi | sw rz, 52(rz) }             # 0050  linea 5
  { nopi | nopi | nopi | nopi }                      # 0051
.L_body_buscar:                                      # bundle 52
  { mov t0, 1 | nopi | nopi | nopi }                 # 0052  linea 6
  { nopi | nopi | nopi | nopi }                      # 0053
  { sub t0, rz, t0 | nopi | nopi | nopi }            # 0054
  { nopi | nopi | nopi | nopi }                      # 0055
  { mov t1, 0 | nopi | nopi | nopi }                 # 0056
  { nopi | nopi | nopi | nopi }                      # 0057
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0058
  { nopi | nopi | nopi | nopi }                      # 0059
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0060
  { nopi | nopi | nopi | nopi }                      # 0061
  { mov r7, 2 | nopi | nopi | nopi }                 # 0062
  { nopi | nopi | nopi | nopi }                      # 0063
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0064
  { nopi | nopi | nopi | nopi }                      # 0065
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0066
  { nopi | nopi | nopi | nopi }                      # 0067
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0068
  { nopi | nopi | nopi | nopi }                      # 0069
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0070
  { nopi | nopi | nopi | nopi }                      # 0071
  { mov r7, 2 | nopi | nopi | nopi }                 # 0072
  { nopi | nopi | nopi | nopi }                      # 0073
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0074
  { nopi | nopi | nopi | nopi }                      # 0075
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0076
  { nopi | nopi | nopi | nopi }                      # 0077
  { nopi | nopi | nopi | sw r4, 0(r6) }              # 0078
  { nopi | nopi | nopi | nopi }                      # 0079
  { mov t0, 1 | nopi | nopi | nopi }                 # 0080  linea 7
  { nopi | nopi | nopi | nopi }                      # 0081
  { sub t0, rz, t0 | nopi | nopi | nopi }            # 0082
  { nopi | nopi | nopi | nopi }                      # 0083
  { mov t1, 1 | nopi | nopi | nopi }                 # 0084
  { nopi | nopi | nopi | nopi }                      # 0085
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0086
  { nopi | nopi | nopi | nopi }                      # 0087
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0088
  { nopi | nopi | nopi | nopi }                      # 0089
  { mov r7, 2 | nopi | nopi | nopi }                 # 0090
  { nopi | nopi | nopi | nopi }                      # 0091
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0092
  { nopi | nopi | nopi | nopi }                      # 0093
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0094
  { nopi | nopi | nopi | nopi }                      # 0095
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0096
  { nopi | nopi | nopi | nopi }                      # 0097
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0098
  { nopi | nopi | nopi | nopi }                      # 0099
  { mov r7, 2 | nopi | nopi | nopi }                 # 0100
  { nopi | nopi | nopi | nopi }                      # 0101
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0102
  { nopi | nopi | nopi | nopi }                      # 0103
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0104
  { nopi | nopi | nopi | nopi }                      # 0105
  { nopi | nopi | nopi | sw r4, 0(r6) }              # 0106
  { nopi | nopi | nopi | nopi }                      # 0107
  { mov t0, 0 | nopi | nopi | nopi }                 # 0108  linea 8
  { nopi | nopi | nopi | nopi }                      # 0109
  { mov t1, 3 | nopi | nopi | nopi }                 # 0110
  { nopi | nopi | nopi | nopi }                      # 0111
  { nopi | nopi | nopi | sw t0, 68(rz) }             # 0112
  { nopi | nopi | nopi | nopi }                      # 0113
  { nopi | nopi | nopi | sw t1, 72(rz) }             # 0114
  { nopi | nopi | nopi | nopi }                      # 0115
.L_for_start_0:                                      # bundle 116
  { nopi | nopi | nopi | lw t0, 68(rz) }             # 0116
  { nopi | nopi | nopi | nopi }                      # 0117
  { nopi | nopi | nopi | lw t1, 72(rz) }             # 0118
  { nopi | nopi | nopi | nopi }                      # 0119
  { nopi | nopi | bge t0, t1, .L_for_end_2 | nopi }  # 0120
  { nopi | nopi | nopi | nopi }                      # 0121
  { mov t0, 0 | nopi | nopi | nopi }                 # 0122  linea 9
  { nopi | nopi | nopi | nopi }                      # 0123
  { mov t1, 4 | nopi | nopi | nopi }                 # 0124
  { nopi | nopi | nopi | nopi }                      # 0125
  { nopi | nopi | nopi | sw t0, 76(rz) }             # 0126
  { nopi | nopi | nopi | nopi }                      # 0127
  { nopi | nopi | nopi | sw t1, 80(rz) }             # 0128
  { nopi | nopi | nopi | nopi }                      # 0129
.L_for_start_3:                                      # bundle 130
  { nopi | nopi | nopi | lw t0, 76(rz) }             # 0130
  { nopi | nopi | nopi | nopi }                      # 0131
  { nopi | nopi | nopi | lw t1, 80(rz) }             # 0132
  { nopi | nopi | nopi | nopi }                      # 0133
  { nopi | nopi | bge t0, t1, .L_for_end_5 | nopi }  # 0134
  { nopi | nopi | nopi | nopi }                      # 0135
  { nopi | nopi | nopi | lw t0, 68(rz) }             # 0136  linea 10
  { nopi | nopi | nopi | nopi }                      # 0137
  { nopi | nopi | nopi | lw t1, 76(rz) }             # 0138
  { nopi | nopi | nopi | nopi }                      # 0139
  { nopi | nopi | nopi | lw r8, 56(r0) }             # 0140
  { nopi | nopi | nopi | nopi }                      # 0141
  { mov r7, 1 | nopi | nopi | nopi }                 # 0142
  { nopi | nopi | nopi | nopi }                      # 0143
  { add r6, r4, r0 | nopi | nopi | nopi }            # 0144
  { nopi | nopi | nopi | nopi }                      # 0145
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0146
  { nopi | nopi | nopi | nopi }                      # 0147
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0148
  { nopi | nopi | nopi | nopi }                      # 0149
  { add r6, r6, r5 | nopi | nopi | nopi }            # 0150
  { nopi | nopi | nopi | nopi }                      # 0151
  { mov r7, 2 | nopi | nopi | nopi }                 # 0152
  { nopi | nopi | nopi | nopi }                      # 0153
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0154
  { nopi | nopi | nopi | nopi }                      # 0155
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0156
  { nopi | nopi | nopi | nopi }                      # 0157
  { nopi | nopi | nopi | lw r8, 56(r0) }             # 0158
  { nopi | nopi | nopi | nopi }                      # 0159
  { mov r7, 1 | nopi | nopi | nopi }                 # 0160
  { nopi | nopi | nopi | nopi }                      # 0161
  { add r6, r4, r0 | nopi | nopi | nopi }            # 0162
  { nopi | nopi | nopi | nopi }                      # 0163
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0164
  { nopi | nopi | nopi | nopi }                      # 0165
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0166
  { nopi | nopi | nopi | nopi }                      # 0167
  { add r6, r6, r5 | nopi | nopi | nopi }            # 0168
  { nopi | nopi | nopi | nopi }                      # 0169
  { mov r7, 2 | nopi | nopi | nopi }                 # 0170
  { nopi | nopi | nopi | nopi }                      # 0171
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0172
  { nopi | nopi | nopi | nopi }                      # 0173
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0174
  { nopi | nopi | nopi | nopi }                      # 0175
  { nopi | nopi | nopi | lw r9, 0(r6) }              # 0176
  { nopi | nopi | nopi | nopi }                      # 0177
  { nopi | nopi | nopi | lw t0, 60(rz) }             # 0178
  { nopi | nopi | nopi | nopi }                      # 0179
  { seq t5, t5, t0 | nopi | nopi | nopi }            # 0180
  { nopi | nopi | nopi | nopi }                      # 0181
  { nopi | nopi | beq t5, rz, .L_if_next_6 | nopi }  # 0182
  { nopi | nopi | nopi | nopi }                      # 0183
  { nopi | nopi | nopi | lw t0, 68(rz) }             # 0184  linea 11
  { nopi | nopi | nopi | nopi }                      # 0185
  { mov t1, 0 | nopi | nopi | nopi }                 # 0186
  { nopi | nopi | nopi | nopi }                      # 0187
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0188
  { nopi | nopi | nopi | nopi }                      # 0189
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0190
  { nopi | nopi | nopi | nopi }                      # 0191
  { mov r7, 2 | nopi | nopi | nopi }                 # 0192
  { nopi | nopi | nopi | nopi }                      # 0193
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0194
  { nopi | nopi | nopi | nopi }                      # 0195
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0196
  { nopi | nopi | nopi | nopi }                      # 0197
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0198
  { nopi | nopi | nopi | nopi }                      # 0199
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0200
  { nopi | nopi | nopi | nopi }                      # 0201
  { mov r7, 2 | nopi | nopi | nopi }                 # 0202
  { nopi | nopi | nopi | nopi }                      # 0203
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0204
  { nopi | nopi | nopi | nopi }                      # 0205
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0206
  { nopi | nopi | nopi | nopi }                      # 0207
  { nopi | nopi | nopi | sw r4, 0(r6) }              # 0208
  { nopi | nopi | nopi | nopi }                      # 0209
  { nopi | nopi | nopi | lw t0, 76(rz) }             # 0210  linea 12
  { nopi | nopi | nopi | nopi }                      # 0211
  { mov t1, 1 | nopi | nopi | nopi }                 # 0212
  { nopi | nopi | nopi | nopi }                      # 0213
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0214
  { nopi | nopi | nopi | nopi }                      # 0215
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0216
  { nopi | nopi | nopi | nopi }                      # 0217
  { mov r7, 2 | nopi | nopi | nopi }                 # 0218
  { nopi | nopi | nopi | nopi }                      # 0219
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0220
  { nopi | nopi | nopi | nopi }                      # 0221
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0222
  { nopi | nopi | nopi | nopi }                      # 0223
  { nopi | nopi | nopi | lw r8, 64(r0) }             # 0224
  { nopi | nopi | nopi | nopi }                      # 0225
  { add r6, r5, r0 | nopi | nopi | nopi }            # 0226
  { nopi | nopi | nopi | nopi }                      # 0227
  { mov r7, 2 | nopi | nopi | nopi }                 # 0228
  { nopi | nopi | nopi | nopi }                      # 0229
  { sll r6, r6, r7 | nopi | nopi | nopi }            # 0230
  { nopi | nopi | nopi | nopi }                      # 0231
  { add r6, r6, r8 | nopi | nopi | nopi }            # 0232
  { nopi | nopi | nopi | nopi }                      # 0233
  { nopi | nopi | nopi | sw r4, 0(r6) }              # 0234
  { nopi | nopi | nopi | nopi }                      # 0235
  { mov t0, 1 | nopi | nopi | nopi }                 # 0236  linea 13
  { nopi | nopi | nopi | nopi }                      # 0237
  { nopi | nopi | nopi | sw t0, 52(rz) }             # 0238
  { nopi | nopi | nopi | nopi }                      # 0239
  { nopi | nopi | jefR rz, rl | nopi }               # 0240  linea 14
  { nopi | nopi | nopi | nopi }                      # 0241
  { nopi | nopi | jef .L_if_end_7 | nopi }           # 0242  linea 10
  { nopi | nopi | nopi | nopi }                      # 0243
.L_if_next_6:                                        # bundle 244
.L_if_end_7:                                         # bundle 244
.L_for_update_4:                                     # bundle 244
  { nopi | nopi | nopi | lw t0, 76(rz) }             # 0244  linea 9
  { nopi | nopi | nopi | nopi }                      # 0245
  { saddi t0, t0, 1 | nopi | nopi | nopi }           # 0246
  { nopi | nopi | nopi | nopi }                      # 0247
  { nopi | nopi | nopi | sw t0, 76(rz) }             # 0248
  { nopi | nopi | nopi | nopi }                      # 0249
  { nopi | nopi | jef .L_for_start_3 | nopi }        # 0250
  { nopi | nopi | nopi | nopi }                      # 0251
.L_for_end_5:                                        # bundle 252
.L_for_update_1:                                     # bundle 252
  { nopi | nopi | nopi | lw t0, 68(rz) }             # 0252  linea 8
  { nopi | nopi | nopi | nopi }                      # 0253
  { saddi t0, t0, 1 | nopi | nopi | nopi }           # 0254
  { nopi | nopi | nopi | nopi }                      # 0255
  { nopi | nopi | nopi | sw t0, 68(rz) }             # 0256
  { nopi | nopi | nopi | nopi }                      # 0257
  { nopi | nopi | jef .L_for_start_0 | nopi }        # 0258
  { nopi | nopi | nopi | nopi }                      # 0259
.L_for_end_2:                                        # bundle 260
  { nopi | nopi | jefR rz, rl | nopi }               # 0260  linea 18
  { nopi | nopi | nopi | nopi }                      # 0261
fin_main:                                            # bundle 262
  { nopi | nopi | jef fin_main | nopi }              # 0262  linea -
.L_arith_error:                                      # bundle 263
  { nopi | nopi | jef .L_arith_error | nopi }        # 0263
  { nopi | nopi | nopi | nopi }                      # 0264

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_buscar                  50
#   .L_body_buscar                  52
#   .L_for_start_0                 116
#   .L_for_start_3                 130
#   .L_if_next_6                   244
#   .L_if_end_7                    244
#   .L_for_update_4                244
#   .L_for_end_5                   252
#   .L_for_update_1                252
#   .L_for_end_2                   260
#   fin_main                       262
#   .L_arith_error                 263

# Variables (direccion de datos)
#
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x0084 size=60 temp_base=0x0090
#       buscar : bool (function) params=[mat, x, pos] area=0x0030 size=84 temp_base=0x0054
#       m : int[3][4] width=48 addr=0x0000 line=1
#
#   Scope 1: FUNCTION buscar
#       pos : int[2] (parameter) width=4 addr=0x0040 line=5
#       x : int (parameter) width=4 addr=0x003C line=5
#       mat : int[3][4] (parameter) width=4 addr=0x0038 line=5
#       hallado : bool (return) width=4 addr=0x0034 line=5
#
#   Scope 2: S buscar
#       i : int width=4 addr=0x0044 line=8
#
#   Scope 3: S buscar
#       j : int width=4 addr=0x004C line=9
#
#   Scope 4: C buscar
#
#   Scope 5: FUNCTION main
#       ok : bool width=4 addr=0x008C line=23
#       p : int[2] width=8 addr=0x0084 line=22
#
#   =======================
