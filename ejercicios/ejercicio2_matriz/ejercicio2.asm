# ccom: ejercicios/ejercicio2_matriz/ejercicio2.cls
  { mov rx, 192 | nopi | nopi | nopi }                        # 0000
  { nopi | nopi | nopi | nopi }                               # 0001
.L_func_main:                                                 # bundle 2
.L_body_main:                                                 # bundle 2
  { mov t0, 0 | nopi | nopi | nopi }                          # 0002  linea 22
  { nopi | nopi | nopi | nopi }                               # 0003
  { nopi | nopi | nopi | sw t0, 132(rz) }                     # 0004
  { mov t0, 0 | nopi | nopi | nopi }                          # 0005
  { nopi | nopi | nopi | nopi }                               # 0006
  { nopi | nopi | nopi | sw t0, 136(rz) }                     # 0007
  { mov t0, 0 | nopi | nopi | nopi }                          # 0008  linea 23
  { nopi | nopi | nopi | nopi }                               # 0009
  { nopi | nopi | nopi | sw t0, 140(rz) }                     # 0010
  { mov t0, 42 | nopi | nopi | nopi }                         # 0011  linea 24
  { nopi | nopi | nopi | nopi }                               # 0012
  { nopi | nopi | nopi | sw t0, 0(rx) }                       # 0013
  { saddi rx, rx, 4 | mov r4, 132 | nopi | nopi }             # 0014
  { nopi | nopi | nopi | nopi }                               # 0015
  { saddi rx, rx, -4 | nopi | nopi | sw r4, 64(r0) }          # 0016
  { nopi | nopi | nopi | nopi }                               # 0017
  { nopi | nopi | nopi | lw t0, 0(rx) }                       # 0018
  { nopi | nopi | nopi | nopi }                               # 0019
  { nopi | nopi | nopi | sw t0, 60(rz) }                      # 0020
  { mov r4, 0 | nopi | nopi | nopi }                          # 0021
  { nopi | nopi | nopi | nopi }                               # 0022
  { nopi | nopi | nopi | sw r4, 56(r0) }                      # 0023
  { nopi | nopi | nopi | sw rl, 0(rx) }                       # 0024
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_buscar | nopi }  # 0025
  { nopi | nopi | nopi | nopi }                               # 0026
  { saddi rx, rx, -4 | nopi | nopi | nopi }                   # 0027
  { nopi | nopi | nopi | nopi }                               # 0028
  { nopi | nopi | nopi | lw rl, 0(rx) }                       # 0029
  { nopi | nopi | nopi | lw t0, 52(rz) }                      # 0030
  { nopi | nopi | nopi | nopi }                               # 0031
  { nopi | nopi | jef fin_main | sw t0, 140(rz) }             # 0032  linea 25  slots 0=0,1=0,2=25,3=24
  { nopi | nopi | nopi | nopi }                               # 0033
.L_func_buscar:                                               # bundle 34
  { nopi | nopi | nopi | sw rz, 52(rz) }                      # 0034  linea 5
  { nopi | nopi | nopi | nopi }                               # 0035
.L_body_buscar:                                               # bundle 36
  { mov t0, 1 | mov t1, 0 | nopi | lw r8, 64(r0) }            # 0036  linea 6
  { mov r7, 2 | nopi | nopi | nopi }                          # 0037
  { sub t0, rz, t0 | add r6, r5, r0 | nopi | nopi }           # 0038
  { nopi | nopi | nopi | nopi }                               # 0039
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0040
  { mov r7, 2 | nopi | nopi | nopi }                          # 0041
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0042
  { add r6, r5, r0 | nopi | nopi | lw r8, 64(r0) }            # 0043
  { mov t1, 1 | nopi | nopi | nopi }                          # 0044  linea 7
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0045  linea 6
  { mov r7, 2 | nopi | nopi | nopi }                          # 0046  linea 7
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0047  linea 6
  { nopi | nopi | nopi | nopi }                               # 0048
  { nopi | nopi | nopi | sw r4, 0(r6) }                       # 0049
  { mov t0, 1 | add r6, r5, r0 | nopi | lw r8, 64(r0) }       # 0050  linea 7
  { nopi | nopi | nopi | nopi }                               # 0051
  { sub t0, rz, t0 | sll r6, r6, r7 | nopi | nopi }           # 0052
  { mov r7, 2 | nopi | nopi | nopi }                          # 0053
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0054
  { add r6, r5, r0 | nopi | nopi | lw r8, 64(r0) }            # 0055
  { mov t1, 3 | nopi | nopi | nopi }                          # 0056  linea 8
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0057  linea 7
  { nopi | nopi | nopi | nopi }                               # 0058
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0059
  { nopi | nopi | nopi | nopi }                               # 0060
  { nopi | nopi | nopi | sw r4, 0(r6) }                       # 0061
  { mov t0, 0 | nopi | nopi | nopi }                          # 0062  linea 8
  { nopi | nopi | nopi | nopi }                               # 0063
  { nopi | nopi | nopi | sw t0, 68(rz) }                      # 0064
  { nopi | nopi | nopi | sw t1, 72(rz) }                      # 0065
  { nopi | nopi | nopi | nopi }                               # 0066
