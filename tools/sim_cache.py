#!/usr/bin/env python3
"""Simulador funcional con métricas de pipeline y caché."""

import argparse
from collections import deque
import json
from pathlib import Path
import sys

from sim_asm import (
    Annotations, AsmError, MEMORY, Simulator, Tracer, address_argument, branch_target, parse,
)


# Cada respuesta lleva aceptaci?n registrada, listo y origen.
SIN_RESPUESTA = (False, False, 0)


class MemoriaPrincipal:
    def __init__(self, mem_div):
        if mem_div not in (2, 4, 5):
            raise AsmError("MEM_DIV debe ser 2, 4 o 5")
        self.tmem = 25 + 7 * mem_div
        self.busy = self.accepted = False
        self.age = self.total_cycles = self.transfer_cycles = 0
        self.read_count = self.write_count = 0

    def combinar(self):
        return self.accepted, self.busy and self.age + 1 >= self.tmem, 0

    def flanco(self, pedido, respuesta):
        self.accepted = False
        self.total_cycles += 1
        if self.busy:
            self.age += 1
            self.transfer_cycles += 1
            self.busy = not respuesta[1]
        elif pedido is not None:
            we, _ = pedido
            self.busy = self.accepted = True
            self.age = 0
            self.write_count += we
            self.read_count += not we

    def contadores(self):
        return {
            "mem_lecturas": self.read_count, "mem_escrituras": self.write_count,
            "mem_transferencia": self.transfer_cycles, "mem_ciclos_totales": self.total_cycles,
        }


class Adaptador:
    def __init__(self):
        self.active = self.sent = False

    def combinar(self, pedido, respuesta, full=False):
        accepted, ready, _ = respuesta
        bajar = not self.active or not (self.sent or accepted)
        return self.active and ready, pedido if bajar else None, None, accepted

    def flanco(self, pedido, salida):
        listo, _, _, accepted = salida
        if listo:
            self.active = self.sent = False
        elif not self.active and pedido is not None:
            self.active = True
        elif self.active and accepted:
            self.sent = True


class Accesos:
    def __init__(self):
        self.total = [0, 0]
        self.aciertos = [0, 0]
        self.fallos = [0, 0]

    def clasificar(self, we, hit):
        (self.aciertos if hit else self.fallos)[we] += 1

    def contadores(self, nivel):
        contadores = {}
        for we, nombre in enumerate(("lecturas", "escrituras")):
            clave = f"{nivel}_{nombre}"
            contadores[clave] = self.total[we]
            contadores[f"{clave}_aciertos"] = self.aciertos[we]
            contadores[f"{clave}_fallos"] = self.fallos[we]
        return contadores


class L1:
    def __init__(self):
        self.tags = [[None, None] for _ in range(64)]
        self.dirty = [[False, False] for _ in range(64)]
        self.lru = [0] * 64
        self.saved = None
        self.sent = self.evicted = False
        self.pending_stalls = self.stall_full_cycles = 0
        self.stalls = [0, 0, 0]
        self.accesos = Accesos()

    def combinar(self, pedido, respuesta, full):
        if self.saved is None and pedido is None:
            return False, None, None, None
        accepted, ready, src = respuesta
        if self.saved is None:
            we, addr = pedido
            index, tag = (addr >> 5) & 63, addr >> 11
            vias = self.tags[index]
            if tag in vias:
                return True, None, None, (we, addr, vias.index(tag), True, False, src, accepted)
            way = vias.index(None) if None in vias else self.lru[index]
        else:
            we, addr, way = self.saved
            index = (addr >> 5) & 63
        needs_push = self.dirty[index][way] and not self.evicted
        blocked = needs_push and full
        push = ((self.tags[index][way] << 6) | index) if needs_push and not full else None
        line = (False, addr >> 5) if not (blocked or self.sent or accepted) else None
        return ready and not blocked, line, push, (we, addr, way, False, blocked, src, accepted)

    def flanco(self, pedido, salida):
        listo, _, push, acceso = salida
        if acceso is None:
            return
        we, addr, way, hit, blocked, src, accepted = acceso
        index = (addr >> 5) & 63
        if self.saved is None:
            self.accesos.total[we] += 1
            self.accesos.clasificar(we, hit)
        if listo:
            self.lru[index] = 1 - way
            if hit:
                self.dirty[index][way] |= we
            else:
                self.tags[index][way] = addr >> 11
                self.dirty[index][way] = we
                self.saved = None
                self.sent = self.evicted = False
                self.stalls[src] += self.pending_stalls
                self.pending_stalls = 0
        else:
            self.saved = we, addr, way
            self.evicted |= push is not None
            self.sent |= not blocked and accepted
            self.stall_full_cycles += blocked
            self.pending_stalls += not blocked

    def contadores(self):
        return {**self.accesos.contadores("l1"),
                "l1_pausa_buffer": self.stalls[2], "l1_pausa_l2": self.stalls[1],
                "l1_pausa_memoria": self.stalls[0], "l1_pausa_lleno": self.stall_full_cycles}


