CC = gcc
CFLAGS = -Wall -g -fsanitize=address -std=c89
EXEC = cg
DESTDIR = /usr/local/bin

.PHONY: all clean install uninstall

${EXEC}: cg.c
		${CC} ${CFLAGS} cg.c -o ${EXEC}

clean:
		rm -f ${EXEC}

install:
		cp -f ${EXEC} ${DESTDIR}

uninstall:
		rm -rf ${DESTDIR}/${EXEC}
