# Instructivo de sustentación

La presentación se maneja con `./presentacion.sh`: muestra 8 pantallas en orden y espera **Enter** entre cada una. Dura unos 15 minutos más preguntas. En cada pantalla, la línea amarilla (`>>`) es la idea que tienes que decir.

## Antes de presentar
1. Con internet, construir la imagen (solo una vez): `docker build -t so-planificador .`
2. Ensayar una pasada completa: `./presentacion.sh`
3. Poner la terminal en pantalla completa y subir la letra (`Ctrl` + `+`). Con menos de ~150 columnas los diagramas de Gantt del escenario 3 se parten en varias líneas.
4. Tener abierto `planificador.c` en un editor, por si piden ver más código.
5. Dejar una segunda terminal lista para el escenario en vivo (ver abajo).

## Pantalla por pantalla
| # | Pantalla | Qué decir | Min |
|---|---|---|---|
| 1 | Diagrama de estados | Hay más procesos que CPUs y el planificador decide quién corre. Recorrer el diagrama con el dedo: LISTO → EJECUCIÓN → BLOQUEADO → LISTO. Presentar los tres algoritmos en una frase cada uno. | 2 |
| 2 | Efecto convoy | Hacer la cuenta en el tablero **antes** de dar Enter: FCFS espera (0+24+27)/3 = 17; SJF espera (0+3+6)/3 = 3. Luego mostrar que la tabla dice lo mismo. Los cortos quedan atrapados detrás del largo: eso es el convoy. | 3 |
| 3 | Pruebas | Esa misma cuenta está escrita como prueba automática. Si el simulador calculara mal, la imagen de Docker ni siquiera se construiría. | 1 |
| 4 | Código | El `struct Proceso` es nuestro PCB: estado, ráfaga actual, tiempo restante, espera. `elegir()` es toda la diferencia entre algoritmos: FCFS y RR toman el primero de la cola, SJF el de menor ráfaga. RR además expropia al agotar el quantum. | 3 |
| 5 | Escenario 2 (CPU y E/S) | Señalar en el log `t=12 CPU1 solicita E/S -> BLOQUEADO` y `t=14 CPU1 termina E/S -> LISTO`. En la tabla: RR responde en 2.8 contra 14.4 de FCFS, pero hace 26 cambios de contexto contra 12. FCFS deja la CPU ociosa (94.4 %). | 2 |
| 6 | Escenario 3 (inanición) | En el Gantt de SJF, LARGO queda de último. Su espera: 2 con FCFS, 38 con SJF. Hallazgo: SJF tiene el mejor promedio (3.57) y equidad 0.965, porque solo sufre **un** proceso. Por eso medimos la espera máxima. Solución real: aging. | 3 |
| 7 | Escenario 4 (quantum) | q=1 da la mejor respuesta (1.2) con 26 cambios de contexto. q=20 da exactamente la fila de FCFS, porque ninguna ráfaga supera 20 y nunca hay expropiación. | 2 |
| 8 | Conclusiones | No hay un algoritmo mejor en todo. Decir las limitaciones antes de que las pregunten: el cambio de contexto cuesta 0 y las ráfagas se conocen de antemano. | 1 |

Si son varios integrantes: uno toma las pantallas 1–3, otro la 4–5, otro la 6–7 y el cierre es de todos.

## La idea en simple
Un banco con **un solo cajero** (la CPU) y varios clientes (los procesos). Alguien decide a quién se atiende: ese es el planificador. Probamos tres reglas:

- **FCFS:** se atiende en orden de llegada. Si el primero trae 200 consignaciones, todos los demás esperan detrás de él.
- **SJF:** pasa primero el que tenga el trámite más corto. La fila se mueve rápido, pero el del trámite largo puede quedarse esperando siempre.
- **Round Robin:** a cada uno se le dan unos minutos (el quantum) y, si no acaba, vuelve al final de la fila. Nadie queda olvidado, pero el cajero pierde tiempo cambiando de cliente.

El proyecto es un programa que simula esa fila y mide qué tan bien le va a cada regla.

## Libreto (para decir en voz alta)
**Pantalla 1: el problema**
> "Profe, en un sistema operativo hay más procesos que CPUs, así que el planificador decide cuál corre. Un proceso pasa por estos estados: llega y queda LISTO, lo despachan y pasa a EJECUCIÓN, si pide entrada/salida queda BLOQUEADO y cuando termina vuelve a LISTO. Nosotros simulamos eso en C y comparamos tres algoritmos: FCFS, SJF y Round Robin."

**Pantalla 2: efecto convoy**
> "Para saber si el simulador calcula bien, usamos el ejemplo del libro de Silberschatz: P1 dura 24, P2 y P3 duran 3. Con FCFS, P1 no espera, P2 espera 24 y P3 espera 27: el promedio es 17. Con SJF pasan primero los cortos y el promedio baja a 3. El programa da los mismos números que la cuenta a mano. A eso se le llama efecto convoy: los cortos quedan atrapados detrás del largo."

**Pantalla 3: pruebas**
> "Esa cuenta está escrita como prueba automática. Si el simulador diera otro resultado, la imagen de Docker no se construye."

