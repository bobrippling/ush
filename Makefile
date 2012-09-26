CFLAGS  = -Wall -Wextra -pedantic -g -std=c99
SHELLS  = /etc/shells
BINPATH = ${PREFIX}bin/ush

include config.mk

OBJ = job.o main.o proc.o util.o readline.o term.o parse.o \
			builtin.o task.o path.o complete.o esc.o glob.o limit.o

all: ush login

ush: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

login: login.o
	${CC} ${CFLAGS} -o $@ login.o

clean:
	rm -f *.o ush login

install: ush
	install -m 555 ush ${BINPATH}
	grep -F ${BINPATH} ${SHELLS} || echo ${BINPATH} >> ${SHELLS}

uninstall:
	rm -f ${BINPATH}
	@grep -F ${BINPATH} ${SHELLS} > /dev/null && echo :: you need to remove ush from ${SHELLS}

.PHONY: all clean install uninstall

builtin.o: builtin.c proc.h job.h parse.h task.h builtin.h term.h esc.h \
 path.h limit.h
complete.o: complete.c util.h path.h
esc.o: esc.c esc.h
glob.o: glob.c parse.h util.h glob.h
job.o: job.c util.h proc.h job.h term.h parse.h config.h
limit.o: limit.c limit.h
login.o: login.c
main.o: main.c util.h proc.h job.h parse.h task.h path.h readline.h \
 term.h config.h
parse.o: parse.c parse.h util.h config.h
path.o: path.c util.h path.h
proc.o: proc.c util.h proc.h builtin.h config.h
readline.o: readline.c complete.h readline.h parse.h util.h task.h esc.h \
 path.h glob.h config.h
task.o: task.c term.h util.h proc.h job.h parse.h task.h
term.o: term.c term.h config.h
util.o: util.c config.h
