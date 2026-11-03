# Informe técnico: Simulador de Planificación de CPU en C

**Curso:** Sistemas Operativos · **Opción 1:** Gestión de procesos y planificación de CPU
**Integrantes:** _(completar)_

---

## 1. Descripción del problema
En un sistema multiprogramado hay más procesos listos que CPUs. El **planificador de corto plazo** decide qué proceso de la cola de listos usa la CPU y por cuánto tiempo. Esa decisión afecta cuánto esperan los procesos, qué tan rápido responde el sistema y si algún proceso queda relegado indefinidamente (inanición).

## 2. Objetivo
Implementar un simulador que modele el ciclo de vida de los procesos y comparar, con métricas cuantitativas, el comportamiento de FCFS, SJF y Round Robin en distintos escenarios de carga.

## 3. Fundamentación del mecanismo
- **Estados de un proceso:** NUEVO → LISTO → EJECUCIÓN → FINALIZADO. Si durante la ejecución pide E/S, pasa a BLOQUEADO y vuelve a LISTO cuando la E/S termina. En RR, si agota su quantum, vuelve de EJECUCIÓN a LISTO (expropiación).
- **Ráfagas:** un proceso alterna ráfagas de CPU y de E/S. Los procesos *limitados por CPU* tienen ráfagas largas y los *limitados por E/S*, ráfagas cortas con bloqueos frecuentes.
- **Cambio de contexto:** guardar el estado (PCB) del proceso saliente y cargar el del entrante. Es trabajo que no aporta al usuario, así que un exceso de cambios desperdicia CPU.
- **Algoritmos:**
  - **FCFS:** atiende en orden de llegada. Es simple y no expropiativo. Sufre el *efecto convoy*: los cortos esperan detrás de un largo.
  - **SJF:** elige la ráfaga de CPU más corta. Es óptimo para la espera promedio, pero exige conocer las ráfagas y puede causar **inanición** de los procesos largos.
  - **Round Robin:** FCFS con expropiación cada *q* unidades. Es equitativo y da buen tiempo de respuesta. Con *q* muy pequeño hay demasiados cambios de contexto; con *q* muy grande degenera en FCFS.

## 4. Diseño de la solución
Es una simulación de **tiempo discreto** en la que cada iteración equivale a 1 unidad de tiempo (tick). Está escrito en **C (C11)**, sin librerías externas, en tres módulos: `planificador.h/.c` (PCB, motor y métricas), `main.c` (lectura de escenarios y reportes) y `test.c` (pruebas). Cada proceso se representa con un `struct Proceso` que funciona como un PCB simplificado (pid, estado, ráfaga actual, tiempo restante, espera, inicio y fin). La cola de listos y la lista de bloqueados son arreglos de índices que conservan el orden FIFO.

Orden de eventos en cada tick (`planificador.c`, función `simular`):
1. Llegan los procesos nuevos (pasan a LISTO).
2. Los procesos cuya E/S terminó pasan de BLOQUEADO a LISTO.
3. En RR, si el proceso en ejecución agotó el quantum, vuelve al final de la cola.
4. Si la CPU está libre, el algoritmo elige un proceso de la cola. Si es distinto del anterior, se cuenta un cambio de contexto.
5. Avanza un tick: el proceso en ejecución consume CPU, los listos acumulan espera y los bloqueados avanzan su E/S.
6. Si terminó la ráfaga de CPU, el proceso pasa a BLOQUEADO (si le queda E/S) o a FINALIZADO.

La política de selección está en la función `elegir()`: FCFS y RR toman el primero de la cola y SJF el de menor ráfaga restante (en empate, el que llegó antes). RR reutiliza la selección de FCFS y añade la expropiación del paso 3.

