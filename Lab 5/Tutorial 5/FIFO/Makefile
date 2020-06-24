CC=gcc
CFLAGS= -std=gnu99 -Wall

all: prog21_server.c prog21_serverB.c prog21_client.c 


prog21_server: prog21_server.c 
	${CC} ${CFLAG} -o prog21_server prog21_server.c

prog21_serverB: prog21_serverB.c 
	${CC} ${CFLAG} -o prog21_serverB prog21_serverB.c

prog21_client: prog21_client.c 
	${CC} ${CFLAG} -o prog21_client prog21_client.c




.PHONY: clean all

clean:
	-rm -f prog21_client prog21_server prog21_serverB