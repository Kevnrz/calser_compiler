#!/bin/bash
# Corre ccom sobre cada .cls y guarda: salida de error, tabla y codigo de salida.
# Uso: ./pruebas/correr_pruebas.sh [archivo_de_salida]
cd "$(dirname "$0")/.." || exit 1

out="${1:-pruebas/resultados.txt}"
: > "$out"

for f in pruebas/*.cls; do
    ./ccom "$f" > /tmp/ccom_out.txt 2> /tmp/ccom_err.txt
    code=$?

    {
        echo "===== $f ====="
        echo "--- stderr ---"
        cat /tmp/ccom_err.txt
        echo "--- tabla ---"
        sed -n '/^Scope 0/,$p' /tmp/ccom_out.txt
        echo "--- codigo de salida: $code ---"
        echo
    } >> "$out"
done

echo "Resultados en $out"
