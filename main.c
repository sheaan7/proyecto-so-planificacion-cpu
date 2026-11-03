/* Ejecuta cada escenario de escenarios/<nombre>.txt con FCFS, SJF y RR (uno por quantum).
 *
 * Uso:  ./simulador                          # todos los escenarios
 *       ./simulador escenarios/x.txt         # uno solo
 * Salida: consola + resultados/<escenario>.txt (Gantt, tablas, log) + resultados/resumen.csv
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "planificador.h"

#define MAX_Q 8

typedef struct {
    char nombre[64];
    char descripcion[256];
    int quantums[MAX_Q], nq;
    Proceso procs[MAX_PROC];
    int n;
} Escenario;

static FILE *archivo; /* copia de la salida en resultados/<escenario>.txt */

/* printf a la consola y al archivo del escenario a la vez */
static void out(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);
    if (archivo) {
        va_start(a, fmt);
        vfprintf(archivo, fmt, a);
        va_end(a);
    }
}

static int entero(const char *tok, int *v) {
    char *fin;
    errno = 0;
    long x = strtol(tok, &fin, 10);
    if (*tok == '\0' || *fin != '\0' || errno || x < INT_MIN || x > INT_MAX)
        return -1;
    *v = (int)x;
    return 0;
}

/* Formato (una directiva por línea, '#' = comentario):
 *   nombre 1_efecto_convoy
 *   descripcion texto libre
 *   quantums 2 5
 *   proceso P1 0 5 3 2      <- pid, llegada, ráfagas CPU E/S CPU ... */
static int cargar(const char *ruta, Escenario *e) {
    FILE *f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "[ERROR] %s: %s\n", ruta, strerror(errno));
        return -1;
    }
    memset(e, 0, sizeof *e);
    char linea[512];
    const char *err = NULL;
    int num = 0;
    while (!err && fgets(linea, sizeof linea, f)) {
        num++;
        linea[strcspn(linea, "\r\n")] = '\0';
        char *clave = strtok(linea, " \t");
        if (!clave || clave[0] == '#')
            continue;
        if (!strcmp(clave, "nombre")) {
            char *v = strtok(NULL, " \t");
            /* se usa como nombre de archivo: nada de '/' ni '..' */
            if (!v || strlen(v) >= sizeof e->nombre ||
                strspn(v, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != strlen(v))
                err = "nombre inválido (solo letras, dígitos, '_' y '-')";
            else
                strcpy(e->nombre, v);
        } else if (!strcmp(clave, "descripcion")) {
            char *v = strtok(NULL, "");
            snprintf(e->descripcion, sizeof e->descripcion, "%s", v ? v + strspn(v, " \t") : "");
        } else if (!strcmp(clave, "quantums")) {
            for (char *v; !err && (v = strtok(NULL, " \t"));)
                if (e->nq == MAX_Q || entero(v, &e->quantums[e->nq]) || e->quantums[e->nq++] <= 0)
                    err = "quantums: máximo 8 enteros positivos";
        } else if (!strcmp(clave, "proceso")) {
            char *pid = strtok(NULL, " \t"), *v = strtok(NULL, " \t");
            Proceso *p = &e->procs[e->n];
            if (e->n == MAX_PROC)
                err = "demasiados procesos";
            else if (!pid || strlen(pid) >= MAX_PID || !v || entero(v, &p->llegada))
                err = "uso: proceso <pid> <llegada> <ráfagas...>";
            else {
                strcpy(p->pid, pid);
                while (!err && (v = strtok(NULL, " \t")))
                    if (p->n_rafagas == MAX_RAF || entero(v, &p->rafagas[p->n_rafagas++]))
                        err = "ráfaga inválida o demasiadas ráfagas";
                e->n++;
            }
        } else {
            err = "directiva desconocida";
        }
    }
    fclose(f);
    if (!err && (!e->nombre[0] || e->n == 0)) {
        err = "falta 'nombre' o algún 'proceso'";
        num = 0;
    }
    if (err) {
        fprintf(stderr, "[ERROR] %s:%d: %s\n", ruta, num, err);
        return -1;
    }
    if (e->nq == 0)
        e->quantums[e->nq++] = 4;
    return 0;
}

