#include "planificador.h"

#include <stdio.h>
#include <string.h>

static void registrar(Resultado *r, int t, const Proceso *p, const char *evento) {
    char linea[LOG_LEN]; /* buffer aparte: p->pid vive dentro de r (evita solapamiento en snprintf) */
    snprintf(linea, sizeof linea, "t=%3d %s %s", t, p->pid, evento);
    if (r->n_log < MAX_LOG)
        memcpy(r->log[r->n_log++], linea, sizeof linea);
}

/* Quita la posición `pos` de un arreglo conservando el orden (la cola es FIFO). */
static void quitar(int *v, int *n, int pos) {
    memmove(v + pos, v + pos + 1, (size_t)(*n - pos - 1) * sizeof *v);
    (*n)--;
}

/* Política de selección: devuelve la posición elegida dentro de la cola de listos. */
static int elegir(const Proceso *ps, const int *cola, int n, Algoritmo alg) {
    int mejor = 0; /* FCFS y RR: el primero de la cola */
    if (alg == SJF)
        for (int i = 1; i < n; i++)
            if (ps[cola[i]].restante < ps[cola[mejor]].restante) /* '<' estricto: empate -> FCFS */
                mejor = i;
    return mejor;
}

static int fallo(Resultado *r, const char *pid, const char *msg) {
    snprintf(r->error, sizeof r->error, "%s: %s", pid, msg);
    return -1;
}

int simular(const Proceso *defs, int n, Algoritmo alg, int quantum, Resultado *r) {
    memset(r, 0, sizeof *r);
    if (n <= 0 || n > MAX_PROC)
        return fallo(r, "escenario", "número de procesos fuera de rango");
    if (alg == RR && quantum <= 0)
        return fallo(r, "RR", "Round Robin requiere quantum > 0");

    Proceso *ps = r->procs;
    r->n = n;
    for (int i = 0; i < n; i++) {
        const Proceso *d = &defs[i];
        if (d->llegada < 0 || d->n_rafagas <= 0 || d->n_rafagas > MAX_RAF || d->n_rafagas % 2 == 0)
            return fallo(r, d->pid, "llegada >= 0 y número impar de ráfagas [CPU, E/S, ..., CPU]");
        for (int k = 0; k < d->n_rafagas; k++)
            if (d->rafagas[k] <= 0)
                return fallo(r, d->pid, "las ráfagas deben ser positivas");
        ps[i] = *d;
        ps[i].estado = NUEVO;
        ps[i].indice = 0;
        ps[i].restante = d->rafagas[0];
        ps[i].espera = 0;
        ps[i].inicio = ps[i].fin = -1;
    }

    int cola[MAX_PROC], nc = 0; /* cola de listos */
    int bloq[MAX_PROC], nb = 0; /* procesos haciendo E/S */
    int actual = -1, usado = 0, ultimo = -1, terminados = 0, t = 0;

    while (terminados < n) {
        if (t >= MAX_TICKS)
            return fallo(r, "escenario", "la simulación supera MAX_TICKS");

        /* 1. Llegadas (creación de procesos) */
        for (int i = 0; i < n; i++)
            if (ps[i].estado == NUEVO && ps[i].llegada == t) {
                ps[i].estado = LISTO;
                cola[nc++] = i;
                registrar(r, t, &ps[i], "creado -> LISTO");
            }

        /* 2. Fin de E/S: BLOQUEADO -> LISTO */
        for (int k = 0; k < nb;) {
            Proceso *p = &ps[bloq[k]];
            if (p->restante == 0) {
                p->indice++;
                p->restante = p->rafagas[p->indice];
                p->estado = LISTO;
                cola[nc++] = bloq[k];
                registrar(r, t, p, "termina E/S -> LISTO");
                quitar(bloq, &nb, k);
            } else {
                k++;
            }
        }

        /* 3. Expropiación por quantum (RR); va al final, después de las llegadas de este tick */
        if (actual >= 0 && alg == RR && usado == quantum) {
            ps[actual].estado = LISTO;
            cola[nc++] = actual;
            registrar(r, t, &ps[actual], "agota quantum -> LISTO");
            actual = -1;
        }

        /* 4. Despacho */
        if (actual < 0 && nc > 0) {
            int k = elegir(ps, cola, nc, alg);
            actual = cola[k];
            quitar(cola, &nc, k);
            ps[actual].estado = EJECUCION;
            usado = 0;
            if (ps[actual].inicio < 0)
                ps[actual].inicio = t;
            if (ultimo >= 0 && ultimo != actual)
                r->cambios++;
            ultimo = actual;
            registrar(r, t, &ps[actual], "-> EJECUCION");
        }

        /* 5. Avanza un tick */
        r->gantt[t] = actual;
        for (int k = 0; k < nc; k++)
            ps[cola[k]].espera++;
        for (int k = 0; k < nb; k++)
            ps[bloq[k]].restante--;
        if (actual >= 0) {
            ps[actual].restante--;
            usado++;
        }
        t++;

        /* 6. Fin de ráfaga de CPU: a E/S o termina */
        if (actual >= 0 && ps[actual].restante == 0) {
            Proceso *p = &ps[actual];
            p->indice++;
            if (p->indice < p->n_rafagas) {
                p->restante = p->rafagas[p->indice];
                p->estado = BLOQUEADO;
                bloq[nb++] = actual;
                registrar(r, t, p, "solicita E/S -> BLOQUEADO");
            } else {
                p->estado = FINALIZADO;
                p->fin = t;
                terminados++;
                registrar(r, t, p, "-> FINALIZADO");
            }
            actual = -1;
        }
    }
    r->ticks = t;
    return 0;
}

