CFLAGS  = -march=native -O2 -fomit-frame-pointer -pipe -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs -Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra -pedantic -ansi

ush: init.o term.o proc.o read.o util.o
	@echo LD $@
	@${CC} ${LDFLAGS} -o $@ $^

%.o:%.c
	@echo CC $@
	@${CC} ${CFLAGS} -c -o $@ $<

mostlyclean:
	@rm -f *.o

clean: mostlyclean
	@rm -f ush

init.o: init.c term.h read.h
proc.o: proc.c proc.h util.h
read.o: read.c read.h util.h proc.h
term.o: term.c term.h
util.o: util.c util.h
