"""Contrato --check: front-end real sin producir ni alterar artefactos."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="calser_check_") as directory:
        folder = Path(directory)
        source = folder / "fuente con espacios.cls"
        for ext in (".bin", ".hex", ".mem", ".asm", ".json"):
            source.with_suffix(ext).write_bytes(b"conservar")
        artifacts = {p: p.read_bytes() for p in folder.iterdir()}

        def run(*args):
            return subprocess.run([str(ROOT / "ccom"), *map(str, args)], cwd=folder, capture_output=True, text=True, timeout=10)

        cases = [("p02_factorial.cls", 0, None), ("e52_caracter_invalido.cls", 1, "error lexico"), ("e16_falta_punto_coma.cls", 1, "error sintactico"), ("e01_no_declarado.cls", 1, "error semantico")]
        for name, code, category in cases:
            source.write_bytes((ROOT / "pruebas" / name).read_bytes())
            before = source.read_bytes()
            result = run("--check", source)
            assert result.returncode == code, result.stderr
            assert not result.stdout, result.stdout
            if category:
                assert category in result.stderr, result.stderr
            else:
                assert result.stderr == "ccom: Análisis correcto: sin errores.\n", result.stderr
            assert source.read_bytes() == before
            assert set(folder.iterdir()) == set(artifacts) | {source}
            assert all(p.read_bytes() == data for p, data in artifacts.items())
        source.write_bytes((ROOT / "pruebas/p02_factorial.cls").read_bytes())
        verbose = run(source, "--check", "-v", "--time")
        assert verbose.returncode == 0 and "analisis semantico ... ok" in verbose.stderr
        assert "tiempo de analisis (completado)" in verbose.stderr
        assert "generacion" not in verbose.stderr and "ensamblado" not in verbose.stderr
        for args in (["-s"], ["-x"], ["-p"], ["-m"], ["-t"], ["--no-schedule"], ["-o", source], ["--ast-json", source]):
            result = run("--check", *args, source)
            assert result.returncode == 2 and "--check solo admite" in result.stderr, result
        assert run("--check").returncode == 2
        assert run("--check", folder / "inexistente.cls").returncode == 1
        assert "--check" in run("--help").stdout
        assert all(p.read_bytes() == data for p, data in artifacts.items())
    print("PASS --check: lexico/sintaxis/semantica, errores de uso y archivos intactos")


if __name__ == "__main__":
    main()
