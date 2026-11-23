#!/usr/bin/env python3
"""Pruebas de temporización, métricas y equivalencia funcional."""

from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import io
import json
from pathlib import Path
import runpy
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tools"))

import sim_asm
from sim_cache import BufferLineas, CacheSimulator, L1, L2, MemoriaPrincipal, SIN_RESPUESTA

MODOS = ("ideal", 0, 1, 2)
ESTADO = ("regs", "memory", "pc", "cycles", "steps", "pending", "warnings",
          "reason", "auth", "kv_lock", "exc", "anchor", "vault", "crypto_count")
PROCESADOR = ("ciclos", "bundles", "instrucciones", "saltos", "pausa_crypto", "pausa_total")
EJERCICIOS = (
    ("ejercicio1_factorial", "ejercicio1", (230, 202, 167, 27, 0, 0)),
    ("ejercicio2_matriz", "ejercicio2", (382, 366, 290, 15, 0, 0)),
    ("ejercicio3_cifrado", "ejercicio3", (17084, 14771, 13978, 2299, 13, 13)),
)
RUN_FUNCIONAL = sim_asm.Simulator.run


def identidad(sim):
    m = sim.metricas
    assert m["ciclos"] == m["bundles"] + m["saltos"] + m["pausa_total"] + 2 - m["ex_anulado"], m
    if sim.cache == "ideal":
        assert m["pausa_memoria"] == 0, m
    else:
        assert m["mem_ciclos_totales"] == m["ciclos"], m


def comparar(base, max_bundles=100000):
    tracer = base.tracer
    if tracer is None:
        notas = sim_asm.Annotations("", len(base.program.bundles))
        base.tracer = sim_asm.Tracer(notas, base.program.labels, 5000)
    inicial = {k: deepcopy(getattr(base, k)) for k in ESTADO}
    traza_inicial = deepcopy(base.tracer)

    def ejecutar(sim):
        try:
            RUN_FUNCIONAL(sim, max_bundles) if sim is base else sim.run(max_bundles)
        except sim_asm.AsmError as error:
            return error
        return None

    try:
        error_base = ejecutar(base)
        ciclos = {}
        metricas = {}
        for modo in MODOS:
            sim = CacheSimulator(base.program, cache=modo)
            for k, v in inicial.items():
                setattr(sim, k, deepcopy(v))
            sim.tracer = deepcopy(traza_inicial)
            error = ejecutar(sim)
            assert (type(error), str(error)) == (type(error_base), str(error_base)), (modo, error)
            for k in ESTADO:
                assert getattr(sim, k) == getattr(base, k), (modo, k)
            assert sim.tracer.text(sim) == base.tracer.text(base), (modo, "traza")
            if error is None:
                identidad(sim)
                ciclos[modo] = sim.ciclos
                metricas[modo] = sim.metricas
        if error_base is not None:
            raise error_base
        assert ciclos["ideal"] <= ciclos[2], ciclos
        return metricas
    finally:
        base.tracer = tracer


def cargar_listado(archivo, datos=None):
    texto = archivo.read_text(encoding="utf-8")
    programa = sim_asm.parse(texto)
    sim = sim_asm.Simulator(programa)
    sim.tracer = sim_asm.Tracer(sim_asm.Annotations(texto, len(programa.bundles)),
                              programa.labels, 5000)
    imagen = archivo.with_suffix(".mem")
    if imagen.is_file():
        sim.load_data_image(imagen.read_text(encoding="utf-8"))
    externo = archivo.parent / "datos.txt"
    if datos is None and externo.is_file():
        datos = externo.read_bytes()
    if datos is not None:
        sim.load_data(datos)
    return sim


