include ../Make.defines

all:	${LIB_OBJS}
		/usr/bin/ar rv ${LIBUNP_NAME} $?
		/usr/bin/ranlib ${LIBUNP_NAME}

clean:
		rm -f ${PROGS} ${CLEANFILES}
		rm ${LIBUNP_NAME}
