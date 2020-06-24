CC=gcc
CFLAGS= -std=gnu99 -Wall

all: prog23_server.c prog23_local.c  


prog23_server: prog23_server.c 
	${CC} ${CFLAG} -Werror -g -o prog23_server prog23_server.c

prog23_local: prog23_local.c 
	${CC} ${CFLAG} -Werror -g -o prog23_local prog23_local.c




.PHONY: clean all

clean:
	-rm -f prog23_local prog23_server