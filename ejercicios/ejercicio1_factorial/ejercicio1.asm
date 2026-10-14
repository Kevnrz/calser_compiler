# ccom: ejercicios/ejercicio1_factorial/ejercicio1.cls
  { mov rx, 120 | nopi | nopi | nopi }                                   # 0000
  { nopi | nopi | nopi | nopi }                                          # 0001
.L_func_main:                                                            # bundle 2
.L_body_main:                                                            # bundle 2
  { mov t0, 5 | nopi | nopi | nopi }                                     # 0002  linea 15
  { nopi | nopi | nopi | nopi }                                          # 0003
  { nopi | nopi | nopi | sw t0, 64(rz) }                                 # 0004
  { mov t0, 0 | nopi | nopi | nopi }                                     # 0005  linea 16
  { nopi | nopi | nopi | nopi }                                          # 0006
  { nopi | nopi | nopi | sw t0, 68(rz) }                                 # 0007
  { nopi | nopi | nopi | lw t0, 64(rz) }                                 # 0008  linea 17
  { nopi | nopi | nopi | nopi }                                          # 0009
  { nopi | nopi | nopi | sw t0, 0(rx) }                                  # 0010
  { saddi rx, rx, 4 | nopi | nopi | nopi }                               # 0011
  { nopi | nopi | nopi | nopi }                                          # 0012
  { saddi rx, rx, -4 | nopi | nopi | nopi }                              # 0013
  { nopi | nopi | nopi | nopi }                                          # 0014
  { nopi | nopi | nopi | lw t0, 0(rx) }                                  # 0015
  { nopi | nopi | nopi | nopi }                                          # 0016
  { nopi | nopi | nopi | sw t0, 8(rz) }                                  # 0017
  { nopi | nopi | nopi | sw rl, 0(rx) }                                  # 0018
  { saddi rx, rx, 4 | nopi | jas rl, .L_func_factorial | nopi }          # 0019
  { nopi | nopi | nopi | nopi }                                          # 0020
  { saddi rx, rx, -4 | nopi | nopi | nopi }                              # 0021
  { nopi | nopi | nopi | nopi }                                          # 0022
  { nopi | nopi | nopi | lw rl, 0(rx) }                                  # 0023
  { nopi | nopi | nopi | lw t0, 4(rz) }                                  # 0024
  { nopi | nopi | nopi | nopi }                                          # 0025
  { nopi | nopi | jef fin_main | sw t0, 68(rz) }                         # 0026  linea 18  slots 0=0,1=0,2=18,3=17
  { nopi | nopi | nopi | nopi }                                          # 0027
.L_func_factorial:                                                       # bundle 28
  { nopi | nopi | nopi | sw rz, 4(rz) }                                  # 0028  linea 1
  { nopi | nopi | nopi | nopi }                                          # 0029
.L_body_factorial:                                                       # bundle 30
  { mov t1, 0 | nopi | nopi | lw t0, 8(rz) }                             # 0030  linea 2
  { nopi | nopi | nopi | nopi }                                          # 0031
  { slt t0, t0, t1 | nopi | nopi | nopi }                                # 0032
  { nopi | nopi | nopi | nopi }                                          # 0033
  { nopi | nopi | beq t0, rz, .L_if_next_0 | nopi }                      # 0034
  { nopi | nopi | nopi | nopi }                                          # 0035
  { nopi | nopi | jefR rz, rl | nopi }                                   # 0036  linea 3
  { nopi | nopi | nopi | nopi }                                          # 0037
  { nopi | nopi | jef .L_if_end_1 | nopi }                               # 0038  linea 2
  { nopi | nopi | nopi | nopi }                                          # 0039
.L_if_next_0:                                                            # bundle 40
.L_if_end_1:                                                             # bundle 40
  { mov t0, 1 | nopi | nopi | nopi }                                     # 0040  linea 5
  { nopi | nopi | nopi | nopi }                                          # 0041
  { nopi | nopi | nopi | sw t0, 4(rz) }                                  # 0042
  { mov t0, 2 | nopi | nopi | nopi }                                     # 0043  linea 6
  { nopi | nopi | nopi | nopi }                                          # 0044
  { nopi | nopi | nopi | sw t0, 12(rz) }                                 # 0045
  { nopi | nopi | nopi | nopi }                                          # 0046
