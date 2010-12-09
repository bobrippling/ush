#include <stdio.h>
#include <string.h>

#include "readline.h"
#include "util.h"

#define BSIZ 256

char ***test();

char ***ureadline()
{
	static int donetest = 0;
	int argc, i;
	char buffer[BSIZ], ***argvp, *nl, *space;

	fputs("% ", stdout);

	if(donetest++ == 1){
		printf("cat /tmp/startx.out | grep -F dwm | sed 's/a/AAA/g'\n");
		return test();
	}

	if(!fgets(buffer, BSIZ, stdin)){
		puts("exit");
		return NULL;
	}

	nl = strchr(buffer, '\n');
	if(nl)
		*nl = '\0';

	if(*buffer == '\0'){
		argvp = umalloc(2 * sizeof(char **));
		argvp[0] = umalloc(2 * sizeof(char *));
		argvp[0][0] = "";
		argvp[0][1] = NULL;
		argvp[1] = NULL;
		return argvp;
	}

	argvp = umalloc(2 * sizeof(char **));
	argvp[1] = NULL;

#define argv argvp[0]
	argc = 0;
	for(space = buffer; space; space = strchr(space + 1, ' '))
		argc++;

	argv = umalloc((argc+1) * sizeof(char *));

	argv[argc] = NULL;
	for(space = strtok(buffer, " "), i = 0;
			space && i < argc;
			space = strtok(NULL, " "), i++)
		argv[i] = ustrdup(space);

	return argvp;
}

char ***test()
{
	char ***argvp = umalloc(4 * sizeof(char **));

	argvp[0] = umalloc(4 * sizeof(char *));
	argvp[1] = umalloc(4 * sizeof(char *));
	argvp[2] = umalloc(4 * sizeof(char *));
	argvp[3] = NULL;

	argvp[0][0] = "grep";
	argvp[0][1] = "dwm";
	argvp[0][2] = "/tmp/startx.out";
	argvp[0][3] = NULL;

	argvp[1][0] = "sed";
	argvp[1][1] = "s/a/AAA/g";
	argvp[1][2] = NULL;

	argvp[2] = NULL;

	return argvp;
}