**Decisiones propias de C:** no se usa memoria dinámica (`malloc`): todo cabe en arreglos de tamaño fijo (`MAX_PROC`, `MAX_RAF`, `MAX_TICKS`, `MAX_LOG`) y cada límite se valida antes de escribir. Así no hay fugas de memoria ni desbordamientos de buffer. Los escenarios usan un formato de texto plano (`escenarios/*.txt`) que se lee con `fgets`/`strtok`/`strtol`, para no depender de una librería de JSON.

**Supuestos:** una CPU, dispositivos de E/S sin contención (cada proceso hace su E/S en paralelo), ráfagas conocidas de antemano y cambio de contexto con costo 0 (se **cuenta**, pero no consume tiempo).

## 5. Métricas
| Métrica | Definición |
|---|---|
| Tiempo de espera | Ticks totales en la cola de LISTOS |
| Tiempo de respuesta | Primer despacho − llegada |
| Tiempo de retorno | Finalización − llegada |
| Retorno normalizado | Retorno / CPU total (1 = nunca esperó) |
| Espera máxima | Peor espera individual (detector de inanición) |
| Cambios de contexto | Veces que la CPU pasa de un proceso a otro distinto |
| Uso de CPU | % de ticks con la CPU ocupada |
| Índice de equidad de Jain | Sobre 1/retorno normalizado: 1 = todos tratados igual |

## 6. Escenarios de prueba
| # | Escenario | Qué busca demostrar |
|---|---|---|
| 1 | Efecto convoy | Ejemplo del libro (P1=24, P2=3, P3=3). Sirve para validar contra cálculo a mano |
| 2 | Mixto CPU/E/S | Bloqueos y reactivación, procesos limitados por CPU frente a limitados por E/S |
| 3 | Inanición | Un proceso largo frente a un flujo continuo de cortos |
| 4 | Variación de quantum | RR con q = 1, 3, 6 y 20 |

## 7. Resultados
_(Tomados de `resultados/`; se regeneran con `make && ./simulador`.)_

**Escenario 1: Efecto convoy**
| Algoritmo | Espera prom. | Respuesta prom. | Retorno prom. | Cambios ctx | Jain |
|---|---|---|---|---|---|
| FCFS | 17.0 | 17.0 | 27.0 | 2 | 0.478 |
| SJF | 3.0 | 3.0 | 13.0 | 2 | 0.933 |
| RR (q=4) | 5.67 | 3.67 | 15.67 | 3 | 0.852 |

**Escenario 2: Mixto CPU/E/S**
| Algoritmo | Espera prom. | Respuesta prom. | Retorno prom. | Cambios ctx | Uso CPU | Tiempo total |
|---|---|---|---|---|---|---|
| FCFS | 27.2 | 14.4 | 44.2 | 12 | 94.4 % | 54 |
| SJF | 18.6 | 12.4 | 35.6 | 12 | 98.1 % | 52 |
| RR (q=2) | 20.6 | 2.8 | 37.6 | 26 | 100 % | 51 |
| RR (q=5) | 24.4 | 5.8 | 41.4 | 17 | 100 % | 51 |

**Escenario 3: Inanición (fila del proceso LARGO)**
| Algoritmo | Espera de LARGO | Respuesta de LARGO | Espera prom. (todos) | Jain |
|---|---|---|---|---|
| FCFS | 2 | 2 | 13.86 | 0.49 |
| SJF | **38** | **38** | 3.57 | 0.965 |
| RR (q=4) | 35 | 2 | 11.57 | 0.674 |

**Escenario 4: Variación de quantum**
| Algoritmo | Espera prom. | Respuesta prom. | Cambios ctx | Jain |
|---|---|---|---|---|
| FCFS | 11.6 | 11.6 | 4 | 0.613 |
| SJF | 9.8 | 9.8 | 4 | 0.658 |
| RR (q=1) | 13.0 | 1.2 | 26 | 0.984 |
| RR (q=3) | 14.4 | 4.4 | 11 | 0.970 |
| RR (q=6) | 13.6 | 8.0 | 6 | 0.887 |
| RR (q=20) | 11.6 | 11.6 | 4 | 0.613 |

