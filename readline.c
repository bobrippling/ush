#include <stdio.h>
#include <string.h>

#include "readline.h"
#include "util.h"

#define BSIZ 256

char **ureadline()
{
	char buffer[BSIZ];
	char **argv, *space;
	char *nl;
	int argc, i;

	fputs("% ", stdout);

	if(!fgets(buffer, BSIZ, stdin)){
		puts("exit");
		return NULL;
	}

	if(*buffer == '\0'){
		argv = umalloc(2 * sizeof(char *));
		argv[0] = "";
		argv[1] = NULL;
		return argv;
	}

	nl = strchr(buffer, '\n');
	if(nl)
		*nl = '\0';

	argc = 0;
	for(space = buffer; space; space = strchr(space + 1, ' '))
		argc++;

	argv = umalloc((argc+1) * sizeof *argv);

	argv[argc] = NULL;
	for(space = strtok(buffer, " "), i = 0;
			space && i < argc;
			space = strtok(NULL, " "), i++)
		argv[i] = ustrdup(space);

	return argv;
}
