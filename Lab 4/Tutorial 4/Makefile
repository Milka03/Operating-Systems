CC=gcc
CFLAGS= -Wall
#LDLIBS = -lpthread -lm

all: archunix4.c


archunix4: archunix4.c 
	${CC} ${CFLAG} -Werror -g -o archunix4 archunix4.c -lrt

# prog18: prog18.c 
# 	${CC} ${CFLAG} -o prog13b prog13b.c

# prog19: prog19.c 
# 	${CC} ${CFLAG} -o prog14 prog14.c

# prog20: prog20.c 
# 	${CC} ${CFLAG} -o prog15 prog15.c



.PHONY: clean all

clean:
	-rm -f archunix4