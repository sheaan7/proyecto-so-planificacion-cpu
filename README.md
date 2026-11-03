# Simulador de Planificación de CPU — Proyecto Final SO (Opción 1)

Simula la gestión de procesos (estados LISTO, EJECUCIÓN, BLOQUEADO y FINALIZADO) y compara **FCFS**, **SJF** (no expropiativo) y **Round Robin** (con varios quantums).

## Requisitos
Python 3.8 o superior. **No necesita dependencias externas** (solo librería estándar).

## Ejecución
```bash
python3 test_planificador.py          # pruebas contra el ejemplo resuelto del libro
python3 main.py                       # corre los 4 escenarios
python3 main.py escenarios/3_inanicion.json   # un escenario
```
Las evidencias quedan en `resultados/`: un `.txt` por escenario (diagrama de Gantt, tabla por proceso, log de cambios de estado y comparación) y `resumen.csv` (se puede abrir en Excel para hacer gráficos).

## Estructura
| Archivo | Contenido |
|---|---|
| `planificador.py` | Motor de simulación, algoritmos y métricas |
| `main.py` | Carga escenarios, ejecuta y guarda resultados |
| `escenarios/*.json` | Escenarios de prueba |
| `test_planificador.py` | Pruebas automáticas |
| `INFORME.md` | Borrador del informe técnico y guion de sustentación |

## Crear un escenario propio
```json
{
  "nombre": "mi_escenario",
  "quantums": [2, 4],
  "procesos": [
    {"pid": "P1", "llegada": 0, "rafagas": [5, 3, 2]}
  ]
}
```
`rafagas` alterna CPU y E/S y debe empezar y terminar en CPU: `[5, 3, 2]` = 5 de CPU, 3 bloqueado en E/S y 2 de CPU.
