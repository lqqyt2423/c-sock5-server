include ../Make.defines

PROGS =	sock5

all:	${PROGS}

sock5: main.o sock5.o helper.o
		${CC} ${CFLAGS} -o $@ main.o sock5.o helper.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