class JerarquiaTest(unittest.TestCase):
    def test_memoria(self):
        for div, tmem in ((2, 39), (4, 53), (5, 60)):
            with self.subTest(mem_div=div):
                memoria = MemoriaPrincipal(div)
                self.assertEqual(memoria.tmem, tmem)
                for k in range(tmem + 2):
                    respuesta = memoria.combinar()
                    self.assertEqual(respuesta[1], k == tmem)
                    memoria.flanco((k > 0, 0), respuesta)
                    self.assertEqual(memoria.accepted, k in (0, tmem + 1))
                self.assertEqual((memoria.transfer_cycles, memoria.read_count, memoria.write_count),
                                 (tmem, 1, 1))
        with self.assertRaises(sim_asm.AsmError):
            MemoriaPrincipal(3)

    def test_l1_y_write_buffer(self):
        l1 = L1()
        l1.tags[2] = [0, 1]
        l1.dirty[2] = [True, True]
        hit = l1.combinar((False, 64), SIN_RESPUESTA, False)
        assert hit[:3] == (True, None, None)
        l1.flanco((False, 64), hit)
        assert l1.lru[2] == 1
        pedido = True, (2 << 11) + 64
        blocked = l1.combinar(pedido, SIN_RESPUESTA, True)
        assert blocked[:3] == (False, None, None)
        l1.flanco(pedido, blocked)
        assert l1.stall_full_cycles == 1
        miss = l1.combinar(pedido, SIN_RESPUESTA, False)
        assert miss[:3] == (False, (False, 130), 66)
        l1.flanco(pedido, miss)
        assert l1.combinar(pedido, SIN_RESPUESTA, False)[2] is None

        buf = BufferLineas(4)
        def edge(pedido=None, push=None, respuesta=SIN_RESPUESTA):
            salida = buf.combinar(pedido, respuesta)
            buf.flanco(pedido, push, respuesta, salida)
            return salida

        edge(push=7)
        assert edge((False, 7))[:2] == ((False, True, 2), (True, 7))
        assert buf.forwarding_count == 1
        assert edge((False, 9), respuesta=(True, False, 0))[1] is None
        assert buf.state == "DRAIN_WAIT"
        assert edge((False, 9), respuesta=(False, True, 0))[0] == SIN_RESPUESTA
        assert not buf.cola
        edge(push=11)
        assert edge((False, 12))[1] == (False, 12)
        assert buf.state == "READ_REQ"

    def test_l2(self):
        l2 = L2()
        def transaccion(pedido):
            for ciclo in range(9):
                salida = l2.combinar(SIN_RESPUESTA, False)
                assert salida[0][1] == (ciclo == 8)
                assert salida[1] is None
                l2.flanco(pedido if ciclo == 0 else None, SIN_RESPUESTA, salida)
            assert l2.state == "IDLE"

        assert l2.lfsr == 0xA5
        for tag, lfsr in enumerate((0x4A, 0x95, 0x2A, 0x54, 0xA9)):
            transaccion((True, tag << 7))
            assert l2.lfsr == lfsr, hex(l2.lfsr)
            way = tag if tag < 4 else 0
            assert l2.tags[0][way] == tag and l2.dirty[0][way]
        transaccion((False, 4 << 7))
        assert l2.accesos.aciertos[0] == 1
        assert l2.accesos.fallos[1] == 5
        assert l2.lfsr == 0xA9


