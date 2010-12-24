CFLAGS  = -Wall -Wextra -pedantic -g
SHELLS  = /etc/shells
BINPATH = ${PREFIX}/bin/ush

ush: job.o main.o proc.o util.o readline.o term.o parse.o \
		builtin.o task.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f *.o ush

install:
	install -m 555 ush ${BINPATH}
	grep -F ${BINPATH} ${SHELLS} || echo ${BINPATH} >> ${SHELLS}

uninstall:
	rm -f ${BINPATH}
	@grep -F ${BINPATH} ${SHELLS} > /dev/null && echo :: you need to remove ush from ${SHELLS}

.PHONY: clean install uninstall

builtin.o: builtin.c proc.h job.h task.h builtin.h term.h
job.o: job.c util.h proc.h job.h term.h config.h
main.o: main.c util.h proc.h job.h task.h readline.h term.h config.h
parse.o: parse.c parse.h util.h config.h
proc.o: proc.c util.h proc.h builtin.h config.h
readline.o: readline.c readline.h parse.h util.h task.h config.h
task.o: task.c util.h proc.h job.h task.h
term.o: term.c term.h config.h
util.o: util.c config.h
