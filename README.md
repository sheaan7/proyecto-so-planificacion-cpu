# Simulador de Planificación de CPU en C — Proyecto Final SO (Opción 1)

Simula la gestión de procesos (creación, estados LISTO, EJECUCIÓN, BLOQUEADO y FINALIZADO) y compara **FCFS**, **SJF** (no expropiativo) y **Round Robin** (con varios quantums), midiendo espera, respuesta, retorno, cambios de contexto, equidad (Jain) e inanición (espera máxima).

## Requisitos
- Compilador C11 (`gcc` o `clang`) y `make`.
- Linux, macOS o WSL (usa `glob.h` y `sys/stat.h` de POSIX).
- **No necesita librerías externas**.

## Compilación y ejecución
```bash
make                    # compila ./simulador y ./test_planificador
make test               # pruebas contra el ejemplo resuelto del libro
./simulador             # corre todos los escenarios de escenarios/
./simulador escenarios/3_inanicion.txt   # un solo escenario
make clean              # borra los binarios
```
Las evidencias quedan en `resultados/`: un `.txt` por escenario (diagrama de Gantt, tabla por proceso, log de cambios de estado y comparación) y `resumen.csv` (se puede abrir en Excel para hacer gráficos).

## Ejecución con Docker (sustentación)
Solo se necesita Docker; el compilador va dentro de la imagen (Alpine + gcc). Las pruebas corren al construir la imagen: si fallan, no se construye.
```bash
docker build -t so-planificador .

# Todos los escenarios, guardando las evidencias en ./resultados del equipo anfitrión
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD/resultados:/app/resultados" so-planificador

# Un solo escenario (solo consola)
docker run --rm so-planificador escenarios/2_mixto_cpu_es.txt

# Pruebas
docker run --rm --entrypoint ./test_planificador so-planificador

# Escenario creado en vivo, sin reconstruir la imagen
docker run --rm -v "$PWD/escenarios:/app/escenarios" so-planificador escenarios/mi_escenario.txt
```
En Windows (PowerShell) se cambia `$PWD` por `${PWD}` y se omite `--user ...`.

## Estructura
| Archivo | Contenido |
|---|---|
| `planificador.h` / `planificador.c` | PCB, motor de simulación, algoritmos y métricas |
| `main.c` | Lee escenarios, ejecuta los algoritmos y guarda resultados |
| `test.c` | Pruebas automáticas (`assert`) |
| `Makefile` | Compilación, pruebas y limpieza |
| `Dockerfile` | Imagen para ejecutar todo en un contenedor |
| `escenarios/*.txt` | Escenarios de prueba |
| `resultados/` | Evidencias generadas |
| `INFORME.md` | Borrador del informe técnico y guion de sustentación |
| `presentacion.sh` | Sustentación guiada paso a paso (`./presentacion.sh`, requiere la imagen Docker) |
| `PRESENTACION.md` | Instructivo: qué decir en cada pantalla del script |

## Crear un escenario propio
```text
# comentario
nombre mi_escenario
descripcion Lo que quiero demostrar
quantums 2 4
proceso P1 0 5 3 2
proceso P2 1 4
```
Cada línea `proceso` es: `pid llegada ráfagas...`. Las ráfagas alternan CPU y E/S y deben empezar y terminar en CPU: `5 3 2` = 5 de CPU, 3 bloqueado en E/S y 2 de CPU.
Si no se pone `quantums`, RR usa q=4. Límites: 64 procesos, 32 ráfagas por proceso y 8 quantums por escenario.
