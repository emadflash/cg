CC = gcc
CFLAGS = -Wall -g -fsanitize=address -std=c89
EXEC = "cg"

cg: cg.c
	${CC} ${CFLAGS} cg.c -o ${EXEC}

clean:
	-rm -f ${EXEC}