## 8. Análisis e interpretación
1. **SJF minimiza la espera promedio** en todos los escenarios (por ejemplo, 3.0 frente a 17.0 de FCFS en el escenario 1), como predice la teoría. Lo logra atendiendo primero a los cortos.
2. **El efecto convoy de FCFS** (escenario 1) aparece porque P2 y P3 esperan 24 y 27 unidades detrás de P1. En el escenario 2 los procesos de E/S quedan atrás de los de CPU, y la CPU incluso queda ociosa (94.4 % de uso) porque todos terminan bloqueados a la vez.
3. **Inanición en SJF** (escenario 3): LARGO espera 38 unidades con SJF y solo 2 con FCFS. Hallazgo importante: el índice de Jain de SJF es alto (0.965) porque solo **un** proceso sufre. Un promedio o un índice global puede esconder la inanición, y por eso medimos también la **espera máxima**. La solución real es el *envejecimiento (aging)*.
4. **RR ofrece el mejor tiempo de respuesta** (2.8 en el escenario 2 frente a 14.4 de FCFS) y la mejor equidad, a costa de más cambios de contexto (26 frente a 12) y de una espera promedio algo mayor. Es el algoritmo indicado para sistemas interactivos.
5. **El quantum es un compromiso** (escenario 4). Con q=1 se obtiene la mejor respuesta (1.2) pero 26 cambios de contexto. Con q=20, mayor que cualquier ráfaga, RR produce **exactamente** los mismos números que FCFS. La regla práctica es que alrededor del 80 % de las ráfagas quepan en el quantum.
6. **Los procesos limitados por E/S se benefician de RR y SJF**: liberan pronto la CPU y mantienen ocupados los dispositivos en paralelo (100 % de uso de CPU con RR en el escenario 2).

## 9. Riesgos y responsabilidades profesionales
- **Rendimiento:** un quantum mal elegido desperdicia CPU en cambios de contexto (en un sistema real cada cambio cuesta microsegundos y además invalida la caché y la TLB).
- **Estabilidad y disponibilidad:** SJF o prioridades sin *aging* pueden dejar procesos críticos sin atender indefinidamente (inanición). En un servidor, eso es una denegación de servicio para ciertos usuarios.
- **Seguridad:** si el planificador confía en la duración que *declara* el proceso (SJF), un proceso malicioso puede mentir para acaparar la CPU. Las estimaciones deben basarse en el comportamiento observado.
- **Seguridad del propio código (C):** en C un índice fuera de rango o un `strcpy` sin control es un desbordamiento de buffer explotable. Por eso cada arreglo tiene un tope y se valida antes de escribir, los números se leen con `strtol` comprobando errores, el nombre del escenario solo admite letras, dígitos, `_` y `-` (se usa como nombre de archivo, así se evita escribir fuera de `resultados/`) y el programa se verificó con `-fsanitize=address,undefined` sin errores.
- **Uso responsable de recursos:** sin memoria dinámica, uso de memoria acotado y conocido; el tope `MAX_TICKS` corta cualquier simulación que no termine; se validan las entradas (ráfagas positivas y en número impar) y no se ejecutan cargas reales sobre la máquina.
- **Integridad de los datos:** si un escenario es inválido se informa el archivo y la línea, se omite y se borra su reporte a medias; el programa termina con código de salida 1 para que el error no pase desapercibido.
- **Integridad de los resultados:** las pruebas automáticas validan el simulador contra un ejemplo resuelto a mano antes de confiar en sus números.

## 10. Limitaciones y mejoras
- **Limitaciones:** el cambio de contexto no consume tiempo, se asumen dispositivos de E/S sin cola (sin contención), una sola CPU y ráfagas conocidas de antemano (irreal para SJF).
- **Mejoras:** costo configurable de cambio de contexto; SRTF (SJF expropiativo) y prioridades con *aging*; estimación de ráfagas por promedio exponencial; colas multinivel con retroalimentación (MLFQ); cola y planificación del dispositivo de E/S; varios núcleos; gráficos automáticos.

