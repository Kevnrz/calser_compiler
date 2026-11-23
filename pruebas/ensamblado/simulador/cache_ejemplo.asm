    { mov r4, 64 | mov r5, 7 | nopi | nopi }
    { nopi | nopi | nopi | nopi }
    { nopi | nopi | nopi | sw r5, 0(r4) }
    { nopi | nopi | nopi | lw r6, 0(r4) }
fin:
    { nopi | nopi | jef fin | nopi }
