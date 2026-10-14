#!/usr/bin/env python3
"""El generador real debe describir todas sus instrucciones para planificar."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROGRAMS = ['pruebas/p02_factorial.cls', 'pruebas/p09_crypto.cls',
            'pruebas/p11_ejercicio3.cls', 'pruebas/programas/factorial_r.cls']
for source in PROGRAMS:
    result = subprocess.run([str(ROOT/'build/revisar_ir'), str(ROOT/source)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    print(result.stdout.strip())
print('PASS IR del generador: factorial, crypto, ejercicio 3 y R sin operaciones desconocidas')
