include ../Make.defines

PROGS =	socks5

all:	${PROGS}

socks5: main.o socks5.o helper.o
		${CC} ${CFLAGS} -o $@ main.o socks5.o helper.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
