CC=gcc
CFLAGS= -std=gnu99 -Wall

all: mq_client.c mq_server.c 


mq_client: mq_client.c 
	${CC} ${CFLAG} -Werror -g -o mq_client mq_client.c -lrt

mq_server: mq_server.c 
	${CC} ${CFLAG} -Werror -g -o mq_server mq_server.c -lrt


.PHONY: clean all

clean:
	-rm -f mq_client mq_server