class BufferLineas:
    def __init__(self, depth):
        self.depth = depth
        self.cola = deque()
        self.state = "IDLE"
        self.saved_read_addr = 0
        self.received_count = self.forwarding_count = self.full_cycles = self.max_occupancy = 0

    @property
    def full(self):
        return len(self.cola) == self.depth

    def combinar(self, pedido, respuesta):
        accepted, ready, src = respuesta
        state = self.state
        lectura = pedido is not None and not pedido[0]
        hit = lectura and pedido[1] in self.cola
        reenviar = hit and state not in ("READ_REQ", "READ_WAIT")
        ruta = None
        bajar = None
        if state == "IDLE":
            if lectura and not hit:
                ruta, bajar = "READ", pedido
            elif self.cola:
                ruta, bajar = "DRAIN", (True, self.cola[0])
        elif state in ("READ_REQ", "READ_WAIT"):
            ruta = "READ"
            if state == "READ_REQ" and not accepted:
                bajar = False, self.saved_read_addr
        else:
            ruta = "DRAIN"
            if state == "DRAIN_REQ" and not accepted:
                bajar = True, self.cola[0]
        subir = respuesta if ruta == "READ" else SIN_RESPUESTA
        if reenviar:
            subir = subir[0], True, 2
        return subir, bajar, ruta, reenviar

    def flanco(self, pedido, push, respuesta, salida):
        _, _, ruta, reenviar = salida
        accepted, ready, _ = respuesta
        full = self.full
        self.full_cycles += full
        self.forwarding_count += reenviar
        if ruta == "DRAIN" and ready:
            self.cola.popleft()
        if push is not None and not full:
            self.cola.append(push)
            self.received_count += 1
            self.max_occupancy = max(self.max_occupancy, len(self.cola))
        if ruta is None:
            return
        if self.state == "IDLE" and ruta == "READ":
            self.saved_read_addr = pedido[1]
        if ready:
            self.state = "IDLE"
        elif self.state == "IDLE":
            self.state = f"{ruta}_REQ"
        elif self.state.endswith("_REQ") and accepted:
            self.state = f"{ruta}_WAIT"

    def contadores(self, nombre):
        return {
            f"{nombre}_recibidas": self.received_count,
            f"{nombre}_forwarding": self.forwarding_count,
            f"{nombre}_ciclos_lleno": self.full_cycles,
            f"{nombre}_ocupacion_max": self.max_occupancy,
        }