.L_while_start_2:                                                        # bundle 47
  { nopi | nopi | nopi | lw t0, 12(rz) }                                 # 0047  linea 7
  { nopi | nopi | nopi | lw t1, 8(rz) }                                  # 0048
  { nopi | nopi | nopi | nopi }                                          # 0049
  { sle t0, t0, t1 | nopi | nopi | nopi }                                # 0050
  { nopi | nopi | nopi | nopi }                                          # 0051
  { nopi | nopi | beq t0, rz, .L_while_end_3 | nopi }                    # 0052
  { nopi | nopi | nopi | nopi }                                          # 0053
  { mov r10, 1 | mov r6, 0 | nopi | lw t0, 4(rz) }                       # 0054  linea 8
  { nopi | nopi | nopi | lw t1, 12(rz) }                                 # 0055
  { add r7, r4, r0 | nopi | nopi | nopi }                                # 0056
  { add r8, r5, r0 | nopi | nopi | nopi }                                # 0057
  { nopi | nopi | nopi | nopi }                                          # 0058
__ccom_arith_0_mul:                                                      # bundle 59
  { nopi | nopi | beq r8, r0, __ccom_arith_2_end | nopi }                # 0059
  { nopi | nopi | nopi | nopi }                                          # 0060
  { and r9, r8, r10 | nopi | nopi | nopi }                               # 0061
  { nopi | nopi | nopi | nopi }                                          # 0062
  { nopi | nopi | beq r9, r0, __ccom_arith_1_skip | nopi }               # 0063
  { nopi | nopi | nopi | nopi }                                          # 0064
  { add r6, r6, r7 | nopi | nopi | nopi }                                # 0065
  { nopi | nopi | nopi | nopi }                                          # 0066
__ccom_arith_1_skip:                                                     # bundle 67
  { sll r7, r7, r10 | srl r8, r8, r10 | jef __ccom_arith_0_mul | nopi }  # 0067
  { nopi | nopi | nopi | nopi }                                          # 0068
__ccom_arith_2_end:                                                      # bundle 69
  { mov t1, 1 | nopi | nopi | sw t2, 4(rz) }                             # 0069  linea 9  slots 0=9,1=0,2=0,3=8
  { nopi | nopi | nopi | lw t0, 12(rz) }                                 # 0070
  { nopi | nopi | nopi | nopi }                                          # 0071
  { add t0, t0, t1 | nopi | nopi | nopi }                                # 0072
  { nopi | nopi | nopi | nopi }                                          # 0073
  { nopi | nopi | jef .L_while_start_2 | sw t0, 12(rz) }                 # 0074  linea 7  slots 0=0,1=0,2=7,3=9
  { nopi | nopi | nopi | nopi }                                          # 0075
.L_while_end_3:                                                          # bundle 76
  { nopi | nopi | jefR rz, rl | nopi }                                   # 0076  linea 11
  { nopi | nopi | nopi | nopi }                                          # 0077
fin_main:                                                                # bundle 78
  { nopi | nopi | jef fin_main | nopi }                                  # 0078  linea -
.L_arith_error:                                                          # bundle 79
  { nopi | nopi | jef .L_arith_error | nopi }                            # 0079

# Tabla de etiquetas (direccion = indice de bundle)
#   .L_func_main                     2
#   .L_body_main                     2
#   .L_func_factorial               28
#   .L_body_factorial               30
#   .L_if_next_0                    40
#   .L_if_end_1                     40
#   .L_while_start_2                47
#   __ccom_arith_0_mul              59  (interna)
#   __ccom_arith_1_skip             67  (interna)
#   __ccom_arith_2_end              69  (interna)
#   .L_while_end_3                  76
#   fin_main                        78
#   .L_arith_error                  79

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