class SimuladorTest(unittest.TestCase):
    def test_ejemplo(self):
        m = comparar(cargar_listado(HERE / "cache_ejemplo.asm"))
        for modo, ciclos, pausa in (("ideal", 5, 0), (0, 111, 106), (2, 66, 61)):
            self.assertEqual(tuple(m[modo][k] for k in PROCESADOR), (ciclos, 3, 3, 0, 0, pausa))
            self.assertEqual(m[modo]["pausa_memoria"], pausa)
            self.assertEqual(m[modo]["ex_anulado"], 0)
        self.assertEqual(tuple(m[0][k] for k in ("mem_lecturas", "mem_escrituras", "mem_transferencia",
                                                 "mem_ciclos_totales")), (1, 1, 105, 111))
        esperado = dict(l1_escrituras=1, l1_escrituras_fallos=1, l1_lecturas=0,
                        l1_pausa_memoria=61, wbuf_recibidas=0, wbuf_forwarding=0,
                        l2_lecturas=1, l2_lecturas_fallos=1, ebuf_recibidas=0,
                        mem_lecturas=1, mem_escrituras=0, mem_transferencia=53,
                        mem_ciclos_totales=66)
        for clave, valor in esperado.items():
            self.assertEqual(m[2][clave], valor, clave)

    def test_ejercicios(self):
        for carpeta, nombre, esperado in EJERCICIOS:
            with self.subTest(programa=nombre):
                m = comparar(cargar_listado(ROOT / "ejercicios" / carpeta / f"{nombre}.asm"))
                self.assertEqual(tuple(m["ideal"][k] for k in PROCESADOR), esperado)
                for modo in (1, 2):
                    pausas_l1 = sum(m[modo][f"l1_pausa_{k}"] for k in ("buffer", "l2", "memoria", "lleno"))
                    self.assertEqual(pausas_l1, m[modo]["pausa_memoria"])

    def test_listados_isa(self):
        for archivo in sorted(HERE.glob("ejemplo_*.asm")):
            with self.subTest(programa=archivo.name):
                comparar(cargar_listado(archivo))

    def test_corte_y_pausas_simultaneas(self):
        for modo in MODOS:
            primero = CacheSimulator(sim_asm.parse("{ nopi | nopi | jef 0 | nopi }"),
                                     cache=modo).run()
            self.assertEqual((primero.ciclos, primero.bundles, primero.ex_anulado), (1, 0, 1))
            programa = sim_asm.parse(
                "{ nopi | nopi | jef 1 | nopi }\n{ nopi | nopi | jef 0 | nopi }"
            )
            limite = CacheSimulator(programa, cache=modo).run(1)
            self.assertEqual((limite.ciclos, limite.bundles, limite.saltos, limite.corte),
                             (4, 1, 1, "sin corte"))
            programa = sim_asm.parse(
                "{ nopi | auth rz | nopi | lw r4, 0(rz) }\n{ nopi | nopi | jef 0 | nopi }"
            )
            sim = CacheSimulator(programa, cache=modo).run()
            self.assertEqual(sim.pausa_crypto, 13)
            self.assertEqual(sim.pausa_total, max(13, sim.pausa_memoria))

    def test_cli_json(self):
        archivo = HERE / "cache_ejemplo.asm"
        with TemporaryDirectory() as tmp:
            archivo_json = Path(tmp) / "metricas.json"
            comando = [sys.executable, str(ROOT / "tools/sim_cache.py"), str(archivo),
                       "--metricas-json", str(archivo_json)]
            clave = None
            for modo in MODOS:
                salida = subprocess.run(comando + ["--cache", str(modo)], capture_output=True,
                                        text=True, encoding="utf-8")
                self.assertEqual(salida.returncode, 0, salida.stderr)
                m = json.loads(archivo_json.read_text(encoding="utf-8"))
                if clave is None:
                    clave = set(m)
                    self.assertEqual(len(clave), 49)
                self.assertEqual(set(m), clave)
                self.assertEqual(m["cache"], modo)
                for prefijo, presente in (("l1_", modo in (1, 2)), ("wbuf_", modo in (1, 2)),
                                          ("l2_", modo == 2), ("ebuf_", modo == 2),
                                          ("amat_", modo == 2)):
                    self.assertTrue(all((valor is not None) == presente for k, valor in m.items()
                                        if k.startswith(prefijo)))


def correr_script(ruta, argumentos):
    with patch.object(sys, "argv", [str(ruta), *argumentos]):
        try:
            runpy.run_path(str(ruta), run_name="__main__")
        except SystemExit as salida:
            return salida.code or 0
    return 0


