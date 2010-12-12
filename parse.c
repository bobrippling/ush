#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "parse.h"
#include "util.h"
#include "config.h"

enum token
{
	TOKEN_STRING,
	TOKEN_PIPE,
	TOKEN_EOL
} current_token;

char *buffer;
char *current_string;

int nexttoken()
{
	switch(*buffer++){
		case '|':
			current_token = TOKEN_PIPE;
			break;

		case '\0':
			current_token = TOKEN_EOL;
			break;

		default:
			current_token = TOKEN_STRING;
			current_string = buffer - 1;
			while(*buffer && !isspace(*buffer))
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
	char ***argvp = umalloc(2 * sizeof(*argvp));
	char  **argv;
	int argvp_idx = 0;

	buffer = in;

	if(token_init())
		return NULL;

	do{
		argv = argvp[argvp_idx] = umalloc(2 * sizeof(*argv));

		if(current_token == TOKEN_STRING){
			/* program */
			int argv_idx = 1;

			argv[0] = current_string;

			nexttoken();
			for(; current_token == TOKEN_STRING; nexttoken()){
				argvp[argvp_idx] = argv = urealloc(argv, (argv_idx + 2) * sizeof(*argv));
				argv[argv_idx++] = ustrdup(current_string);
				argv[argv_idx  ] = NULL;
			}

			if(current_token == TOKEN_EOL){
				break;
			}else if(current_token == TOKEN_PIPE){
				argvp = urealloc(argvp, (argvp_idx + 2) * sizeof(*argvp));
				argvp_idx++;
			}
		}else{
			fputs("expected: program to run\n", stderr);
			return NULL;
		}
	}while(1);

	argvp[argvp_idx + 1] = NULL;
	return argvp;
#endif
}
