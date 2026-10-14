# ccom: ejercicios/ejercicio1_factorial/ejercicio1.cls
  { mov rx, 120 | nopi | nopi | nopi }                      # 0000
  { nopi | nopi | nopi | nopi }                             # 0001
.L_func_main:                                               # bundle 2
.L_body_main:                                               # bundle 2
  { mov t0, 5 | nopi | nopi | nopi }                        # 0002  linea 15
  { nopi | nopi | nopi | nopi }                             # 0003
  { nopi | nopi | nopi | sw t0, 64(rz) }                    # 0004
  { nopi | nopi | nopi | nopi }                             # 0005
  { mov t0, 0 | nopi | nopi | nopi }                        # 0006  linea 16
  { nopi | nopi | nopi | nopi }                             # 0007
  { nopi | nopi | nopi | sw t0, 68(rz) }                    # 0008
  { nopi | nopi | nopi | nopi }                             # 0009
  { nopi | nopi | nopi | lw t0, 64(rz) }                    # 0010  linea 17
  { nopi | nopi | nopi | nopi }                             # 0011
  { nopi | nopi | nopi | sw t0, 0(rx) }                     # 0012
  { nopi | nopi | nopi | nopi }                             # 0013
  { saddi rx, rx, 4 | nopi | nopi | nopi }                  # 0014
  { nopi | nopi | nopi | nopi }                             # 0015
  { saddi rx, rx, -4 | nopi | nopi | nopi }                 # 0016
  { nopi | nopi | nopi | nopi }                             # 0017
  { nopi | nopi | nopi | lw t0, 0(rx) }                     # 0018
  { nopi | nopi | nopi | nopi }                             # 0019
  { nopi | nopi | nopi | sw t0, 8(rz) }                     # 0020
  { nopi | nopi | nopi | nopi }                             # 0021
  { nopi | nopi | nopi | sw rl, 0(rx) }                     # 0022
  { nopi | nopi | nopi | nopi }                             # 0023
  { saddi rx, rx, 4 | nopi | nopi | nopi }                  # 0024
  { nopi | nopi | nopi | nopi }                             # 0025
  { nopi | nopi | jas rl, .L_func_factorial | nopi }        # 0026
  { nopi | nopi | nopi | nopi }                             # 0027
  { saddi rx, rx, -4 | nopi | nopi | nopi }                 # 0028
  { nopi | nopi | nopi | nopi }                             # 0029
  { nopi | nopi | nopi | lw rl, 0(rx) }                     # 0030
  { nopi | nopi | nopi | nopi }                             # 0031
  { nopi | nopi | nopi | lw t0, 4(rz) }                     # 0032
  { nopi | nopi | nopi | nopi }                             # 0033
  { nopi | nopi | nopi | sw t0, 68(rz) }                    # 0034
  { nopi | nopi | nopi | nopi }                             # 0035
  { nopi | nopi | jef fin_main | nopi }                     # 0036  linea 18
  { nopi | nopi | nopi | nopi }                             # 0037
.L_func_factorial:                                          # bundle 38
  { nopi | nopi | nopi | sw rz, 4(rz) }                     # 0038  linea 1
  { nopi | nopi | nopi | nopi }                             # 0039
.L_body_factorial:                                          # bundle 40
  { nopi | nopi | nopi | lw t0, 8(rz) }                     # 0040  linea 2
  { nopi | nopi | nopi | nopi }                             # 0041
  { mov t1, 0 | nopi | nopi | nopi }                        # 0042
  { nopi | nopi | nopi | nopi }                             # 0043
  { slt t0, t0, t1 | nopi | nopi | nopi }                   # 0044
  { nopi | nopi | nopi | nopi }                             # 0045
  { nopi | nopi | beq t0, rz, .L_if_next_0 | nopi }         # 0046
  { nopi | nopi | nopi | nopi }                             # 0047
  { nopi | nopi | jefR rz, rl | nopi }                      # 0048  linea 3
  { nopi | nopi | nopi | nopi }                             # 0049
  { nopi | nopi | jef .L_if_end_1 | nopi }                  # 0050  linea 2
  { nopi | nopi | nopi | nopi }                             # 0051
.L_if_next_0:                                               # bundle 52
.L_if_end_1:                                                # bundle 52
  { mov t0, 1 | nopi | nopi | nopi }                        # 0052  linea 5
  { nopi | nopi | nopi | nopi }                             # 0053
  { nopi | nopi | nopi | sw t0, 4(rz) }                     # 0054
  { nopi | nopi | nopi | nopi }                             # 0055
  { mov t0, 2 | nopi | nopi | nopi }                        # 0056  linea 6
  { nopi | nopi | nopi | nopi }                             # 0057
  { nopi | nopi | nopi | sw t0, 12(rz) }                    # 0058
  { nopi | nopi | nopi | nopi }                             # 0059