static void tabla_comparacion(char nombres[][16], const Resumen *s, int n) {
    out("%-10s %11s %14s %12s %10s %16s %9s %12s %12s\n", "algoritmo", "espera_prom", "respuesta_prom",
        "retorno_prom", "espera_max", "cambios_contexto", "uso_cpu_%", "tiempo_total", "equidad_jain");
    for (int i = 0; i < n; i++)
        out("%-10s %11.2f %14.2f %12.2f %10d %16d %9.1f %12d %12.3f\n", nombres[i], s[i].espera_prom,
            s[i].respuesta_prom, s[i].retorno_prom, s[i].espera_max, s[i].cambios, s[i].uso_cpu,
            s[i].tiempo_total, s[i].jain);
}

static int correr(const Escenario *e, FILE *csv) {
    static Resultado r; /* static: ~300 KB, demasiado para la pila */
    MetricaProc m[MAX_PROC];
    Resumen res[2 + MAX_Q];
    char nombres[2 + MAX_Q][16], gantt[8192], ruta[128];
    int nconf = 2 + e->nq;

    snprintf(ruta, sizeof ruta, "resultados/%s.txt", e->nombre);
    archivo = fopen(ruta, "w");
    if (!archivo)
        fprintf(stderr, "[AVISO] no se pudo escribir %s: %s\n", ruta, strerror(errno));

    out("ESCENARIO: %s\n%s\n\n", e->nombre, e->descripcion);
    for (int c = 0; c < nconf; c++) {
        Algoritmo alg = c == 0 ? FCFS : c == 1 ? SJF : RR;
        int q = c < 2 ? 0 : e->quantums[c - 2];
        if (alg == RR)
            snprintf(nombres[c], sizeof nombres[c], "RR(q=%d)", q);
        else
            strcpy(nombres[c], alg == FCFS ? "FCFS" : "SJF");

        if (simular(e->procs, e->n, alg, q, &r) != 0) {
            fprintf(stderr, "[ERROR] %s: %s\n", e->nombre, r.error);
            if (archivo) {
                fclose(archivo);
                remove(ruta); /* no dejar evidencias a medias */
            }
            archivo = NULL;
            return -1;
        }
        metricas(&r, m, &res[c]);
        gantt_texto(&r, gantt, sizeof gantt);

        out("=== %s ===\nGantt: %s\n\n", nombres[c], gantt);
        out("%-8s %7s %4s %6s %9s %7s %12s\n", "pid", "llegada", "cpu", "espera", "respuesta", "retorno", "retorno_norm");
        for (int i = 0; i < r.n; i++)
            out("%-8s %7d %4d %6d %9d %7d %12.2f\n", r.procs[i].pid, r.procs[i].llegada, m[i].cpu,
                m[i].espera, m[i].respuesta, m[i].retorno, m[i].retorno_norm);
        out("\nLog de estados:\n");
        for (int i = 0; i < r.n_log; i++)
            out("%s\n", r.log[i]);
        out("\n");
    }
    out("=== COMPARACIÓN ===\n");
    tabla_comparacion(nombres, res, nconf);
    out("\n");

    for (int c = 0; c < nconf; c++)
        fprintf(csv, "%s,%s,%.2f,%.2f,%.2f,%d,%d,%.1f,%d,%.3f\n", e->nombre, nombres[c], res[c].espera_prom,
                res[c].respuesta_prom, res[c].retorno_prom, res[c].espera_max, res[c].cambios, res[c].uso_cpu,
                res[c].tiempo_total, res[c].jain);
    if (archivo)
        fclose(archivo);
    archivo = NULL;
    return 0;
}

int main(int argc, char **argv) {
    glob_t g = {0};
    char **rutas = argv + 1;
    size_t n = (size_t)argc - 1;
    if (n == 0) {
        if (glob("escenarios/*.txt", 0, NULL, &g) != 0) {
            fprintf(stderr, "No hay escenarios en escenarios/*.txt\n");
            return 1;
        }
        rutas = g.gl_pathv;
        n = g.gl_pathc;
    }
    if (mkdir("resultados", 0755) != 0 && errno != EEXIST) {
        perror("resultados");
        return 1;
    }
    FILE *csv = fopen("resultados/resumen.csv", "w");
    if (!csv) {
        perror("resultados/resumen.csv");
        return 1;
    }
    fprintf(csv, "escenario,algoritmo,espera_prom,respuesta_prom,retorno_prom,espera_max,"
                 "cambios_contexto,uso_cpu_%%,tiempo_total,equidad_jain\n");

    static Escenario e;
    int fallos = 0;
    for (size_t i = 0; i < n; i++)
        if (cargar(rutas[i], &e) != 0 || correr(&e, csv) != 0)
            fallos++;

    fclose(csv);
    globfree(&g);
    printf("Resultados guardados en resultados/\n");
    return fallos ? 1 : 0;
}
