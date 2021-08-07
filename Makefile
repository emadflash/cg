CC = gcc
CFLAGS = -Wall -g -fsanitize=address -std=c89
EXEC = "cg"

cg: main.c
	${CC} ${CFLAGS} main.c -o ${EXEC}

clean:
	-rm -f ${EXEC}
