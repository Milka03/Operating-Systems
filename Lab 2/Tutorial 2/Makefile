CC=gcc
CFLAG=-Wall

all: prog13a.c prog13b.c prog14.c prog15.c prog16a.c prog16b.c 


prog13a: prog13a.c 
	${CC} ${CFLAG} -o prog13a prog13a.c

prog13b: prog13b.c 
	${CC} ${CFLAG} -o prog13b prog13b.c

prog14: prog14.c 
	${CC} ${CFLAG} -o prog14 prog14.c

prog15: prog15.c 
	${CC} ${CFLAG} -o prog15 prog15.c

prog16a: prog16a.c 
	${CC} ${CFLAG} -o prog16a prog16a.c 

prog16b: prog16b.c 
	${CC} ${CFLAG} -o prog16b prog16b.c 


.PHONY: clean all

clean:
	-rm -f prog13a prog13b prog14 prog15 prog16a prog16b