.L_for_start_0:                                               # bundle 67
  { nopi | nopi | nopi | lw t0, 68(rz) }                      # 0067
  { nopi | nopi | nopi | lw t1, 72(rz) }                      # 0068
  { nopi | nopi | nopi | nopi }                               # 0069
  { nopi | nopi | bge t0, t1, .L_for_end_2 | nopi }           # 0070
  { nopi | nopi | nopi | nopi }                               # 0071
  { mov t0, 0 | mov t1, 4 | nopi | nopi }                     # 0072  linea 9
  { nopi | nopi | nopi | nopi }                               # 0073
  { nopi | nopi | nopi | sw t0, 76(rz) }                      # 0074
  { nopi | nopi | nopi | sw t1, 80(rz) }                      # 0075
  { nopi | nopi | nopi | nopi }                               # 0076
.L_for_start_3:                                               # bundle 77
  { nopi | nopi | nopi | lw t0, 76(rz) }                      # 0077
  { nopi | nopi | nopi | lw t1, 80(rz) }                      # 0078
  { nopi | nopi | nopi | nopi }                               # 0079
  { nopi | nopi | bge t0, t1, .L_for_end_5 | nopi }           # 0080
  { nopi | nopi | nopi | nopi }                               # 0081
  { mov r7, 1 | nopi | nopi | lw t0, 68(rz) }                 # 0082  linea 10
  { nopi | nopi | nopi | lw t1, 76(rz) }                      # 0083
  { add r6, r4, r0 | nopi | nopi | lw r8, 56(r0) }            # 0084
  { nopi | nopi | nopi | nopi }                               # 0085
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0086
  { nopi | nopi | nopi | nopi }                               # 0087
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0088
  { mov r7, 2 | nopi | nopi | nopi }                          # 0089
  { add r6, r6, r5 | nopi | nopi | nopi }                     # 0090
  { nopi | nopi | nopi | nopi }                               # 0091
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0092
  { mov r7, 1 | nopi | nopi | nopi }                          # 0093
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0094
  { add r6, r4, r0 | nopi | nopi | lw r8, 56(r0) }            # 0095
  { nopi | nopi | nopi | nopi }                               # 0096
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0097
  { nopi | nopi | nopi | nopi }                               # 0098
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0099
  { mov r7, 2 | nopi | nopi | nopi }                          # 0100
  { add r6, r6, r5 | nopi | nopi | nopi }                     # 0101
  { nopi | nopi | nopi | nopi }                               # 0102
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0103
  { nopi | nopi | nopi | nopi }                               # 0104
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0105
  { nopi | nopi | nopi | nopi }                               # 0106
  { nopi | nopi | nopi | lw r9, 0(r6) }                       # 0107
  { nopi | nopi | nopi | lw t0, 60(rz) }                      # 0108
  { nopi | nopi | nopi | nopi }                               # 0109
  { seq t5, t5, t0 | nopi | nopi | nopi }                     # 0110
  { nopi | nopi | nopi | nopi }                               # 0111
  { nopi | nopi | beq t5, rz, .L_if_next_6 | nopi }           # 0112
  { nopi | nopi | nopi | nopi }                               # 0113
  { mov t1, 0 | mov r7, 2 | nopi | lw t0, 68(rz) }            # 0114  linea 11
  { nopi | nopi | nopi | lw r8, 64(r0) }                      # 0115
  { add r6, r5, r0 | nopi | nopi | nopi }                     # 0116
  { nopi | nopi | nopi | nopi }                               # 0117
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0118
  { mov r7, 2 | nopi | nopi | nopi }                          # 0119
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0120
  { add r6, r5, r0 | nopi | nopi | lw r8, 64(r0) }            # 0121
  { mov t1, 1 | nopi | nopi | nopi }                          # 0122  linea 12
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0123  linea 11
  { mov r7, 2 | nopi | nopi | nopi }                          # 0124  linea 12
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0125  linea 11
  { nopi | nopi | nopi | nopi }                               # 0126
  { nopi | nopi | nopi | sw r4, 0(r6) }                       # 0127
  { add r6, r5, r0 | nopi | nopi | lw t0, 76(rz) }            # 0128  linea 12
  { nopi | nopi | nopi | lw r8, 64(r0) }                      # 0129
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0130
  { mov r7, 2 | nopi | nopi | nopi }                          # 0131
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0132
  { add r6, r5, r0 | nopi | nopi | lw r8, 64(r0) }            # 0133
  { nopi | nopi | nopi | nopi }                               # 0134
  { sll r6, r6, r7 | nopi | nopi | nopi }                     # 0135
  { nopi | nopi | nopi | nopi }                               # 0136
  { add r6, r6, r8 | nopi | nopi | nopi }                     # 0137
  { nopi | nopi | nopi | nopi }                               # 0138
  { nopi | nopi | nopi | sw r4, 0(r6) }                       # 0139
  { mov t0, 1 | nopi | nopi | nopi }                          # 0140  linea 13
  { nopi | nopi | nopi | nopi }                               # 0141
  { nopi | nopi | jefR rz, rl | sw t0, 52(rz) }               # 0142  linea 14  slots 0=0,1=0,2=14,3=13
  { nopi | nopi | nopi | nopi }                               # 0143
  { nopi | nopi | jef .L_if_end_7 | nopi }                    # 0144  linea 10
  { nopi | nopi | nopi | nopi }                               # 0145