class L2:
    def __init__(self):
        self.tags = [[None] * 4 for _ in range(128)]
        self.dirty = [[False] * 4 for _ in range(128)]
        self.state = "IDLE"
        self.accepted = False
        self.saved_we = False
        self.saved_addr = self.age = self.stall_full_cycles = 0
        self.pushed = self.sent = False
        self.lfsr = 0xA5
        self.accesos = Accesos()

    def combinar(self, respuesta, full):
        accepted, ready, src = respuesta
        if self.state == "IDLE" or (self.state == "LOOKUP" and self.age != 8):
            return (self.accepted, False, src), None, None, None
        idx, tag = self.saved_addr & 127, self.saved_addr >> 7
        vias = self.tags[idx]
        hit = tag in vias
        victim = vias.index(None) if None in vias else self.lfsr & 3
        if self.state == "WAIT_LINE":
            line = (False, self.saved_addr) if not (self.sent or accepted) else None
            return (self.accepted, ready, src), line, None, (victim, False, False)
        if hit:
            return (self.accepted, True, 1), None, None, (vias.index(tag), True, False)
        must_evict = self.dirty[idx][victim] and not self.pushed
        blocked = must_evict and full
        if blocked:
            return (self.accepted, False, src), None, None, (victim, False, True)
        push = (vias[victim] << 7) | idx if must_evict else None
        if self.saved_we:
            return (self.accepted, True, 1), None, push, (victim, False, False)
        line = (False, self.saved_addr) if not (self.sent or accepted) else None
        return (self.accepted, ready, src), line, push, (victim, False, False)

    def flanco(self, pedido, respuesta, salida):
        self.accepted = False
        if self.state == "IDLE":
            if pedido is not None:
                self.accepted = True
                self.saved_we, self.saved_addr = pedido
                self.age = 1
                self.pushed = self.sent = False
                self.accesos.total[self.saved_we] += 1
                self.state = "LOOKUP"
            return
        subir, _, push, acceso = salida
        if acceso is None:
            self.age += 1
            return
        way, hit, blocked = acceso
        if self.state == "LOOKUP":
            self.accesos.clasificar(self.saved_we, hit)
        if blocked:
            self.stall_full_cycles += 1
            self.state = "MISS"
            return
        self.pushed |= push is not None
        self.sent |= respuesta[0]
        if subir[1]:
            idx = self.saved_addr & 127
            if hit:
                self.dirty[idx][way] |= self.saved_we
            else:
                self.tags[idx][way] = self.saved_addr >> 7
                self.dirty[idx][way] = self.saved_we
                bits = self.lfsr
                feedback = ((bits >> 7) ^ (bits >> 5) ^ (bits >> 4) ^ (bits >> 3)) & 1
                self.lfsr = ((bits << 1) & 255) | feedback
            self.state = "IDLE"
        else:
            self.state = "WAIT_LINE"

    def contadores(self):
        return {**self.accesos.contadores("l2"), "l2_pausa_lleno": self.stall_full_cycles}


