#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "readline.h"
#include "parse.h"
#include "util.h"
#include "config.h"

#define BSIZ 256

static char *prompt_and_line(void);


char ****ureadline(int *eof)
{
	char ****ret;
	char *buffer, *nl;

	*eof = 0;

	buffer = prompt_and_line();
	if(!buffer){
		*eof = 1;
		return NULL;
	}

	nl = strchr(buffer, '\n');
	if(nl)
		*nl = '\0';

	if(*buffer == '\0')
		return NULL;

	ret = parse(buffer);
	free(buffer);
	return ret;
}

static char *prompt_and_line()
{
	/* here's where the tab completion magic will happen, kids */
	char *buffer = umalloc(BSIZ);
	fputs("% ", stdout);

	if(!fgets(buffer, BSIZ, stdin)){
		if(ferror(stdin))
			perror("ush: read()");
		else
			puts("exit");
		free(buffer);
		return NULL;
	}
	return buffer;
}