.L_if_next_6:                                                 # bundle 146
.L_if_end_7:                                                  # bundle 146
.L_for_update_4:                                              # bundle 146
  { nopi | nopi | nopi | lw t0, 76(rz) }                      # 0146  linea 9
  { nopi | nopi | nopi | nopi }                               # 0147
  { saddi t0, t0, 1 | nopi | nopi | nopi }                    # 0148
  { nopi | nopi | nopi | nopi }                               # 0149
  { nopi | nopi | jef .L_for_start_3 | sw t0, 76(rz) }        # 0150
  { nopi | nopi | nopi | nopi }                               # 0151
.L_for_end_5:                                                 # bundle 152
.L_for_update_1:                                              # bundle 152
  { nopi | nopi | nopi | lw t0, 68(rz) }                      # 0152  linea 8
  { nopi | nopi | nopi | nopi }                               # 0153
  { saddi t0, t0, 1 | nopi | nopi | nopi }                    # 0154
  { nopi | nopi | nopi | nopi }                               # 0155
  { nopi | nopi | jef .L_for_start_0 | sw t0, 68(rz) }        # 0156
  { nopi | nopi | nopi | nopi }                               # 0157
.L_for_end_2:                                                 # bundle 158
  { nopi | nopi | jefR rz, rl | nopi }                        # 0158  linea 18
  { nopi | nopi | nopi | nopi }                               # 0159
fin_main:                                                     # bundle 160
  { nopi | nopi | jef fin_main | nopi }                       # 0160  linea -
.L_arith_error:                                               # bundle 161
  { nopi | nopi | jef .L_arith_error | nopi }                 # 0161

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_buscar                  34
#   .L_body_buscar                  36
#   .L_for_start_0                  67
#   .L_for_start_3                  77
#   .L_if_next_6                   146
#   .L_if_end_7                    146
#   .L_for_update_4                146
#   .L_for_end_5                   152
#   .L_for_update_1                152
#   .L_for_end_2                   158
#   fin_main                       160
#   .L_arith_error                 161

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