class Jerarquia:
    def __init__(self, cache, mem_div, wbuf_depth=4, ebuf_depth=8):
        if cache not in (0, 1, 2):
            raise AsmError("configuraci?n CACHE inv?lida")
        self.memoria = MemoriaPrincipal(mem_div)
        self.nucleo = L1() if cache else Adaptador()
        self.wbuf = BufferLineas(wbuf_depth) if cache else None
        self.l2 = L2() if cache == 2 else None
        self.ebuf = BufferLineas(ebuf_depth) if self.l2 else None

    def ciclo(self, req, we, addr):
        if not req and not self.memoria.busy:
            if self.wbuf is None:
                reposo = not self.nucleo.active
            else:
                reposo = (self.nucleo.saved is None and self.wbuf.state == "IDLE"
                          and not self.wbuf.cola)
                if self.l2:
                    reposo = (reposo and self.l2.state == "IDLE"
                              and self.ebuf.state == "IDLE" and not self.ebuf.cola)
            if reposo:
                self.memoria.total_cycles += 1
                return False
        pedido = (bool(we), addr & 0xFFFF) if req else None
        memoria = self.memoria.combinar()
        # Las respuestas se propagan hacia el n?cleo hasta estabilizar las se?ales.
        resp_wbuf = memoria if self.wbuf is None else SIN_RESPUESTA
        resp_ebuf = SIN_RESPUESTA
        resp_l2 = SIN_RESPUESTA if self.l2 else memoria
        full_wbuf = self.wbuf.full if self.wbuf else False
        full_ebuf = self.ebuf.full if self.ebuf else False
        anterior = None
        for _ in range(8):
            salida = self.nucleo.combinar(pedido, resp_wbuf, full_wbuf)
            wbuf = self.wbuf.combinar(salida[1], resp_l2) if self.wbuf else None
            l2 = self.l2.combinar(resp_ebuf, full_ebuf) if self.l2 else None
            ebuf = self.ebuf.combinar(l2[1], memoria) if self.ebuf else None
            actual = salida, wbuf, l2, ebuf
            if actual == anterior:
                break
            anterior = actual
            if wbuf:
                resp_wbuf = wbuf[0]
            if l2:
                resp_l2, resp_ebuf = l2[0], ebuf[0]
        else:
            raise RuntimeError("la jerarqu?a no converge")

        # Cada flanco usa las entradas calculadas con el estado previo de todos.
        self.nucleo.flanco(pedido, salida)
        inferior = salida[1]
        if wbuf:
            self.wbuf.flanco(salida[1], salida[2], resp_l2, wbuf)
            inferior = wbuf[1]
        elif inferior is not None:
            we, addr = inferior
            inferior = we, addr >> 5
        if l2:
            self.l2.flanco(inferior, resp_ebuf, l2)
            self.ebuf.flanco(l2[1], l2[2], memoria, ebuf)
            inferior = ebuf[1]
        self.memoria.flanco(inferior, memoria)
        return salida[0]

    def contadores(self):
        contadores = self.memoria.contadores()
        if self.wbuf:
            contadores.update(self.nucleo.contadores())
            contadores.update(self.wbuf.contadores("wbuf"))
        if self.l2:
            contadores.update(self.l2.contadores())
            contadores.update(self.ebuf.contadores("ebuf"))
        return contadores


def claves_cache(nivel, pausas):
    accesos = (
        f"{nivel}_{operacion}{sufijo}"
        for operacion in ("lecturas", "escrituras")
        for sufijo in ("", "_aciertos", "_fallos")
    )
    return (*accesos, *(f"{nivel}_pausa_{p}" for p in pausas),
            f"{nivel}_hit_rate", f"{nivel}_miss_rate")


def claves_buffer(nombre):
    return tuple(f"{nombre}_{k}" for k in
                 ("recibidas", "forwarding", "ciclos_lleno", "ocupacion_max"))


GRUPOS = (
    ("Configuraci?n", ("cache", "mem_div", "tmem", "corte")),
    ("Procesador", ("ciclos", "bundles", "instrucciones", "ipc", "saltos",
                    "pausa_crypto", "pausa_memoria", "pausa_total", "ex_anulado")),
    ("L1", claves_cache("l1", ("buffer", "l2", "memoria", "lleno"))),
    ("Write buffer", claves_buffer("wbuf")),
    ("L2", claves_cache("l2", ("lleno",))),
    ("Eviction buffer", claves_buffer("ebuf")),
    ("Memoria principal", ("mem_lecturas", "mem_escrituras", "mem_transferencia",
                           "mem_ciclos_totales", "mem_utilizacion")),
    ("AMAT", ("amat_medido", "amat_formula")),
)


def tasa(numerador, denominador):
    return numerador / denominador if denominador else 0.0


