CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

all: simulador test_planificador

simulador: main.c planificador.c planificador.h
	$(CC) $(CFLAGS) -o $@ main.c planificador.c

test_planificador: test.c planificador.c planificador.h
	$(CC) $(CFLAGS) -o $@ test.c planificador.c -lm

test: test_planificador
	./test_planificador

run: simulador
	./simulador

clean:
	rm -f simulador test_planificador

.PHONY: all test run clean