.L_while_start_2:                                           # bundle 60
  { nopi | nopi | nopi | lw t0, 12(rz) }                    # 0060  linea 7
  { nopi | nopi | nopi | nopi }                             # 0061
  { nopi | nopi | nopi | lw t1, 8(rz) }                     # 0062
  { nopi | nopi | nopi | nopi }                             # 0063
  { sle t0, t0, t1 | nopi | nopi | nopi }                   # 0064
  { nopi | nopi | nopi | nopi }                             # 0065
  { nopi | nopi | beq t0, rz, .L_while_end_3 | nopi }       # 0066
  { nopi | nopi | nopi | nopi }                             # 0067
  { nopi | nopi | nopi | lw t0, 4(rz) }                     # 0068  linea 8
  { nopi | nopi | nopi | nopi }                             # 0069
  { nopi | nopi | nopi | lw t1, 12(rz) }                    # 0070
  { nopi | nopi | nopi | nopi }                             # 0071
  { mov r10, 1 | nopi | nopi | nopi }                       # 0072
  { nopi | nopi | nopi | nopi }                             # 0073
  { add r7, r4, r0 | nopi | nopi | nopi }                   # 0074
  { nopi | nopi | nopi | nopi }                             # 0075
  { add r8, r5, r0 | nopi | nopi | nopi }                   # 0076
  { nopi | nopi | nopi | nopi }                             # 0077
  { mov r6, 0 | nopi | nopi | nopi }                        # 0078
  { nopi | nopi | nopi | nopi }                             # 0079
__ccom_arith_0_mul:                                         # bundle 80
  { nopi | nopi | beq r8, r0, __ccom_arith_2_end | nopi }   # 0080
  { nopi | nopi | nopi | nopi }                             # 0081
  { and r9, r8, r10 | nopi | nopi | nopi }                  # 0082
  { nopi | nopi | nopi | nopi }                             # 0083
  { nopi | nopi | beq r9, r0, __ccom_arith_1_skip | nopi }  # 0084
  { nopi | nopi | nopi | nopi }                             # 0085
  { add r6, r6, r7 | nopi | nopi | nopi }                   # 0086
  { nopi | nopi | nopi | nopi }                             # 0087
__ccom_arith_1_skip:                                        # bundle 88
  { sll r7, r7, r10 | nopi | nopi | nopi }                  # 0088
  { nopi | nopi | nopi | nopi }                             # 0089
  { srl r8, r8, r10 | nopi | nopi | nopi }                  # 0090
  { nopi | nopi | nopi | nopi }                             # 0091
  { nopi | nopi | jef __ccom_arith_0_mul | nopi }           # 0092
  { nopi | nopi | nopi | nopi }                             # 0093
__ccom_arith_2_end:                                         # bundle 94
  { nopi | nopi | nopi | sw t2, 4(rz) }                     # 0094
  { nopi | nopi | nopi | nopi }                             # 0095
  { nopi | nopi | nopi | lw t0, 12(rz) }                    # 0096  linea 9
  { nopi | nopi | nopi | nopi }                             # 0097
  { mov t1, 1 | nopi | nopi | nopi }                        # 0098
  { nopi | nopi | nopi | nopi }                             # 0099
  { add t0, t0, t1 | nopi | nopi | nopi }                   # 0100
  { nopi | nopi | nopi | nopi }                             # 0101
  { nopi | nopi | nopi | sw t0, 12(rz) }                    # 0102
  { nopi | nopi | nopi | nopi }                             # 0103
  { nopi | nopi | jef .L_while_start_2 | nopi }             # 0104  linea 7
  { nopi | nopi | nopi | nopi }                             # 0105
.L_while_end_3:                                             # bundle 106
  { nopi | nopi | jefR rz, rl | nopi }                      # 0106  linea 11
  { nopi | nopi | nopi | nopi }                             # 0107
fin_main:                                                   # bundle 108
  { nopi | nopi | jef fin_main | nopi }                     # 0108  linea -
.L_arith_error:                                             # bundle 109
  { nopi | nopi | jef .L_arith_error | nopi }               # 0109
  { nopi | nopi | nopi | nopi }                             # 0110

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_factorial               38
#   .L_body_factorial               40
#   .L_if_next_0                    52
#   .L_if_end_1                     52
#   .L_while_start_2                60
#   __ccom_arith_0_mul              80  (interna)
#   __ccom_arith_1_skip             88  (interna)
#   __ccom_arith_2_end              94  (interna)
#   .L_while_end_3                 106
#   fin_main                       108
#   .L_arith_error                 109

# Variables (direccion de datos)
#
#   ===== SYMBOL TABLE =====
#   Scope 0: GLOBAL
#       main : void (function) params=[] area=0x0040 size=56 temp_base=0x0048
#       factorial : int (function) params=[n] area=0x0000 size=64 temp_base=0x0010
#
#   Scope 1: FUNCTION factorial
#       i : int width=4 addr=0x000C line=6
#       n : int (parameter) width=4 addr=0x0008 line=1
#       res : int (return) width=4 addr=0x0004 line=1
#
#   Scope 2: C factorial
#
#   Scope 3: L factorial
#
#   Scope 4: FUNCTION main
#       f : int width=4 addr=0x0044 line=16
#       x : int width=4 addr=0x0040 line=15
#
#   =======================
