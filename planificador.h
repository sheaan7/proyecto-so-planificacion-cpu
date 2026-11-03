/* Simulador de planificación de CPU (tiempo discreto, 1 tick = 1 unidad).
 *
 * Cada proceso tiene ráfagas alternadas: {CPU, E/S, CPU, E/S, ..., CPU}.
 * Estados: NUEVO -> LISTO -> EJECUCION -> (BLOQUEADO -> LISTO ...) -> FINALIZADO.
 * Algoritmos: FCFS, SJF (no expropiativo) y Round Robin (quantum configurable).
 */
#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

/* Límites fijos: sin memoria dinámica, nada puede crecer sin control. */
#define MAX_PROC 64
#define MAX_RAF 32
#define MAX_PID 16
#define MAX_TICKS 10000 /* tope de seguridad contra bucles infinitos */
#define MAX_LOG 4000
#define LOG_LEN 64

typedef enum { NUEVO, LISTO, EJECUCION, BLOQUEADO, FINALIZADO } Estado;
typedef enum { FCFS, SJF, RR } Algoritmo;

/* PCB simplificado */
typedef struct {
    char pid[MAX_PID];
    int llegada;
    int rafagas[MAX_RAF];
    int n_rafagas;
    Estado estado;
    int indice;   /* ráfaga actual */
    int restante; /* tiempo que le queda a la ráfaga actual */
    int espera;   /* ticks en la cola de listos */
    int inicio;   /* primer tick en CPU (-1 = nunca) */
    int fin;      /* tick de finalización */
} Proceso;

typedef struct {
    Proceso procs[MAX_PROC];
    int n;
    int gantt[MAX_TICKS]; /* índice del proceso en CPU en cada tick, -1 = ociosa */
    int ticks;
    char log[MAX_LOG][LOG_LEN];
    int n_log;
    int cambios; /* cambios de contexto */
    char error[128];
} Resultado;

typedef struct {
    int cpu, espera, respuesta, retorno;
    double retorno_norm;
} MetricaProc;

typedef struct {
    double espera_prom, respuesta_prom, retorno_prom;
    int espera_max, cambios, tiempo_total;
    double uso_cpu, jain;
} Resumen;

/* Simula los procesos de `defs` (solo se leen pid, llegada, rafagas y n_rafagas).
 * Devuelve 0 si todo bien, -1 si la entrada es inválida (motivo en r->error). */
int simular(const Proceso *defs, int n, Algoritmo alg, int quantum, Resultado *r);

/* Métricas por proceso (m debe tener r->n posiciones) y resumen global. */
void metricas(const Resultado *r, MetricaProc *m, Resumen *s);

/* Gantt comprimido por tramos: "| P1 0-3 | P2 3-5 | ... |" */
void gantt_texto(const Resultado *r, char *buf, int len);

#endif
