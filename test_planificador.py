"""Pruebas contra el ejemplo resuelto a mano de Silberschatz (Operating System Concepts, cap. 5).
Ejecutar: python3 test_planificador.py
"""
from planificador import metricas, simular

PROCS = [{"pid": "P1", "llegada": 0, "rafagas": [24]},
         {"pid": "P2", "llegada": 0, "rafagas": [3]},
         {"pid": "P3", "llegada": 0, "rafagas": [3]}]


def esperas(alg, q=None):
    filas, _ = metricas(simular(PROCS, alg, q))
    return [f["espera"] for f in filas]


assert esperas("FCFS") == [0, 24, 27]      # promedio 17
assert esperas("SJF") == [6, 0, 3]         # promedio 3
assert esperas("RR", 4) == [6, 4, 7]       # promedio 5.66

# Con E/S: P1 usa CPU 2, se bloquea 3, vuelve 1. P2 aprovecha la CPU mientras P1 está bloqueado.
res = simular([{"pid": "P1", "llegada": 0, "rafagas": [2, 3, 1]},
               {"pid": "P2", "llegada": 0, "rafagas": [4]}], "FCFS")
assert "".join(x[-1] for x in res["gantt"]) == "1122221", res["gantt"]
assert res["procesos"][0].fin == 7 and res["procesos"][0].espera == 1

for malo in ([{"pid": "X", "llegada": 0, "rafagas": [2, 3]}], [{"pid": "X", "llegada": 0, "rafagas": [0]}]):
    try:
        simular(malo, "FCFS")
        raise AssertionError("debió rechazar ráfagas inválidas")
    except ValueError:
        pass

print("OK: todas las pruebas pasaron")
