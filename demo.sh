#!/usr/bin/env bash
# Demo en vivo por escenario: corre el real y luego una variación mínima.
# Uso: ./demo.sh        (los cuatro)      ./demo.sh 3   (solo el escenario 3)
# En cada variación, 'e' abre el escenario en $EDITOR (nano por defecto) para cambiar valores en vivo.
# Las variaciones van a una carpeta temporal: no toca escenarios/ ni resultados/.
set -euo pipefail
cd "$(dirname "$0")"

IMG=so-planificador
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

command -v docker >/dev/null || { echo "Falta Docker."; exit 1; }
docker image inspect "$IMG" >/dev/null 2>&1 || docker build -t "$IMG" .

idea() { printf '\033[33m>> %s\033[0m\n\n' "$1"; }
titulo() { clear 2>/dev/null || true; printf '\033[1;36m%s\033[0m\n' "$1"; idea "$2"; }
pausa() { printf '\n\033[2m[Enter para continuar, Ctrl+C para salir]\033[0m'; read -r _; echo; }
filtro() { grep -E '^(===|Gantt|LARGO|algoritmo|FCFS|SJF|RR)'; } # Gantt y tabla, sin el log
real() { docker run --rm "$IMG" "escenarios/$1.txt" | filtro; }

exec 3<&0 # el teclado: dentro de variacion() stdin es el heredoc

# Recibe por stdin las líneas del escenario, las muestra y las corre.
# Con 'e' abre el escenario en el editor para cambiar valores y lo vuelve a correr.
variacion() {
    { echo "# proceso <pid> <llegada> <ráfagas CPU E/S CPU ...>"; echo "nombre demo"; cat; } >"$TMP/demo.txt"
    while :; do
        printf '\033[1;32mEscenario:\033[0m\n'
        grep -vE '^(#|nombre)' "$TMP/demo.txt" || true
        echo
        docker run --rm -v "$TMP:/app/escenarios:ro" "$IMG" escenarios/demo.txt | filtro ||
            echo "Escenario inválido: corrígelo con 'e'."
        printf '\n\033[2m[e + Enter: cambiar valores y volver a correr | Enter: continuar]\033[0m '
        read -r op <&3 || op=
        echo
        [ "$op" = e ] || break
        ${EDITOR:-nano} "$TMP/demo.txt" <&3 || true # un editor que falla no tumba la demo
    done
}

e1() {
    titulo "Escenario 1: efecto convoy" "A mano: FCFS (0+24+27)/3 = 17. SJF (0+3+6)/3 = 3."
    real 1_efecto_convoy
    pausa
    idea "El mismo caso con el largo de último: los tres dan 3. El convoy es culpa del orden."
    variacion <<'EOF'
quantums 4
proceso P2 0 3
proceso P3 0 3
proceso P1 0 24
EOF
}

e2() {
    titulo "Escenario 2: CPU contra E/S" "Respuesta 14.40 (FCFS) contra 2.80 (RR q=2). FCFS deja la CPU ociosa: 94.4 %."
    real 2_mixto_cpu_es
    pausa
    idea "La vida de ES1 con FCFS: llega en t=2 y toca la CPU en t=22. Recorre el diagrama de estados."
    docker run --rm "$IMG" escenarios/2_mixto_cpu_es.txt | sed -n '/=== FCFS ===/,/=== SJF ===/p' | grep '^t=.*ES1'
    pausa
    idea "Ejemplo mínimo: con FCFS la CPU queda vacía (- 11-16) mientras B hace E/S; con RR no."
    variacion <<'EOF'
quantums 2
proceso A 0 10
proceso B 0 1 5 1
EOF
}

e3() {
    titulo "Escenario 3: inanición" "LARGO espera 2 con FCFS y 38 con SJF, y aun así SJF tiene el mejor promedio."
    real 3_inanicion
    pausa
    idea "Ejemplo mínimo: LARGO espera 2 con FCFS y 8 con SJF."
    variacion <<'EOF'
quantums 2
proceso C1 0 3
proceso LARGO 1 10
proceso C2 2 3
proceso C3 5 3
EOF
    idea "Agregamos un corto más (C4): LARGO pasa de 8 a 11 con SJF. Cada corto le suma 3."
    variacion <<'EOF'
quantums 2
proceso C1 0 3
proceso LARGO 1 10
proceso C2 2 3
proceso C3 5 3
proceso C4 8 3
EOF
}

e4() {
    titulo "Escenario 4: el quantum" "De q=1 a q=20 la respuesta sube de 1.20 a 11.60 y los cambios de contexto bajan de 26 a 4."
    real 4_variacion_quantum
    pausa
    idea "¿Desde qué quantum RR se vuelve FCFS? Desde 10, la ráfaga más larga: nadie agota su turno."
    variacion <<'EOF'
quantums 10
proceso A 0 10
proceso B 1 4
proceso C 2 7
proceso D 3 2
proceso E 4 5
EOF
}

case "${1:-todos}" in
    1) e1 ;;
    2) e2 ;;
    3) e3 ;;
    4) e4 ;;
    todos) e1; e2; e3; e4 ;;
    *) echo "Uso: ./demo.sh [1-4]"; exit 1 ;;
esac
