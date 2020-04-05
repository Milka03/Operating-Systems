
CC=gcc
CFLAGS=-Wall

all : prog1.c prog2.c prog3.c prog4.c prog5.c prog6.c prog7.c prog8.c prog9.c prog10.c prog11.c prog12.c

prog1: prog1.c
	gcc -Wall -o prog1 prog1.c


prog1b: prog1b.o 
	gcc -Wall -o prog1b prog1b.o 

prog1b.o: prog1b.c
	gcc -Wall -o prog1b.o -c prog1b.c

prog2: prog2.o 
	gcc -Wall -o prog2 prog2.o 

prog2.o: prog2.c
	gcc -Wall -o prog2.o -c prog2.c


prog3: prog3.c
	gcc -Wall -o prog3 prog3.c

prog4: prog4.c
	${CC} ${CFLAGS} -o prog4 prog4.c

prog5: prog5.c
	${CC} ${CFLAGS} -o prog5 prog5.c


prog6: prog6.c
	${CC} ${CFLAGS} -o prog6 prog6.c

prog7: prog7.c
	${CC} ${CFLAGS} -o prog7 prog7.c

prog8: prog8.c
	${CC} ${CFLAGS} -o prog8 prog8.c

prog9: prog9.c
	${CC} ${CFLAGS} -o prog9 prog9.c

prog10: prog10.c
	${CC} ${CFLAGS} -o prog10 prog10.c

prog11: prog11.c
	${CC} ${CFLAGS} -o prog11 prog11.c

prog12: prog12.c
	${CC} ${CFLAGS} -o prog12 prog12.c

.PHONY: clean all

clean:
	-rm -f prog1b.o prog1b prog1 prog2.o prog2 prog3 prog4 prog5 prog6 prog7 prog8 prog9 prog10 prog11 prog12