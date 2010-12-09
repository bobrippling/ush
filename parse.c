#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "parse.h"

enum token
{
	TOKEN_STRING,
	TOKEN_PIPE
} current_token;

char *buffer;
char *current_string;

int nexttoken()
{
	switch(*buffer++){
		case '|':
			current_token = TOKEN_PIPE;
			break;

		default:
			current_token = TOKEN_STRING;
			current_string = buffer - 1;
			while(!isspace(*buffer))
				buffer++;
			*buffer++ = '\0';
	}
	return 0;
}

int token_init()
{
	while(isspace(*buffer))
		buffer++;

	return nexttoken();
}

char ***parse(char *in)
{
#ifdef OLD
	int argc, i;
	char ***argvp;

	argvp = umalloc(2 * sizeof(char **));
	argvp[1] = NULL;

# define argv argvp[0]
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
#else
	char ***argvp = umalloc(2 * sizeof(char **));
	char **argv;
	int i = 0;

	buffer = in;

	argvp[1] = NULL; /* FIXME: vector<char *> */
	argv = argvp[0] = umalloc(2 * sizeof(char *));

	if(token_init())
		return NULL;

	do{
		if(current_token == TOKEN_STRING){
			/* program */

			argvp[i] = current_string;

			nexttoken();
			for(; current_token == TOKEN_STRING; nexttoken())
				list_add(current_string);

			/* assert(current_token == TOKEN_PIPE); */
			etc();
		}else{
			fputs("expected: program to run\n", stderr);
			return NULL;
		}
	}while(1);


#endif
}
