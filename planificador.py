"""Simulador de planificación de CPU (tiempo discreto, 1 tick = 1 unidad).

Cada proceso tiene una lista de ráfagas alternadas: [CPU, E/S, CPU, E/S, ..., CPU].
Estados: NUEVO -> LISTO -> EJECUCION -> (BLOQUEADO -> LISTO ...) -> FINALIZADO.
Algoritmos: FCFS, SJF (no expropiativo) y Round Robin (quantum configurable).
"""

LISTO, EJECUCION, BLOQUEADO, FINALIZADO = "LISTO", "EJECUCION", "BLOQUEADO", "FINALIZADO"


class Proceso:
    def __init__(self, pid, llegada, rafagas):
        if llegada < 0 or not rafagas or len(rafagas) % 2 == 0 or min(rafagas) <= 0:
            raise ValueError(f"{pid}: llegada >= 0 y ráfagas positivas impares [CPU, E/S, ..., CPU]")
        self.pid, self.llegada, self.rafagas = pid, llegada, list(rafagas)
        self.estado = None          # None = aún no ha llegado
        self.indice = 0             # ráfaga actual
        self.restante = rafagas[0]  # tiempo que le queda a la ráfaga actual
        self.espera = 0             # ticks en la cola de listos
        self.inicio = None          # primer tick en CPU (para tiempo de respuesta)
        self.fin = None

    @property
    def cpu_total(self):
        return sum(self.rafagas[0::2])


# Cada política elige de la cola de listos (lista en orden de llegada a la cola).
def fcfs(cola):
    return cola[0]


def sjf(cola):
    return min(cola, key=lambda p: p.restante)  # min() devuelve el primero en empate -> FCFS


def simular(procesos_def, algoritmo, quantum=None):
    """procesos_def: lista de dicts {pid, llegada, rafagas}. Devuelve dict con gantt, log y procesos."""
    elegir = {"FCFS": fcfs, "SJF": sjf, "RR": fcfs}[algoritmo]
    if algoritmo == "RR" and (not quantum or quantum <= 0):
        raise ValueError("Round Robin requiere quantum > 0")

    procs = [Proceso(d["pid"], d["llegada"], d["rafagas"]) for d in procesos_def]
    cola, bloqueados, gantt, log = [], [], [], []
    actual, usado, ultimo, cambios, t = None, 0, None, 0, 0

    while any(p.estado != FINALIZADO for p in procs):
        # 1. Llegadas (creación de procesos)
        for p in procs:
            if p.estado is None and p.llegada == t:
                p.estado = LISTO
                cola.append(p)
                log.append(f"t={t:>3} {p.pid} creado -> LISTO")
        # 2. Fin de E/S: BLOQUEADO -> LISTO
        for p in [b for b in bloqueados if b.restante == 0]:
            bloqueados.remove(p)
            p.indice += 1
            p.restante, p.estado = p.rafagas[p.indice], LISTO
            cola.append(p)
            log.append(f"t={t:>3} {p.pid} termina E/S -> LISTO")
        # 3. Expropiación por quantum (RR); va al final, después de las llegadas de este tick
        if actual and algoritmo == "RR" and usado == quantum:
            actual.estado = LISTO
            cola.append(actual)
            log.append(f"t={t:>3} {actual.pid} agota quantum -> LISTO")
            actual = None
        # 4. Despacho
        if actual is None and cola:
            actual = elegir(cola)
            cola.remove(actual)
            actual.estado, usado = EJECUCION, 0
            if actual.inicio is None:
                actual.inicio = t
            if ultimo is not None and ultimo is not actual:
                cambios += 1
            ultimo = actual
            log.append(f"t={t:>3} {actual.pid} -> EJECUCION")
        # 5. Avanza un tick
        gantt.append(actual.pid if actual else "-")
        for p in cola:
            p.espera += 1
        for p in bloqueados:
            p.restante -= 1
        if actual:
            actual.restante -= 1
            usado += 1
        t += 1
        # 6. Fin de ráfaga de CPU: a E/S o termina
        if actual and actual.restante == 0:
            actual.indice += 1
            if actual.indice < len(actual.rafagas):
                actual.restante, actual.estado = actual.rafagas[actual.indice], BLOQUEADO
                bloqueados.append(actual)
                log.append(f"t={t:>3} {actual.pid} solicita E/S -> BLOQUEADO")
            else:
                actual.estado, actual.fin = FINALIZADO, t
                log.append(f"t={t:>3} {actual.pid} -> FINALIZADO")
            actual = None

    return {"gantt": gantt, "log": log, "procesos": procs, "cambios_contexto": cambios}


def metricas(res):
    """Métricas por proceso y promedios. Retorno = fin - llegada; respuesta = 1er despacho - llegada."""
    filas = []
    for p in res["procesos"]:
        retorno = p.fin - p.llegada
        filas.append({"pid": p.pid, "llegada": p.llegada, "cpu": p.cpu_total,
                      "espera": p.espera, "respuesta": p.inicio - p.llegada,
                      "retorno": retorno, "retorno_norm": round(retorno / p.cpu_total, 2)})
    n, total = len(filas), len(res["gantt"])
    prom = lambda k: round(sum(f[k] for f in filas) / n, 2)
    esperas = [f["espera"] for f in filas]
    resumen = {"espera_prom": prom("espera"), "respuesta_prom": prom("respuesta"),
               "retorno_prom": prom("retorno"), "espera_max": max(esperas),
               "cambios_contexto": res["cambios_contexto"],
               "uso_cpu_%": round(100 * (total - res["gantt"].count("-")) / total, 1),
               "tiempo_total": total,
               # Índice de Jain sobre 1/retorno_norm: 1 = trato totalmente equitativo
               "equidad_jain": round(jain([1 / f["retorno_norm"] for f in filas]), 3)}
    return filas, resumen


def jain(xs):
    return sum(xs) ** 2 / (len(xs) * sum(x * x for x in xs))


def gantt_texto(gantt):
    """Comprime el gantt por tramos: | P1 0-3 | P2 3-5 | ..."""
    tramos, ini = [], 0
    for i in range(1, len(gantt) + 1):
        if i == len(gantt) or gantt[i] != gantt[ini]:
            tramos.append(f"{gantt[ini]} {ini}-{i}")
            ini = i
    return "| " + " | ".join(tramos) + " |"
