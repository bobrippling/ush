#include <stdio.h>
#include <string.h>

#include "builtin.h"

#define BUILTIN(n) int n(int argc, char **argv)

#define TODO() \
	(void)argc; \
	printf("%s: TODO!\n", *argv); \
	return 1

BUILTIN(echo)
{
	int i = 1, eol = 1;

	if(argc == 1)
		return 0;

	if(!strcmp(argv[i], "-n")){
		eol = 0;
		i++;
	}

	for(; i < argc; i++){
		fputs(argv[i], stdout);
		if(i < argc-1)
			putchar(' ');
	}

	if(eol)
		putchar('\n');

	return 0;
}

BUILTIN(ls)
{
	TODO();
}

BUILTIN(ps)
{
	TODO();
}

BUILTIN(kill)
{
	TODO();
}
