#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "parse.h"
#include "util.h"
#include "config.h"

#define GET_CURRENT_STRING(v) do{ v = current_string; current_string = NULL; }while(0)

enum token
{
	TOKEN_STRING,
	TOKEN_PIPE,
	TOKEN_SEMI_COLON,
	TOKEN_EOL,
	TOKEN_UNKNOWN
} current_token;

char *buffer = NULL;
char *current_string = NULL;

const char *token_to_string(enum token t)
{
	switch(t){
		case TOKEN_STRING:     return "STRING";
		case TOKEN_PIPE:       return "PIPE";
		case TOKEN_SEMI_COLON: return "SEMI_COLON";
		case TOKEN_EOL:        return "EOL";
		case TOKEN_UNKNOWN:    return "UNKNOWN";
	}
	return "TOKEN_?";
}

int iswordchar(char c)
{
	return c && !isspace(c) && c != '|' && c != ';';
}

char nextchar()
{
	while(*buffer && isspace(*buffer))
		buffer++;

	if(*buffer)
		return *buffer++;
	else
		return 0;
}

char peeknextchar()
{
	return *buffer;
}

void nexttoken()
{
	char c = nextchar();

	switch(c){
		case '|':
			current_token = TOKEN_PIPE;
			return;

		case ';':
			current_token = TOKEN_SEMI_COLON;
			return;

		case '\0':
			current_token = TOKEN_EOL;
			return;
	}

	if(iswordchar(c)){
		int len = 2; /* 1 for the initial char, 1 for \0 */
		char *start = buffer - 1;

		do{
			c = peeknextchar();
			if(iswordchar(c)){
				nextchar();
				len++;
			}else
				break;
		}while(1);

		free(current_string);
		current_string = ustrndup(start, len);
		current_token = TOKEN_STRING;
	}else{
		fprintf(stderr, "unknown character: %c\n", c);
		current_token = TOKEN_UNKNOWN;
	}
}

int token_init()
{
	nexttoken();
	return 0;
}

char ****parse(char *in)
{
	char ****argvpp = umalloc(2 * sizeof(*argvpp));
	int argvp_idx = 0, argvpp_idx = 0;
#define argvp (argvpp[argvpp_idx])
#define argv  (argvp[argvp_idx])

	argvpp[argvpp_idx + 1] = NULL;

	buffer = in;

	if(token_init())
		return NULL;

	argvp = umalloc(2 * sizeof(*argvp));
	argvp[argvp_idx + 1] = NULL;


	do{
		argv = umalloc(2 * sizeof(*argv));
		argv[1] = NULL;

		if(current_token == TOKEN_STRING){
			/* program */
			int argv_idx = 1;

			GET_CURRENT_STRING(argv[0]);

			nexttoken();
			for(; current_token == TOKEN_STRING; nexttoken()){
				argv = urealloc(argv, (argv_idx + 2) * sizeof(*argv));
				GET_CURRENT_STRING(argv[argv_idx++]);
				argv[argv_idx] = NULL;
			}

			if(current_token == TOKEN_EOL){
				break;
			}else if(current_token == TOKEN_PIPE){
				nexttoken();
				argvp_idx++;
				argvp = urealloc(argvp, (argvp_idx + 2) * sizeof(*argvp));
				argvp[argvp_idx + 1] = NULL;
			}else if(current_token == TOKEN_SEMI_COLON){
				nexttoken();
				argvpp_idx++;
				argvpp = urealloc(argvpp, (argvpp_idx + 2) * sizeof(*argvpp));
				argvpp[argvpp_idx + 1] = NULL;

				argvp_idx = 0;
				argvp  = umalloc(2 * sizeof(*argvp));
				argvp[argvp_idx + 1] = NULL;
			}else
				goto unexpected;
		}else{
unexpected:
			/* FIXME: free */
			fprintf(stderr, "unexpected token: %s\n", token_to_string(current_token));
			return NULL;
		}
	}while(1);

	return argvpp;
}
