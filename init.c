#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "term.h"
#include "read.h"

char interactive;

static void usage(const char *);

static void usage(const char *n)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n", n);
	fputs(" -i: Assume interactive shell\n"
				"     (job + terminal control)\n"
				" -c: Execute command string\n"
				, stderr);
}

/* main screen turn on */
int main(int argc, char **argv)
{
	int i;

	interactive = isatty(STDIN_FILENO);

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-i"))
			interactive = 1;
		else if(!strcmp(argv[i], "-c")){
			if(++i < argc)
				/* TODO */
				fputs("TODO: -c\n", stderr);
			else
				fputs("-c needs an argument\n", stderr);
		}else{
			usage(*argv);
			return 1;
		}

	if(!term_init())
		return 1;

	readloop();

	term_term();

	return 0;
}
