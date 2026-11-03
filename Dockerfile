# Imagen para la sustentación: compila, corre las pruebas y ejecuta el simulador.
FROM alpine:3.20

RUN apk add --no-cache build-base

WORKDIR /app
COPY Makefile *.c *.h ./
COPY escenarios/ escenarios/

# Si las pruebas fallan, la imagen no se construye.
RUN make && make test

# Sin argumentos corre todos los escenarios; se puede pasar uno: escenarios/3_inanicion.txt
ENTRYPOINT ["./simulador"]