def verificar_listados_de_pasadas(carpeta):
    """Ejecuta las variantes que las pruebas del optimizador compilan por separado."""
    casos = []
    optimizador = ROOT / "pruebas/optimizador"
    for pasada in ("dce", "rename", "reorder", "spill", "unroll"):
        directorio = optimizador / pasada
        esperados = json.loads((directorio / "esperados.json").read_text(encoding="utf-8"))
        for nombre, caso in esperados.items():
            opcion = f"--opt={pasada}"
            if nombre == "_corpus":
                for fuente in caso:
                    casos.extend(((ROOT / fuente, ["-O0"]), (ROOT / fuente, ["-O1", opcion])))
            elif pasada == "unroll":
                fuente = ROOT / caso["fuente"]
                casos.extend(((fuente, ["-O0"]), (fuente, ["-O1", opcion, *caso["opciones"]])))
            else:
                fuente = directorio / f"{nombre}.cls"
                if pasada != "spill":
                    casos.append((fuente, ["-O1"]))
                casos.append((fuente, ["-O1", opcion]))

    archivo = carpeta / "pasada.asm"
    datos = (bytes(1024), bytes(range(256)) * 4)
    for fuente, opciones in casos:
        resultado = subprocess.run(
            [str(ROOT / "ccom"), *opciones, "-s", str(fuente.relative_to(ROOT)),
             "-o", str(archivo.with_suffix(".bin"))],
            cwd=ROOT, capture_output=True, text=True, timeout=60,
        )
        assert resultado.returncode == 0, (fuente, opciones, resultado.stderr)
        for contenido in datos:
            cargar_listado(archivo, contenido).run(2000000)


def equivalencia_check():
    """Repite las ejecuciones funcionales de las suites con los cuatro modos."""
    suites = (
        ("pruebas/ensamblado/simulador/probar.py", ()),
        ("pruebas/ensamblado/bundles/probar_organizador.py", ()),
        ("pruebas/generador_o0/programas/probar.py", ()),
        ("pruebas/generador_o1/tac_emit/comparar.py", ()),
        ("pruebas/generador_o1/tac_emit/comparar.py", ("--opt=reorder",)),
        ("pruebas/generador_o1/tac_emit/comparar.py", ("--opt=spill",)),
        ("pruebas/generador_o1/tac_emit/comparar.py", ("--opt=unroll,rename,dce,reorder,spill",)),
        ("pruebas/rutinas/crypto/probar.py", ("build/emitir_crypto",)),
        ("pruebas/rutinas/crypto/probar_integracion.py", ()),
        ("pruebas/medir/probar.py", ()),
    )
    casos = 0
    usar_subproceso = subprocess.run

    def verificar(sim, max_bundles=100000):
        nonlocal casos
        if isinstance(sim, CacheSimulator):
            return RUN_FUNCIONAL(sim, max_bundles)
        comparar(sim, max_bundles)
        casos += 1
        return sim

    def ejecutar(comando, *args, **kwargs):
        # Las pruebas también invocan el simulador original desde su CLI.
        if comando[0] == sys.executable:
            for i, op in enumerate(comando[1:3], 1):
                ruta = Path(op)
                if ruta not in (ROOT / "tools/sim_asm.py", ROOT / "tools/medir.py"):
                    continue
                with redirect_stdout(io.StringIO()) as out, redirect_stderr(io.StringIO()) as err:
                    codigo = correr_script(ruta, comando[i + 1:])
                streams = out.getvalue(), err.getvalue()
                if not kwargs.get("text"):
                    streams = tuple(s.encode("utf-8") for s in streams)
                return subprocess.CompletedProcess(comando, codigo, *streams)
        return usar_subproceso(comando, *args, **kwargs)

    with TemporaryDirectory() as tmp, patch.object(sim_asm.Simulator, "run", verificar), \
            patch.object(subprocess, "run", ejecutar):
        for ruta, argumentos in (*suites, ("tools/validar_bundles.py", ("--output", tmp))):
            antes = casos
            with redirect_stdout(io.StringIO()) as salida:
                codigo = correr_script(ROOT / ruta, argumentos)
            assert codigo == 0 and casos > antes, (ruta, argumentos, codigo, salida.getvalue())
            opciones = " " + " ".join(argumentos) if argumentos and ruta != "tools/validar_bundles.py" else ""
            print(f"PASS equivalencia {ruta}{opciones} ({casos - antes} ejecuciones, cuatro modos)")
        antes = casos
        verificar_listados_de_pasadas(Path(tmp))
        print(f"PASS equivalencia pasadas individuales ({casos - antes} ejecuciones, cuatro modos)")


if __name__ == "__main__":
    con_compilador = "--con-compilador" in sys.argv
    if con_compilador:
        sys.argv.remove("--con-compilador")
    resultado = unittest.main(exit=False, verbosity=2).result
    if not resultado.wasSuccessful():
        sys.exit(1)
    if con_compilador:
        equivalencia_check()