**Pantalla 4: el código**
> "Cada proceso es este struct, que es nuestro PCB: guarda el estado, cuánto le falta a la ráfaga, cuánto ha esperado y cuándo empezó y terminó. La diferencia entre los algoritmos está en esta función, `elegir`: FCFS y Round Robin toman el primero de la cola, y SJF busca el de menor ráfaga. Round Robin además saca al proceso cuando se le acaba el quantum."

**Pantalla 5: CPU contra entrada/salida**
> "Aquí mezclamos procesos que usan mucha CPU con procesos que piden mucha entrada/salida. En el log se ve: en t=12 CPU1 pide E/S y queda BLOQUEADO, y en t=14 termina y vuelve a LISTO. En la tabla, Round Robin con quantum 2 responde en 2.8 unidades y FCFS en 14.4. El costo es que hace 26 cambios de contexto contra 12."

**Pantalla 6: inanición**
> "Este es el hallazgo más importante. Hay un proceso largo y van llegando cortos todo el tiempo. Con FCFS el largo espera 2; con SJF espera 38, porque siempre hay uno más corto que pasa primero. Eso es inanición. Lo interesante es que SJF tiene el mejor promedio de espera, 3.57. O sea, el promedio se ve muy bien y esconde al proceso que está sufriendo. Por eso medimos también la espera máxima. En un sistema real esto se corrige con aging: al que lleva mucho esperando se le sube la prioridad."

**Pantalla 7: el quantum**
> "Aquí corremos Round Robin con distintos quantums. Con quantum 1 la respuesta es la mejor, 1.2, pero hay 26 cambios de contexto. Con quantum 20, que es más grande que cualquier ráfaga, nunca se interrumpe a nadie y los resultados son idénticos a FCFS. Entonces el quantum es un equilibrio: muy pequeño desperdicia CPU, muy grande pierde la gracia."

**Pantalla 8: cierre**
> "La conclusión es que no hay un algoritmo mejor en todo. SJF da la menor espera pero puede causar inanición; FCFS es simple pero sufre el convoy; Round Robin es el más justo y el que mejor responde, pero cambia más de contexto. Nuestro simulador tiene límites: el cambio de contexto no cuesta tiempo y las ráfagas se conocen de antemano, que en la vida real no pasa. Por eso los sistemas reales como Linux combinan las tres ideas."

## Palabras que debes poder definir
- **Ráfaga:** tramo seguido en que el proceso usa la CPU (o la E/S).
- **Tiempo de espera:** tiempo que pasa en la cola de listos sin ejecutarse.
- **Tiempo de respuesta:** lo que tarda en usar la CPU por primera vez desde que llegó.
- **Cambio de contexto:** guardar el estado de un proceso y cargar el de otro; es trabajo que no le sirve al usuario.
- **Quantum:** el turno máximo que Round Robin le da a cada proceso.
- **Inanición:** un proceso que nunca, o casi nunca, recibe la CPU.
- **Expropiativo:** que el sistema le puede quitar la CPU a un proceso antes de que termine. Round Robin lo es; nuestro FCFS y SJF no.

## Tres números para no olvidar
No hace falta memorizar el libreto. Con **17 contra 3** (convoy), **38 contra 2** (inanición) y **quantum 20 = FCFS**, más la analogía del banco, se puede explicar cualquier pantalla con palabras propias.

## Escenario en vivo
Si el profesor pide un caso ("¿y si llega un proceso de 50?"), en la segunda terminal:
```bash
cat > escenarios/demo.txt <<'EOF'
nombre demo
descripcion Caso pedido en la sustentación
quantums 2 4
proceso P1 0 50
proceso P2 1 4
proceso P3 2 4
EOF
docker run --rm -v "$PWD/escenarios:/app/escenarios" so-planificador escenarios/demo.txt
```
Cada línea es `proceso <pid> <llegada> <ráfagas>`; las ráfagas alternan CPU y E/S y siempre empiezan y terminan en CPU (`5 3 2` = 5 de CPU, 3 de E/S, 2 de CPU).

Ojo con este ejemplo: SJF sale **igual** a FCFS (espera 33.67). No es un error: nuestro SJF no es expropiativo y P1 llega solo en t=0, así que arranca y nadie lo saca. Es el argumento para mencionar SRTF (SJF expropiativo) como mejora. RR baja la espera a 5.67.

## Si algo falla
- **Docker no arranca:** compilar y correr directo: `make && ./simulador escenarios/3_inanicion.txt`.
- **No hay compilador ni Docker:** las salidas ya están guardadas en `resultados/*.txt` y `resultados/resumen.csv`; se muestran con `less`.
- **Te pierdes en el script:** `Ctrl+C` y volver a lanzarlo; cada pantalla es independiente.

## Preguntas
Las respuestas cortas están al final de `INFORME.md` ("Preguntas probables"). Las tres más seguras: por qué SJF es óptimo, cómo se corrige la inanición y dónde se cuenta un cambio de contexto (paso 4 de `simular()` en `planificador.c`).