/* Retorno = fin - llegada; respuesta = primer despacho - llegada.
 * Equidad: índice de Jain sobre 1/retorno_norm (1 = trato totalmente equitativo). */
void metricas(const Resultado *r, MetricaProc *m, Resumen *s) {
    double suma = 0, suma2 = 0;
    memset(s, 0, sizeof *s);
    for (int i = 0; i < r->n; i++) {
        const Proceso *p = &r->procs[i];
        int cpu = 0;
        for (int k = 0; k < p->n_rafagas; k += 2)
            cpu += p->rafagas[k];
        m[i].cpu = cpu;
        m[i].espera = p->espera;
        m[i].respuesta = p->inicio - p->llegada;
        m[i].retorno = p->fin - p->llegada;
        m[i].retorno_norm = (double)m[i].retorno / cpu;

        s->espera_prom += m[i].espera;
        s->respuesta_prom += m[i].respuesta;
        s->retorno_prom += m[i].retorno;
        if (m[i].espera > s->espera_max)
            s->espera_max = m[i].espera;
        double x = 1.0 / m[i].retorno_norm;
        suma += x;
        suma2 += x * x;
    }
    s->espera_prom /= r->n;
    s->respuesta_prom /= r->n;
    s->retorno_prom /= r->n;
    s->jain = suma * suma / (r->n * suma2);

    int ocupados = 0;
    for (int t = 0; t < r->ticks; t++)
        ocupados += r->gantt[t] >= 0;
    s->uso_cpu = 100.0 * ocupados / r->ticks;
    s->cambios = r->cambios;
    s->tiempo_total = r->ticks;
}

void gantt_texto(const Resultado *r, char *buf, int len) {
    int pos = snprintf(buf, len, "|");
    for (int ini = 0, i = 1; i <= r->ticks && pos < len; i++)
        if (i == r->ticks || r->gantt[i] != r->gantt[ini]) {
            const char *pid = r->gantt[ini] >= 0 ? r->procs[r->gantt[ini]].pid : "-";
            pos += snprintf(buf + pos, len - pos, " %s %d-%d |", pid, ini, i);
            ini = i;
        }
}