## 11. Conclusiones
No existe un algoritmo mejor en todo. SJF optimiza la espera pero sacrifica a los procesos largos, FCFS es justo en el orden pero ineficiente con cargas mixtas, y RR prioriza la interactividad y la equidad a cambio de cambios de contexto. Por eso los sistemas operativos reales (Linux CFS, Windows) combinan ideas de los tres: rebanadas de tiempo, prioridades dinámicas y favorecer a los procesos limitados por E/S.

## 12. Referencias
- Silberschatz, A., Galvin, P. B., Gagne, G. *Operating System Concepts*, 10.ª ed., Wiley, 2018. Cap. 3 (Procesos) y cap. 5 (Planificación de CPU).
- Tanenbaum, A. S., Bos, H. *Modern Operating Systems*, 4.ª ed., Pearson, 2014. Cap. 2.4 (Planificación).
- Arpaci-Dusseau, R., Arpaci-Dusseau, A. *Operating Systems: Three Easy Pieces*, caps. 7–8. https://pages.cs.wisc.edu/~remzi/OSTEP/
- Jain, R., Chiu, D., Hawe, W. *A Quantitative Measure of Fairness and Discrimination for Resource Allocation in Shared Computer Systems*, DEC-TR-301, 1984.

---

# Anexo: guion de sustentación (20 min, 4 integrantes)
| Min | Quién | Contenido |
|---|---|---|
| 0–4 | Integrante 1 | Problema, estados de un proceso (dibujar el diagrama de estados), qué es el planificador y el cambio de contexto |
| 4–8 | Integrante 2 | FCFS, SJF y RR en la pizarra con el escenario 1 a mano (P1=24, P2=3, P3=3). Mostrar que el programa da lo mismo: `docker run --rm --entrypoint ./test_planificador so-planificador` |
| 8–13 | Integrante 3 | Diseño: el `struct Proceso` (PCB) y los 6 pasos del tick en `simular()`. Demo en vivo con `docker run --rm so-planificador escenarios/2_mixto_cpu_es.txt`, leyendo el log de estados (BLOQUEADO → LISTO) |
| 13–18 | Integrante 4 | Resultados: inanición (esc. 3), compromiso del quantum (esc. 4), riesgos y limitaciones |
| 18–20 | Todos | Conclusiones y preguntas |

**Antes de la sustentación:** ejecutar `docker build -t so-planificador .` con internet (descarga Alpine y gcc). Después la demo funciona sin conexión.

**Preguntas probables y respuestas cortas**
- *¿Por qué SJF es óptimo?* Mover un proceso corto delante de uno largo reduce la espera del corto más de lo que aumenta la del largo.
- *¿Cómo sabe SJF la duración en la vida real?* No la sabe, la estima con un promedio exponencial de las ráfagas anteriores.
- *¿Cómo se corrige la inanición?* Con aging: aumentar la prioridad de un proceso según el tiempo que lleva esperando.
- *¿Por qué RR con q=20 es igual a FCFS?* Porque ninguna ráfaga supera 20, así que nunca hay expropiación.
- *¿Por qué con q=1 hay más cambios de contexto y aun así la espera promedio no mejora?* Porque todos avanzan en paralelo y terminan tarde. Mejora la *respuesta*, no la *espera*.
- *¿Dónde se cuenta un cambio de contexto en el código?* En el paso 4 de `simular()` (`planificador.c`), cuando el proceso despachado es distinto del último que usó la CPU.
- *¿Por qué no usaron `malloc`?* Con arreglos de tamaño fijo el uso de memoria está acotado, no hay fugas y cada límite se valida explícitamente. El costo es que hay máximos (64 procesos, 10 000 ticks).
- *¿Cómo evitan desbordamientos de buffer?* Se comprueba cada tope antes de escribir, se usa `snprintf` en vez de `sprintf`, y se probó con AddressSanitizer.
