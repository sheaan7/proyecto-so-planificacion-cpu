/* Pruebas contra el ejemplo resuelto a mano de Silberschatz (Operating System Concepts, cap. 5).
 * Ejecutar: make test
 */
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "planificador.h"

static Resultado r;

static Proceso proc(const char *pid, int llegada, int n, ...) {
    Proceso p = {0};
    va_list a;
    va_start(a, n);
    strcpy(p.pid, pid);
    p.llegada = llegada;
    p.n_rafagas = n;
    for (int i = 0; i < n; i++)
        p.rafagas[i] = va_arg(a, int);
    va_end(a);
    return p;
}

static void esperas(Algoritmo alg, int q, int e1, int e2, int e3) {
    Proceso ps[] = {proc("P1", 0, 1, 24), proc("P2", 0, 1, 3), proc("P3", 0, 1, 3)};
    assert(simular(ps, 3, alg, q, &r) == 0);
    assert(r.procs[0].espera == e1 && r.procs[1].espera == e2 && r.procs[2].espera == e3);
}

int main(void) {
    esperas(SJF, 0, 6, 0, 3);    /* promedio 3 */
    esperas(RR, 4, 6, 4, 7);     /* promedio 5.66 */
    esperas(FCFS, 0, 0, 24, 27); /* promedio 17 */

    MetricaProc m[3];
    Resumen s;
    metricas(&r, m, &s); /* sobre el último: FCFS */
    assert(fabs(s.espera_prom - 17.0) < 1e-9 && s.cambios == 2 && s.tiempo_total == 30);

    /* Con E/S: P1 usa CPU 2, se bloquea 3, vuelve 1. P2 aprovecha la CPU mientras P1 está bloqueado. */
    Proceso es[] = {proc("P1", 0, 3, 2, 3, 1), proc("P2", 0, 1, 4)};
    assert(simular(es, 2, FCFS, 0, &r) == 0);
    int esperado[] = {0, 0, 1, 1, 1, 1, 0};
    assert(r.ticks == 7 && memcmp(r.gantt, esperado, sizeof esperado) == 0);
    assert(r.procs[0].fin == 7 && r.procs[0].espera == 1);

    /* Entradas inválidas: número par de ráfagas, ráfaga 0, RR sin quantum */
    Proceso par = proc("X", 0, 2, 2, 3), cero = proc("X", 0, 1, 0);
    assert(simular(&par, 1, FCFS, 0, &r) == -1);
    assert(simular(&cero, 1, FCFS, 0, &r) == -1);
    assert(simular(es, 2, RR, 0, &r) == -1);

    puts("OK: todas las pruebas pasaron");
    return 0;
}
