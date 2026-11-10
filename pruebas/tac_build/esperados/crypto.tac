initial_data bytes=16
  @00000000 = 00001000
  @00000004 = 00002000
  @00000008 = 00003000
  @0000000c = 00004000

function main (entry B0)
B0: successors [B1]
  goto B1
B1: successors []
  t0 = neg 193210613
  autenticar [t0]
  cargar_llave [0, 0, 19114957]
  t1 = neg 1985229329
  cargar_llave [0, 1, t1]
  cargar_llave [0, 2, 324508639]
  cargar_llave [0, 3, 610839776]
  sellar []
  t2 = origen
  t3 = cifrado
  cifrar [t2, t3, 128, 0]
  t4 = cifrado
  t5 = vuelta
  descifrar [t4, t5, 128, 0]
  cerrar_sesion []
  t6 = origen
  t7 = negado
  cifrar [t6, t7, 1, 0]
  autenticar [999]
  cambiar_ancla [5]
  cifrar_total = 0
  return

