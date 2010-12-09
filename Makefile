CFLAGS = -Wall -Wextra -pedantic -g

ush: job.o main.o proc.o util.o readline.o term.o parse.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f *.o ush

job.o: job.c util.h proc.h job.h
main.o: main.c util.h job.h proc.h readline.h term.h
parse.o: parse.c parse.h
proc.o: proc.c util.h proc.h
readline.o: readline.c readline.h parse.h util.h
term.o: term.c term.h
util.o: util.c