class CacheSimulator(Simulator):
    def __init__(self, program, warn=None, cache=2, mem_div=4):
        super().__init__(program, warn)
        if cache not in ("ideal", 0, 1, 2):
            raise AsmError("configuración CACHE inválida")
        if mem_div not in (2, 4, 5):
            raise AsmError("MEM_DIV debe ser 2, 4 o 5")
        self.cache = cache
        self.mem_div = mem_div
        self.jerarquia = None if cache == "ideal" else Jerarquia(cache, mem_div)
        self.corte = "sin corte"
        self.ex_anulado = 0
        self.ciclos = self.bundles = self.instrucciones = self.saltos = 0
        self.pausa_crypto = self.pausa_memoria = self.pausa_total = 0
        self._jef_finales = set()
        for pc, bundle in enumerate(program.bundles):
            salto = bundle.slots[2]
            if (salto.mnemonic == "jef"
                    and branch_target(salto.args[-1], pc, program.labels, 22) == pc):
                self._jef_finales.add(pc)
        self._foto = {}
        self.metricas = {}
        self._ciclo()
        if 0 in self._jef_finales:
            self._guardar_foto(1)
        self._ciclo()

    def _ciclo(self, req=False, we=False, addr=0):
        self.ciclos += 1
        return self.jerarquia.ciclo(req, we, addr) if self.jerarquia else True

    def _contadores(self):
        return {
            "ciclos": self.ciclos, "bundles": self.bundles,
            "instrucciones": self.instrucciones, "saltos": self.saltos,
            "pausa_crypto": self.pausa_crypto, "pausa_memoria": self.pausa_memoria,
            "pausa_total": self.pausa_total, "ex_anulado": self.ex_anulado,
        }

    def _guardar_foto(self, ex_anulado):
        self._foto = self._contadores()
        self._foto["ex_anulado"] = ex_anulado
        self._foto_jerarquia = self.jerarquia.contadores() if self.jerarquia else {}

    def step(self):
        slots = self.program.bundles[self.pc].slots
        auth = slots[1].mnemonic == "auth"
        op = slots[3]
        req = op.mnemonic in MEMORY
        we = op.mnemonic in ("sw", "sb")
        addr = 0
        if req:
            _, offset, base = op.args
            addr = self.regs[base] + offset
        old_cycles = self.cycles
        sigue = super().step()
        if not sigue:
            for nombre, valor in self._foto.items():
                setattr(self, nombre, valor)
            self.corte = "jef"
            return False

        # El modelo funcional añade exactamente un ciclo por salto tomado.
        tomado = self.cycles - old_cycles > 1 + 13 * auth
        if tomado:
            self.saltos += 1
        anticipar_corte = self.pc in self._jef_finales
        resta_crypto = 13 * auth
        while True:
            # Se conserva el estado previo al flanco de confirmaci?n.
            if anticipar_corte and not tomado and not resta_crypto:
                self._guardar_foto(0)
            listo = self._ciclo(req, we, addr)
            pausa_memoria = req and not listo
            req = pausa_memoria
            if not resta_crypto and not pausa_memoria:
                break
            self.pausa_crypto += bool(resta_crypto)
            self.pausa_memoria += pausa_memoria
            self.pausa_total += 1
            resta_crypto = max(0, resta_crypto - 1)
        self.bundles += 1
        self.instrucciones += sum(op.mnemonic != "nopi" for op in slots)
        if tomado:
            if anticipar_corte:
                self._guardar_foto(1)
            self._ciclo()
        return True

    def run(self, max_bundles=100000):
        super().run(max_bundles)
        esperado = self.bundles + self.saltos + self.pausa_total + 2 - self.ex_anulado
        if self.ciclos != esperado:
            raise RuntimeError(f"identidad de ciclos: {self.ciclos} != {esperado}")
        self.metricas = dict.fromkeys(k for _, claves in GRUPOS for k in claves)
        self.metricas.update(cache=self.cache, mem_div=self.mem_div, corte=self.corte,
                             ipc=tasa(self.instrucciones, self.ciclos), **self._contadores())
        if self.jerarquia:
            actual = self.jerarquia.contadores() if self.corte == "sin corte" else self._foto_jerarquia
            self.metricas.update(actual)
            self.metricas["tmem"] = self.jerarquia.memoria.tmem
            self.metricas["mem_utilizacion"] = tasa(actual["mem_transferencia"],
                                                   actual["mem_ciclos_totales"])
            for nivel in ("l1", "l2"):
                if f"{nivel}_lecturas" not in actual:
                    continue
                accesos = actual[f"{nivel}_lecturas"] + actual[f"{nivel}_escrituras"]
                for metrica, nombre in (("hit_rate", "aciertos"), ("miss_rate", "fallos")):
                    cantidad = actual[f"{nivel}_lecturas_{nombre}"] + actual[f"{nivel}_escrituras_{nombre}"]
                    self.metricas[f"{nivel}_{metrica}"] = tasa(cantidad, accesos)
            if self.cache == 2:
                accesos_l1 = actual["l1_lecturas"] + actual["l1_escrituras"]
                self.metricas["amat_medido"] = 1 + tasa(self.pausa_memoria, accesos_l1)
                self.metricas["amat_formula"] = 1 + self.metricas["l1_miss_rate"] * (
                    8 + self.metricas["l2_miss_rate"] * self.metricas["tmem"]
                )
        return self

    def report(self):
        _, estado = super().report().split("\n", 1)
        primero = (f"Fin: {self.reason}; bundles ejecutados: {self.steps}; "
                   f"ciclos: {self.ciclos} (CACHE={self.cache}, MEM_DIV={self.mem_div})")
        metricas = "\n".join(f"  {k} = {v}" for k, v in self.metricas.items() if v is not None)
        return f"{primero}\nMétricas\n{metricas}\n{estado}"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("asm", type=Path, help="listado con etiquetas y bundles")
    parser.add_argument("--max-bundles", type=int, default=100000, help="l?mite de bundles")
    parser.add_argument("--data-image", type=Path, help="imagen est?tica .mem de ccom")
    parser.add_argument("--load-data", type=Path, help="archivo binario para cargar en memoria")
    parser.add_argument("--data-address", type=address_argument, default=0x1000,
                        help="direcci?n de carga decimal o hexadecimal (por defecto 0x1000)")
    parser.add_argument("--dump-memory", type=Path, help="guardar los 64 KiB de memoria final")
    parser.add_argument("--traza", type=Path, help="guardar la traza de ejecuci?n")
    parser.add_argument("--traza-max", type=int, default=5000, help="l?mite de eventos de la traza")
    parser.add_argument("--cache", choices=("ideal", "0", "1", "2"), default="2")
    parser.add_argument("--mem-div", type=int, choices=(2, 4, 5), default=4)
    parser.add_argument("--metricas-json", type=Path, help="guardar las m?tricas en JSON")
    args = parser.parse_args(argv)

    try:
        asm = args.asm.read_text()
        program = parse(asm)
        sim = CacheSimulator(program, lambda msg: print(msg, file=sys.stderr),
                             args.cache if args.cache == "ideal" else int(args.cache),
                             args.mem_div)
        entradas = {p.resolve() for p in (args.asm, args.data_image, args.load_data) if p}
        if args.traza_max != 5000 and not args.traza:
            raise AsmError("--traza-max requiere --traza")
        if args.traza:
            if args.traza_max <= 0:
                raise AsmError("--traza-max debe ser positivo")
            if args.traza.resolve() in entradas:
                raise AsmError("la traza no debe sobrescribir un archivo de entrada")
            sim.tracer = Tracer(Annotations(asm, len(program.bundles)),
                                program.labels, args.traza_max)
        if args.data_image:
            sim.load_data_image(args.data_image.read_text())
        if args.load_data:
            sim.load_data(args.load_data.read_bytes(), args.data_address)
        elif args.data_address != 0x1000:
            raise AsmError("--data-address requiere --load-data")
        try:
            sim.run(args.max_bundles)
        finally:
            if sim.tracer:
                args.traza.write_text(sim.tracer.text(sim), encoding="utf-8")
        if args.dump_memory:
            if args.dump_memory.resolve() in entradas:
                raise AsmError("el volcado de memoria no debe sobrescribir un archivo de entrada")
            args.dump_memory.write_bytes(sim.memory)
        if args.metricas_json:
            if args.metricas_json.resolve() in entradas:
                raise AsmError("el JSON de m?tricas no debe sobrescribir un archivo de entrada")
            args.metricas_json.write_text(json.dumps(sim.metricas, indent=2) + "\n",
                                          encoding="utf-8")
        print(sim.report())
        return int(bool(sim.warnings) or sim.reason == "límite de bundles")
    except (OSError, UnicodeError, AsmError) as error:
        print(f"sim_cache: error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
