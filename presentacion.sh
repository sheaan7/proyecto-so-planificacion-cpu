#!/usr/bin/env bash
# Sustentación guiada: muestra cada paso y espera Enter. Uso: ./presentacion.sh
# Todo corre dentro del contenedor (imagen so-planificador); no escribe en resultados/.
set -euo pipefail
cd "$(dirname "$0")"

IMG=so-planificador
N=0

paso() { # paso "Título" "Idea clave"
    N=$((N + 1))
    clear 2>/dev/null || true
    printf '\033[1;36m[%d/8] %s\033[0m\n\033[33m>> %s\033[0m\n\n' "$N" "$1" "$2"
}
pausa() { printf '\n\033[2m[Enter para continuar, Ctrl+C para salir]\033[0m'; read -r _; }
cmd() { printf '\033[1;32m$ %s\033[0m\n' "$*"; "$@"; }
sim() { docker run --rm "$IMG" "escenarios/$1.txt"; }
gantt() { grep -E '^(===|Gantt)' | grep -v COMPARA; }
comparacion() { sed -n '/=== COMPARACIÓN ===/,/^$/p'; }

command -v docker >/dev/null || { echo "Falta Docker."; exit 1; }
docker image inspect "$IMG" >/dev/null 2>&1 || cmd docker build -t "$IMG" .

paso "Simulador de planificación de CPU (FCFS, SJF, Round Robin)" \
     "Hay más procesos listos que CPUs: el planificador decide quién corre y cuánto."
cat <<'EOF'
              llega            despacho             termina
   NUEVO ───────────► LISTO ───────────► EJECUCIÓN ───────────► FINALIZADO
                       ▲  ▲                 │   │
                       │  └── agota quantum ┘   │ pide E/S
                       │        (solo RR)       ▼
                       └──── termina E/S ─── BLOQUEADO

   FCFS: orden de llegada      SJF: la ráfaga más corta      RR: turnos de q unidades
EOF
pausa

paso "Validación: el ejemplo del libro (efecto convoy)" \
     "A mano: FCFS espera (0+24+27)/3 = 17. SJF espera (0+3+6)/3 = 3. El programa da lo mismo."
cmd cat escenarios/1_efecto_convoy.txt
echo
out=$(sim 1_efecto_convoy)
gantt <<<"$out"
comparacion <<<"$out"
pausa

paso "Pruebas automáticas" \
     "Los números del simulador se comprueban contra el cálculo a mano antes de confiar en ellos."
cmd docker run --rm --entrypoint ./test_planificador "$IMG"
pausa

paso "Diseño: el PCB y la política de selección" \
     "Cada proceso es un struct (PCB). Los tres algoritmos solo cambian en elegir()."
sed -n '/PCB simplificado/,/} Proceso;/p' planificador.h
echo
sed -n '/^static int elegir/,/^}/p' planificador.c
pausa

paso "Escenario 2: procesos de CPU contra procesos de E/S" \
     "Mirar el log: 'solicita E/S -> BLOQUEADO' y luego 'termina E/S -> LISTO'."
out=$(sim 2_mixto_cpu_es)
sed -n '/Log de estados/,/^$/p' <<<"$out" | head -22
echo "..."
comparacion <<<"$out"
pausa

paso "Escenario 3: inanición" \
     "LARGO espera 2 con FCFS y 38 con SJF, pero SJF tiene el mejor promedio: el promedio esconde al que sufre."
out=$(sim 3_inanicion)
gantt <<<"$out"
echo
echo "Fila del proceso LARGO (FCFS, SJF, RR):"
grep -E '^(pid|LARGO) ' <<<"$out" | awk 'NR==1 || /^LARGO/'
comparacion <<<"$out"
pausa

paso "Escenario 4: el quantum es un compromiso" \
     "q=1: mejor respuesta (1.2) pero 26 cambios de contexto. q=20: idéntico a FCFS."
comparacion <<<"$(sim 4_variacion_quantum)"
pausa

paso "Conclusiones" "No hay un algoritmo mejor en todo."
cat <<'EOF'
   SJF   mínima espera promedio, pero deja sin atender a los procesos largos (inanición).
   FCFS  simple y sin inanición, pero sufre el efecto convoy.
   RR    mejor respuesta y equidad, a cambio de más cambios de contexto.

   Riesgos: quantum mal elegido desperdicia CPU; SJF sin aging niega el servicio a algunos.
   Límites: cambio de contexto con costo 0, una CPU, ráfagas conocidas de antemano.
   Los SO reales combinan los tres: rebanadas de tiempo + prioridades dinámicas + aging.

   Escenario en vivo (lo que pida el profesor):
   docker run --rm -v "$PWD/escenarios:/app/escenarios" so-planificador escenarios/demo.txt
EOF
echo
