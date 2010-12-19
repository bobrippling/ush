CFLAGS = -Wall -Wextra -pedantic -g

ush: job.o main.o proc.o util.o readline.o term.o parse.o builtin.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f *.o ush

builtin.o: builtin.c proc.h job.h builtin.h
job.o: job.c util.h proc.h job.h config.h
main.o: main.c util.h job.h proc.h readline.h term.h config.h
parse.o: parse.c parse.h util.h config.h
proc.o: proc.c util.h proc.h builtin.h config.h
readline.o: readline.c readline.h parse.h util.h config.h
term.o: term.c term.h config.h
util.o: util.c config.h
