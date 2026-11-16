# Demostración manual de las plantillas de crypto_para_ccom.md §4.
# No es salida de ccom; su integración al AST sigue pendiente.
# Cargar 1024 bytes en 0x1000 con --load-data.
# auth usa x99 = 0xF47BD70B contra el ancla de fabrica 0xFEFF95FF.
{ mov t6, -5897 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll t6, t6, t11 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi t6, t6, 22283 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | auth t6 | nopi | nopi }
{ mov t6, 583 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll t6, t6, t11 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi t6, t6, 11213 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | kv_write t6, 0, 0 | nopi | nopi }
{ mov t6, -60585 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll t6, t6, t11 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi t6, t6, 19951 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | kv_write t6, 0, 1 | nopi | nopi }
{ mov t6, 9903 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll t6, t6, t11 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi t6, t6, 7135 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | kv_write t6, 0, 2 | nopi | nopi }
{ mov t6, 18641 | mov t11, 15 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ sll t6, t6, t11 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ saddi t6, t6, 11488 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | kv_write t6, 0, 3 | nopi | nopi }
{ nopi | kv_lock | nopi | nopi }
{ mov t4, 0 | mov t5, 128 | nopi | nopi }
{ mov t2, 4096 | mov t3, 8192 | nopi | nopi }
{ nopi | nopi | bge rz, t5, fin_cifra_1 | nopi }
cifra_1: { nopi | nopi | nopi | lw t0, 0(t2) }
{ nopi | nopi | nopi | lw t1, 4(t2) }
{ saddi t2, t2, 8 | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }
{ saddi t4, t4, 1 | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }
{ nopi | nopi | nopi | sw t0, 0(t3) }
{ saddi t3, t3, 8 | nopi | blt t4, t5, cifra_1 | sw t1, 4(t3) }
fin_cifra_1:
{ mov t4, 0 | mov t5, 128 | nopi | nopi }
{ mov t2, 8192 | mov t3, 12288 | nopi | nopi }
{ nopi | nopi | bge rz, t5, fin_descifra_2 | nopi }
descifra_2: { nopi | nopi | nopi | lw t0, 4(t2) }
{ nopi | nopi | nopi | lw t1, 0(t2) }
{ saddi t2, t2, 8 | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 3 | nopi | nopi }
{ saddi t4, t4, 1 | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 2 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 1 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 0 | nopi | nopi }
{ nopi | nopi | nopi | sw t0, 4(t3) }
{ saddi t3, t3, 8 | nopi | blt t4, t5, descifra_2 | sw t1, 0(t3) }
fin_descifra_2:
{ nopi | logout | nopi | nopi }
{ mov t4, 0 | mov t5, 1 | nopi | nopi }
{ mov t2, 4096 | mov t3, 16384 | nopi | nopi }
{ nopi | nopi | bge rz, t5, fin_cifra_3 | nopi }
cifra_3: { nopi | nopi | nopi | lw t0, 0(t2) }
{ nopi | nopi | nopi | lw t1, 4(t2) }
{ saddi t2, t2, 8 | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 0 | nopi | nopi }
{ saddi t4, t4, 1 | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 1 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t0, t0, t1, 0, 2 | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | feis t1, t1, t0, 0, 3 | nopi | nopi }
{ nopi | nopi | nopi | sw t0, 0(t3) }
{ saddi t3, t3, 8 | nopi | blt t4, t5, cifra_3 | sw t1, 4(t3) }
fin_cifra_3:
{ mov t6, 999 | nopi | nopi | nopi }
{ nopi | nopi | nopi | nopi }
{ nopi | auth t6 | nopi | nopi }
fin_main: { nopi | nopi | jef fin_main | nopi }
