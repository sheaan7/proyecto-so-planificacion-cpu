"""Ejecuta cada escenario de escenarios/*.json con FCFS, SJF y RR (uno por quantum).

Uso:  python3 main.py                       # todos los escenarios
      python3 main.py escenarios/x.json     # uno solo
Salida: consola + resultados/<escenario>.txt (Gantt, tablas, log) + resultados/resumen.csv
"""
import csv
import glob
import json
import os
import sys

from planificador import gantt_texto, metricas, simular


def tabla(filas):
    cols = list(filas[0])
    anchos = [max(len(str(c)), *(len(str(f[c])) for f in filas)) for c in cols]
    linea = lambda vals: "  ".join(str(v).rjust(a) for v, a in zip(vals, anchos))
    return "\n".join([linea(cols), linea("-" * a for a in anchos)] + [linea(f.values()) for f in filas])


def correr(ruta):
    with open(ruta, encoding="utf-8") as f:
        esc = json.load(f)
    configs = [("FCFS", None), ("SJF", None)] + [("RR", q) for q in esc.get("quantums", [4])]
    salida, resumenes = [f"ESCENARIO: {esc['nombre']}\n{esc.get('descripcion', '')}\n"], []
    for alg, q in configs:
        nombre = f"RR(q={q})" if q else alg
        res = simular(esc["procesos"], alg, q)
        filas, resumen = metricas(res)
        resumenes.append({"escenario": esc["nombre"], "algoritmo": nombre, **resumen})
        salida += [f"=== {nombre} ===", "Gantt: " + gantt_texto(res["gantt"]), "",
                   tabla(filas), "", "Log de estados:", *res["log"], ""]
    salida += ["=== COMPARACIÓN ===", tabla([{k: v for k, v in r.items() if k != "escenario"}
                                           for r in resumenes])]
    return esc["nombre"], "\n".join(salida), resumenes


def main():
    rutas = sys.argv[1:] or sorted(glob.glob(os.path.join(os.path.dirname(__file__) or ".", "escenarios", "*.json")))
    if not rutas:
        sys.exit("No hay escenarios en escenarios/*.json")
    os.makedirs("resultados", exist_ok=True)
    todos = []
    for ruta in rutas:
        try:
            nombre, texto, resumenes = correr(ruta)
        except (OSError, json.JSONDecodeError, KeyError, ValueError) as e:
            print(f"[ERROR] {ruta}: {e}", file=sys.stderr)
            continue
        print(texto, "\n")
        with open(os.path.join("resultados", f"{nombre}.txt"), "w", encoding="utf-8") as f:
            f.write(texto + "\n")
        todos += resumenes
    if todos:
        with open(os.path.join("resultados", "resumen.csv"), "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(todos[0]))
            w.writeheader()
            w.writerows(todos)
        print("Resultados guardados en resultados/")


if __name__ == "__main__":
    main()
